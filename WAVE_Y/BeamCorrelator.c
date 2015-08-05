/*
  MCS-DROS: Monitor and Control System - Data Recorder Operating Software
  Copyright (C) 2009-2011  Virginia Tech, Christopher Wolfe <chwolfe2@vt.edu>

  This file is part of MCS-DROS.

  MCS-DROS is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  MCS-DROS is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with MCS-DROS.  If not, see <http://www.gnu.org/licenses/>.
 */
/*
 * BeamCorrelator.c
 *
 *  Created on: Jan 25, 2009
 *      Author: chwolfe2
 */
#include <malloc.h>
#include <string.h>



#include <pthread.h>
#include <errno.h>
#include <math.h>
#include <time.h>
#include <unistd.h>
#include <sys/time.h>
#include <stdarg.h>
#include <pthread.h>

#include "BeamCorrelator.h"
#include "BeamCorrelatorMacros.h"
#include "Log.h"
#include "Profiler.h"
#include "Time.h"

#ifdef DEBUG_XCP
#include <assert.h>
#else
#define assert(x)
#endif

static void LOG(char* logentryFormat, ...){
	char logentry[8173];
	va_list args;
	va_start (args, logentryFormat);
	vsprintf (logentry,logentryFormat, args);
	//vprintf(logentryFormat, args);fflush(stdout);
	va_end (args);
	char buffer[8192];
	sprintf(buffer,"%06lu-%09lu : %s",getMJD(),getMPM(),logentry);
	printf("%s\n",buffer);
}

/********************************************************
// constants
********************************************************/
#define FREQ_CODE_UNINITIALIZED 0xFEEDBEEF

/********************************************************
// Look up table for unpacking quickly
 ********************************************************/
static UnpackedSample LUT[256];
static uint32_t SatLUT[256];

static bool LUT_initialized=false;


/********************************************************
 * frame unpacking
 ********************************************************/
static inline void __initLUT();
static inline void __unpack(BeamCorrelator* bc, int b_idx, DrxFrame* frame);

/********************************************************
 * creation/destruction
 ********************************************************/
static inline bool __beamCorrelatorInit(    BeamCorrelator* bc);
static inline void __beamCorrelatorDestroy( BeamCorrelator* bc);
static inline bool __blockInit(             BeamCorrelator* bc, int b_idx);
static inline void __blockDestroy(          BeamCorrelator* bc, int b_idx);

/********************************************************
 * use/reuse
 ********************************************************/
static inline void __blockReset(        	BeamCorrelator* bc, int b_idx);

static inline void __blockStart(			BeamCorrelator* bc,int b_idx);


#define  S_IDATA_BLOCKSIZE_BYTES(bc) (bc->options.nInts * NUM_STREAMS * bc->options.nSamplesPerOutputFrame * sizeof(UnpackedSample))
#define  S_ACC_BLOCKSIZE_BYTES(bc)   (bc->options.nSamplesPerOutputFrame * NUM_TUNINGS * sizeof(ComplexType))


typedef uint32_t DropCode;
#define DC_BEFORE      0
#define DC_NOT_FILLING 1
#define DC_THREAD_CREATION 1

const char* blockStateNames[] = {"BS_INVALID", "BS_EMPTY", "BS_FILLING", "BS_PROCESSING", "BS_DONE", "BS_DROPPED"};


static inline bool __blockInit(BeamCorrelator* bc, int b_idx){
	assert(bc!=NULL);
	assert(isValidBlock(b_idx));
	bzero( (void*) &bc->blocks[b_idx].setup, sizeof(DrxBlockSetup));
	bc->blocks[b_idx].state 			= BS_INVALID;
	bc->blocks[b_idx].thread    		= (pthread_t) 0;
	bc->blocks[b_idx].threadDone    	= false;
	bc->blocks[b_idx].setup.freqCode[0] = FREQ_CODE_UNINITIALIZED;
	bc->blocks[b_idx].setup.freqCode[1] = FREQ_CODE_UNINITIALIZED;
	bc->blocks[b_idx].options			= bc -> options;

	// create input space
	bc->blocks[b_idx].iData = (UnpackedSample*) malloc (S_IDATA_BLOCKSIZE_BYTES(bc));
	if (! bc->blocks[b_idx].iData)			{
		LOG("[BeamCorrelator] Error: XCP_Create could not allocate iData storage.");
		return false;
	}
	// create accumulator output space
	bc->blocks[b_idx].accumulator = (ComplexType*) malloc (S_ACC_BLOCKSIZE_BYTES(bc));
	if (! bc->blocks[b_idx].accumulator)	{
		LOG("[BeamCorrelator] Error: XCP_Create could not allocate accumulator storage.");
		free(bc->blocks[b_idx].iData);	bc->blocks[b_idx].iData = NULL;
		return false;
	}
	bc->blocks[b_idx].state 		= BS_EMPTY;
	return true;
}
static inline void __blockDestroy(BeamCorrelator* bc, int b_idx){
	assert(bc!=NULL);
	assert(isValidBlock(b_idx));
	void* ignored;
	switch(bc->blocks[b_idx].state){
	case BS_EMPTY:
	case BS_FILLING:
	case BS_DONE:
	case BS_PROCESSING:
	case BS_DROPPED:
		if(bc->blocks[b_idx].thread != (pthread_t) 0){
			pthread_cancel(bc->blocks[b_idx].thread);
			pthread_join(bc->blocks[b_idx].thread,&ignored);
		}
		free(bc->blocks[b_idx].accumulator);
		free(bc->blocks[b_idx].iData);
		bc->blocks[b_idx].accumulator = NULL;
		bc->blocks[b_idx].iData = NULL;
	case BS_INVALID:
	default:
		bc->blocks[b_idx].state = BS_INVALID;
	}

}
static inline bool __beamCorrelatorInit(BeamCorrelator* bc){
	//TRACE();
	assert(bc!=NULL);
	int b_idx;
	bc->in      = 0;
	bc->f_start = 0;
	bc->p_start = 0;
	bc->out     = 0;
	for (b_idx=0; b_idx<NUM_BLOCKS; b_idx++){
		if (!__blockInit(bc,b_idx)){
			LOG("[BeamCorrelator] Error: XCP_Create could not initialize block %d.",b_idx);
			for(--b_idx; b_idx >= 0; b_idx++){
				__blockDestroy(bc,b_idx);
			}
			return false;
		}
	}
	return true;
}
static inline void __beamCorrelatorDestroy(BeamCorrelator* bc){
	//TRACE();
	assert(bc!=NULL);
	int b_idx;
	bc->in      = 0;
	bc->f_start = 0;
	bc->p_start = 0;
	bc->out     = 0;
	for (b_idx=0; b_idx<NUM_BLOCKS; b_idx++){
		__blockDestroy(bc,b_idx);
	}
}


static const char * BSNAMES[]={
		"BS_INVALID",
		"BS_EMPTY",
		"BS_FILLING",
		"BS_PROCESSING",
		"BS_DONE",
		"BS_DROPPED"
};

// use
static inline void __blockSetSetup(BeamCorrelator* bc, int b_idx, DrxFrame* frame,uint64_t timeTag0){
	assert(bc!=NULL);
	assert(frame!=NULL);
	assert(isValidBlock(b_idx));
	//assert(bc->blocks[b_idx].state == BS_EMPTY);
	if (bc->blocks[b_idx].state != BS_EMPTY){
		printf("STATE = '%s'\n",BSNAMES[bc->blocks[b_idx].state]);
		assert(bc->blocks[b_idx].state == BS_EMPTY);
	}

	bc->blocks[b_idx].setup.beam      		= frame->header.drx_beam;
	bc->blocks[b_idx].setup.decFactor 		= frame->header.decFactor;
	bc->blocks[b_idx].setup.timeOffset      = frame->header.timeOffset;
	bc->blocks[b_idx].setup.timeTagStep     = frame->header.decFactor * DRX_SAMPLES_PER_FRAME;
	bc->blocks[b_idx].setup.timeTag0		= timeTag0;
	bc->blocks[b_idx].setup.timeTagN		= timeTag0 +
		(
			(((bc->options.nInts *  bc->options.nSamplesPerOutputFrame) / DRX_SAMPLES_PER_FRAME) - 1) *
			bc->blocks[b_idx].setup.timeTagStep
		);
	// unrolled forloop 0..NUM_TUNINGS-1
	bc->blocks[b_idx].setup.freqCode[0]     = FREQ_CODE_UNINITIALIZED;
	bc->blocks[b_idx].setup.freqCode[1]     = FREQ_CODE_UNINITIALIZED;
	bc->blocks[b_idx].setup.stepPhase       = STEP_PHASE_DRX(frame->header.timeTag,frame->header.decFactor);
	bc->blocks[b_idx].setup.freqCode[T_IDX(frame->header.drx_tuning)] =
			frame->header.freqCode;
	bc->blocks[b_idx].state = BS_FILLING;

}

static inline void __blockUpdateSetup(BeamCorrelator* bc, int b_idx, DrxFrame* frame){
	assert(bc!=NULL);
	assert(frame!=NULL);
	assert(isValidBlock(b_idx));
	assert(bc->blocks[b_idx].state == BS_FILLING);
	if(unlikely( bc->blocks[b_idx].setup.freqCode[T_IDX(frame->header.drx_tuning)] == FREQ_CODE_UNINITIALIZED)){
		bc->blocks[b_idx].setup.freqCode[T_IDX(frame->header.drx_tuning)] = frame->header.freqCode;
	}
}

//reuse

static inline void __blockReset(BeamCorrelator* bc, int b_idx){
	assert(bc!=NULL);
	assert(isValidBlock(b_idx));
	assert(bc->blocks[b_idx].state != BS_INVALID);
	void* ignored;
	if (bc->blocks[b_idx].thread != (pthread_t) 0 )
		pthread_join(bc->blocks[b_idx].thread,&ignored);
	bc->blocks[b_idx].thread        	= (pthread_t) 0;
	bc->blocks[b_idx].threadDone    	= false;
	bc->blocks[b_idx].fill             	= 0;
	bc->blocks[b_idx].isPrimer 			= false;
	bc->blocks[b_idx].state 			= BS_EMPTY;
	bc->blocks[b_idx].satCount			= 0;
	bzero((void*)&bc->blocks[b_idx].setup, sizeof(DrxBlockSetup));
}

// tests
static inline bool __blockSetupMatches(BeamCorrelator* bc, int b_idx, DrxFrame* frame){
	assert(bc!=NULL);
	assert(frame!=NULL);
	assert(isValidBlock(b_idx));
	assert(bc->blocks[b_idx].state != BS_EMPTY);
	assert(bc->blocks[b_idx].state != BS_INVALID);
	return (
			( bc->blocks[b_idx].setup.decFactor         		== frame->header.decFactor ) &&
			( bc->blocks[b_idx].setup.beam              		== frame->header.drx_beam  ) &&
			( bc->blocks[b_idx].setup.timeOffset        		== frame->header.timeOffset) &&
			( bc->blocks[b_idx].setup.stepPhase        			== STEP_PHASE_DRX(frame->header.timeTag, frame->header.decFactor))  &&
			(
					( bc->blocks[b_idx].setup.freqCode[T_IDX(frame->header.drx_tuning)] == FREQ_CODE_UNINITIALIZED  ) ||
					( bc->blocks[b_idx].setup.freqCode[T_IDX(frame->header.drx_tuning)] == frame->header.freqCode   )
			)
	);
}


static inline void __logDroppedBlock(BeamCorrelator* bc, int b_idx,DropCode code){
	//TRACE();
	assert(bc!=NULL);
	assert(isValidBlock(b_idx));
	switch(code){
	case DC_THREAD_CREATION:
		LOG("[BeamCorrelator] Error: Dropped block due to inability to create processing threads");
		return;
	default:
		LOG("[BeamCorrelator] Warning: Dropped block for unknown reason");
		return;
	}
}
static inline void __logDroppedFrame(DrxFrame* frame,DropCode code){
	//TRACE();
	assert(frame!=NULL);
	switch(code){
	case DC_BEFORE:
	case DC_NOT_FILLING:
		LOG("[BeamCorrelator] Warning: Dropped frame which arrived after its block had started (time=%lu, beam=%hhu, tuning=%hhu, pol=%hhu).", frame->header.timeTag, frame->header.drx_beam, frame->header.drx_tuning, frame->header.drx_polarization);
		return;
	default:
		LOG("[BeamCorrelator] Warning: Dropped frame for unknown reason (time=%lu, beam=%hhu, tuning=%hhu, pol=%hhu).", frame->header.timeTag, frame->header.drx_beam, frame->header.drx_tuning, frame->header.drx_polarization);
	}
}


static inline void __dropBlock(BeamCorrelator* bc, int b_idx, DropCode dc){
	//TRACE();
	assert(bc!=NULL);
	assert(isValidBlock(b_idx));
	assert(
			(bc->blocks[b_idx].state == BS_FILLING)    ||
			(bc->blocks[b_idx].state == BS_PROCESSING)
	);
	bc->blocks[b_idx].state = BS_DROPPED;
	__logDroppedBlock(bc,b_idx, dc);
}



static inline void __initLUT(){
	if (!LUT_initialized){
		int byte=0;
		for (byte=0; byte<256; byte++){
			signed char _i=(byte & 0xf0);
			signed char _q=(byte & 0x0f)<<4;
			LUT[byte].i=(RealType)(_i>>4);
			LUT[byte].q=(RealType)(_q>>4);
			RealType magSquared = ((LUT[byte].i * LUT[byte].i) + (LUT[byte].q * LUT[byte].q));
			SatLUT[byte] = ( magSquared >= 49.0);
		}
		LUT_initialized=true;
	}
}
/*
static inline void __printFrame(DrxFrame*f){
	int i=0;


	printf("==============================================================================\n");
	printf("== Beam:              %d\n" , (int)f->header.drx_beam);
	printf("== Tuning:            %d\n" , (int)f->header.drx_tuning);
	printf("== Polarization:      %d\n" , (int)f->header.drx_polarization);
	printf("== Frequency Code:    %d\n" , (int)f->header.freqCode);
	printf("== Decimation Factor: %d\n" , (int)f->header.decFactor);
	printf("== Time Offset:       %d\n" , (int)f->header.timeOffset);
	printf("== Time Tag:          %ld\n" , (long int)f->header.timeTag);
	printf("== Status Flags:      %lu\n" , f->header.statusFlags);
	printf("==============================================================================\n");
	for(i=0; i<128; i++){
		printf("%+2hhd %+2hhd ", (int)f->samples[i].i, (int)f->samples[i].q);
		if ((i & 0xf) == 0xf){
			printf("\n");
		}
	}
	printf("==============================================================================\n");

}
static inline void  __checkIsCVframe(DrxFrame*f){
	int i=1;
	while((i<4096) && (f->samples[i].packed == f->samples[0].packed)){
		i++;
	}
	if (i==4096){
		__printFrame(f);
		printf("Constant Value Frame detected.\n");
		exit(-1);
	}
}
 */
static inline bool __blockIsFull(BeamCorrelator* bc, int b_idx){
	return (bc->blocks[b_idx].fill == (bc->options.nInts * bc->options.nSamplesPerOutputFrame * NUM_STREAMS));
}
static inline void __unpack(BeamCorrelator* bc, int b_idx, DrxFrame* frame){
	//static int cnt=0;
	//PROFILE_START(TMR_X_UNPACK);
	//static int fpr=0;
	int bi  = b_idx;
	uint32_t t = (uint32_t) frame->header.drx_tuning-1;
	uint32_t p = (uint32_t) frame->header.drx_polarization;
	uint32_t pck_idx = (uint32_t) ((frame->header.timeTag - bc->blocks[b_idx].setup.timeTag0) / bc->blocks[b_idx].setup.timeTagStep);

	uint32_t index =
			(pck_idx * DRX_SAMPLES_PER_FRAME) +
			((t*NUM_POLS) + p) * bc->options.nInts * bc->options.nSamplesPerOutputFrame;


#ifdef DEBUG_XCP_UNPACKING
	printf("---------------------------------------------------\n");
	printf("-- Frame timetag:               %19lu\n",frame->header.timeTag);
	printf("--                         %s\n", HumanTimeFromTag(frame->header.timeTag));
	printf("-- Block timetag0:              %19lu\n",bc->blocks[b_idx].setup.timeTag0);
	printf("--                         %s\n", HumanTimeFromTag(bc->blocks[b_idx].setup.timeTag0));
	printf("-- Frame stepPhase:                       %9lu\n",STEP_PHASE_DRX(frame->header.timeTag,frame->header.decFactor));
	printf("-- Block stepPhase:                       %9lu\n",bc->blocks[bi].setup.stepPhase);
	printf("-- Frame decFactor:                       %9d\n",frame->header.decFactor);
	printf("-- Block decFactor:                       %9d\n",bc->blocks[bi].setup.decFactor);
	printf("-- Block timeTagStep:                     %9lu\n",bc->blocks[bi].setup.timeTagStep);
	printf("-- Tuning Index:                          %9u\n",t);
	printf("-- Pol Index:                             %9u\n",p);
	printf("-- Packet Index:                          %9u\n",pck_idx);
	printf("-- Final Index:                           %9u\n",index);
	printf("---------------------------------------------------\n");
#endif /*DEBUG_XCP_UNPACKING*/



	UnpackedSample* where =	&bc->blocks[bi].iData[index];
	uint32_t i=0;
	uint32_t satCount = 0;
	for (i=0; i<DRX_SAMPLES_PER_FRAME; i++){
		where[i].i =  LUT[frame->samples[i].packed].i;
		where[i].q =  LUT[frame->samples[i].packed].q;
		satCount   += SatLUT[frame->samples[i].packed];
/*
		if (((p==0) && ((where[i].i != 6.0) || (where[i].q != 1.0))) || ((p==1) && ((where[i].i != 1.0) || (where[i].q != 0.0)))){
			printf("Error in receive/decode.\n");
		}
*/

	}
	bc->blocks[bi].satCount += satCount;
	/*
	if (fpr<12){
		printf("> T=%2u, P=%2u, D[0-7] = ",t,p);
		for (i=0; i<8; i++){
			printf("%5.3f %+5.3fi, ",where[i].i,where[i].q);
		}
		printf("\n");
		fpr++;
	} else {
		exit(-1);
	}
*/


	bc->blocks[bi].fill += (DRX_SAMPLES_PER_FRAME);
	__blockUpdateSetup(bc,b_idx,frame);
	if (__blockIsFull(bc,b_idx)){
		bc->counters.blocksStartedFull++;
		__blockStart(bc,b_idx);
	}
	//PROFILE_STOP(TMR_X_UNPACK);

}




typedef int32_t Order;
#define ORDER_BEFORE	-1
#define ORDER_EQUAL		0
#define ORDER_WITHIN	0
#define ORDER_AFTER		1

/*
static inline uint64_t __frameDeltaPre(BeamCorrelator* bc, int b_idx, DrxFrame* frame){
	uint64_t delta = (bc->blocks[b_idx].setup.timeTag0 - frame->header.timeTag);
	return delta;
}
static inline uint64_t __frameDeltaPost(BeamCorrelator* bc, int b_idx, DrxFrame* frame){
	uint64_t delta = (frame->header.timeTag - bc->blocks[b_idx].setup.timeTag0);
	return delta;
}
*/

static inline Order __getOrder(BeamCorrelator* bc, int b_idx, DrxFrame* frame){
	assert(bc!=NULL);
	assert(frame!=NULL);
	assert(isValidBlock(b_idx));
	assert(bc->blocks[b_idx].state != BS_EMPTY);
	assert(bc->blocks[b_idx].state != BS_INVALID);
	if (frame->header.timeTag >= bc->blocks[b_idx].setup.timeTag0){
		if (frame->header.timeTag <= bc->blocks[b_idx].setup.timeTagN){
			return ORDER_WITHIN;
		} else {
			return ORDER_AFTER;
		}
	} else {
		return ORDER_BEFORE;
	}
}

static pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
void getMutex(){
	pthread_mutex_lock(&mutex);
}
void releaseMutex(){
	pthread_mutex_unlock(&mutex);
}


static void __DoCalc(XCPDataBlock* block) {
	assert(block!=NULL);
	//getMutex();

	int ints       = block->options.nInts;
	int outSamples = block->options.nSamplesPerOutputFrame;
	int j;			// samples/outputframe iterator
	int i;			// integration iterator
	int t;			// tuning iterator

	// iterate over tunings
	for (t=0; t<NUM_TUNINGS; t++){
		UnpackedSample* t_x = &block->iData[((t*NUM_POLS) + 0) * ints * outSamples];
		UnpackedSample* t_y = &block->iData[((t*NUM_POLS) + 1) * ints * outSamples];
		ComplexType* t_acc = &block->accumulator[t * block->options.nSamplesPerOutputFrame];
		// compute W[i] = <Z[j]>

		for (i=0; i<outSamples; i++){
			(*t_acc)[0] = 0.0f;
			(*t_acc)[1] = 0.0f;
			// correlate Z[j] = X[j] * conj(Y[j])
			for(j=0; j<ints; j++){
				RealType re = (t_x->re * ( t_y->re)) + (t_x->im * t_y->im);
				RealType im= (t_x->re * (-t_y->im)) + (t_x->im * t_y->re);
				/*
				if (
						(t_x->re != 6.0) ||
						(t_x->im != 1.0) ||
						(t_y->re != 1.0) ||
						(t_y->im != 0.0)
						){
					printf("Error @ result=%6d sample#=%6d, : ___________ X=%5.3f %+5.3fi,    Y=%5.3f %+5.3fi,    X*conj(Y)=%5.3f %+5.3fi\n",i,j,t_x->re,t_x->im,t_y->re,t_y->im,re,im);
				}
				*/
				(*t_acc)[0] += re;
				(*t_acc)[1] += im;
				t_x++;
				t_y++;
			}
			// <Z[j]> = Sum(Z[j])/N
			(*t_acc)[0] /= (RealType)ints;
			(*t_acc)[1] /= (RealType)ints;
			t_acc++;
		}
	}
	//releaseMutex();

	// clear input for reuse
	bzero((void*) block->iData,
		(
			block->options.nInts *
			block->options.nSamplesPerOutputFrame *
			NUM_STREAMS *
			sizeof(UnpackedSample)
		)
	);
	block->threadDone = true;


}

static void* __CalcWrapper(void* arg){
	assert(arg!=NULL);
	//PROFILE_START(TMR_THREAD_0);
	//struct timeval start, stop;
	//gettimeofday(&start, NULL);
	__DoCalc((XCPDataBlock*) arg);
	//PROFILE_STOP(TMR_THREAD_0);
	//gettimeofday(&stop, NULL);
	//size_t elapsed = ((size_t)stop.tv_sec * 1000000l + (size_t)stop.tv_usec) -  ((size_t)start.tv_sec * 1000000l + (size_t)start.tv_usec);
	//printf("thread %d completed in %lu us.\n",((ThreadInfo*) arg)->s_idx, elapsed);
	return NULL;
}


static inline void __advanceFillPointer(BeamCorrelator* bc){
	while(	(bc->f_start != bc->in) &&(	(bc->blocks[bc->f_start].state == BS_PROCESSING) || (bc->blocks[bc->f_start].state == BS_DROPPED)	))	{
		QUEUE_INCREMENT(bc->f_start, NUM_BLOCKS);
	}
}
static inline void __advanceProcessingPointer(BeamCorrelator* bc){
	while(	(bc->p_start != bc->f_start) &&(	(bc->blocks[bc->p_start].state == BS_DONE) || (bc->blocks[bc->p_start].state == BS_DROPPED)	))	{
		QUEUE_INCREMENT(bc->p_start, NUM_BLOCKS);
	}
}
static inline void __advanceOutPointer(BeamCorrelator* bc){
	while(	(bc->out != bc->p_start) && ( (bc->blocks[bc->out].state == BS_DROPPED)	))	{
		if(bc->blocks[bc->out].state == BS_DROPPED){
			LOG("@@@@@@@@@@ dismissed dropped block %d.",bc->out);
		}
		__blockReset(bc,bc->out);
		QUEUE_INCREMENT(bc->out, NUM_BLOCKS);
	}
}

static inline void __blockStart(BeamCorrelator* bc,int b_idx){
	assert(bc!=NULL);
	assert(isValidBlock(b_idx));
	assert(bc->blocks[b_idx].state == BS_FILLING);

	// start thread
	assert(bc->blocks[b_idx].thread == (pthread_t) 0);
	if (__blockIsFull(bc,b_idx)){
		if( pthread_create(
				&bc->blocks[b_idx].thread,
				&bc->attr,
				__CalcWrapper, (void*) &bc->blocks[b_idx]))	{
			LOG("[BeamCorrelator] Error: Could not start thread for block %d, pthread_create reported '%s'.", b_idx, strerror(errno));
			bc->blocks[b_idx].state = BS_DROPPED;
			bc->counters.blocksDroppedFailedStart++;
		} else {
			bc->blocks[b_idx].state = BS_PROCESSING;
			bc->counters.blocksStarted++;
		}
	} else {
		LOG("[BeamCorrelator] Error: Could not start block %d, fill was %u, expecting %u\n.", b_idx, bc->blocks[b_idx].fill, (bc->options.nInts * bc->options.nSamplesPerOutputFrame * NUM_STREAMS));
		bc->counters.blocksDroppedUnderfull++;
		bc->blocks[b_idx].state = BS_DROPPED;
	}
	if (b_idx == bc->f_start){
		__advanceFillPointer(bc);
	}

}

BeamCorrelator* XCP_Create			(BeamCorrelatorOptions* options){
	//PROFILE_START(TMR_XCP_CREATE);

	if (!options){
		LOG("[BeamCorrelator] Error: invalid options pointer supplied to XCP_Create.");
		//profilerStop(TMR_XCP_CREATE);
		return NULL;
	}
	BeamCorrelator* bc = (BeamCorrelator*) malloc(sizeof(BeamCorrelator));
	if (!bc) {
		LOG("[BeamCorrelator] Error: XCP_Create could not allocate beamCorrelator structure.");
		//PROFILE_STOP(TMR_XCP_CREATE);
		return NULL;
	}
	bzero((void*)bc,sizeof(BeamCorrelator));
	bc->options = *options;

	if (pthread_attr_init(&bc->attr)){
		LOG("[BeamCorrelator] Error: XCP_Create could not set up thread attributes.");
		free(bc); return NULL;
	}
	if (pthread_attr_setdetachstate(&bc->attr, PTHREAD_CREATE_JOINABLE)){
		LOG("[BeamCorrelator] Error: XCP_Create could not set up thread attributes.");
		pthread_attr_destroy(&bc->attr);
		free(bc); return NULL;
	}
	if (pthread_attr_setscope(&bc->attr, PTHREAD_SCOPE_SYSTEM)){
		LOG("[BeamCorrelator] Error: XCP_Create could not set up thread attributes.");
		pthread_attr_destroy(&bc->attr);
		free(bc); return NULL;
	}
	if (pthread_attr_setschedpolicy(&bc->attr, SCHED_FIFO)){
		LOG("[BeamCorrelator] Error: XCP_Create could not set up thread attributes.");
		pthread_attr_destroy(&bc->attr);
		free(bc); return NULL;
	}



	__initLUT();
	if (!__beamCorrelatorInit(bc)){
		LOG("[BeamCorrelator] Error: XCP_Create could not initialize beamCorrelator.");
		free(bc);
		//PROFILE_STOP(TMR_XCP_CREATE);
		return NULL;
	}
	LOG("[BeamCorrelator]: BeamCorrelator created.");
	//PROFILE_STOP(TMR_XCP_CREATE);
	return bc;
}

StatusCode    XCP_Insert			(BeamCorrelator* bc, DrxFrame* frame){
	//PROFILE_START(TMR_XCP_INSERT);
	int b_idx = 0;
	int bprev;
	uint64_t timeTag0;
	for (b_idx = bc->f_start; b_idx != bc->in; QUEUE_INCREMENT(b_idx, NUM_BLOCKS)){
		if (__blockSetupMatches(bc,b_idx,frame)){
			switch(__getOrder(bc,b_idx,frame)){
			case ORDER_BEFORE:
				if (b_idx == bc->f_start){
					__logDroppedFrame(frame,DC_BEFORE);
					//PROFILE_STOP(TMR_XCP_INSERT);
					bc->counters.framesDroppedArrivedLate++;
					return SUCCESS;
				} else {
					// possible out-of order on the block level
					continue;
				}
			case ORDER_WITHIN:
				if (bc->blocks[b_idx].state == BS_FILLING){
					__unpack(bc,b_idx,frame);
					bc->counters.framesInsertedExisting++;
					bc->counters.framesInserted++;
				} else {
					bc->counters.framesDroppedArrivedLate++;
					__logDroppedFrame(frame,DC_NOT_FILLING);
				}
				//PROFILE_STOP(TMR_XCP_INSERT);
				return SUCCESS;
			case ORDER_AFTER:
				continue;
			default:
				LOG("[BeamCorrelator]: Error: Reached unreachable code in file '%s' at line %d. Program error detected.",__FILE__, __LINE__);
			}
		} else {
			continue;
		}
	}
	if (QUEUE_FULL(bc->in, bc->out, NUM_BLOCKS)){
		//PROFILE_STOP(TMR_XCP_INSERT);
		return NOT_READY;
	} else {
		b_idx = bc->in;
		timeTag0   = frame->header.timeTag;
		if (!QUEUE_EMPTY(bc->in, bc->out, NUM_BLOCKS)){
			bprev = QUEUE_PRED(b_idx, NUM_BLOCKS);
			if (bc->blocks[bprev].state != BS_EMPTY)
				if (__blockSetupMatches(bc,bprev,frame)){
					uint64_t blockStep =
							bc->blocks[bprev].setup.timeTagStep *
							((bc->options.nInts *  bc->options.nSamplesPerOutputFrame) / DRX_SAMPLES_PER_FRAME);
					uint64_t blockStepPhase = bc->blocks[bprev].setup.timeTag0 % blockStep;
					timeTag0 = ((((timeTag0-blockStepPhase) / blockStep) * blockStep)+blockStepPhase);
					bc->counters.framesInsertedAltTimeTag++;
				}
		}
		__blockSetSetup(bc,b_idx,frame,timeTag0);
		__unpack(bc,b_idx,frame);
		if (bc->counters.framesInsertedNew == 0){
			bc->blocks[0].isPrimer = true;
		}
		if (bc->counters.framesInsertedNew == 1){
			if (bc->blocks[0].state == BS_FILLING)
				__blockStart(bc,0);
		}
		bc->counters.framesInsertedNew++;
		bc->counters.framesInserted++;

		QUEUE_INCREMENT(bc->in,NUM_BLOCKS);
		//PROFILE_STOP(TMR_XCP_INSERT);
		return SUCCESS;
	}
}


void    XCP_Monitor			(BeamCorrelator* bc){
	// advance the processing start partition
	//__advanceFillPointer(bc);
	__advanceProcessingPointer(bc);
	//__advanceOutPointer(bc);
	//PROFILE_START(TMR_XCP_MONITOR);
	int blks_filling    = QUEUE_USED(bc->in,      bc->f_start, NUM_BLOCKS);
	int blks_processing = QUEUE_USED(bc->f_start, bc->p_start, NUM_BLOCKS);
	int b_idx;
	// check on partially filled blocks to start
	if (blks_filling > bc->options.highWater){
		bc->counters.blocksStartedHighWater++;
		__blockStart(bc,bc->f_start);
	}

	// check on processing blocks to see if they're done
	for (b_idx = bc->p_start; b_idx != bc->f_start; QUEUE_INCREMENT(b_idx,NUM_BLOCKS)){
		switch(bc->blocks[b_idx].state){
		case BS_INVALID:
		case BS_EMPTY:
		case BS_FILLING:
			assert(false);
		case BS_DROPPED:
		case BS_DONE:
			continue;
		case BS_PROCESSING:
			if(bc->blocks[b_idx].threadDone){
				bc->counters.blocksCompleted++;
				bc->blocks[b_idx].state = BS_DONE;
			}
		}
	}
	//PROFILE_STOP(TMR_XCP_MONITOR);
}

StatusCode    XCP_NextResult		(BeamCorrelator* bc, XCPDataBlock** result){
	//PROFILE_START(TMR_XCP_NEXT);

	assert(bc!=NULL);
	assert(result!= NULL);
	// discard any dropped blocks at the head of the queue
	__advanceOutPointer(bc);

	// check to see if we have any data
	if (unlikely(QUEUE_EMPTY(bc->p_start, bc->out, NUM_BLOCKS))){
		//PROFILE_STOP(TMR_XCP_NEXT);
		return NOT_READY;
	} else {
		assert(bc->blocks[bc->out].state == BS_DONE);
		*result = & bc->blocks[bc->out];
		bc->counters.resultsRequested++;
		//PROFILE_STOP(TMR_XCP_NEXT);
		return SUCCESS;
	}
}

StatusCode    XCP_ReleaseResult	(BeamCorrelator* bc, XCPDataBlock* result){
	//PROFILE_START(TMR_XCP_RELEASE);
	assert(result == & bc->blocks[bc->out]);
	assert(result->state == BS_DONE);
/*
	int i=0;
	int j=0;

	printf("---------------------------BLOCK-----------------");
	for(j=0; j < NUM_TUNINGS; j++){
		for(i=0; i < bc->options.nSamplesPerOutputFrame; i++){
			printf("Tuning %2d, Sample %2d: %5.3f %+5.3fi\n", j, i, bc->blocks[bc->out].accumulator[i][0], bc->blocks[bc->out].accumulator[i][1]);
		}
	}
	printf("-----------------------END-BLOCK-----------------");
*/
	__blockReset(bc,bc->out);
	bc->counters.resultsReleased++;
	QUEUE_INCREMENT(bc->out,NUM_BLOCKS);
	//PROFILE_STOP(TMR_XCP_RELEASE);
	return SUCCESS;
}


void          XCP_Destroy			(BeamCorrelator** bc){
	int i=0;
	//PROFILE_START(TMR_XCP_DESTROY);
	if (bc){
		if (*bc){

			printf("Correlator Queue info @ destruction.\n");
			printf("IN          %9u.\n", (*bc)->in);
			printf("F_START     %9u.\n", (*bc)->f_start);
			printf("P_START     %9u.\n", (*bc)->p_start);
			printf("OUT         %9u.\n", (*bc)->out);
			/*
			printf("///////////// Used Blocks /////////////\n");
			for (i=(*bc)->out; i!=(*bc)->in; QUEUE_INCREMENT(i,NUM_BLOCKS)){
				printf("\tBLOCK %d: %s\n",i,blockStateNames[(*bc)->blocks[i].state] );
			}
			printf("//////////// Unused Blocks ////////////\n");
			for (i=(*bc)->in; i!=(*bc)->out; QUEUE_INCREMENT(i,NUM_BLOCKS)){
				printf("\tBLOCK %d: %s\n",i,blockStateNames[(*bc)->blocks[i].state] );
			}
			printf("/////////// End Block Info ////////////\n");
			*/

			__beamCorrelatorDestroy(*bc);
			*bc = NULL;
			LOG("[BeamCorrelator]: BeamCorrelator destroyed.");
		} else {
			LOG("[BeamCorrelator]: Error: Tried to destroy beamCorrelator, but pointer was NULL.");
		}
	} else {
		LOG("[BeamCorrelator]: Error: Tried to destroy beamCorrelator, but pointer to pointer was NULL.");
	}
	//PROFILE_STOP(TMR_XCP_DESTROY);
}




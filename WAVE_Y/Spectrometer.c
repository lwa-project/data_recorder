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
 * Spectrometer.c
 *
 *  Created on: Jan 25, 2009
 *      Author: chwolfe2
 */
#include <malloc.h>
#include <string.h>

#ifdef DEBUG_SPECTROMETER
	#include <assert.h>
#else
	#define assert(x)
#endif

#include <pthread.h>
#include <errno.h>
#include <math.h>
#include <time.h>
#include <unistd.h>
#include <sys/time.h>
#include <stdarg.h>

#include "Spectrometer.h"
#include "SpectrometerMacros.h"
#include "Log.h"
#include "Profiler.h"
#include "Time.h"

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
/*
int counter[32];
void printCounters(){
	int i=0;
	for (i=0;i<22;i++){
		printf("COUNTER %d = %d\n",i,counter[i]);
		counter[i] = 0;
	}
}
*/
//int insex,insnew = 0;
// drx constants

// constants
#define FREQ_CODE_UNINITIALIZED 0xFEEDBEEF
typedef uint32_t DropCode;
	#define DC_BEFORE      0
	#define DC_NOT_FILLING 1
	#define DC_THREAD_CREATION 1

typedef int32_t Order;
	#define ORDER_BEFORE	-1
	#define ORDER_EQUAL		0
	#define ORDER_WITHIN	0
	#define ORDER_AFTER		1


// Look up table for unpacking quickly
static UnpackedSample LUT[256];
static uint32_t SatLUT[256];
static bool LUT_initialized=false;
static const char * BSNAMES[]={
	"BS_INVALID",
	"BS_EMPTY",
	"BS_FILLING",
	"BS_PROCESSING",
	"BS_DONE",
	"BS_DROPPED"
};

/********************************************************
* frame unpacking
********************************************************/
static inline void __initLUT();
static inline void __unpack(Spectrometer* s, int b_idx, DrxFrame* frame);

/********************************************************
* creation/destruction
********************************************************/
static inline bool __spectrometerInit(    Spectrometer* s);
static inline void __spectrometerDestroy( Spectrometer* s);
static inline bool __blockInit(           Spectrometer* s, int b_idx);
static inline void __blockDestroy(        Spectrometer* s, int b_idx);
static inline bool __cellInit(            Spectrometer* s, int b_idx, int s_idx);
static inline void __cellDestroy(         Spectrometer* s, int b_idx, int s_idx);

/********************************************************
* use/reuse
********************************************************/
static inline void __blockReset(          Spectrometer* s, int b_idx);
static inline void __cellReset(           Spectrometer* s, int b_idx, int s_idx);

static inline void __blockStart(			Spectrometer* s,int b_idx);
static inline void __cellStart(				Spectrometer* s, int b_idx, int s_idx);



static inline bool __cellInit(Spectrometer* s, int b_idx, int s_idx){
	assert(s!=NULL);
	assert(isValidBlock(b_idx));
	assert(isValidStream(s_idx));
	// copy options for local access by thread
	s->blocks[b_idx].cells[s_idx].id			= s_idx;
	s->blocks[b_idx].cells[s_idx].accumulator 	= &s->blocks[b_idx].accumulator[s_idx * S_ACC_SIZE_SAMPLES(s)];
	s->blocks[b_idx].cells[s_idx].iData 		= &s->blocks[b_idx].iData[s_idx * S_IDATA_SIZE_SAMPLES(s)];
	s->blocks[b_idx].cells[s_idx].oData 		= &s->blocks[b_idx].oData[s_idx * S_ODATA_SIZE_SAMPLES(s)];
	s->blocks[b_idx].cells[s_idx].fill 			= 0;
	s->blocks[b_idx].cells[s_idx].options		= s->options;
	s->blocks[b_idx].cells[s_idx].thread		= (pthread_t) 0;
	s->blocks[b_idx].cells[s_idx].plan			= FFT_PLAN_MANY(
		1,																		/* int            rank    */
		&s->blocks[b_idx].cells[s_idx].options.nFreqChan,			/* const int *    n       */
		s->blocks[b_idx].cells[s_idx].options.nInts,				    /* int            howmany */
		(ComplexType *)s->blocks[b_idx].cells[s_idx].iData,         	/* fftw_complex * in      */
		NULL,																	/* const int *    inembed */
		1,																		/* int            istride */
		s->blocks[b_idx].cells[s_idx].options.nFreqChan,				/* int            idist   */
		(ComplexType *)s->blocks[b_idx].cells[s_idx].oData,        	/* fftw_complex * out      */
		NULL,																	/* const int *    onembed */
		1,																		/* int            ostride */
		s->blocks[b_idx].cells[s_idx].options.nFreqChan,				/* int            odist   */
		FFTW_FORWARD, 															/* int            sign    */
		FFTW_ESTIMATE															/* unsigned       flags   */
	);
	if (!s->blocks[b_idx].cells[s_idx].plan){
		LOG("[Spectrometer] Error: Spec_Create could not create FFT plan for cell %d of block %d (options = %u / %u / %u / %u).",s_idx,b_idx, s->options.nFreqChan, s->options.nInts, s->options.highWater, s->options.minimumFill);
		s->blocks[b_idx].cells[s_idx].state	= CS_INVALID;
		return false;
	} else {
		// plan created successfully
		s->blocks[b_idx].cells[s_idx].state	= CS_NOT_STARTED;
		return true;
	}
}
static inline void __cellDestroy(Spectrometer* s, int b_idx, int s_idx){
	assert(s!=NULL);
	assert(isValidBlock(b_idx));
	assert(isValidStream(s_idx));
	void* ignored;
	switch(s->blocks[b_idx].cells[s_idx].state){
		case CS_RUNNING:
			if(s->blocks[b_idx].cells[s_idx].thread != (pthread_t) 0){
				pthread_cancel(s->blocks[b_idx].cells[s_idx].thread);
				pthread_join(s->blocks[b_idx].cells[s_idx].thread,&ignored);
			}
		case CS_DONE:
		case CS_ERROR:
		case CS_NOT_STARTED:
			if (s->blocks[b_idx].cells[s_idx].plan)
				FFT_PLAN_DESTROY(s->blocks[b_idx].cells[s_idx].plan);
		case CS_INVALID:
		default:
			s->blocks[b_idx].cells[s_idx].thread = (pthread_t) 0;
			s->blocks[b_idx].cells[s_idx].plan   = NULL;
			s->blocks[b_idx].cells[s_idx].state  = CS_INVALID;
			return;
	}
}

static inline bool __blockInit(Spectrometer* s, int b_idx){
	assert(s!=NULL);
	assert(isValidBlock(b_idx));
	int s_idx=0;
	bzero( (void*) &s->blocks[b_idx].setup, sizeof(DrxBlockSetup));
	s->blocks[b_idx].setup.freqCode[0] = FREQ_CODE_UNINITIALIZED;
	s->blocks[b_idx].setup.freqCode[1] = FREQ_CODE_UNINITIALIZED;
	s->blocks[b_idx].state 			= BS_INVALID;
	// create FFT input space
	s->blocks[b_idx].iData = (UnpackedSample*) FFT_MALLOC (S_IDATA_BLOCKSIZE_BYTES(s));
	if (! s->blocks[b_idx].iData)			{
		LOG("[Spectrometer] Error: Spec_Create could not allocate iData storage.");
		return false;
	}

	// create FFT output space
	s->blocks[b_idx].oData = (UnpackedSample*) FFT_MALLOC (S_ODATA_BLOCKSIZE_BYTES(s));
	if (! s->blocks[b_idx].oData)			{
		LOG("[Spectrometer] Error: Spec_Create could not allocate oData storage.");
		FFT_FREE(s->blocks[b_idx].iData);	s->blocks[b_idx].iData = NULL;
		return false;
	}

	// create accumulator output space
	s->blocks[b_idx].accumulator = (RealType*) malloc     (S_ACC_BLOCKSIZE_BYTES(s));
	if (! s->blocks[b_idx].accumulator)	{
		LOG("[Spectrometer] Error: Spec_Create could not allocate accumulator storage.");
		FFT_FREE(s->blocks[b_idx].oData);	s->blocks[b_idx].oData = NULL;
		FFT_FREE(s->blocks[b_idx].iData);	s->blocks[b_idx].iData = NULL;
		return false;
	}

	for (s_idx=0; s_idx<NUM_STREAMS; s_idx++){
		if (!__cellInit(s,b_idx,s_idx)){
			LOG("[Spectrometer] Error: Spec_Create could not initialize cell %d of block %d.",s_idx,b_idx);
			// rollback
			for (--s_idx; s_idx >= 0; s_idx--){
				__cellDestroy(s,b_idx,s_idx);
			}
			free(s->blocks[b_idx].accumulator);	s->blocks[b_idx].accumulator = NULL;
			FFT_FREE(s->blocks[b_idx].oData);		s->blocks[b_idx].oData = NULL;
			FFT_FREE(s->blocks[b_idx].iData);		s->blocks[b_idx].iData = NULL;
			return false;
		}
	}
	s->blocks[b_idx].state = BS_EMPTY;
	return true;
}
static inline void __blockDestroy(Spectrometer* s, int b_idx){
	assert(s!=NULL);
	assert(isValidBlock(b_idx));
	int s_idx=0;
	switch(s->blocks[b_idx].state){
	case BS_EMPTY:
	case BS_FILLING:
	case BS_DONE:
	case BS_PROCESSING:
	case BS_DROPPED:
		for (s_idx=0; s_idx<NUM_STREAMS; s_idx++){
			__cellDestroy(s,b_idx,s_idx);
		}
		free(s->blocks[b_idx].accumulator);	s->blocks[b_idx].accumulator = NULL;
		FFT_FREE(s->blocks[b_idx].oData);		s->blocks[b_idx].oData = NULL;
		FFT_FREE(s->blocks[b_idx].iData);		s->blocks[b_idx].iData = NULL;
	case BS_INVALID:
	default:
		s->blocks[b_idx].state = BS_INVALID;
	}
}
static inline bool __spectrometerInit(Spectrometer* s){
	//TRACE();
	assert(s!=NULL);
	int b_idx;
	s->in      = 0;
	s->f_start = 0;
	s->p_start = 0;
	s->out     = 0;
	for (b_idx=0; b_idx<NUM_BLOCKS; b_idx++){
		if (!__blockInit(s,b_idx)){
			LOG("[Spectrometer] Error: Spec_Create could not initialize block %d.",b_idx);
			for(--b_idx; b_idx >= 0; b_idx++){
				__blockDestroy(s,b_idx);
			}
			return false;
		}
	}
	return true;
}
static inline void __spectrometerDestroy(Spectrometer* s){
	//TRACE();
	assert(s!=NULL);
	int b_idx;
	s->in      = 0;
	s->f_start = 0;
	s->p_start = 0;
	s->out     = 0;
	for (b_idx=0; b_idx<NUM_BLOCKS; b_idx++){
		__blockDestroy(s,b_idx);
	}
}



// use
static inline void __blockSetSetup(Spectrometer* s, int b_idx, DrxFrame* frame,uint64_t timeTag0){
	assert(s!=NULL);
	assert(frame!=NULL);
	assert(isValidBlock(b_idx));
	//assert(s->blocks[b_idx].state == BS_EMPTY);
	if (s->blocks[b_idx].state != BS_EMPTY){
		printf("STATE = '%s'\n",BSNAMES[s->blocks[b_idx].state]);
		assert(s->blocks[b_idx].state == BS_EMPTY);
	}

	s->blocks[b_idx].setup.beam      		= frame->header.drx_beam;
	s->blocks[b_idx].setup.decFactor 		= frame->header.decFactor;
	s->blocks[b_idx].setup.timeOffset      = frame->header.timeOffset;
	s->blocks[b_idx].setup.timeTagStep     = frame->header.decFactor * DRX_SAMPLES_PER_FRAME;
	s->blocks[b_idx].setup.timeTag0		= timeTag0;
	s->blocks[b_idx].setup.timeTagN		= timeTag0 + ((FRAMES_PER_BLOCK((&s->options))-1) * s->blocks[b_idx].setup.timeTagStep);
	// unrolled forloop 0..NUM_TUNINGS-1
	s->blocks[b_idx].setup.freqCode[0]     = FREQ_CODE_UNINITIALIZED;
	s->blocks[b_idx].setup.freqCode[1]     = FREQ_CODE_UNINITIALIZED;
	s->blocks[b_idx].setup.stepPhase       = STEP_PHASE_DRX(frame->header.timeTag,frame->header.decFactor);
	s->blocks[b_idx].setup.freqCode[T_IDX(frame->header.drx_tuning)] =
			frame->header.freqCode;
	s->blocks[b_idx].state = BS_FILLING;

}

static inline void __blockUpdateSetup(Spectrometer* s, int b_idx, DrxFrame* frame){
	assert(s!=NULL);
	assert(frame!=NULL);
	assert(isValidBlock(b_idx));
	assert(s->blocks[b_idx].state == BS_FILLING);
	if(unlikely( s->blocks[b_idx].setup.freqCode[T_IDX(frame->header.drx_tuning)] == FREQ_CODE_UNINITIALIZED)){
		s->blocks[b_idx].setup.freqCode[T_IDX(frame->header.drx_tuning)] = frame->header.freqCode;
	}
}

//reuse
static inline void __cellReset(Spectrometer* s, int b_idx, int s_idx){
	assert(s!=NULL);
	assert(isValidBlock(b_idx));
	assert(isValidStream(s_idx));
	assert(s->blocks[b_idx].cells[s_idx].state  != CS_INVALID    );
	assert(s->blocks[b_idx].cells[s_idx].state  != CS_RUNNING    );
	void* ignored=NULL;
	if (s->blocks[b_idx].cells[s_idx].thread != (pthread_t) 0 ){
		pthread_join(s->blocks[b_idx].cells[s_idx].thread,&ignored);
	}
	s->blocks[b_idx].cells[s_idx].thread   = (pthread_t) 0 ;
	s->blocks[b_idx].cells[s_idx].fill     = 0;
	s->blocks[b_idx].cells[s_idx].satCount = 0;
	s->blocks[b_idx].cells[s_idx].state	   = CS_NOT_STARTED;
}

static inline void __blockReset(Spectrometer* s, int b_idx){
	assert(s!=NULL);
	assert(isValidBlock(b_idx));
	assert(s->blocks[b_idx].state != BS_INVALID);
	// unrolling for loop from 0 to NUM_STREAMS
	__cellReset(s, b_idx, 0 /*s_idx*/);
	__cellReset(s, b_idx, 1 /*s_idx*/);
	__cellReset(s, b_idx, 2 /*s_idx*/);
	__cellReset(s, b_idx, 3 /*s_idx*/);
	s->blocks[b_idx].state = BS_EMPTY;
	s->blocks[b_idx].isPrimer = false;
}

// tests
static inline bool __blockSetupMatches(Spectrometer* s, int b_idx, DrxFrame* frame){
	assert(s!=NULL);
	assert(frame!=NULL);
	assert(isValidBlock(b_idx));
	assert(s->blocks[b_idx].state != BS_EMPTY);
	assert(s->blocks[b_idx].state != BS_INVALID);
	return (
			( s->blocks[b_idx].setup.decFactor         		== frame->header.decFactor ) &&
			( s->blocks[b_idx].setup.beam              		== frame->header.drx_beam  ) &&
			( s->blocks[b_idx].setup.timeOffset        		== frame->header.timeOffset) &&
			( s->blocks[b_idx].setup.stepPhase        			== STEP_PHASE_DRX(frame->header.timeTag, frame->header.decFactor))  &&
			(
					( s->blocks[b_idx].setup.freqCode[T_IDX(frame->header.drx_tuning)] == FREQ_CODE_UNINITIALIZED  ) ||
					( s->blocks[b_idx].setup.freqCode[T_IDX(frame->header.drx_tuning)] == frame->header.freqCode   )
			)
	);
}
/*
static inline bool __frameTimeInBlock(Spectrometer* s, int b_idx, DrxFrame* frame){
	return (frame->header.timeTag >= s->blocks[b_idx].setup.timeTag0) && (frame->header.timeTag <= s->blocks[b_idx].setup.timeTagN);
}
static inline bool __frameTimePreceedsBlock(Spectrometer* s, int b_idx, DrxFrame* frame){
	return (frame->header.timeTag < s->blocks[b_idx].setup.timeTag0);
}
static inline uint64_t __frameDeltaPre(Spectrometer* s, int b_idx, DrxFrame* frame){
	uint64_t delta = (s->blocks[b_idx].setup.timeTag0 - frame->header.timeTag);
	return delta;
}
static inline uint64_t __frameDeltaPost(Spectrometer* s, int b_idx, DrxFrame* frame){
	uint64_t delta = (frame->header.timeTag - s->blocks[b_idx].setup.timeTag0);
	return delta;
}
*/
static inline Order __getOrder(Spectrometer* s, int b_idx, DrxFrame* frame){
	assert(s!=NULL);
	assert(frame!=NULL);
	assert(isValidBlock(b_idx));
	assert(s->blocks[b_idx].state != BS_EMPTY);
	assert(s->blocks[b_idx].state != BS_INVALID);
	if (frame->header.timeTag >= s->blocks[b_idx].setup.timeTag0)
		if (frame->header.timeTag <= s->blocks[b_idx].setup.timeTagN)
			return ORDER_WITHIN;
		else
			return ORDER_AFTER;
	else
		return ORDER_BEFORE;

}

static inline bool __blockIsFull(Spectrometer* s, int b_idx){
	return (
			(s->blocks[b_idx].cells[0].fill == s->options.nInts) &&
			(s->blocks[b_idx].cells[1].fill == s->options.nInts) &&
			(s->blocks[b_idx].cells[2].fill == s->options.nInts) &&
			(s->blocks[b_idx].cells[3].fill == s->options.nInts)
	);
}

static inline void __logDroppedBlock(Spectrometer* s, int b_idx,DropCode code){
	//TRACE();
	assert(s!=NULL);
	assert(isValidBlock(b_idx));
	switch(code){
	case DC_THREAD_CREATION:
		LOG("[Spectrometer] Error: Dropped block due to inability to create processing threads");
		return;
	default:
		LOG("[Spectrometer] Warning: Dropped block for unknown reason");
		return;
	}
}
static inline void __logDroppedFrame(DrxFrame* frame,DropCode code){
	//TRACE();
	assert(frame!=NULL);
	switch(code){
	case DC_BEFORE:
	case DC_NOT_FILLING:
		LOG("[Spectrometer] Warning: Dropped frame which arrived after its block had started (time=%lu, beam=%hhu, tuning=%hhu, pol=%hhu).", frame->header.timeTag, frame->header.drx_beam, frame->header.drx_tuning, frame->header.drx_polarization);
		return;
	default:
		LOG("[Spectrometer] Warning: Dropped frame for unknown reason (time=%lu, beam=%hhu, tuning=%hhu, pol=%hhu).", frame->header.timeTag, frame->header.drx_beam, frame->header.drx_tuning, frame->header.drx_polarization);
	}
}


static inline void __dropBlock(Spectrometer* s, int b_idx, DropCode dc){
	//TRACE();
	assert(s!=NULL);
	assert(isValidBlock(b_idx));
	assert(
			(s->blocks[b_idx].state == BS_FILLING)    ||
			(s->blocks[b_idx].state == BS_PROCESSING)
	);
	s->blocks[b_idx].state = BS_DROPPED;
	__logDroppedBlock(s,b_idx, dc);
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
static inline void __printCell(Spectrometer* s, int b_idx, int s_idx){
	int n=0;int i=0;
	for (i=0;i<s->options.nInts; i++){
		for (n=0;n<s->options.nFreqChan; n++){
			printf("%+2.2f %+2.2f ",
					s->blocks[b_idx].cells[s_idx].iData[i*s->options.nFreqChan + n].re,
					s->blocks[b_idx].cells[s_idx].iData[i*s->options.nFreqChan + n].im
					);
		}
		printf("\n");
	}
}
static inline void __printCellAt(Spectrometer* s, int b_idx, int s_idx,int ofs){
	int n=0;int i=0;
	for (i=0;i<s->options.nInts; i++){
		for (n=0;n<s->options.nFreqChan; n++){
			printf("%+2.2f %+2.2f ",
					s->blocks[b_idx].cells[s_idx].iData[i*s->options.nFreqChan + n + ofs].re,
					s->blocks[b_idx].cells[s_idx].iData[i*s->options.nFreqChan + n + ofs].im
					);
		}
		printf("\n");
	}
}
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
static inline void __unpack(Spectrometer* s, int b_idx, DrxFrame* frame){
	//static int cnt=0;
	//PROFILE_START(TMR_X_UNPACK);
	int bi  = b_idx;
	int ci  = F_STREAM_INDEX(frame);
	int ofs = SBF_IDATA_OFFSET(s,bi,frame);
	//printf("unpacking %lu into block w/ tt0 %lu (dt=%lu (df=%lu)) at address %d (decFactor=%hu)\n", frame->header.timeTag, s->blocks[bi].setup.timeTag0, (frame->header.timeTag- s->blocks[bi].setup.timeTag0), (frame->header.timeTag- s->blocks[bi].setup.timeTag0)/s->blocks[bi].setup.timeTagStep, ofs, s->blocks[bi].setup.decFactor);
	UnpackedSample* where =	&s->blocks[bi].cells[ci].iData[ofs];
	uint32_t i=0;
	for (i=0; i<DRX_SAMPLES_PER_FRAME; i++){
		where[i].i = LUT[frame->samples[i].packed].i;
		where[i].q = LUT[frame->samples[i].packed].q;
		s->blocks[bi].cells[ci].satCount += SatLUT[frame->samples[i].packed];
	}
	//__checkIsCVframe(frame);
	/*__printFrame(frame);
	__printCellAt(s,bi,ci,ofs);
	if (++cnt > 15) exit(-1);
	*/
	s->blocks[bi].cells[ci].fill += (DRX_SAMPLES_PER_FRAME / (uint32_t) s->options.nFreqChan);
	__blockUpdateSetup(s,b_idx,frame);
	if (__blockIsFull(s,b_idx)){
		s->counters.blocksStartedFull++;
		__blockStart(s,b_idx);
	}
	//PROFILE_STOP(TMR_X_UNPACK);

}



Spectrometer* Spec_Create			(SpectrometerOptions* options){
	//PROFILE_START(TMR_SPEC_CREATE);

	if (!options){
		LOG("[Spectrometer] Error: invalid options pointer supplied to Spec_Create.");
		profilerStop(TMR_SPEC_CREATE);
		return NULL;
	}
	Spectrometer* s = (Spectrometer*) malloc(sizeof(Spectrometer));
	if (!s) {
		LOG("[Spectrometer] Error: Spec_Create could not allocate spectrometer structure.");
		//PROFILE_STOP(TMR_SPEC_CREATE);
		return NULL;
	}
	bzero((void*)s,sizeof(Spectrometer));
	s->options = *options;

	if (pthread_attr_init(&s->attr)){
		LOG("[Spectrometer] Error: Spec_Create could not set up thread attributes.");
		free(s); return NULL;
	}
	if (pthread_attr_setdetachstate(&s->attr, PTHREAD_CREATE_JOINABLE)){
		LOG("[Spectrometer] Error: Spec_Create could not set up thread attributes.");
		pthread_attr_destroy(&s->attr);
		free(s); return NULL;
	}
	if (pthread_attr_setscope(&s->attr, PTHREAD_SCOPE_SYSTEM)){
		LOG("[Spectrometer] Error: Spec_Create could not set up thread attributes.");
		pthread_attr_destroy(&s->attr);
		free(s); return NULL;
	}
	if (pthread_attr_setschedpolicy(&s->attr, SCHED_FIFO)){
		LOG("[Spectrometer] Error: Spec_Create could not set up thread attributes.");
		pthread_attr_destroy(&s->attr);
		free(s); return NULL;
	}



	__initLUT();
	if (!__spectrometerInit(s)){
		LOG("[Spectrometer] Error: Spec_Create could not initialize spectrometer.");
		free(s);
		//PROFILE_STOP(TMR_SPEC_CREATE);
		return NULL;
	}
	LOG("[Spectrometer]: Spectrometer created.");
	//PROFILE_STOP(TMR_SPEC_CREATE);
	return s;
}


static void __DoCalc(CalcCell* cell) {
	assert(cell!=NULL);
	int ints  = cell->options.nInts;
	int freqs = cell->options.nFreqChan;
	int f;			// frequency channel iterator
	int i;			// integration iterator
	int ii = 0;		// indexing convenience, always equals i*freqs
	uint32_t mask = freqs >> 1;
	// perform FFT
	fftwf_execute(cell->plan);


	//integrate results
	for(i=0,ii=0; i < ints; i++, ii+=freqs){
		for(f=0; f<freqs; f++){
			RealType re = cell->oData[ii + f].re;
			RealType im = cell->oData[ii + f].im;
			if (unlikely(i==0)){
				cell->accumulator[f^mask] = (re*re) + (im*im);
			} else {
				cell->accumulator[f^mask] += (re*re) + (im*im);
			}
		}
	}
	// clear input for reuse
	bzero((void*) cell->iData, O_IDATA_SIZE_BYTES((&(cell->options))));
	cell->state = CS_DONE;
}

static void* __CalcWrapper(void* arg){
	assert(arg!=NULL);
	//PROFILE_START(TMR_THREAD_0 + ((CalcCell*) arg)->id);
	//struct timeval start, stop;
	//gettimeofday(&start, NULL);
	__DoCalc((CalcCell*) arg);
	//PROFILE_STOP(TMR_THREAD_0 + ((CalcCell*) arg)->id);
	//gettimeofday(&stop, NULL);
	//size_t elapsed = ((size_t)stop.tv_sec * 1000000l + (size_t)stop.tv_usec) -  ((size_t)start.tv_sec * 1000000l + (size_t)start.tv_usec);
	//printf("thread %d completed in %lu us.\n",((ThreadInfo*) arg)->s_idx, elapsed);
	return NULL;
}
/*
static inline void __writeCell(Spectrometer* s, int b_idx, int s_idx){

	char buf[16384];
	sprintf(buf,
			"==========================================================================\n"
			"== Block %4d   Cell %1d   | dec:%6hu, to:%6hu\n"
			"==  beam %1hhu / tuning %1hhu / pol %1hhu\n"
			"==  f0 %12u, f1 %12u\n"
			"==  T0 %15lu, Tn %15lu, Ts %15lu, Sp %15lu\n"
			"==========================================================================\n",
			b_idx,
			s_idx,
			s->blocks[b_idx].setup.decFactor,
			s->blocks[b_idx].setup.timeOffset,

			s->blocks[b_idx].setup.freqCode[0],
			s->blocks[b_idx].setup.freqCode[1],

			s->blocks[b_idx].setup.beam,
			((s_idx >> 2)+1),
			(s_idx & 0x1),
			s->blocks[b_idx].setup.timeTag0,
			s->blocks[b_idx].setup.timeTagN,
			s->blocks[b_idx].setup.timeTagStep,
			s->blocks[b_idx].setup.stepPhase
	);
	if (write(cell_fd,(void*)buf,strlen(buf)) != strlen(buf)){
		printf("Write Error...\n");
		exit(-1);
	}

	int n=0;
	int i=0;
	for (i=0;i<s->options.nInts; i++){

		for (n=0;n<s->options.nFreqChan; n++){
			sprintf(buf,"%2.2f %+2.2fi, ",
					s->blocks[b_idx].cells[s_idx].iData[i*s->options.nFreqChan + n].re,
					s->blocks[b_idx].cells[s_idx].iData[i*s->options.nFreqChan + n].im
					);
			if (write(cell_fd,(void*)buf,strlen(buf)) != strlen(buf)){
				printf("Write Error...\n");
				exit(-1);
			}
		}
		sprintf(buf,"\n");
		if (write(cell_fd,(void*)buf,strlen(buf)) != strlen(buf)){
			printf("Write Error...\n");
			exit(-1);
		}
	}
	close(cell_fd);
	printf("Cell written to file...\n");
}
*/

static inline void __cellStart(Spectrometer* s, int b_idx, int s_idx){
	//static int cnt = 0;
	assert(s!=NULL);
	assert(isValidBlock(b_idx));
	assert(isValidStream(s_idx));
	assert(s->blocks[b_idx].cells[s_idx].state == CS_NOT_STARTED);
	assert(s->blocks[b_idx].cells[s_idx].thread == (pthread_t) 0);
	s->blocks[b_idx].cells[s_idx].state = CS_RUNNING;
	/*
	if (++cnt==10000){
		__writeCell(s,b_idx,s_idx);
	}*/
	if (s->blocks[b_idx].cells[s_idx].fill >= s->blocks[b_idx].cells[s_idx].options.minimumFill){
		//__printCell(s,b_idx,s_idx);
		//exit(-1);
		if( pthread_create(
			&s->blocks[b_idx].cells[s_idx].thread,
			&s->attr,
			__CalcWrapper, (void*) &s->blocks[b_idx].cells[s_idx]))
		{
				LOG("[Spectrometer] Error: Could not start thread %d of block %d, pthread_create reported '%s'.", s_idx, b_idx, strerror(errno));
				s->blocks[b_idx].cells[s_idx].state = CS_ERROR;
				s->counters.cellsDroppedFailedStart++;
		} else {
			s->counters.cellsStarted++;
		}
	} else {
		if (!s->blocks[b_idx].isPrimer){
			LOG("[Spectrometer] Notice: Could not start thread %d of block %d due to underfilling (fill = %u).", s_idx, b_idx, s->blocks[b_idx].cells[s_idx].fill);
		}
		s->blocks[b_idx].cells[s_idx].state = CS_ERROR;
		s->counters.cellsDroppedUnderfull++;
	}
}

static inline void __advanceFillPointer(Spectrometer* s){
	while(	(s->f_start != s->in) &&(	(s->blocks[s->f_start].state == BS_PROCESSING) || (s->blocks[s->f_start].state == BS_DROPPED)	))	{
			QUEUE_INCREMENT(s->f_start, NUM_BLOCKS);
	}
}
static inline void __advanceProcessingPointer(Spectrometer* s){
	while(	(s->p_start != s->f_start) &&(	(s->blocks[s->p_start].state == BS_DONE) || (s->blocks[s->p_start].state == BS_DROPPED)	))	{
			QUEUE_INCREMENT(s->p_start, NUM_BLOCKS);
	}
}
static inline void __advanceOutPointer(Spectrometer* s){
	while(	(s->out != s->p_start) && ( (s->blocks[s->out].state == BS_DROPPED)	))	{
		if(s->blocks[s->out].state == BS_DROPPED){
			LOG("@@@@@@@@@@ dismissed dropped block %d.",s->out);
		}
		__blockReset(s,s->out);
		QUEUE_INCREMENT(s->out, NUM_BLOCKS);
	}
}

static inline void __blockStart(Spectrometer* s,int b_idx){
	assert(s!=NULL);
	assert(isValidBlock(b_idx));
	assert(s->blocks[b_idx].state == BS_FILLING);


	// unrolling for loop from 0 to NUM_STREAMS
	__cellStart(s, b_idx, 0 /*s_idx*/);
	__cellStart(s, b_idx, 1 /*s_idx*/);
	__cellStart(s, b_idx, 2 /*s_idx*/);
	__cellStart(s, b_idx, 3 /*s_idx*/);
	s->blocks[b_idx].state = BS_PROCESSING;
	// advance the filling start partition
	s->counters.blocksStarted++;
	if (b_idx == s->f_start){
		__advanceFillPointer(s);
	}
}

StatusCode    Spec_Insert			(Spectrometer* s, DrxFrame* frame){
	//PROFILE_START(TMR_SPEC_INSERT);
	int b_idx = 0;
	int bprev;
	uint64_t timeTag0;
	for (b_idx = s->f_start; b_idx != s->in; QUEUE_INCREMENT(b_idx, NUM_BLOCKS)){
		if (__blockSetupMatches(s,b_idx,frame)){
			switch(__getOrder(s,b_idx,frame)){
			case ORDER_BEFORE:
				if (b_idx == s->f_start){
					__logDroppedFrame(frame,DC_BEFORE);
					//PROFILE_STOP(TMR_SPEC_INSERT);
					s->counters.framesDroppedArrivedLate++;
					return SUCCESS;
				} else {
					// possible out-of order on the block level
					continue;
				}
			case ORDER_WITHIN:
				if (s->blocks[b_idx].state == BS_FILLING){
					__unpack(s,b_idx,frame);
					s->counters.framesInsertedExisting++;
					s->counters.framesInserted++;
				} else {
					s->counters.framesDroppedArrivedLate++;
					__logDroppedFrame(frame,DC_NOT_FILLING);
				}
				//PROFILE_STOP(TMR_SPEC_INSERT);
				return SUCCESS;
			case ORDER_AFTER:
				continue;
			default:
				LOG("[Spectrometer]: Error: Reached unreachable code in file '%s' at line %d. Program error detected.",__FILE__, __LINE__);
			}
		} else {
			continue;
		}
	}
	if (QUEUE_FULL(s->in, s->out, NUM_BLOCKS)){
		//PROFILE_STOP(TMR_SPEC_INSERT);
		return NOT_READY;
	} else {
		b_idx = s->in;
		timeTag0   = frame->header.timeTag;
		if (!QUEUE_EMPTY(s->in, s->out, NUM_BLOCKS)){
			bprev = QUEUE_PRED(b_idx, NUM_BLOCKS);
			if (s->blocks[bprev].state != BS_EMPTY)
				if (__blockSetupMatches(s,bprev,frame)){
					uint64_t blockStep = s->blocks[bprev].setup.timeTagStep * FRAMES_PER_BLOCK((&(s->options)));
					uint64_t blockStepPhase = s->blocks[bprev].setup.timeTag0 % blockStep;
					timeTag0 = ((((timeTag0-blockStepPhase) / blockStep) * blockStep)+blockStepPhase);
					s->counters.framesInsertedAltTimeTag++;
				}
		}
		__blockSetSetup(s,b_idx,frame,timeTag0);
		__unpack(s,b_idx,frame);
		if (s->counters.framesInsertedNew == 0){
			s->blocks[0].isPrimer = true;
		}
		if (s->counters.framesInsertedNew == 1){
			if (s->blocks[0].state == BS_FILLING)
				__blockStart(s,0);
		}
		s->counters.framesInsertedNew++;
		s->counters.framesInserted++;

		QUEUE_INCREMENT(s->in,NUM_BLOCKS);
		//PROFILE_STOP(TMR_SPEC_INSERT);
		return SUCCESS;
	}
}


void    Spec_Monitor			(Spectrometer* s){


	//PROFILE_START(TMR_SPEC_MONITOR);
	int blks_filling    = QUEUE_USED(s->in,      s->f_start, NUM_BLOCKS);
	int blks_processing = QUEUE_USED(s->f_start, s->p_start, NUM_BLOCKS);
	int b_idx;
	// check on partially filled blocks to start
	if (blks_filling > s->options.highWater){
		s->counters.blocksStartedHighWater++;
		__blockStart(s,s->f_start);
	}

	// check on processing blocks to see if they're done
	if (blks_processing){
		for (b_idx = s->p_start; b_idx != s->f_start; QUEUE_INCREMENT(b_idx,NUM_BLOCKS)){
			switch(s->blocks[b_idx].state){
			case BS_INVALID:
			case BS_EMPTY:
			case BS_FILLING:
				assert(false);
			case BS_DROPPED:
			case BS_DONE:
				continue;
			case BS_PROCESSING:
				if(__blockDone(s->blocks[b_idx])){
					s->counters.blocksCompleted++;
					s->blocks[b_idx].state = BS_DONE;
				}
			}
		}
		// advance the processing start partition
		__advanceProcessingPointer(s);
	}
	//PROFILE_STOP(TMR_SPEC_MONITOR);
}

StatusCode    Spec_NextResult		(Spectrometer* s, SpecDataBlock** result){
	//PROFILE_START(TMR_SPEC_NEXT);

	assert(s!= NULL);
	assert(result!= NULL);
	// discard any dropped blocks at the head of the queue
	__advanceOutPointer(s);

	// check to see if we have any data
	if (unlikely(QUEUE_EMPTY(s->p_start, s->out, NUM_BLOCKS))){
		//PROFILE_STOP(TMR_SPEC_NEXT);
		return NOT_READY;
	} else {
		assert(s->blocks[s->out].state == BS_DONE);
		*result = & s->blocks[s->out];
		s->counters.spectraRequested++;
		//PROFILE_STOP(TMR_SPEC_NEXT);
		return SUCCESS;
	}
}

StatusCode    Spec_ReleaseResult	(Spectrometer* s, SpecDataBlock* result){
	//PROFILE_START(TMR_SPEC_RELEASE);
	assert(result == & s->blocks[s->out]);
	assert(result->state == BS_DONE);
	__blockReset(s,s->out);
	s->counters.spectraReleased++;
	QUEUE_INCREMENT(s->out,NUM_BLOCKS);
	//PROFILE_STOP(TMR_SPEC_RELEASE);
	return SUCCESS;
}


void          Spec_Destroy			(Spectrometer** s){
	//PROFILE_START(TMR_SPEC_DESTROY);
	if (s){
		if (*s){
			__spectrometerDestroy(*s);
			*s = NULL;
			LOG("[Spectrometer]: Spectrometer destroyed.");
		} else {
			LOG("[Spectrometer]: Error: Tried to destroy spectrometer, but pointer was NULL.");
		}
	} else {
		LOG("[Spectrometer]: Error: Tried to destroy spectrometer, but pointer to pointer was NULL.");
	}
	//PROFILE_STOP(TMR_SPEC_DESTROY);
}




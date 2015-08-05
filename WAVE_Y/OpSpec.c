/*
 * OpSpec.c
 *
 *  Created on: Jan 29, 2012
 *      Author: chwolfe2
 */
#include "OpSpec.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <malloc.h>
#include <unistd.h>

#include <time.h>

#include "Globals.h"
#include "Spectrometer.h"
#include "Profiler.h"

#ifdef DEBUG_SPECTROMETER
	#include <assert.h>
#else
	#define assert(x)
#endif

#define TV_TO_DBL_US(a) ((((double)a.tv_sec) * 1000000.0f) + ((double)a.tv_usec))
#define TV_ELAPSED(a,b) ((TV_TO_DBL_US(b) - TV_TO_DBL_US(a)) / 1000000.0f)
#define SPEC_MAX_FREQ_CHANNELS 4096


static int spec_fd = -1;
//int cell_fd = -1;

static Spectrometer * spec = NULL;
static SpectrometerOptions options;
static DrxFrame inFrame;

static SpecDataBlock* outBlock         = NULL;
static bool      outBlockOccupied = false;
static bool      inFrameOccupied  = false;
static bool      spectraOccupied  = false;
static bool      ioerror          = false;
static bool 	  started 		   = false;



static char spectra[sizeof(DrxSpectraHeader) + (SPEC_MAX_FREQ_CHANNELS * sizeof(RealType) * NUM_STREAMS)];
static DrxSpectraHeader* specHdr = (DrxSpectraHeader*) spectra;
static size_t spectraSize;

static struct timeval specStart;
static double specNextReport = 0;
static double specTime =0.0;

static inline void __reportSpecStats(){
	double wbw = ((double)globals.opData.opFileCpyDmpCurrentPos) / (1024.0 * specTime);
	double cbw = ((double)spec->counters.blocksCompleted) / (specTime);

	printf("==========================================================================================\n");
	printf("== Spectrometer Report:       Frequency Ch. %6u   Integration count %8u         ==\n",spec->options.nFreqChan,spec->options.nInts);
	printf("==                                                                                      ==\n");
	printf("==  Frames                                                                              ==\n");
	printf("==    %9u / %9u / %9u               (inserted / valid / invalid)      ==\n",spec->counters.framesInserted,spec->counters.framesReceivedGood,spec->counters.framesReceivedInvalid);
	printf("==    %9u / %9u / %9u               (start / join / dropped-late)     ==\n",spec->counters.framesInsertedNew, spec->counters.framesInsertedExisting,spec->counters.framesDroppedArrivedLate);
	printf("==    %9u                                       (using old timetag phase)         ==\n",spec->counters.framesInsertedAltTimeTag);
	printf("==  Blocks                                                                              ==\n");
	printf("==    %9u / %9u / %9u               (started / completed / dropped)   ==\n",spec->counters.blocksStarted,spec->counters.blocksCompleted,spec->counters.blocksDropped);
	printf("==    %9u / %9u                           (full-start / highwater-start)    ==\n",spec->counters.blocksStartedFull,spec->counters.blocksStartedHighWater);
	printf("==  Cells                                                                               ==\n");
	printf("==    %9u / %9u / %9u               (started / fail-start / dropped)  ==\n",spec->counters.cellsStarted,spec->counters.cellsDroppedFailedStart,spec->counters.cellsDroppedUnderfull);
	printf("==  Spectra                                                                             ==\n");
	printf("==    %9u / %9u                           (requested / prepped )            ==\n",spec->counters.spectraRequested,spec->counters.spectraPrepped);
	printf("==    %9u / %9u                           (written / released )             ==\n",spec->counters.spectraWritten,spec->counters.spectraReleased);
	printf("==  Queue                                                                               ==\n");
	printf("==    %9u / %9u                           (used / size)                     ==\n",QUEUE_USED(spec->in,spec->out,NUM_BLOCKS),NUM_BLOCKS);
	printf("==    %9u                                       (filling)                         ==\n",QUEUE_USED(spec->in,spec->f_start,NUM_BLOCKS));
	printf("==    %9u                                       (processing)                      ==\n",QUEUE_USED(spec->f_start,spec->p_start,NUM_BLOCKS));
	printf("==    %9u                                       (done or error)                   ==\n",QUEUE_USED(spec->p_start,spec->out,NUM_BLOCKS));
	printf("==                                                                                      ==\n");
	printf("==  Runtime: %9.2lfs     Write B/W: %9.4lf KB/s     Compute B/W: %9.4lf Blk/s  ==\n", specTime, wbw, cbw);
	printf("==========================================================================================\n");

}
/*
static inline void __printFrame(){
	static int cnt=0;
	if (cnt++ != 300) return;
	__reportSpecStats();
	int i=0;
	DrxFrame*f = &inFrame;

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
	for(i=0; i<4096; i++){
		printf("%+ 2hhd %+ 2hhd ", (int)f->samples[i].i, (int)f->samples[i].q);
		if ((i & 0xf) == 0xf){
			printf("\n");
		}
	}
	printf("==============================================================================\n");
	exit(-1);

}

*/
StatusCode OpSpecInit(){
	//PROFILE_START(TMR_OPSPEC_INIT);
	//TRACE();

	bzero((void*) &inFrame, sizeof(DrxFrame));
	inFrameOccupied=false;

	outBlock = NULL;
	outBlockOccupied = false;

	spectraSize = sizeof(DrxSpectraHeader) + (globals.opData.TransformLength * sizeof(RealType) * NUM_STREAMS);
	bzero((void*) &spectra, spectraSize);
	spectraOccupied = false;

	ioerror=false;

	bzero((void*)&options, sizeof(SpectrometerOptions));
	options.highWater   = (uint64_t) globals.opData.highWater;
	options.minimumFill = (uint64_t) globals.opData.minFill;
	options.nFreqChan   = globals.opData.TransformLength;
	options.nInts		= globals.opData.IntegrationCount;

	char spec_name[1024];
	sprintf(spec_name,"/LWADATA/%06lu_%09lu_spectrometer.dat",globals.opData.opStartMJD,globals.opData.opReference);
	spec_fd = open(spec_name,  O_WRONLY | O_TRUNC | O_CREAT | O_APPEND , S_IROTH | S_IRUSR |  S_IRGRP | S_IWOTH | S_IWUSR |  S_IWGRP );
	if (spec_fd==-1){
		Log_Add("[OPERATION SPECTROMETER] Error: couldn't open spectrogram file.");
		//PROFILE_STOP(TMR_OPSPEC_INIT);
		return FAILURE;
	}
	///// DEBUG
	//cell_fd = open("/LWADATA/cell.dat",  O_WRONLY | O_TRUNC | O_CREAT | O_APPEND , S_IROTH | S_IRUSR |  S_IRGRP | S_IWOTH | S_IWUSR |  S_IWGRP );
	//if (spec_fd==-1){
	//	exit(-1);
	//}
	///// DEBUG
	spec = Spec_Create(&options);
	if (!spec){
		Log_Add("[OPERATION SPECTROMETER] Error: couldn't create spectrometer object.");
		close(spec_fd); spec_fd=-1;
		//PROFILE_STOP(TMR_OPSPEC_INIT);
		return FAILURE;
	}
	globals.opData.opStatus =OP_STATUS_INITIALIZED;
	globals.opData.opFileCpyDmpCurrentPos = 0;
	Log_Add("[OPERATION SPECTROMETER] Initialized ...");
	//PROFILE_STOP(TMR_OPSPEC_INIT);
	specNextReport = 0;
	specTime = 0.0;
	started=false;
	return SUCCESS;
}

static inline void fixByteOrder(DrxFrame* frame){
	frame->header.freqCode   = __builtin_bswap32(frame->header.freqCode);
	frame->header.decFactor  = (frame->header.decFactor << 8)  | (frame->header.decFactor >> 8);
	frame->header.timeOffset = (frame->header.timeOffset << 8) | (frame->header.timeOffset >> 8);
	frame->header.timeTag    = __builtin_bswap64(frame->header.timeTag);
}

static inline bool verifyFrame(DrxFrame* frame){
	//PROFILE_START(TMR_OPSPEC_VERIFY);
	bool res = (
			!(
				(frame->header.syncCode != 0x5CDEC0DE)         			||
				((frame->header.frameCount & 0xFFFFFF00) != 0) 			||
				/*(frame->header.secondsCount != 0)              			||*/
				!isValidBeam(         frame->header.drx_beam)           ||
				!isValidPolarization( frame->header.drx_polarization)   ||
				!isValidTuning(       frame->header.drx_tuning)
			)
	);
	//PROFILE_STOP(TMR_OPSPEC_VERIFY);
	if (res){
		spec->counters.framesReceivedGood++;
	} else {
		spec->counters.framesReceivedInvalid++;
	}
	//__printFrame();
	return res;
}

static void prepOutBlock(){
	//PROFILE_START(TMR_OPSPEC_PREP);
	spec->counters.spectraPrepped++;
	specHdr->MAGIC1		= 0xC0DEC0DE;
	specHdr->nFreqs		= spec->options.nFreqChan;
	specHdr->nInts		= spec->options.nInts;
	specHdr->beam      	= outBlock->setup.beam;
	specHdr->decFactor 	= outBlock->setup.decFactor;
	specHdr->timeTag0  	= outBlock->setup.timeTag0;
	specHdr->timeOffset	= outBlock->setup.timeOffset;
	int i=0;
	for(i=0;i<NUM_STREAMS;i++){
		specHdr->errors[i] = outBlock->cells[i].state!=CS_DONE;
		if (specHdr->errors[i]){
			specHdr->fills[i] = 0;
			specHdr->satCount[i] = 0;
			bzero((void*)&spectra[sizeof(DrxSpectraHeader) + (i * specHdr->nFreqs * sizeof(RealType))], (specHdr->nFreqs * sizeof(RealType)) );
		} else {
			specHdr->fills[i] = outBlock->cells[i].fill;
			specHdr->satCount[i] = outBlock->cells[i].satCount;
			memcpy((void*)&spectra[sizeof(DrxSpectraHeader) + (i * specHdr->nFreqs * sizeof(RealType))], (void*)outBlock->cells[i].accumulator, (specHdr->nFreqs * sizeof(RealType)) );
		}
	}
	specHdr->freqCode[0] = outBlock->setup.freqCode[0];
	specHdr->freqCode[1] = outBlock->setup.freqCode[1];
	specHdr->MAGIC2		= 0xED0CED0C;
	//PROFILE_STOP(TMR_OPSPEC_PREP);
}

static bool writeSpectra(){
	//PROFILE_START(TMR_OPSPEC_WRITE);
	ssize_t res = write(spec_fd, (void*) spectra, spectraSize);
	if (res<0){
		if (unlikely(errno != EWOULDBLOCK)){
			Log_Add("[OPERATION SPECTROMETER] Error: Encountered I/O error writing to spectrogram file: '%s'", strerror(errno));
			ioerror=true;
		}
		//PROFILE_STOP(TMR_OPSPEC_WRITE);
		return false;
	} else if (unlikely(res == 0)){
		Log_Add("[OPERATION SPECTROMETER] wrote 0 bytes (header)");
		//PROFILE_STOP(TMR_OPSPEC_WRITE);
		return false;
	} else if (unlikely(res != spectraSize)){
		// we consider this an IO error, even though it's not technically
		Log_Add("[OPERATION SPECTROMETER] Error: couldn't write all of spectrogram; only %ld bytes written out of %ld bytes",res, spectraSize);
		ioerror=true;
		//PROFILE_STOP(TMR_OPSPEC_WRITE);
		return false;
	} else {
		globals.opData.opFileCpyDmpCurrentPos += spectraSize;
		spec->counters.spectraWritten++;
		//PROFILE_STOP(TMR_OPSPEC_WRITE);
		return true;
	}
}


StatusCode OpSpecRun(){
	StatusCode sc;
	struct timeval temp;
	if (!started){
		gettimeofday(&specStart, NULL);
		started=true;
	}
	gettimeofday(&temp, NULL);
	specTime = TV_ELAPSED(specStart,temp);
	if (specTime > specNextReport){
		__reportSpecStats();
		specNextReport += 3.0f;
	}



	//PROFILE_START(TMR_OPSPEC_RUN);

	if (ioerror) {
		//PROFILE_STOP(TMR_OPSPEC_RUN);
		return FAILURE;
	}

	if (!globals.opData.rxSocketOpen){
		Log_Add("[OPERATION SPECTROMETER] Opening socket on port %d",globals.DataInPort);
		if (Socket_OpenRecieve(globals.rxSocket,globals.DataInPort)!=SUCCESS){
			Log_Add("[OPERATION SPECTROMETER] Error: couldn't open data socket.");
			//PROFILE_STOP(TMR_OPSPEC_RUN);
			return FAILURE;
		} else {
			globals.opData.rxSocketOpen = true;
		}
	}
	if (!inFrameOccupied){
		size_t bytesReceived = 0;
		//PROFILE_START(TMR_OPSPEC_RECEIVE);
		sc = Socket_Recieve(globals.rxSocket, (char * ) &inFrame, &bytesReceived);
		//PROFILE_STOP(TMR_OPSPEC_RECEIVE);
		switch (sc){
		case SUCCESS:
			if (bytesReceived != sizeof(DrxFrame)){
				Log_Add("[OPERATION SPECTROMETER] Dropped packet of invalid size (%lu) bytes",bytesReceived);
				break;
			} else {
				/*****************************************************/
				// TT-LAG measurements
				globals.opData.rx_time_uspe_recent    =  getMicrosSinceEpoch(__builtin_bswap64(inFrame.header.timeTag));
				globals.opData.local_time_uspe_recent = getMicrosSinceEpoch(0);
				if (!globals.opData.lag_measure_initialized){
					globals.opData.lag_measure_initialized = 1;
					globals.opData.rx_time_uspe_first    = globals.opData.rx_time_uspe_recent;
					globals.opData.local_time_uspe_first = globals.opData.local_time_uspe_recent;
				}
				/*****************************************************/
				fixByteOrder(&inFrame);
				if (likely(verifyFrame(&inFrame))){
					inFrameOccupied=true;
				} else {
					Log_Add("[OPERATION SPECTROMETER] Dropped DRX packet with illegal values specified for sync-code, tuning, polarization, beam, seconds count, or frame count.");
				}

			}
		case NOT_READY:
			break;
		default:
		case FAILURE:
			Log_Add("[OPERATION SPECTROMETER] Error: socket error");
			//PROFILE_STOP(TMR_OPSPEC_RUN);
			return FAILURE;
		}
	}
	if (inFrameOccupied){
		//PROFILE_START(TMR_OPSPEC_INSERT);
		sc = Spec_Insert(spec,&inFrame);
		//PROFILE_STOP(TMR_OPSPEC_INSERT);
		switch(sc){
		case SUCCESS:
			inFrameOccupied=false;
			break;
		case NOT_READY: break;
		default:
		case FAILURE:
			Log_Add("[OPERATION SPECTROMETER] Error: reached unreachable code @ %s:%d",__FILE__,__LINE__);
			//PROFILE_STOP(TMR_OPSPEC_RUN);
			return FAILURE;
		}
	}
	//PROFILE_START(TMR_OPSPEC_MONITOR);
	Spec_Monitor(spec);
	//PROFILE_STOP(TMR_OPSPEC_MONITOR);
	if (!outBlockOccupied){
		//PROFILE_START(TMR_OPSPEC_NEXT);
		sc = Spec_NextResult(spec,&outBlock);
		//PROFILE_STOP(TMR_OPSPEC_NEXT);
		switch(sc){
		case NOT_READY: break;
		default:
		case FAILURE:
			Log_Add("[OPERATION SPECTROMETER] Error: reached unreachable code @ %s:%d",__FILE__,__LINE__);
			//PROFILE_STOP(TMR_OPSPEC_RUN);
			return FAILURE;
		case SUCCESS:
			outBlockOccupied=true;
			break;
		}
	}

	if (outBlockOccupied && !spectraOccupied){
		prepOutBlock();
		spectraOccupied  = true;
		outBlockOccupied = false;
	}

	if (spectraOccupied){
		if (writeSpectra()){
			spectraOccupied=false;
			//PROFILE_START(TMR_OPSPEC_RELEASE);
			sc = Spec_ReleaseResult(spec,outBlock);
			//PROFILE_STOP(TMR_OPSPEC_RELEASE);
			if (sc!=SUCCESS){
				Log_Add("[OPERATION SPECTROMETER] Error: reached unreachable code @ %s:%d",__FILE__,__LINE__);
				//PROFILE_STOP(TMR_OPSPEC_RUN);
				return FAILURE;
			}
		}
	}
	//PROFILE_STOP(TMR_OPSPEC_RUN);
	return NOT_READY;
}

StatusCode OpSpecCleanup(){
	//TRACE();
	//PROFILE_START(TMR_OPSPEC_CLEANUP);
	if (globals.opData.rxSocketOpen){
		Log_Add("[OPERATION SPECTROMETER] Closing socket on port %d",globals.DataInPort);
		Socket_Close(globals.rxSocket);
		globals.opData.rxSocketOpen = false;
	}

	if (spec_fd != -1) { close(spec_fd); spec_fd = -1; }
	//if (cell_fd != -1) { close(cell_fd); cell_fd = -1; }
	if (spec){
		//TRACE();
		Spec_Destroy(&spec);
	}
	Log_Add("[OPERATION SPECTROMETER] CleanedUp ...");
	globals.opData.opStatus = OP_STATUS_COMPLETE;
	globals.opData.opCleanupComplete = true;
	//PROFILE_STOP(TMR_OPSPEC_CLEANUP);
	return SUCCESS;
}

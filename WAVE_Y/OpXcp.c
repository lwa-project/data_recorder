/*
 * OpXcp.c
 *
 *  Created on: Jan 29, 2012
 *      Author: chwolfe2
 */
#include "OpXcp.h"
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
#include "BeamCorrelator.h"
#include "Profiler.h"

#ifdef DEBUG_XCP
	#include <assert.h>
#else
	#define assert(x)
#endif

#define XCP_MAX_SAMPLES_PER_BLOCK 4096
#define TV_TO_DBL_US(a) ((((double)a.tv_sec) * 1000000.0f) + ((double)a.tv_usec))
#define TV_ELAPSED(a,b) ((TV_TO_DBL_US(b) - TV_TO_DBL_US(a)) / 1000000.0f)


static int 						result_fd 		 = -1;
static BeamCorrelator * 		corr 			 = NULL;
static BeamCorrelatorOptions 	options;
static DrxFrame 				inFrame;
static XCPDataBlock* 			outBlock         = NULL;
static bool      				outBlockOccupied = false;
static bool     				inFrameOccupied  = false;
static bool      				resultsOccupied  = false;
static bool      				ioerror          = false;
static bool 	  				started 		 = false;

static char 					results [sizeof(DrxXcpResultsHeader) + (XCP_MAX_SAMPLES_PER_BLOCK * sizeof(ComplexType) * NUM_TUNINGS)];
static DrxXcpResultsHeader* 	resultsHdr 		 = (DrxXcpResultsHeader*) results;
static size_t 					resultsSize;
static struct timeval 			resultStart;
static double 					resultNextReport = 0;
static double 					resultTime 		 = 0.0;

static inline void __reportXcpStats(){
	double wbw = ((double)globals.opData.opFileCpyDmpCurrentPos) / (1024.0 * resultTime);
	double cbw = ((double)corr->counters.blocksCompleted) / (resultTime);

	printf("==========================================================================================\n");
	printf("== Beam Correlator Report:  Samples/Output frame %6u   Integration count %8u    ==\n",corr->options.nSamplesPerOutputFrame,corr->options.nInts);
	printf("==                                                                                      ==\n");
	printf("==  Frames                                                                              ==\n");
	printf("==    %9u / %9u / %9u               (inserted / valid / invalid)      ==\n",corr->counters.framesInserted,corr->counters.framesReceivedGood,corr->counters.framesReceivedInvalid);
	printf("==    %9u / %9u / %9u               (start / join / dropped-late)     ==\n",corr->counters.framesInsertedNew, corr->counters.framesInsertedExisting,corr->counters.framesDroppedArrivedLate);
	printf("==    %9u                                       (using old timetag phase)         ==\n",corr->counters.framesInsertedAltTimeTag);
	printf("==  Blocks                                                                              ==\n");
	printf("==    %9u / %9u                           (started / completed)             ==\n",corr->counters.blocksStarted,corr->counters.blocksCompleted);
	printf("==    %9u / %9u                           (failed-start / failed-underfull) ==\n",corr->counters.blocksDroppedFailedStart,corr->counters.blocksDroppedUnderfull);
	printf("==    %9u / %9u                           (full-start / highwater-start)    ==\n",corr->counters.blocksStartedFull,corr->counters.blocksStartedHighWater);
	printf("==  Results                                                                             ==\n");
	printf("==    %9u / %9u                           (requested / prepped )            ==\n",corr->counters.resultsRequested,corr->counters.resultsPrepped);
	printf("==    %9u / %9u                           (written / released )             ==\n",corr->counters.resultsWritten,corr->counters.resultsReleased);
	printf("==  Queue                                                                               ==\n");
	printf("==    %9u / %9u                           (used / size)                     ==\n",QUEUE_USED(corr->in,corr->out,NUM_BLOCKS),NUM_BLOCKS);
	printf("==    %9u                                       (filling)                         ==\n",QUEUE_USED(corr->in,corr->f_start,NUM_BLOCKS));
	printf("==    %9u                                       (processing)                      ==\n",QUEUE_USED(corr->f_start,corr->p_start,NUM_BLOCKS));
	printf("==    %9u                                       (done or error)                   ==\n",QUEUE_USED(corr->p_start,corr->out,NUM_BLOCKS));
	printf("==                                                                                      ==\n");
	printf("==  Runtime: %9.2lfs     Write B/W: %9.4lf KB/s     Compute B/W: %9.4lf Blk/s  ==\n", resultTime, wbw, cbw);
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
StatusCode OpXcpInit(){
	//PROFILE_START(TMR_OPXCP_INIT);
	//TRACE();

	bzero((void*) &inFrame, sizeof(DrxFrame));
	inFrameOccupied=false;

	outBlock = NULL;
	outBlockOccupied = false;

	resultsSize = sizeof(DrxXcpResultsHeader) + (globals.opData.TransformLength * sizeof(RealType) * NUM_STREAMS);
	bzero((void*) &results, resultsSize);
	resultsOccupied = false;

	ioerror=false;

	bzero((void*)&options, sizeof(BeamCorrelatorOptions));
	options.highWater               = (uint64_t) globals.opData.highWater;
	options.minimumFill             = (uint64_t) globals.opData.minFill;
	options.nSamplesPerOutputFrame  = globals.opData.TransformLength;
	options.nInts		            = globals.opData.IntegrationCount;

	char spec_name[1024];
	sprintf(spec_name,"/LWADATA/%06lu_%09lu_xcp.dat",globals.opData.opStartMJD,globals.opData.opReference);
	result_fd = open(spec_name,  O_WRONLY | O_TRUNC | O_CREAT | O_APPEND , S_IROTH | S_IRUSR |  S_IRGRP | S_IWOTH | S_IWUSR |  S_IWGRP );
	if (result_fd==-1){
		Log_Add("[OPERATION XCP] Error: couldn't open results file.");
		//PROFILE_STOP(TMR_OPXCP_INIT);
		return FAILURE;
	}
	///// DEBUG
	//cell_fd = open("/LWADATA/cell.dat",  O_WRONLY | O_TRUNC | O_CREAT | O_APPEND , S_IROTH | S_IRUSR |  S_IRGRP | S_IWOTH | S_IWUSR |  S_IWGRP );
	//if (result_fd==-1){
	//	exit(-1);
	//}
	///// DEBUG
	corr = XCP_Create(&options);
	if (!corr){
		Log_Add("[OPERATION XCP] Error: couldn't create beam correlator object.");
		close(result_fd); result_fd=-1;
		//PROFILE_STOP(TMR_OPXCP_INIT);
		return FAILURE;
	}
	globals.opData.opStatus =OP_STATUS_INITIALIZED;
	globals.opData.opFileCpyDmpCurrentPos = 0;
	Log_Add("[OPERATION XCP] Initialized ...");
	//PROFILE_STOP(TMR_OPXCP_INIT);
	resultNextReport = 0;
	resultTime = 0.0;
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
	//PROFILE_START(TMR_OPXCP_VERIFY);
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
	//PROFILE_STOP(TMR_OPXCP_VERIFY);
	if (res){
		corr->counters.framesReceivedGood++;
	} else {
		corr->counters.framesReceivedInvalid++;
	}
	//__printFrame();
	return res;
}

static void prepOutBlock(){
	//PROFILE_START(TMR_OPXCP_PREP);
	corr->counters.resultsPrepped++;
	resultsHdr->MAGIC1		            = 0xC0DEC0DE;
	resultsHdr->nSamplesPerOutputFrame	= corr->options.nSamplesPerOutputFrame;
	resultsHdr->nInts					= corr->options.nInts;
	resultsHdr->beam      				= outBlock->setup.beam;
	resultsHdr->decFactor 				= outBlock->setup.decFactor;
	resultsHdr->timeTag0  				= outBlock->setup.timeTag0;
	resultsHdr->timeOffset				= outBlock->setup.timeOffset;
	resultsHdr->satCount				= outBlock->satCount;

	size_t dest_offset = sizeof(DrxXcpResultsHeader);
	size_t src_offset  = 0;
	size_t length      = resultsHdr->nSamplesPerOutputFrame * sizeof(ComplexType) * NUM_TUNINGS;
	memcpy((void*)&results[dest_offset], (void*)&outBlock->accumulator[src_offset], length );
	resultsHdr->freqCode[0] = outBlock->setup.freqCode[0];
	resultsHdr->freqCode[1] = outBlock->setup.freqCode[1];
	resultsHdr->MAGIC2		= 0xED0CED0C;
	//PROFILE_STOP(TMR_OPXCP_PREP);
}

static bool writeResults(){
	//PROFILE_START(TMR_OPXCP_WRITE);
	ssize_t res = write(result_fd, (void*) results, resultsSize);
	if (res<0){
		if (unlikely(errno != EWOULDBLOCK)){
			Log_Add("[OPERATION XCP] Error: Encountered I/O error writing to results file: '%s'", strerror(errno));
			ioerror=true;
		}
		//PROFILE_STOP(TMR_OPXCP_WRITE);
		return false;
	} else if (unlikely(res == 0)){
		Log_Add("[OPERATION XCP] wrote 0 bytes (header)");
		//PROFILE_STOP(TMR_OPXCP_WRITE);
		return false;
	} else if (unlikely(res != resultsSize)){
		// we consider this an IO error, even though it's not technically
		Log_Add("[OPERATION XCP] Error: couldn't write all of results; only %ld bytes written out of %ld bytes",res, resultsSize);
		ioerror=true;
		//PROFILE_STOP(TMR_OPXCP_WRITE);
		return false;
	} else {
		globals.opData.opFileCpyDmpCurrentPos += resultsSize;
		corr->counters.resultsWritten++;
		//PROFILE_STOP(TMR_OPXCP_WRITE);
		return true;
	}
}


StatusCode OpXcpRun(){
	StatusCode sc;
	struct timeval temp;
	if (!started){
		gettimeofday(&resultStart, NULL);
		started=true;
	}
	gettimeofday(&temp, NULL);
	resultTime = TV_ELAPSED(resultStart,temp);
	if (resultTime > resultNextReport){
		__reportXcpStats();
		resultNextReport += 3.0f;
	}



	//PROFILE_START(TMR_OPXCP_RUN);

	if (ioerror) {
		//PROFILE_STOP(TMR_OPXCP_RUN);
		return FAILURE;
	}

	if (!globals.opData.rxSocketOpen){
		Log_Add("[OPERATION XCP] Opening socket on port %d",globals.DataInPort);
		if (Socket_OpenRecieve(globals.rxSocket,globals.DataInPort)!=SUCCESS){
			Log_Add("[OPERATION XCP] Error: couldn't open data socket.");
			//PROFILE_STOP(TMR_OPXCP_RUN);
			return FAILURE;
		} else {
			globals.opData.rxSocketOpen = true;
		}
	}
	if (!inFrameOccupied){
		size_t bytesReceived = 0;
		//PROFILE_START(TMR_OPXCP_RECEIVE);
		sc = Socket_Recieve(globals.rxSocket, (char * ) &inFrame, &bytesReceived);
		//PROFILE_STOP(TMR_OPXCP_RECEIVE);
		switch (sc){
		case SUCCESS:
			if (bytesReceived != sizeof(DrxFrame)){
				Log_Add("[OPERATION XCP] Dropped packet of invalid size (%lu) bytes",bytesReceived);
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
					Log_Add("[OPERATION XCP] Dropped DRX packet with illegal values specified for sync-code, tuning, polarization, beam, seconds count, or frame count.");
				}

			}
		case NOT_READY:
			break;
		default:
		case FAILURE:
			Log_Add("[OPERATION XCP] Error: socket error");
			//PROFILE_STOP(TMR_OPXCP_RUN);
			return FAILURE;
		}
	}
	if (inFrameOccupied){
		//PROFILE_START(TMR_OPXCP_INSERT);
		sc = XCP_Insert(corr,&inFrame);
		//PROFILE_STOP(TMR_OPXCP_INSERT);
		switch(sc){
		case SUCCESS:
			inFrameOccupied=false;
			break;
		case NOT_READY: break;
		default:
		case FAILURE:
			Log_Add("[OPERATION XCP] Error: reached unreachable code @ %s:%d",__FILE__,__LINE__);
			//PROFILE_STOP(TMR_OPXCP_RUN);
			return FAILURE;
		}
	}
	//PROFILE_START(TMR_OPXCP_MONITOR);
	XCP_Monitor(corr);
	//PROFILE_STOP(TMR_OPXCP_MONITOR);
	if (!outBlockOccupied){
		//PROFILE_START(TMR_OPXCP_NEXT);
		sc = XCP_NextResult(corr,&outBlock);
		//PROFILE_STOP(TMR_OPXCP_NEXT);
		switch(sc){
		case NOT_READY: break;
		default:
		case FAILURE:
			Log_Add("[OPERATION XCP] Error: reached unreachable code @ %s:%d",__FILE__,__LINE__);
			//PROFILE_STOP(TMR_OPXCP_RUN);
			return FAILURE;
		case SUCCESS:
			outBlockOccupied=true;
			break;
		}
	}

	if (outBlockOccupied && !resultsOccupied){
		prepOutBlock();
		resultsOccupied  = true;
		outBlockOccupied = false;
	}

	if (resultsOccupied){
		if (writeResults()){
			resultsOccupied=false;
			//PROFILE_START(TMR_OPXCP_RELEASE);
			sc = XCP_ReleaseResult(corr,outBlock);
			//PROFILE_STOP(TMR_OPXCP_RELEASE);
			if (sc!=SUCCESS){
				Log_Add("[OPERATION XCP] Error: reached unreachable code @ %s:%d",__FILE__,__LINE__);
				//PROFILE_STOP(TMR_OPXCP_RUN);
				return FAILURE;
			}
		}
	}
	//PROFILE_STOP(TMR_OPXCP_RUN);
	return NOT_READY;
}

StatusCode OpXcpCleanup(){
	//TRACE();
	//PROFILE_START(TMR_OPXCP_CLEANUP);
	if (globals.opData.rxSocketOpen){
		Log_Add("[OPERATION XCP] Closing socket on port %d",globals.DataInPort);
		Socket_Close(globals.rxSocket);
		globals.opData.rxSocketOpen = false;
	}

	if (result_fd != -1) { close(result_fd); result_fd = -1; }
	//if (cell_fd != -1) { close(cell_fd); cell_fd = -1; }
	if (corr){
		//TRACE();
		XCP_Destroy(&corr);
	}
	Log_Add("[OPERATION XCP] CleanedUp ...");
	globals.opData.opStatus = OP_STATUS_COMPLETE;
	globals.opData.opCleanupComplete = true;
	//PROFILE_STOP(TMR_OPXCP_CLEANUP);
	return SUCCESS;
}

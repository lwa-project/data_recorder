
#ifndef BEAMCORRELATOR_H_
#define BEAMCORRELATOR_H_

#include "Defines.h"
#include "DataTypes.h"
#include "DRX.h"
#include <stdint.h>

typedef struct __BeamCorrelatorOptions{
	// supplied geometry info
	int32_t			nSamplesPerOutputFrame;
	int32_t			nInts;
	int32_t			highWater;
	int32_t			minimumFill;	  // number of FFTs which must be present, below which a block will be dropped it the input queue contains above the latency Threshold
}BeamCorrelatorOptions;

// define the type of FFT plan (float)
typedef fftwf_plan 		 PlanType;
#define FFT_MALLOC 		 fftwf_malloc
#define FFT_FREE   		 fftwf_free
#define FFT_PLAN_MANY 	 fftwf_plan_many_dft
#define FFT_PLAN_DESTROY fftwf_destroy_plan

typedef uint8_t BlockState;
	#define BS_INVALID		0
	#define BS_EMPTY 		1
	#define BS_FILLING		2
	#define BS_PROCESSING	3
	#define BS_DONE			4
	#define BS_DROPPED		5


// all of the information required to perform all FFT+integration for all pols,tunings
typedef struct __XCPDataBlock{
	BlockState			  state;
	bool 				  isPrimer;
	DrxBlockSetup		  setup;
	UnpackedSample*		  iData;
	ComplexType*		  accumulator;
	pthread_t		      thread;
	bool			      threadDone;
	BeamCorrelatorOptions options;
	uint32_t			  fill;
	uint32_t			  satCount;
} __attribute__((packed)) XCPDataBlock;

typedef struct __BeamCorrelatorCounters{
	uint32_t				framesReceivedGood;
	uint32_t				framesReceivedInvalid;
	uint32_t				framesInserted;
	uint32_t				framesInsertedExisting;
	uint32_t				framesInsertedNew;
	uint32_t				framesInsertedAltTimeTag;
	uint32_t				framesDroppedArrivedLate;
	uint32_t				blocksStarted;
	uint32_t				blocksStartedFull;
	uint32_t				blocksStartedHighWater;
//	uint32_t				blocksDropped;
	uint32_t				blocksDroppedFailedStart;
	uint32_t				blocksDroppedUnderfull;
	uint32_t				blocksCompleted;
	uint32_t				resultsRequested;
	uint32_t				resultsPrepped;
	uint32_t				resultsWritten;
	uint32_t				resultsReleased;
} __attribute__((packed)) BeamCorrelatorCounters;

typedef struct __BeamCorrelator{
	BeamCorrelatorOptions		options;
	uint32_t 			 	in;			// start of empty space
	uint32_t 			 	f_start;	// start of filling blocks
	uint32_t 			 	p_start;	// start of processing/discarded blocks
	uint32_t 			 	out;		// start of finished blocks
	BeamCorrelatorCounters	counters;
	XCPDataBlock		 	blocks[NUM_BLOCKS];
	pthread_attr_t 			attr;
} __attribute__((packed)) BeamCorrelator;

typedef struct __DrxXcpResultsHeader{
	uint32_t			MAGIC1;     // must always equal 0xC0DEC0DE
	uint64_t 			timeTag0;
	uint16_t 			timeOffset;
	uint16_t 			decFactor;
	uint32_t 			freqCode[NUM_TUNINGS];
	uint8_t 			beam;
	uint32_t 			nSamplesPerOutputFrame;
	uint32_t 			nInts;
	uint32_t 			satCount;
	uint32_t			MAGIC2;     // must always equal 0xED0CED0C
} __attribute__((packed)) DrxXcpResultsHeader;

BeamCorrelator* XCP_Create			(BeamCorrelatorOptions* options);
StatusCode    XCP_Insert			(BeamCorrelator* bc, DrxFrame* frame);
void          XCP_Monitor			(BeamCorrelator* bc);
StatusCode    XCP_NextResult		(BeamCorrelator* bc, XCPDataBlock** result);
StatusCode    XCP_ReleaseResult	(BeamCorrelator* bc, XCPDataBlock* result);
void          XCP_Destroy			(BeamCorrelator** bc);

#define TRACE_XCP 0
#define TRACE() if (TRACE_SPEC) {printf("Trace: %s in file '%s' @ line %d \n", __FUNCTION__, __FILE__, __LINE__); fflush(stdout);}
#define DEBUG_XCP 1
//#define DEBUG_XCP_UNPACKING 1

#endif /* BEAMCORRELATOR_H_ */

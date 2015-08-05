/*
 * Spectrometer2.h
 *
 *  Created on: Jan 29, 2012
 *      Author: chwolfe2
 */

#ifndef SPECTROMETER2_H_
#define SPECTROMETER2_H_

#include "Defines.h"
#include <fftw3.h>
#include "DataTypes.h"
#include "DRX.h"
#include <stdint.h>

typedef struct __SpectrometerOptions{
	// supplied geometry info
	int32_t			nFreqChan;
	int32_t			nInts;
	int32_t			highWater;
	int32_t			minimumFill;	  // number of FFTs which must be present, below which a block will be dropped it the input queue contains above the latency Threshold
}SpectrometerOptions;

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


typedef volatile uint8_t CellState;
	#define CS_INVALID		0
	#define CS_NOT_STARTED  1
	#define CS_RUNNING      2
	#define CS_ERROR		3
	#define CS_DONE			4



typedef struct __CalcCell{
	CellState			state;
	PlanType			plan;
	uint32_t			fill;
	UnpackedSample*		iData;
	UnpackedSample*		oData;
	RealType*			accumulator;
	pthread_t		    thread;
	SpectrometerOptions options;
	uint32_t 			id;
	uint32_t 			satCount;
}__attribute__((packed)) CalcCell;

// all of the information required to perform all FFT+integration for all pols,tunings
typedef struct __SpecDataBlock{
	BlockState			state;
	bool 				isPrimer;
	CalcCell			cells[NUM_STREAMS];
	DrxBlockSetup		setup;
	UnpackedSample*		iData;
	UnpackedSample*		oData;
	RealType*			accumulator;
} __attribute__((packed)) SpecDataBlock;

typedef struct __SpectrometerCounters{
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
	uint32_t				blocksDropped;
	uint32_t				blocksCompleted;
	uint32_t				cellsStarted;
	uint32_t				cellsDroppedFailedStart;
	uint32_t				cellsDroppedUnderfull;
	uint32_t				spectraRequested;
	uint32_t				spectraPrepped;
	uint32_t				spectraWritten;
	uint32_t				spectraReleased;
} __attribute__((packed)) SpectrometerCounters;

typedef struct __Spectrometer{
	SpectrometerOptions		options;
	uint32_t 			 	in;			// start of empty space
	uint32_t 			 	f_start;	// start of filling blocks
	uint32_t 			 	p_start;	// start of processing/discarded blocks
	uint32_t 			 	out;		// start of finished blocks
	SpectrometerCounters	counters;
	SpecDataBlock		 	blocks[NUM_BLOCKS];
	pthread_attr_t 			attr;
} __attribute__((packed)) Spectrometer;

typedef struct __DrxSpectraHeader{
	uint32_t			MAGIC1;     // must always equal 0xC0DEC0DE
	uint64_t 			timeTag0;
	uint16_t 			timeOffset;
	uint16_t 			decFactor;
	uint32_t 			freqCode[NUM_TUNINGS];
	uint32_t			fills[NUM_TUNINGS * NUM_POLS];
	uint8_t				errors[NUM_TUNINGS * NUM_POLS];
	uint8_t 			beam;
	uint32_t 			nFreqs;
	uint32_t 			nInts;
	uint32_t 			satCount[NUM_TUNINGS * NUM_POLS];
	uint32_t			MAGIC2;     // must always equal 0xED0CED0C
} __attribute__((packed)) DrxSpectraHeader;

Spectrometer* Spec_Create			(SpectrometerOptions* options);
StatusCode    Spec_Insert			(Spectrometer* s, DrxFrame* frame);
void          Spec_Monitor			(Spectrometer* s);
StatusCode    Spec_NextResult		(Spectrometer* s, SpecDataBlock** result);
StatusCode    Spec_ReleaseResult	(Spectrometer* s, SpecDataBlock* result);
void          Spec_Destroy			(Spectrometer** s);

#define TRACE_SPEC 1
#define TRACE() if (TRACE_SPEC) {printf("Trace: %s in file '%s' @ line %d \n", __FUNCTION__, __FILE__, __LINE__); fflush(stdout);}

#endif /* SPECTROMETER2_H_ */

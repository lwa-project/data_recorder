/*
 * DRX.h
 *
 *  Created on: May 29, 2012
 *      Author: chwolfe2
 */

#ifndef DRX_H_
#define DRX_H_
#include "DataTypes.h"

// frame header is common to both packed (as received), and unpacked(converted to floating point) DRX frames
typedef struct __DrxFrameHeader{
	uint32_t syncCode;
	union {
		union {
			uint8_t  id;
			struct {
				uint8_t drx_beam:3;
				uint8_t drx_tuning:3;
				uint8_t drx_ignored:1;
				uint8_t drx_polarization:1;
			};
		};
		uint32_t frameCount;
	};
	uint32_t secondsCount;
	uint16_t decFactor;
	uint16_t timeOffset;
	uint64_t timeTag;
	uint32_t freqCode;
	uint32_t statusFlags;
}__attribute__((packed)) DrxFrameHeader;
// DRX frame as received
typedef struct __DrxFrame{ // Jake's drx frame struct
	DrxFrameHeader header;
	PackedSample   samples[DRX_SAMPLES_PER_FRAME];
} __attribute__((packed)) DrxFrame;

// common attributes for a DRX stream (some derived)
typedef struct __DrxBlockSetup{
	uint64_t 			timeTag0;
	uint64_t 			timeTagN;
	uint64_t 			stepPhase;
	uint64_t 			timeTagStep;
	uint16_t 			decFactor;
	uint16_t 			timeOffset;
	uint8_t 			beam;
	uint32_t 			freqCode[NUM_TUNINGS];
}__attribute__((packed))  DrxBlockSetup;

#define STEP_PHASE(tag,dec,spf) ( ((uint64_t)tag) % (((uint64_t) dec) * ((uint64_t) spf)) )
#define STEP_PHASE_DRX(tag,dec) ( STEP_PHASE(tag,dec,DRX_SAMPLES_PER_FRAME) )

#endif /* DRX_H_ */

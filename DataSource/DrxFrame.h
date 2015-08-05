/*
 * DrxFrame.h
 *
 *  Created on: Jan 29, 2012
 *      Author: chwolfe2
 */

#ifndef DRXFRAME_H_
#define DRXFRAME_H_
#ifdef __cplusplus
extern "C"{
#endif

#define DRX_SAMPLES_PER_FRAME 4096

#define Fs_Day (196l* 1000000l * 60l *60l * 24l)/*16934400000000l*/

#include <fftw3.h>
#include <stdint.h>
#include "Complex.h"


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
// alias to the above
typedef DrxFrame	PackedDrxFrame;

typedef struct __UnpackedDrxFrame{
	DrxFrameHeader header;
	UnpackedSample   samples[DRX_SAMPLES_PER_FRAME];
} __attribute__((packed)) UnpackedDrxFrame;

#ifdef __cplusplus
}
#endif


#endif /* DRXFRAME_H_ */

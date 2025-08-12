/*
 * DrxFrame.h
 *
 *  Created on: Jan 29, 2012
 *      Author: chwolfe2
 */

#ifndef DRX8FRAME_H_
#define DRX8FRAME_H_
#ifdef __cplusplus
extern "C"{
#endif

#define DRX8_SAMPLES_PER_FRAME 4096

#define Fs_Day (196l* 1000000l * 60l *60l * 24l)/*16934400000000l*/

#include <fftw3.h>
#include <stdint.h>
#include "Complex.h"


typedef struct __Drx8FrameHeader{
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
}__attribute__((packed)) Drx8FrameHeader;

// DRX8 frame as received
typedef struct __Drx8Frame{ // Jake's drx frame struct
	Drx8FrameHeader header;
	PackedSample8   samples[DRX8_SAMPLES_PER_FRAME];
} __attribute__((packed)) Drx8Frame;
// alias to the above
typedef Drx8Frame	PackedDrx8Frame;

typedef struct __UnpackedDrx8Frame{
	Drx8FrameHeader header;
	UnpackedSample   samples[DRX8_SAMPLES_PER_FRAME];
} __attribute__((packed)) UnpackedDrx8Frame;

#ifdef __cplusplus
}
#endif


#endif /* DRX8FRAME_H_ */

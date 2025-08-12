#ifndef DRX8FRAME_H_
#define DRX8FRAME_H_
#ifdef __cplusplus
extern "C"{
#endif

#define DRX8_SAMPLES_PER_FRAME 4096

#define Fs_Day (196l* 1000000l * 60l *60l * 24l) /*16934400000000l*/

#include <fftw3.h>
#include <stdint.h>
#include "../Signals/Complex.h"



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
}__attribute__((packed)) DrxFrameHeader;

// DRX8 frame as received
typedef struct __Drx8Frame{ // Jake's drx frame struct
	DrxFrameHeader header;
	PackedSample8  samples[DRX8_SAMPLES_PER_FRAME];
} __attribute__((packed)) Drx8Frame;
// alias to the above
typedef Drx8Frame	PackedDrx8Frame;

typedef struct __UnpackedDrx8Frame{
	Drx8FrameHeader header;
	UnpackedSample   samples[DRX8_SAMPLES_PER_FRAME];
} __attribute__((packed)) UnpackedDrx8Frame;

#define DRX8_FRAME_SIZE (sizeof(Drx8Frame))

#define DRX8_TUNINGS       2l
#define DRX8_POLARIZATIONS 2l
#define DRX8_STREAMS       4l

const uint64_t Drx8SampleRates[] = {
	  250000lu,
	  500000lu,
	 1000000lu,
	 2000000lu,
	 4900000lu,
	 9800000lu,
	19600000lu
};

const uint64_t Drx8DecFactors[] = {
	784lu,
	392lu,
	196lu,
	 98lu,
	 40lu,
	 20lu,
	 10lu
};

const uint64_t Drx8TimeTagSteps[] = {
		DRX8_SAMPLES_PER_FRAME * 784lu,
		DRX8_SAMPLES_PER_FRAME * 392lu,
		DRX8_SAMPLES_PER_FRAME * 196lu,
		DRX8_SAMPLES_PER_FRAME *  98lu,
		DRX8_SAMPLES_PER_FRAME *  40lu,
		DRX8_SAMPLES_PER_FRAME *  20lu,
		DRX8_SAMPLES_PER_FRAME *  10lu
};
const uint64_t Drx8DataRates[] = {
		((  250000lu * DRX8_STREAMS * DRX8_FRAME_SIZE) / DRX8_SAMPLES_PER_FRAME),
		((  500000lu * DRX8_STREAMS * DRX8_FRAME_SIZE) / DRX8_SAMPLES_PER_FRAME),
		(( 1000000lu * DRX8_STREAMS * DRX8_FRAME_SIZE) / DRX8_SAMPLES_PER_FRAME),
		(( 2000000lu * DRX8_STREAMS * DRX8_FRAME_SIZE) / DRX8_SAMPLES_PER_FRAME),
		(( 4900000lu * DRX8_STREAMS * DRX8_FRAME_SIZE) / DRX8_SAMPLES_PER_FRAME),
		(( 9800000lu * DRX8_STREAMS * DRX8_FRAME_SIZE) / DRX8_SAMPLES_PER_FRAME),
		((19600000lu * DRX8_STREAMS * DRX8_FRAME_SIZE) / DRX8_SAMPLES_PER_FRAME)
};
const double Drx8TimeSteps[] = {
		1.0f /  250000.0f,
		1.0f /  500000.0f,
		1.0f / 1000000.0f,
		1.0f / 2000000.0f,
		1.0f / 4900000.0f,
		1.0f / 9800000.0f,
		1.0f /19600000.0f
};


#ifdef __cplusplus
}
#endif


#endif /* DRX8FRAME_H_ */

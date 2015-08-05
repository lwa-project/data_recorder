#ifndef TBNFRAME_H_
#define TBNFRAME_H_

#ifdef __cplusplus
extern "C"{
#endif

#define TBN_SAMPLES_PER_FRAME 512

#define Fs_Day (196l* 1000000l * 60l *60l * 24l) /*16934400000000l*/

#include <fftw3.h>
#include <stdint.h>
#include "../Signals/Complex.h"


typedef struct __TbnFrameHeader{
	uint32_t syncCode;
	union {
		uint8_t  id;
		uint32_t frameCount;
	};
	uint32_t secondsCount;
	union {
		struct {
			uint16_t tbn_tbn_bit_n:1;    // 0-> tbn, 1-> not tbn
			uint16_t tbn_stand_pol:15;   // 1..519 -> (1->260, X..Y)
		};
		uint16_t tbn_id;
	};
	uint16_t unassigned;
	uint64_t timeTag;
}__attribute__((packed)) TbnFrameHeader;

// TBN frame as received
typedef struct __TbnFrame{
	TbnFrameHeader  header;
	PackedSample8   samples[TBN_SAMPLES_PER_FRAME];
} __attribute__((packed)) TbnFrame;
// alias to the above
typedef TbnFrame	PackedTbnFrame;

typedef struct __UnpackedTbnFrame{
	TbnFrameHeader  header;
	UnpackedSample  samples[TBN_SAMPLES_PER_FRAME];
} __attribute__((packed)) UnpackedTbnFrame;

#define TBN_FRAME_SIZE (sizeof(TbnFrame))


#define TBN_TUNINGS            	2l
#define TBN_POLARIZATIONS     	2l
#define TBN_STREAMS            	(TBN_TUNINGS*TBN_POLARIZATIONS)


const uint64_t TbnSampleRates[] = {
	  1000lu,
	  3125lu,
	  6250lu,
	 12500lu,
	 25000lu,
	 50000lu,
	100000lu
};
const uint64_t TbnDecFactors[] = {
	196000lu,
	 62720lu,
	 31360lu,
	 15680lu,
	  7840lu,
	  3920lu,
	  1960lu
};
const uint64_t TbnTimeTagSteps[] = {
	100352000lu,
	 32112640lu,
	 16056320lu,
	  8028160lu,
	  4014080lu,
	  2007040lu,
	  1003520lu
};
const uint64_t TbnDataRates[] = {
	  1064375lu,
	  3326172lu,
	  6652344lu,
	 13304688lu,
	 26609375lu,
	 53218750lu,
	106437500lu
};
const double TbnTimeSteps[] = {
	1.0f /   1000.0l,
	1.0f /   3125.0l,
	1.0f /   6250.0l,
	1.0f /  12500.0l,
	1.0f /  25000.0l,
	1.0f /  50000.0l,
	1.0f / 100000.0l
};









#ifdef __cplusplus
}
#endif


#endif /* TBNFRAME_H_ */

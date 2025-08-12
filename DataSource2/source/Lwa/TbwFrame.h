

#ifndef TBWFRAME_H_
#define TBWFRAME_H_


#ifdef __cplusplus
extern "C"{
#endif

#define TBW_SAMPLES_PER_FRAME_4BIT 1200
#define TBW_SAMPLES_PER_FRAME_12BIT 400
#define TBW_12BIT_BYTES ((TBW_SAMPLES_PER_FRAME_12BIT * 12) / 8)

#define Fs_Day (196l* 1000000l * 60l *60l * 24l) /*16934400000000l*/

#include <fftw3.h>
#include <stdint.h>
#include "../Signals/Complex.h"


typedef struct __TbwFrameHeader{
	uint32_t syncCode;
	union {
		uint8_t  id;
		uint32_t frameCount;
	};
	uint32_t secondsCount;
	union {
		struct {
				uint16_t tbw_tbw_bit:1; // 1-> tbw,    0-> not tbw
				uint16_t tbw_4bit:1;    // 1 -> 4-bit, 0 -> 12-bit
				uint16_t tbw_stand:14;  // 1->260
			};
		uint16_t tbw_id;
	};
	uint16_t unassigned;
	uint64_t timeTag;
}__attribute__((packed)) TbwFrameHeader;

// TBW frame as received
typedef struct __TbwFrame{
	TbwFrameHeader  header;
	union{
		PackedSample4   samples_4bit[TBW_SAMPLES_PER_FRAME_4BIT];
		uint8_t         samples_12bit[TBW_12BIT_BYTES];
	};
} __attribute__((packed)) TbwFrame;
// alias to the above
typedef TbwFrame	PackedTbwFrame;

typedef struct __UnpackedTbwFrame4{
	TbwFrameHeader  header;
	UnpackedSample  samples_4bit[TBW_SAMPLES_PER_FRAME_4BIT];
} __attribute__((packed)) UnpackedTbwFrame4;

typedef struct __UnpackedTbwFrame12{
	TbwFrameHeader  header;
	UnpackedSample  samples_4bit[TBW_SAMPLES_PER_FRAME_4BIT];
} __attribute__((packed)) UnpackedTbwFrame12;


#define TBW_FRAME_SIZE (sizeof(TbwFrame))

#ifdef __cplusplus
}
#endif


#endif /* TBWFRAME_H_ */

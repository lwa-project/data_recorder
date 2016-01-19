// ========================= DROSv2 License preamble===========================
// Software for DROSv2 is covered jointly under GPLv3 and Boost 1.0 licenses,
// to the extent required by included Boost sources and GPL sources, and to the
// more restrictive case pertaining thereunto, as defined herebelow. Beyond those
// requirements, any code not explicitly restricted by either of thowse two license
// models shall be deemed to be licensed under the GPLv3 license and subject to
// those restrictions.
//
// Copyright 2012, Virginia Tech, University of New Mexico, and Christopher Wolfe
//
// ========================= Boost License ====================================
// Boost Software License - Version 1.0 - August 17th, 2003
//
// Permission is hereby granted, free of charge, to any person or organization
// obtaining a copy of the software and accompanying documentation covered by
// this license (the "Software") to use, reproduce, display, distribute,
// execute, and transmit the Software, and to prepare derivative works of the
// Software, and to permit third-parties to whom the Software is furnished to
// do so, all subject to the following:
//
// The copyright notices in the Software and this entire statement, including
// the above license grant, this restriction and the following disclaimer,
// must be included in all copies of the Software, in whole or in part, and
// all derivative works of the Software, unless such copies or derivative
// works are solely in the form of machine-executable object code generated by
// a source language processor.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
// SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
// FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
//
// =============================== GPL V3 ====================================
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

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
	 100000lu,
	 200000lu,
	 400000lu,
	 800000lu,
	1600000lu
};
const uint64_t TbnDecFactors[] = {
	196000lu,
	 62720lu,
	 31360lu,
	 15680lu,
	  7840lu,
	  3920lu,
	  1960lu,
	   980lu,
	   460lu,
	   245lu,
	   122lu
};
const uint64_t TbnTimeTagSteps[] = {
	100352000lu,
	 32112640lu,
	 16056320lu,
	  8028160lu,
	  4014080lu,
	  2007040lu,
	  1003520lu,
	   501760lu,
	   250880lu,
	   125440lu,
	    62720lu
};
const uint64_t TbnDataRates[] = {
	   1064375lu,
	   3326172lu,
	   6652344lu,
	  13304688lu,
	  26609375lu,
	  53218750lu,
	 106437500lu,
	 212875000lu,
	 425750000lu,
	 851500000lu,
	1703000000lu
};
const double TbnTimeSteps[] = {
	1.0f /   1000.0l,
	1.0f /   3125.0l,
	1.0f /   6250.0l,
	1.0f /  12500.0l,
	1.0f /  25000.0l,
	1.0f /  50000.0l,
	1.0f / 100000.0l,
	1.0f / 200000.0l,
	1.0f / 400000.0l,
	1.0f / 800000.0l,
	1.0f /1600000.0l
};









#ifdef __cplusplus
}
#endif


#endif /* TBNFRAME_H_ */

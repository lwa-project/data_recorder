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

#ifndef CORFRAME_H_
#define CORFRAME_H_

#ifdef __cplusplus
extern "C"{
#endif

#define COR_SAMPLES_PER_FRAME 288

#define Fs_Day (196l* 1000000l * 60l *60l * 24l) /*16934400000000l*/

#include <fftw3.h>
#include <stdint.h>
#include "../Signals/Complex.h"


typedef struct __CorFrameHeader{
	uint32_t syncCode;
	union {
		uint8_t  id;
		uint32_t frameCount;
	};
	uint32_t secondsCount;
	uint16_t freq_chan;
	uint16_t cor_gain;
	uint64_t timeTag;
	uint32_t cor_navg;
	uint16_t stand_i;
	uint16_t stand_j;
}__attribute__((packed)) CorFrameHeader;

// COR frame as received
typedef struct __CorFrame{
	CorFrameHeader  header;
	ComplexType     samples[COR_SAMPLES_PER_FRAME];
} __attribute__((packed)) CorFrame;
// alias to the above
typedef CorFrame	PackedCorFrame;

typedef struct __UnpackedCorFrame{
	CorFrameHeader  header;
	UnpackedSample  samples[COR_SAMPLES_PER_FRAME];
} __attribute__((packed)) UnpackedCorFrame;


#define COR_FRAME_SIZE (sizeof(CorFrame))
#define COR_TUNINGS            	2l
#define COR_POLARIZATIONS     	2l
#define COR_STREAMS            	(COR_TUNINGS*COR_POLARIZATIONS)


#ifdef __cplusplus
}
#endif


#endif /* CORFRAME_H_ */

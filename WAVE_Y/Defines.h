/*
  MCS-DROS: Monitor and Control System - Data Recorder Operating Software
  Copyright (C) 2009-2010  Virginia Tech, Christopher Wolfe <chwolfe2@vt.edu>

  This file is part of MCS-DROS.

  MCS-DROS is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  MCS-DROS is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with MCS-DROS.  If not, see <http://www.gnu.org/licenses/>.
*/
/*
 * Defines.h
 *
 *  Created on: Oct 18, 2009
 *      Author: chwolfe2
 */

#ifndef DEFINES_H_
#define DEFINES_H_
#pragma pack(1)

typedef int boolean;
typedef boolean bool;
#define true 1
#define false 0

#define True 1
#define False 0

#define TRUE 1
#define FALSE 0

#define forever 1
#define always 1
#define never 0


typedef int StatusCode;
#define SUCCESS 0
#define FAILURE 1
#define NOT_READY 2

#define statusString(s) ((s==SUCCESS) ? "SUCCESS" : ((s==FAILURE) ? "FAILURE" : "NOT_READY"))

#define EXIT_CODE_FAIL           0x00
#define EXIT_CODE_FLUSH_DATA     0x01
#define EXIT_CODE_FLUSH_LOG      0x08
#define EXIT_CODE_FLUSH_SCHEDULE 0x04
#define EXIT_CODE_FLUSH_CONFIG   0x02
#define EXIT_CODE_SHUTDOWN       0x10
#define EXIT_CODE_REBOOT         0x20

#define RECEIVE_QUEUE_SIZE       1536l

#define DRX_SAMPLES_PER_FRAME	 4096

// macros for gcc builtins
#define __isPowerOf2(x) (__builtin_popcount(x)==1)

//#define likely(x)       __builtin_expect((x),1)
//#define unlikely(x)     __builtin_expect((x),0)
#define likely(x)      (x)
#define unlikely(x)    (x)

// macros for queue arithmetic
#define QUEUE_EMPTY(in, out, size) (out == in)
#define QUEUE_FULL(in, out, size)  ( ((in + 1) % size) == out )
#define QUEUE_USED(in, out, size)  ( (in >= out) ? (in-out) : ((in+size)-out) )
#define QUEUE_FREE(in, out, size)  (size - QUEUE_USED(in, out,size) - 1)
#define QUEUE_PRED(x, size)  ( (x + size - 1) % size)
#define QUEUE_SUCC(x, size)  ( (x + 1) % size)
#define QUEUE_DECREMENT(x, size)   (x = QUEUE_PRED(x, size))
#define QUEUE_INCREMENT(x, size)   (x = QUEUE_SUCC(x, size))


#define NUM_BLOCKS 256

#define NUM_BEAMS	           4
#define NUM_TUNINGS	           2
#define NUM_POLS	           2
#define NUM_STREAMS	           (NUM_TUNINGS * NUM_POLS)

#define MIN_BEAM_INDEX         1
#define MIN_TUN_INDEX          1
#define MIN_POL_INDEX          0
#define MIN_STREAM_INDEX       0

#define MAX_BEAM_INDEX         (MIN_BEAM_INDEX   + NUM_BEAMS - 1)
#define MAX_TUN_INDEX          (MIN_TUN_INDEX    + NUM_TUNINGS - 1)
#define MAX_POL_INDEX          (MIN_POL_INDEX    + NUM_POLS - 1)
#define MAX_STREAM_INDEX       (MIN_STREAM_INDEX + NUM_STREAMS - 1)

#define IN_RANGE(a, min, max)  ((a >= min) && (a <= max))

#define isValidBeam(b)         IN_RANGE(b, MIN_BEAM_INDEX,   MAX_BEAM_INDEX)
#define isValidTuning(t)       IN_RANGE(t, MIN_TUN_INDEX,    MAX_TUN_INDEX)
#define isValidPolarization(p) IN_RANGE(p, MIN_POL_INDEX,    MAX_POL_INDEX)
#define isValidStream(s)       IN_RANGE(s, MIN_STREAM_INDEX, MAX_STREAM_INDEX)
#define isValidBlock(b)        IN_RANGE(b, 0, (NUM_BLOCKS-1))



#define DRX_SAMPLES_PER_FRAME 4096

#define DRX_SAMPLE_FREQUENCY	19600000l
#define SECONDS_PER_DAY			(24l * 60l * 60l)
#define DRX_SAMPLES_DAY         (DRX_SAMPLE_FREQUENCY * SECONDS_PER_DAY)

#define FILE_MAX_NAME_LENGTH 		    1023

#endif /* DEFINES_H_ */

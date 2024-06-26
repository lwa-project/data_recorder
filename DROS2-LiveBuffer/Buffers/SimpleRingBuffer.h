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

#ifndef SIMPLERINGBUFFER_H_
#define SIMPLERINGBUFFER_H_
#include "GenericBuffer.h"
#include "BufferTypes.h"

#include <cstdlib>

class SimpleRingBuffer {
public:
	SimpleRingBuffer(const size_t& size, const size_t& overflow);
	void* nextIn( const size_t& size)                    const;
	void  doneIn( const size_t& sz,  const void* ptr);
	void* nextOut(const size_t& size)                    /*not const*/;
	void  doneOut(const size_t& sz,  const void* ptr);
	void signalProducerDone();
	bool isProducerDone()const;
	Allocation* getFractionalCompletion();
	void reset();
	bool isValid()const;
	virtual ~SimpleRingBuffer();
	void    print()const;

protected:
	void* buf;
	volatile size_t in;
	volatile size_t out;
	const size_t 	size;
	const size_t 	overflow;
	const size_t 	allocSize;
	bool            producerDone;
	Allocation      remnant;
	bool 			valid;
private:
	SimpleRingBuffer(const SimpleRingBuffer& toCopy):
		size(0),
		overflow(0),
		allocSize(0){}
	SimpleRingBuffer& operator =(const SimpleRingBuffer& toCopy){return *this;}
};

#endif /* SIMPLERINGBUFFER_H_ */

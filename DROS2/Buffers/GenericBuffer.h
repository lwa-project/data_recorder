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

#ifndef MULTISTAGERINGQUEUE_H_
#define MULTISTAGERINGQUEUE_H_
#include "../Common/misc.h"

#include <cstdlib>

class GenericBuffer {
public:
#ifdef CONFIG_UNIT_TESTS
	friend void __GB_unitTestThread1();
	friend void __GB_unitTest_Producer();
	friend void __GB_unitTest_StageN_Consumer(int stage);
	static void unitTest();
	static GenericBuffer* utgb;
#endif
	GenericBuffer(size_t objSize, size_t objCount, size_t numStages);
	void*   nextIn()const;
	void    doneIn(void* ptr);
	void*   nextOut()const;
	void    doneOut(void* ptr);
	void*   nextStage(size_t stage)const;
	void    doneStage(size_t stage, void* ptr);
	bool    advStage(size_t stage);
	bool    isValid()const;
	size_t  usedInStage(size_t stage)const;
	size_t  used()const;
	size_t  size()const;
	void    print()const;
	void*   peekStage(size_t stage, size_t peekAhead)const;
	void    reset();
	void    validitycheck() const;


	virtual ~GenericBuffer();
private:
	GenericBuffer(const GenericBuffer& toCopy):valid(false),buf(0),stage_indices(0),objSize(0),objCount(0),numStages(0){}
	NOASSIGN_DEF(GenericBuffer);
	bool  valid;
	void * buf;
	volatile size_t* stage_indices;
	const size_t   objSize;
	const size_t   objCount;
	const size_t   numStages;
};
#define PTR_TO(base,oSize,oIndex) ((void*)(&((char*)(base))[(oSize)*(oIndex)]))
#define PTR_TO_BYTE(base,b) ((void*)(&((char*)(base))[(b)]))
#define RQ_USED(in,out,size) ((in>=out)?(in-out):(in+size-out))
#define RQ_FREE(in,out,size) (size-1-(RQ_USED(in,out,size)))
#define RQ_NEXT(idx,size)    (((idx)+1)%size)

#endif /* MULTISTAGERINGQUEUE_H_ */

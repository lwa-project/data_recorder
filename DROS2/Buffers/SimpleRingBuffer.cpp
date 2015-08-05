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

#include "SimpleRingBuffer.h"
#include "../Common/misc.h"
#include "../Common/branchPredict.h"
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <iomanip>
using namespace std;

SimpleRingBuffer::SimpleRingBuffer(const size_t& size, const size_t& overflow):
	buf(NULL),
	in(0),
	out(0),
	size(size),
	overflow(overflow),
	allocSize(size+overflow),
	producerDone(false),
	remnant(),
	valid(false)
{
	buf = malloc(allocSize);
	if (!buf) return;
	valid=true;
}

size_t lastout=0;
size_t lastin=0;

void* SimpleRingBuffer::nextIn(const size_t& sz)const{
	if (unlikely(!valid)) return NULL;
	size_t _in  = in;
	size_t _out = out;
	if ((RQ_USED(_in,_out,size) + sz) >= size){
		return NULL;
	} else {
		if (unlikely((_in+sz) > allocSize)){
			FATAL(USAGE_ERROR,"nextIn request exceeds overflow capability");
		}
		lastin = (size_t) PTR_TO_BYTE(buf,_in);
		return PTR_TO_BYTE(buf,_in);
	}
}
void  SimpleRingBuffer::doneIn(const size_t& sz, const void* ptr){
	static int cnt = 0;
	if (unlikely(!valid)) return;
	size_t _in  = in;
	//size_t _out = out;
	if (unlikely(ptr != PTR_TO_BYTE(buf,_in))){
		size_t _lastin = (size_t) ptr;
		cout << _lastin << " != " << lastin<< endl;
		cout << "cnt=" << cnt << endl;
		FATAL(USAGE_ERROR,"called doneIn on other than input offset");
	}
	if (unlikely((_in+sz) > size)){
		if (unlikely((_in+sz) > allocSize)){
			FATAL(USAGE_ERROR,"doneIn request exceeds overflow capability");
		}
		memcpy(buf,PTR_TO_BYTE(buf,_in),size-_in);
	}
	in=(_in+sz) % size;
	cnt++;
}

void* SimpleRingBuffer::nextOut(const size_t& sz){
	if (unlikely(!valid)) return NULL;
	size_t _out = out;
	size_t _in  = in;
	size_t used = RQ_USED(_in,_out,size);
	if (used < sz){
		return NULL;
	} else {
		if (unlikely((_out+sz) > size)){
			if (unlikely((_out+sz) > allocSize)){
				FATAL(USAGE_ERROR,"nextOut request exceeds overflow capability");
			}
			memcpy(PTR_TO_BYTE(buf,size),PTR_TO_BYTE(buf,_out),sz-(size-_out));
		}
		lastout = (size_t) PTR_TO_BYTE(buf,_out);
		return PTR_TO_BYTE(buf,_out);
	}
}

void  SimpleRingBuffer::doneOut(const size_t& sz, const void* ptr){
	if (unlikely(!valid)) return;
	size_t _out = out;
	//size_t _in  = in;
	if (unlikely(ptr != PTR_TO_BYTE(buf,_out))){
		size_t _lastout = (size_t) ptr;
		cout << _lastout << " != " << lastout<< endl;
		FATAL(USAGE_ERROR,"called doneOut on other than output offset");
	}
	if (unlikely((_out+sz) > allocSize)){
		FATAL(USAGE_ERROR,"doneOut request exceeds overflow capability");
	}
	out=(_out+sz) % size;
}


void SimpleRingBuffer::signalProducerDone(){
	if (unlikely(!valid)) return;
	producerDone=true;
}

bool SimpleRingBuffer::isProducerDone()const{
	return valid && producerDone;
}

Allocation* SimpleRingBuffer::getFractionalCompletion(){
	if (unlikely(!valid)) return NULL;
	if (unlikely(!producerDone))
		FATAL(USAGE_ERROR,"called getFractionalCompletion but consumer wasn't finished")
	size_t _in  = in;
	size_t _out = out;
	size_t used = RQ_USED(_in,_out,size);
	if (unlikely((_out+used) > size)){
		if (unlikely((_out+used) > allocSize)){
			FATAL(USAGE_ERROR,"getFractionalCompletion request exceeds overflow capability");
		}
		memcpy(PTR_TO_BYTE(buf,size),PTR_TO_BYTE(buf,_out),used-(size-_out));
	}
	remnant.iov_base = PTR_TO_BYTE(buf,_out);
	remnant.iov_len  = used;
	return &remnant;
}

void SimpleRingBuffer::reset(){
	if (unlikely(!valid)) return;
	in=0;
	out=0;
	producerDone=false;
}
bool SimpleRingBuffer::isValid()const{
	return valid;
}
SimpleRingBuffer::~SimpleRingBuffer() {
	if (buf!=NULL){
		free(buf);
	}
}
void SimpleRingBuffer::print()const{
	size_t _in  = in;
	size_t _out = out;
	size_t used = RQ_USED(_in,_out,size);
	cout << "\tSRB    [ " << setw(10) << _in << " / " << setw(10) << _out << " / " << setw(15) << used << " ]";
	progressbar((used*100ll)/size,25);
	cout << "\n";
}


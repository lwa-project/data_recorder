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


#ifndef CAPTURE_H_
#define CAPTURE_H_
#include <stdint.h>
#include <string.h>

#include "../Messaging/IpSpec.h"

class Capture{

	typedef struct __CapChunk{
		TicketBuffer::Ticket* _ticket;
		uint32_t              _index;
		uint32_t              _count;
	}CapChunk;


public:
	Capture(size_t max_chunks):
	_fs(0),
	_N0(0),
	_n_chunks(max_chunks),
	_n(),
	_cur_chunk_in(0),
	_chunk_ofs(0),

	_cur_chunk_out(0),
	_commit_chunk(0),
	_commit_ofs(0),

	_rate_limit(0.0),
	_d_addr(),

	_chunks(new CapChunk[max_chunks]),

	_use_rate_limit(false),
	_start_time()

	{

	}

	virtual ~Capture(){
		delete(_chunks);
	}
	////////////////////////////////////////
	///                                  ///
	///  producer interface              ///
	///                                  ///
	////////////////////////////////////////
	// change the capture rules
	void set_rule(size_t _max_chunks, struct sockaddr_storage& d_addr, double rate_limit, size_t N0){

		assert(_n == 0);
		if (_max_chunks != _n_chunks){
			_n_chunks = _max_chunks;
			delete [] _chunks;
			_chunks = new CapChunk[_max_chunks];
		}
		_N0              = N0;
		_rate_limit      = rate_limit;
		_use_rate_limit  = _rate_limit > 0.0;
		_d_addr          = d_addr;
	}

	// insert from ticket, return true if no more insertion is possible
	bool insert(TicketBuffer::Ticket* ticket, size_t& t_ofs){
		if (!_n){
			// first insert
			_fs = ticket->fsize;
		}
		if (ticket->fsize != _fs){
			// bad size, so finish off the capture
			return true;
		}
		size_t avail                    = ticket->cnt_used - t_ofs;
		size_t needed                   = _N0 - _n;
		size_t toUse                    = (avail < needed) ? avail : needed;
		_chunks[_cur_chunk_in]._ticket  = ticket;
		_chunks[_cur_chunk_in]._index   = t_ofs;
		_chunks[_cur_chunk_in]._count   = toUse;
		_n                             += toUse;
		t_ofs                          += toUse;
		ticket->reGet(1);    // increment reference count
		++_cur_chunk_in;
		return ((_n == _N0) || (_cur_chunk_in == _n_chunks));
	}

	// check if completely filled to spec
	bool isComplete()const{
		return (_n == _N0) && (_n != 0);
	}

	// reset unsent capture
	void cancel(){ // only valid on cap which hasn't been enqueued for transmission
		for(size_t i=0; i<_cur_chunk_in; i++){
			_chunks[i]._ticket->put();
		}
		_n            = 0;
		_cur_chunk_in = 0;
		_chunk_ofs    = 0;
		_N0           = 0;
		_fs           = 0;
		_commit_chunk = 0;
		_commit_ofs   = 0;
	}
	// nomen est omen
	void prepare_for_transmission(){
		_n             = 0;
		_cur_chunk_out = 0;
		_chunk_ofs     = 0;
		_commit_chunk  = 0;
		_commit_ofs    = 0;
	}

	////////////////////////////////////////
	///                                  ///
	///  consumer interface              ///
	///                                  ///
	////////////////////////////////////////
	struct iovec* next_out(bool& is_last_in_chunk, bool& is_last_in_capture){
		TicketBuffer::Ticket* t = _chunks[_cur_chunk_out]._ticket;
		size_t base = _chunks[_cur_chunk_out]._index;
		size_t size = _chunks[_cur_chunk_out]._count;
		struct iovec* rv = &(t->iovs[_chunk_ofs+base]);
		++_chunk_ofs;
		is_last_in_chunk = (_chunk_ofs == size);
		if (is_last_in_chunk){
			_chunk_ofs = 0;   // reset offset
			++_cur_chunk_out; // move on to next chunk
			is_last_in_capture = (_cur_chunk_out = _cur_chunk_in);
		}
		return rv;
	}

	bool commit_out(){
		TicketBuffer::Ticket* t = _chunks[_commit_chunk]._ticket;
		size_t size = _chunks[_commit_chunk]._count;
		++_commit_ofs;
		++_n;
		if (_commit_ofs == size){
			// last in chunk
			_commit_ofs = 0; // reset offset
			t->put();        // decrement reference count
			++_commit_chunk; // move on to next chunk
			if (_commit_chunk == _cur_chunk_in){
				// last in capture
				return true;
			}
		}
		return false;
	}

	void abort_all(){
		for (size_t i=_commit_chunk; i<_cur_chunk_in; i++){
			TicketBuffer::Ticket* t = _chunks[i]._ticket;
			t->put();
		}
		_chunk_ofs     = 0;
		_commit_ofs    = 0;
		_cur_chunk_out = _cur_chunk_in;
		_commit_chunk  = _cur_chunk_in;
	}

	void start_clock(){
		if (_use_rate_limit)
			_start_time = Time::now();
	}

	void do_rate_govern(){
		while(_use_rate_limit){
			TimeStamp _now = Time::now();
			double elapsed = ((double) Time::getTimeStampDelta_ms(_start_time,_now)) / 1000.0;

			if (elapsed==0.0) // minimum timestamp diff is 1 ms;
				elapsed = 0.001;

			double rate = ((double) (_n*_fs)) / elapsed;

			if (rate > _rate_limit){
				pthread_yield();
			} else {
				break;
			}
		}
	}

	size_t get_frame_size()const{return _fs;}
	size_t get_frame_count()const{return _N0;}
	size_t get_max_chunks()const{return _n_chunks;}
	size_t get_used_chunks()const{return _cur_chunk_in;}
	size_t get_n()const{return _n;}
	struct sockaddr_storage* get_addr(){return &_d_addr;}



private:
	// capture options
	uint32_t _fs;                    // frame size
	uint32_t _N0;                    // frame count (hold)
	uint32_t _n_chunks;              // number of chunks

	// load/unload status
	uint32_t _n;                     // completion (0 to _N0)
	uint32_t _cur_chunk_in;          // index of next-to-be-removed output chunk
	uint32_t _chunk_ofs;             // offset within current chunk (when output)

	uint32_t _cur_chunk_out;         // index of next-to-be-removed output chunk
	uint32_t _commit_chunk;          // index of next chunk to be committed
	uint32_t _commit_ofs;            // offset within next chunk to be committed


	// receiver options
	double _rate_limit;              // output bandwidth
	struct sockaddr_storage _d_addr; // output address

	// chunk buffer
	CapChunk* _chunks;               // MaxChunks=256 chunks is about enough for 1 Gib of capture, modulo chunks of size 4MiB rounded to nearest frame multiple


	bool      _use_rate_limit;       // enable/disable rate governing (determined when rule is set)
	TimeStamp _start_time;           // rate governing



};


#endif /* CAPTURE_H_ */
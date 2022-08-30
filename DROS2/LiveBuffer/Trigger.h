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

#ifndef TRIGGER_H_
#define TRIGGER_H_

class Trigger {
public:
	Trigger(bool coalesce=true): _coalesce(coalesce), _trg(0), _chk(0), _enabled(true){
		/* nothing to do */
	}

	virtual ~Trigger(){
		/* nothing to do */
	}

	/* deactivate triggering (no effect on currently servicing event) */
	void disable(){
		_enabled=false;
		reset();
	}

	/* activate triggering */
	void enable(){
		_enabled=true;
	}

	/* clear queued-but-not-serviced trigger events */
	void reset(){
		if (_coalesce){
			/* when coalescing, no events are queued, so nothing to do here */
		} else {
			size_t trg = _trg;
			while (true){
				if (trg == 0){
					return;
				} else {
					if (__sync_bool_compare_and_swap(&_trg, trg, 0)){
						return;
					} else {
						continue;
					}
				}
			}
		}
	}

	// initiate a trigger event
	void trigger(){
		if (!_enabled)
			return;
		if (_coalesce){
			__sync_bool_compare_and_swap(&_trg, 0, 1);
		} else {
			// atomic inc _trg
			while (true){
				size_t trg = _trg;
				if (__sync_bool_compare_and_swap(&_trg, trg, trg+1)){
					return;
				}
			}
		}
	}

	// check trigger condition
	bool check(size_t& waiting_trigger_count){
		if (_coalesce){
			bool rv = 	(
							(_trg != 0) &&
							(__sync_bool_compare_and_swap(&_chk, 0,1))
						);
			waiting_trigger_count = (rv ? 1 : 0);
			return rv;
		} else {
			while (true){
				size_t trg = _trg;
				if (trg == 0){
					waiting_trigger_count = 0;
					return false;
				} else {
					if (__sync_bool_compare_and_swap(&_trg, trg, trg-1)){
						waiting_trigger_count = trg;
						return true;
					} else {
						continue;
					}
				}
			}
		}
		// unreachable
		return false;
	}


	// acknowledge completion of trigger action
	void service(){
		if (_coalesce){
			while (!__sync_bool_compare_and_swap(&_trg, 1, 0)){
			} /* in theory, new trigger could happen here, but will skip until the following completes */
			while (!__sync_bool_compare_and_swap(&_chk, 1, 0)){
			}
		} else {
			/* nothing required for non-coalescing triggers */
		}
	}

	volatile bool   _coalesce;  // determines whether not-previously-checked
	volatile size_t _trg;       // trigger count/flag;
	volatile size_t _chk;       // check flag;
	volatile bool   _enabled;   // flag; nomen est omen

};

#endif /* TRIGGER_H_ */

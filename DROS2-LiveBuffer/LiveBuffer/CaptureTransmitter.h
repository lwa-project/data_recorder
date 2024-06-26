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

#ifndef CAPTURETRANSMITTER_H_
#define CAPTURETRANSMITTER_H_


#include "../Threading/GenericThread.h"
#include "../Buffers/LockQueue.h"
#include "../Messaging/IpSpec.h"
#include "../System/Time.h"
#include "Capture.h"

class CaptureTransmitter: public GenericThread {
public:
	CaptureTransmitter(LockQueue<Capture>* in,LockQueue<Capture>* out):
		GenericThread("CaptureTransmitter", PG_IO, 0),
		_sd(-1),
		_in(in),
		_out(out),
		_running(true),
		_started(false),
		_done(false){
		assert(in != NULL);
		assert(out != NULL);
		_sd = socket(AF_INET, SOCK_DGRAM, 0);
		assert(_sd != -1);
	}
	virtual ~CaptureTransmitter(){
		if (_sd != -1)
			close(_sd);
	}

	virtual void run(){
		LOGC(L_INFO,"Starting", ACTOR_COLORS);
		_started=true;
		while (_running && !interrupted){
			Capture* cap = _in->remove();
			if (cap != NULL){
				///LOGC(L_DEBUG,"CAPTURE TRANSMISSION START", ACTOR_WARNING_COLORS);
				send_capture(cap);
				///LOGC(L_DEBUG,"CAPTURE TRANSMISSION FINISH", ACTOR_WARNING_COLORS);
				_out->insert(cap);
			} else {
				usleep(100000); continue;
			}
		}
		_done=true;
		LOGC(L_INFO,"Finished", ACTOR_COLORS);
	}

	void shutdown(){
		LOGC(L_INFO,"Shutting down transmitter", ACTOR_COLORS);
		LOGC(L_INFO,"Sending transmitter shutdown signal", ACTOR_COLORS);
		_running=false;
		LOGC(L_INFO,"Waiting on transmitter shutdown confirmed", ACTOR_COLORS);
		while(_started && !_done){
			usleep(100000);
		}
		LOGC(L_INFO,"Transmitter shutdown confirmed", ACTOR_COLORS);
	}

#define MAX_MSG_PER_TRANS 256
#define TRANSMITTER_FLAGS 0

	void send_capture(Capture* cap){
		bool last_in_chunk = false;
		bool last_in_cap   = false;
		bool done          = false;
		bool error         = false;

		struct mmsghdr msgs[MAX_MSG_PER_TRANS];      // supports frame sizes as small as 256 bytes
		size_t burst_size_max = 65536; // 64K


		// set up transaction limits
		size_t fsize          = cap->get_frame_size();
		if (!fsize){
			cap->abort_all();
			return;
		}
		size_t mmpt = burst_size_max / fsize;
		if (!mmpt > MAX_MSG_PER_TRANS)	mmpt = MAX_MSG_PER_TRANS;
		if (!mmpt)                      mmpt = 1;

		// set up mmsghdr struct
		for (size_t i=0; i<mmpt; i++){
			msgs[i].msg_hdr.msg_control    = NULL;
			msgs[i].msg_hdr.msg_controllen = 0;
			msgs[i].msg_hdr.msg_name       = cap->get_addr();
			msgs[i].msg_hdr.msg_namelen    = sizeof(struct sockaddr_storage);
			msgs[i].msg_hdr.msg_iovlen     = 1;
		}

		// start the transfer clock
		cap->start_clock();


		while (true){
			// prepare transaction
			size_t count = 0;
			while ((count < mmpt) && !last_in_chunk && !last_in_cap){
				msgs[count].msg_len           = 0;
				msgs[count].msg_hdr.msg_flags = TRANSMITTER_FLAGS;
				msgs[count].msg_hdr.msg_iov = cap->next_out(last_in_chunk, last_in_cap);
				count++;
			}

			if (count != 0){
				// send
				size_t  count_sent = 0;
				bool error      = false;
				while ((!error) && (count_sent < count)){
					size_t remaining = count - count_sent;
					int res = sendmmsg(_sd, &msgs[count_sent], remaining, TRANSMITTER_FLAGS);
					if (res >= 0){
						count_sent+=(size_t)res;
					} else {
						if (	(errno != EWOULDBLOCK) &&
								(errno != EINTR)       &&
								(errno != 512)						){
							// unhandleable io error; terminate transmission of capture;
							error = true;
						}
					}
				}
				// commit (release) chunks
				for (size_t i=0; i<count; i++){
					if (cap->commit_out()){
						assert(last_in_cap);
						done = true;
					}
				}

				// throttle
				cap->do_rate_govern();

			} else {
				// count == 0, which is an error
				error = true;
			}

			// cleanup, continuation
			if (error && !done){
				cap->abort_all();
			}
			if (error || done){
				break;
			}

		}
	}

	bool isStarted()const{
		return _started;
	}

	bool isDone()const{
		return _done;
	}






private:
	DECLARE_ACCESS_MUTEX(config);
	int                     _sd;             // socket descriptor

	LockQueue<Capture>*     _in;             // inbound  queue for captures (not yet sent)
	LockQueue<Capture>*     _out;            // outbound queue for captures (sent)

	volatile bool           _running;        // flag, nomen est omen
	volatile bool           _started;        // flag, nomen est omen
	volatile bool           _done;           // flag, nomen est omen
	volatile bool           _addr_changed;   // flag, nomen est omen
};

#endif /* CAPTURETRANSMITTER_H_ */

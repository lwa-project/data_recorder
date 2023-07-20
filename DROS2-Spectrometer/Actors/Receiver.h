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

#ifndef RECEIVER_H_
#define RECEIVER_H_

#include <sys/fcntl.h>
#include <fcntl.h>
#include <errno.h>
#include <netdb.h>
#include <stdio.h>
#include "../Threading/GenericThread.h"
#include "../System/Log.h"
#include "../Data/DataFormat.h"
#include "../Data/LwaDataFormats.h"
#include "../Streaming/TicketBuffer.h"
#include "../Common/misc.h"
#include "../System/PeriodicReportTimer.h"


#define RX_REPORT_INTERVAL 5000

#define NSEC_PER_SEC (1000000000ll)
#define IDEAL_TRANSFER_SIZE 4194304   /*4MB*/
//#define INITIAL_BUFFER_SIZE 2147483648 /*2 GiB*/
#define INITIAL_BUFFER_SIZE (1048576lu*2048lu)
//#define INITIAL_BUFFER_SIZE 33554432 /*32MB*/



#define POST_TICKET(c)   	\
	/*LOGC(L_FATAL, "[Receiver] POST " + LXS(c), FATAL_COLORS );*/ \
	t->notifyFill(c); 		\
	t = NULL

#define CANCEL_TICKET() 	\
	t->returnEmpty();   \
	t=NULL

#define RESUME_RECEPTION()	\
	continue

#define ABORT_RECEPTION()	\
	buf->uninit();			\
	finalize();			\
	_done=true;			\
	return


/*
#define BAIL_NO_TICKET()\
	buf->uninit();\
	finalize();\
	_done=true;\
	return

#define BAIL_WITH_TICKET()\
	t->returnEmpty();\
	t=NULL;\
	BAIL_NO_TICKET()

#define CONTINUE_NO_TICKET()\
	continue

#define CONTINUE_WITH_TICKET()\
	t->returnEmpty();\
	t=NULL;\
	CONTINUE_NO_TICKET()
*/

class Receiver: public GenericThread {
public:
	Receiver(string _dataAddr, unsigned short _dataPort, TicketBuffer* _buf, bool _nonblocking=false):
		GenericThread("Receiver", PG_IO, 0),
		sd(-1),
		local_adr(),
		sd_flags(0),
		sd_m_flags(0),
		dataAddr(_dataAddr),
		dataPort(_dataPort),
		nonblocking(_nonblocking),
		buf(_buf),
		currentFormat(DataFormat::getFormatByName(DataFormat::defaultFormatName)),
		bufsize(0),
		transferSize(0),
		currentDrxDecFactor(-1),
		resetRequired(true),
		newFormat(DataFormat::getFormatByName(DataFormat::defaultFormatName)),
		newTransferSize(IDEAL_TRANSFER_SIZE),
		newBufSize(INITIAL_BUFFER_SIZE),
		_done(false),
		_started(false),
		rptTimer(RX_REPORT_INTERVAL),
		lag_initial(0),
		lag_latest(0),
		lag_initialized(false),
		bytesReceived(0),
		externallyClosed(false)
	{

	}

	virtual ~Receiver(){

	}

	bool setNewFmtDrx(int decFactor, bool isDrx8){
		static TimeStamp lastErrorLogged = Time::now();
		static size_t errCount = 0;
		if(isDrx8) {
			switch(decFactor){
				case 784: newFormat = DataFormat::getFormatByName("DRX8_FILT_1"); return true; break;
				case 392: newFormat = DataFormat::getFormatByName("DRX8_FILT_2"); return true; break;
				case 196: newFormat = DataFormat::getFormatByName("DRX8_FILT_3"); return true; break;
				case 98:  newFormat = DataFormat::getFormatByName("DRX8_FILT_4"); return true; break;
				case 40:  newFormat = DataFormat::getFormatByName("DRX8_FILT_5"); return true; break;
				case 20:  newFormat = DataFormat::getFormatByName("DRX8_FILT_6"); return true; break;
				case 10:  newFormat = DataFormat::getFormatByName("DRX8_FILT_7"); return true; break;
				default:
					if (Time::compareTimestamps(Time::addTime(lastErrorLogged, 5000), Time::now()) <=0){
						lastErrorLogged = Time::now();
						LOGC(L_FATAL, "[Receiver] Bad DRX8 Decimation factor: " + LXS(decFactor) + " {"+LXS(errCount)+" previous occurrences}", FATAL_COLORS );
						errCount = 0;
					} else {
						errCount++;
					}
					break;
			}
			newFormat = DataFormat::getFormatByName("DEFAULT_DRX8");
		} else {
			switch(decFactor){
				case 784: newFormat = DataFormat::getFormatByName("DRX_FILT_1"); return true; break;
				case 392: newFormat = DataFormat::getFormatByName("DRX_FILT_2"); return true; break;
				case 196: newFormat = DataFormat::getFormatByName("DRX_FILT_3"); return true; break;
				case 98:  newFormat = DataFormat::getFormatByName("DRX_FILT_4"); return true; break;
				case 40:  newFormat = DataFormat::getFormatByName("DRX_FILT_5"); return true; break;
				case 20:  newFormat = DataFormat::getFormatByName("DRX_FILT_6"); return true; break;
				case 10:  newFormat = DataFormat::getFormatByName("DRX_FILT_7"); return true; break;
				default:
					if (Time::compareTimestamps(Time::addTime(lastErrorLogged, 5000), Time::now()) <=0){
						lastErrorLogged = Time::now();
						LOGC(L_FATAL, "[Receiver] Bad DRX Decimation factor: " + LXS(decFactor) + " {"+LXS(errCount)+" previous occurrences}", FATAL_COLORS );
						errCount = 0;
					} else {
						errCount++;
					}
					break;
			}
			newFormat = DataFormat::getFormatByName("DEFAULT_DRX");
		}
		return true;
	}

	size_t getReceiveRate(){
		static bool initialized=false;
		static TimeStamp last(0,0);
		if (!initialized){
			last = Time::now();
			initialized = true;
		}
		TimeStamp present = Time::now();
		millisecond_delta elapsed = Time::getTimeStampDelta_ms(last,present);
		if (!elapsed) elapsed=1;

		size_t temp = bytesReceived * 1000ll / elapsed;
		bytesReceived = 0;
		last=present;
		return temp;
	}
	void report(){
		stringstream ss("");
		int pct = buf->usagePct();
		size_t buf_usage = buf->usage();
		ss << "Buffers:     ";
		if ((pct >= 0)  && (pct < 25)) ss << ANSI::colorSpec(basic,green,black);
		if ((pct >= 25) && (pct < 50)) ss << ANSI::colorSpec(bold,yellow,black);
		if ((pct >= 50) && (pct < 75)) ss << ANSI::colorSpec(bold,red,black);
		if (pct >= 75)                 ss << ANSI::colorSpec(blink,red,black);
		progressbar(pct, 30, ss);
		ss << ANSI::colorSpec(basic,cyan,black);
		ss << "  " << Storage::humanReadable(buf_usage);
		ss << ANSI::colorSpec(basic,white,black);
		ss << "\nRate:        ";
		size_t rate = getReceiveRate();
		pct = (int)((rate * 100ll) / (1048576ll*120ll));
		if ((pct >= 0)  && (pct < 25)) ss << ANSI::colorSpec(basic,green,black);
		if ((pct >= 25) && (pct < 50)) ss << ANSI::colorSpec(basic,cyan,black);
		if ((pct >= 50) && (pct < 95)) ss << ANSI::colorSpec(basic,yellow,black);
		if (pct >= 95)                 ss << ANSI::colorSpec(basic,red,black);
		progressbar(pct, 30, ss);
		ss << ANSI::colorSpec(basic,cyan,black);
		ss << "  " << Storage::humanReadableBW(rate);
		ss << ANSI::colorSpec(basic,white,black);
		int subs = buf->getSubscriberCount();
		ss << "\nSubscribers: ";
		switch (subs){
		case 0:  ss << ANSI::colorSpec(basic,cyan,black)   << "None"; break;
		case 1:  ss << ANSI::colorSpec(basic,green,black)  << LXS(subs); break;
		case 2:  ss << ANSI::colorSpec(basic,yellow,black) << LXS(subs); break;
		default: ss << ANSI::colorSpec(basic,red,black)    << LXS(subs); break;
		}
		ss << ANSI::colorSpec(basic,white,black);
		ss << "\nFormat:      " << currentFormat.getName();
		LOG(L_INFO, ss.str());
	}
	virtual void packetBurn(){
		externallyClosed=true;
		char buf[8192];
		bzero((void*)buf, 8192);
		int burnsd = socket(AF_INET, SOCK_DGRAM, 0);
		struct sockaddr_storage sa;
		bzero((void*)&sa, sizeof(struct sockaddr_storage));
		sa.ss_family = AF_INET;
		inet_pton(AF_INET,  "127.0.0.1", &(((struct sockaddr_in *)&sa)->sin_addr));
		((struct sockaddr_in *)&sa)->sin_port = htons(dataPort);
		while (!_done){
			sendto(burnsd,buf,0,MSG_DONTWAIT,(struct sockaddr*)&sa,(socklen_t)sizeof(struct sockaddr_storage));
		}
		close(burnsd);
	}
	virtual void run(){
		if (!initialize()){
			LOGC(L_FATAL, "[Receiver] Can't start", FATAL_COLORS );
			ABORT_RECEPTION();
			/*BAIL_NO_TICKET();*/
		}
		_started = true;
		while(!isInterrupted() && !externallyClosed){

			if (rptTimer.isTimeToReport()){
				report();
			}

			// get a ticket to fill
			TicketBuffer::Ticket* t = NULL;
			while (((t=buf->nextFreeTicket())==NULL)){
				// spin, hopefully not for long
				// check to make sure we weren't canceled.
				if (isInterrupted()){
					ABORT_RECEPTION();
					/*BAIL_NO_TICKET();*/
				}
			}
			// initialize the ticket
			__clearTicket(t);
			// set up the timeout structure
			// NOTE: tv_nsec is actually interpreted as micro seconds
			// NOTE: recvmmsg, at least on ubuntu 12.04, completely ignores the timeout argument, so we set
			// the timeout manually on the socket using setsockopt
/*			struct timespec to;
			to.tv_sec  = 0;
			to.tv_nsec = 500000;
*/
			/*
			int cres = setsockopt(sd, SOL_SOCKET, SO_RCVTIMEO, (char *)&to,  sizeof(struct timespec));
			if (cres<0){
				LOGC(L_WARNING, "[Receiver]: can't get time to set timeout for receive: " + string(strerror(errno)), ACTOR_WARNING_COLORS );
				CONTINUE_NO_TICKET();
			}
*/
			// read the socket
			int res = recvmmsg(sd,t->mhdrs,t->cnt_alloc,sd_m_flags,NULL);
			//LOGC(L_WARNING, " ______________________________________________________[Receiver]", ACTOR_WARNING_COLORS );
			// process the received data
			if (res<=0){

				if (res==0 || (errno == 512) || (errno==EAGAIN) || (errno==EINTR)){
					//timeout, interrupt, or non-blocking
					CANCEL_TICKET();
					RESUME_RECEPTION();
					/* CONTINUE_WITH_TICKET(); */
				} else {
					LOGC(L_ERROR, "[Receiver] recvmmsg() error '" + string(strerror(errno)) + "'", ACTOR_ERROR_COLORS );
					CANCEL_TICKET();
					ABORT_RECEPTION();
					/* BAIL_WITH_TICKET(); */
				}
			} else {
			// something received
				size_t fsize         = currentFormat.getFrameSize();
				bool   lookMore      = false;
				bool   drxRateChange = false;
				size_t cnt           = (size_t) res;
				uint16_t newDrxDecFactor;
				for (size_t c=0; c<(size_t)res; c++){
					bytesReceived += t->mhdrs[c].msg_len;
				}
				
				// measure lag, we do this on the last packet of the block since we receive large blocks at a time
				size_t tt = 0;
				switch(fsize){
					case DRX_FRAME_SIZE:
						tt = __builtin_bswap64(*((size_t*)(&((DrxFrame*)t->iovs[cnt-1].iov_base)->header.timeTag)));
						break;
					case TBN_FRAME_SIZE:
						tt = __builtin_bswap64(*((size_t*)(&((TbnFrame*)t->iovs[cnt-1].iov_base)->header.timeTag)));
						break;
					case TBW_FRAME_SIZE:
						tt = __builtin_bswap64(*((size_t*)(&((TbwFrame*)t->iovs[cnt-1].iov_base)->header.timeTag)));
						break;
					case TBF_FRAME_SIZE:
						tt = __builtin_bswap64(*((size_t*)(&((TbfFrame*)t->iovs[cnt-1].iov_base)->header.timeTag)));
						break;
					case COR_FRAME_SIZE:
						tt = __builtin_bswap64(*((size_t*)(&((CorFrame*)t->iovs[cnt-1].iov_base)->header.timeTag)));
					  break;
					case DRX8_FRAME_SIZE:
						tt = __builtin_bswap64(*((size_t*)(&((DrxFrame*)t->iovs[cnt-1].iov_base)->header.timeTag)));
						break;
					default : break;
				}
				if (tt!=0){
					microsecond now     = Time::micros_getSinceEpoch();
					microsecond tt_uspe = Time::micros_getSinceEpoch(tt);
					lag_latest = ((microsecond_delta)now)-((microsecond_delta)tt_uspe);
					Time::micros_getSinceEpoch(tt);
					if (!lag_initialized){
						lag_initialized = true;
						lag_initial = lag_latest;
					}
				}

				// quick check the common case, all packets right size, no change in decfactors
				for (size_t j=0; j<cnt; j++){
					if (fsize == DRX_FRAME_SIZE || fsize == DRX8_FRAME_SIZE){
						newDrxDecFactor = bswap16(((DrxFrame*) t->frames[j])->header.decFactor);
						if (newDrxDecFactor != currentDrxDecFactor){
							drxRateChange=true;
						}
					}
					if ((size_t) t->mhdrs[j].msg_len != fsize){
						lookMore=true;
						break;
					}
				}
				
				// check in case drx rate changed
				if ((fsize == DRX_FRAME_SIZE || fsize == DRX8_FRAME_SIZE) && drxRateChange && !lookMore){
					if (!setNewFmtDrx(newDrxDecFactor, (fsize == DRX8_FRAME_SIZE))){
						CANCEL_TICKET();
						RESUME_RECEPTION();
						/* CONTINUE_WITH_TICKET(); */
					}
				}
				
				if (!lookMore){
					POST_TICKET(cnt);
					RESUME_RECEPTION();
					/* CONTINUE_NO_TICKET(); */
				}
				
#define IDX_ERROR   0
#define IDX_EMPTY   1
#define IDX_TBN     2
#define IDX_TBW     3
#define IDX_TBF     4
#define IDX_COR     5
#define IDX_DRX     6
#define IDX_DRX8    7
#define IDX_ODDBALL 8
				
				// some count variables for deeper inspection
				size_t n[8]    = {0,0,0,0,0,0,0,0};         // in order : error, empty, tbn, tbw, tbf, cor, drx, odd
				size_t last[8] = {0,0,0,0,0,0,0,0};         // in order : error, empty, tbn, tbw, tbf, cor, drx, odd
				int    sz[8]   = {0,-1,TBN_FRAME_SIZE,TBW_FRAME_SIZE,TBF_FRAME_SIZE,COR_FRAME_SIZE,DRX_FRAME_SIZE,DRX8_FRAME_SIZE,-2}; // in order : error, empty, tbn, tbw, tbf, cor, drx, drx8, odd
				size_t curIdx;
				// count packet sizes
				for (size_t j=0; j<(size_t) res; j++){
					switch(t->mhdrs[j].msg_len){
						case    0:            n[IDX_EMPTY]++;   last[IDX_EMPTY]=j; break;
						case TBN_FRAME_SIZE:  n[IDX_TBN]++;     last[IDX_TBN]=j;   break;
						case TBW_FRAME_SIZE:  n[IDX_TBW]++;     last[IDX_TBW]=j;   break;
						case TBF_FRAME_SIZE:  n[IDX_TBF]++;     last[IDX_TBF]=j;   break;
						case COR_FRAME_SIZE:  n[IDX_COR]++;     last[IDX_COR]=j;   break;
						case DRX_FRAME_SIZE:  n[IDX_DRX]++;     last[IDX_DRX]=j;   break;
						case DRX8_FRAME_SIZE: n[IDX_DRX8]++;    last[IDX_DRX8]=j;  break;
						default:
							LOGC(L_DEBUG, "Bad size: " + LXS(t->mhdrs[j].msg_len), TRACE_COLORS);
							n[IDX_ODDBALL]++; last[IDX_ODDBALL]=j; break;
					}
				}
				switch(t->fsize){
					case    0:            curIdx = IDX_EMPTY;   break;
					case TBN_FRAME_SIZE:  curIdx = IDX_TBN;     break;
					case TBW_FRAME_SIZE:  curIdx = IDX_TBW;     break;
					case TBF_FRAME_SIZE:  curIdx = IDX_TBF;     break;
					case COR_FRAME_SIZE:  curIdx = IDX_COR;     break;
					case DRX_FRAME_SIZE:  curIdx = IDX_DRX;     break;
					case DRX8_FRAME_SIZE: curIdx = IDX_DRX8;    break;
					default:              curIdx = IDX_ODDBALL; break;
				}
				
				// find the maximum count of packets whose size is not our current size
				size_t max_count      = 0;
				int    new_frame_size = -3;
				size_t last_seen      = 0;
				for (size_t j=0; j<8; j++){
					if (j != curIdx){
						if (n[j] > max_count){
							max_count      = n[j];
							new_frame_size = sz[j];
							last_seen      = last[j];
						}
					}
				}
				
				// check that one of the 8 cases was a clear winner
				if (new_frame_size == -3){
					LOGC(L_FATAL, "[Receiver] Program error: lookmore triggerred, but no clear change.", FATAL_COLORS );
					LOGC(L_FATAL, "[Receiver] n     " + LXS((size_t) res), FATAL_COLORS );
					LOGC(L_FATAL, "[Receiver] n[0]  " + LXS(n[0]), FATAL_COLORS );
					LOGC(L_FATAL, "[Receiver] n[1]  " + LXS(n[1]), FATAL_COLORS );
					LOGC(L_FATAL, "[Receiver] n[2]  " + LXS(n[2]), FATAL_COLORS );
					LOGC(L_FATAL, "[Receiver] n[3]  " + LXS(n[3]), FATAL_COLORS );
					LOGC(L_FATAL, "[Receiver] n[4]  " + LXS(n[4]), FATAL_COLORS );
					LOGC(L_FATAL, "[Receiver] n[5]  " + LXS(n[5]), FATAL_COLORS );
					LOGC(L_FATAL, "[Receiver] n[6]  " + LXS(n[6]), FATAL_COLORS );
					LOGC(L_FATAL, "[Receiver] n[7]  " + LXS(n[7]), FATAL_COLORS );
					LOGC(L_FATAL, "[Receiver] n[8]  " + LXS(n[8]), FATAL_COLORS );
					CANCEL_TICKET();
					RESUME_RECEPTION();
					/* CONTINUE_WITH_TICKET(); */
				}
				
				// check that the winner wasn't the existing case
				if (new_frame_size == (int) t->fsize){
					LOGC(L_FATAL, "[Receiver] Program error: reached unreachable code. : " __FILE__ + LXS(__LINE__), FATAL_COLORS );
					CANCEL_TICKET();
					RESUME_RECEPTION();
					/* CONTINUE_WITH_TICKET(); */
				}
				
				// get the new format based on deeper packet inspection once we know the new size
				DrxFrame* f; // only drx might require looking at the actual frame
				switch(new_frame_size){
					case    0:
						// ignore, not a mode change
						LOGC(L_DEBUG, "[Receiver] Idle receiver. " , FATAL_COLORS );
						t->returnEmpty();
						continue;
						break;
					case   -1:
						// error state
						LOGC(L_FATAL, "[Receiver] Multiple I/O errors. " , FATAL_COLORS );
						CANCEL_TICKET();
						RESUME_RECEPTION();
						/* CONTINUE_WITH_TICKET(); */
					case   -2:
						// error state; many bad sizes
						LOGC(L_FATAL, "[Receiver] Multiple Bad data frames. " , FATAL_COLORS );
						CANCEL_TICKET();
						RESUME_RECEPTION();
						/* CONTINUE_WITH_TICKET(); */
					case TBN_FRAME_SIZE:
						// changed to TBN
						newFormat = DataFormat::getFormatByName("DEFAULT_TBN");
						break;
					case TBW_FRAME_SIZE:
						// changed to TBW
						newFormat = DataFormat::getFormatByName("DEFAULT_TBW");
						break;
					case TBF_FRAME_SIZE:
						// changed to TBF
						newFormat = DataFormat::getFormatByName("DEFAULT_TBF");
						break;
					case COR_FRAME_SIZE:
						// changed to COR
						newFormat = DataFormat::getFormatByName("DEFAULT_COR");
						break;
					case DRX_FRAME_SIZE:
						// changed to DRX
						f = (DrxFrame*) t->frames[last_seen];
						if (!setNewFmtDrx(bswap16(f->header.decFactor, false))){
							CANCEL_TICKET();
							RESUME_RECEPTION();
							/* CONTINUE_WITH_TICKET(); */
						}
						break;
					case DRX8_FRAME_SIZE:
						// changed to DRX8
						f = (Drx8Frame*) t->frames[last_seen];
						if (!setNewFmtDrx(bswap16(f->header.decFactor, true))){
							CANCEL_TICKET();
							RESUME_RECEPTION();
							/* CONTINUE_WITH_TICKET(); */
						}
						break;
					default:
						LOGC(L_FATAL, "[Receiver] Program error: reached unreachable code. : " __FILE__ + LXS(__LINE__), FATAL_COLORS );
						CANCEL_TICKET();
						ABORT_RECEPTION();
						/* BAIL_WITH_TICKET(); */
				}
				
				
				// now we have a new format, so set the reset required flag and leave
				// (next thread to lock both phase locks will actually do the reset
				LOGC(L_INFO, "[Receiver] Detected a data mode change to '"+newFormat.getName()+"'", ACTOR_COLORS );
				t->returnEmpty();
				
				if (!__doReset()){
					LOGC(L_FATAL, "[Receiver] failed to reset ticket buffer modes", FATAL_COLORS );
					CANCEL_TICKET();
					ABORT_RECEPTION();
					/* BAIL_WITH_TICKET(); */
				}
				
			}//
		} // while loop
		finalize();
		_done=true;
	} // run()




	virtual bool initialize(){
		LOGC(L_INFO, "[Receiver] initializing ...", ACTOR_COLORS );
		// return true if initialized successfully
		newFormat       = DataFormat::getFormatByName(DataFormat::defaultFormatName);
		newTransferSize = IDEAL_TRANSFER_SIZE;
		newBufSize      = INITIAL_BUFFER_SIZE;
		if (!__doReset()){
			LOGC(L_ERROR, "[Receiver] reset ticket buffer failed on initialization.", ACTOR_ERROR_COLORS );
			return false;
		}
		return __openSocket();
	}

	virtual void finalize(){
		LOGC(L_INFO, "[Receiver] finalizing ...", ACTOR_COLORS );
		__closeSocket();
	}

	uint16_t bswap16(uint16_t x){
		return ((x&0xff00)>>8)|((x&0x00ff)<<8);
	}

	bool isStarted() const { return _started; }
	bool isDone() const {    return _done;    }
	millisecond getLag(bool initial)const{
		return (initial) ? lag_initial : lag_latest;
	}



private:
	// socket stuff
	volatile int                   sd;
	struct sockaddr_storage        local_adr;
	int 				           sd_flags;   // indiv flags
	int 				           sd_m_flags; // multi flags
	string                         dataAddr;
	unsigned short                 dataPort;
	bool                           nonblocking;

	// buffer stuff
	TicketBuffer*                  buf;
	DataFormat                     currentFormat;
	size_t                         bufsize;
	size_t                         transferSize;
	int                            currentDrxDecFactor; // to detect mode changes

	// buffer geometry change stuff
	volatile bool                  resetRequired;
	DataFormat                     newFormat;
	size_t                         newTransferSize;
	size_t                         newBufSize;

	volatile bool _done;
	volatile bool _started;

	PeriodicReportTimer            rptTimer;
	microsecond_delta			   lag_initial;
	microsecond_delta			   lag_latest;
	bool						   lag_initialized;

	size_t                         bytesReceived;
	volatile bool                  externallyClosed;
	///////////////////////////////////////////////////////////////
	// private methods
	///////////////////////////////////////////////////////////////
	bool __openSocket(){
		sd  					    					= -1;
		(*((sockaddr_in*)&local_adr)).sin_family 		= AF_INET;
		(*((sockaddr_in*)&local_adr)).sin_port			= htons(dataPort);
        ::inet_pton(AF_INET, dataAddr.c_str(), &(((struct sockaddr_in *)&local_adr)->sin_addr));
		memset((*((sockaddr_in*)&local_adr)).sin_zero, '\0', sizeof (*((sockaddr_in*)&local_adr)).sin_zero);
		// create the socket
		sd = socket(AF_INET, SOCK_DGRAM, 0);
		if (sd < 0)	{
			cout << "[SOCKET] Error: socket() error\n";
			return false;
		}

		// Determine multicast status...
        int multicast = 0;
        sockaddr_in *sa4 = reinterpret_cast<sockaddr_in*>(&local_adr);
        if( ((sa4->sin_addr.s_addr & 0xFF) >= 224) \
            && ((sa4->sin_addr.s_addr & 0xFF) < 240) ) {
            multicast = 1;
        }
        if( multicast ) {
            LOGC(L_INFO, "[Receiver] multicast is requested on "+dataAddr, ACTOR_COLORS);
            cout << "[Receiver] multicast is requested\n";
        }
        
        // ... and work accordingly
        int result = 1;
        if( !multicast ) {
            // Normal address
            // bind the socket to localhost::port specified above
            result = ::bind(sd, (struct sockaddr *) &local_adr, (socklen_t)sizeof(struct sockaddr_storage));
            if (result != 0) {
                close(sd);
                LOGC(L_ERROR, "[Receiver] bind() error '" + string(strerror(errno)) + "'", ACTOR_ERROR_COLORS );
                cout << "[SOCKET] Error: bind() error\n";
                return false;
            }
        } else {
            // Multicast address
            // Setup the INADDR_ANY socket base to bind to
            struct sockaddr_in base_address;
            memset(&base_address, 0, sizeof(sockaddr_in));
            base_address.sin_family      = AF_INET;
            base_address.sin_port        = htons(dataPort);
            base_address.sin_addr.s_addr = INADDR_ANY;
            result = ::bind(sd, (struct sockaddr *) &local_adr, (socklen_t)sizeof(struct sockaddr_storage));
            if (result != 0) {
                close(sd);
                LOGC(L_ERROR, "[Receiver] bind() error '" + string(strerror(errno)) + "'", ACTOR_ERROR_COLORS );
                cout << "[SOCKET] Error: bind() error\n";
                return false;
            }
            
            // Deal with joining the multicast group
            struct ip_mreq mreq;
            memset(&mreq, 0, sizeof(ip_mreq));
            mreq.imr_multiaddr.s_addr = reinterpret_cast<sockaddr_in*>(&local_adr)->sin_addr.s_addr;
            mreq.imr_interface.s_addr = INADDR_ANY;
            result = ::setsockopt(sd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq, sizeof(mreq));
            if (result != 0) {
                close(sd);
                LOGC(L_ERROR, "[Receiver] setsockopt() error '" + string(strerror(errno)) + "'", ACTOR_ERROR_COLORS );
                cout << "[SOCKET] Error: setsockopt() error\n";
                return false;
            }
            LOGC(L_INFO, "[Receiver] multicast is done", ACTOR_COLORS);
        }
        
		if (nonblocking){
			//* set socket to be non-blocking
			result = fcntl(sd, F_GETFL);
			if (result < 0){
				LOGC(L_ERROR, "[Receiver] fcntl(sd, F_GETFL) error '" + string(strerror(errno)) + "'", ACTOR_ERROR_COLORS );
				close(sd);
				return false;
			}
			result = result | O_NONBLOCK;
			result = fcntl(sd, F_SETFL, result);
			if (result < 0){
				LOGC(L_ERROR, "[Receiver] fcntl(sd, F_SETFL) error '" + string(strerror(errno)) + "'", ACTOR_ERROR_COLORS );
				close(sd);
				return false;
			}
			sd_flags   = MSG_DONTWAIT | MSG_TRUNC;
			sd_m_flags = MSG_DONTWAIT | MSG_TRUNC;
		} else {
			sd_m_flags = MSG_WAITALL | MSG_TRUNC;
		}

		LOGC(L_INFO, "[Receiver] opened data socket on port " + LXS(dataPort), ACTOR_COLORS );

		cout << "Socket opened on port "<< dataPort << ".\n";
		return true;
	}
	void __closeSocket(){
		if (sd != -1){
			close(sd);
			sd = -1;
			LOGC(L_INFO, "[Receiver] closed data socket on port " + LXS(dataPort), ACTOR_ERROR_COLORS );
		} else {
			LOGC(L_WARNING, "[Receiver] Tried to close already closed socket on on port" + LXS(dataPort), ACTOR_ERROR_COLORS );
		}
	}

	void __clearTicket(TicketBuffer::Ticket* t, bool returnEmptyOnComplete = false){
		size_t count = t->cnt_alloc;
		for (size_t i=0; i<count; i++){
			t->mhdrs[i].msg_len=0;
			t->mhdrs[i].msg_hdr.msg_flags = MSG_TRUNC;
		}
		if (returnEmptyOnComplete)
			t->returnEmpty();
	}

	bool __doReset(){
		size_t fsize = newFormat.getFrameSize();
		if (!newBufSize || !newTransferSize || !fsize){
			LOGC(L_ERROR, "[Receiver] supplied 0 for one of the ticket buffer dimensions.", ACTOR_ERROR_COLORS );
			this->resetRequired    = false;
			return false;
		}
		if (newTransferSize > newBufSize){
			LOGC(L_ERROR, "[Receiver] tried to set transfer size larger than buf size.", ACTOR_ERROR_COLORS );
			this->resetRequired    = false;
			return false;
		}
		size_t newFramesPerTicket    = newTransferSize / fsize;
		size_t newFramesPerTicketFix = newFramesPerTicket * fsize;
		size_t tickets               = newBufSize / newFramesPerTicketFix;
		if (tickets < 3){
			LOGC(L_ERROR, "[Receiver] tried to set ticket buffer with too few tickets.", ACTOR_ERROR_COLORS );
			return false;
		}
		buf->reset(newBufSize, fsize, newFramesPerTicket);
		this->bufsize             = newBufSize;
		this->currentFormat       = newFormat;
		this->transferSize        = newTransferSize;
		this->currentDrxDecFactor = newFormat.getDecFactor();
		this->resetRequired       = false;
		return true;

	}



};



#endif /* RECEIVER_H_ */

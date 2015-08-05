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

#ifndef TICKETBUFFER_H_
#define TICKETBUFFER_H_
#include "../Threading/LockHelper.h"
#include "../System/Log.h"
#include "../System/Storage.h"
#include "../Primitives/StdTypes.h"


#include "../Data/LwaDataFormats.h"

#define ALLOC(v, newSize, type)													\
{																				\
		type* tmp = (type*) realloc(v, newSize);								\
		if (!tmp){																\
			LOGC(L_FATAL, "Can't allocate memory for " #v, FATAL_COLORS);		\
			return false;  														\
		} else {																\
			haveResources = true;												\
			v = tmp;															\
		}																		\
}

typedef struct __TB_Geometry{
	__TB_Geometry(const size_t& _size=0, const  size_t& _framesize=0, const  size_t& _framesPerTicket=0):
		size(_size),framesize(_framesize),framesPerTicket(_framesPerTicket){
	}

	__TB_Geometry(const __TB_Geometry& tc):size(tc.size),framesize(tc.framesize),framesPerTicket(tc.framesPerTicket){
	}


	size_t size;
	size_t framesize;
	size_t framesPerTicket;
}TB_Geometry;

class TicketBuffer {
public:

	////////////////////////////////////////////////////////
	//
	// The Ticket encapsulates where data is, and provides a
	// mechanism for multiple consumers to use the same data.
	// When all consumers have finished with the data,
	// the ticket is returned to the free pool
	//
	////////////////////////////////////////////////////////
	class Ticket{
		friend class TicketBuffer;
		friend class ProducerProxyTicket;
	public:
		Ticket():frames(NULL),iovs(NULL),mhdrs(NULL),cnt_alloc(0),valid(NULL),fsize(0),cnt_used(0),owner(NULL),ref_cnt(0),_continuous(false),_proxyCancelled(true){}
		virtual ~Ticket(){
		}
		void**          frames;     // set by buffer
		struct iovec*   iovs;       // set by buffer
		struct mmsghdr* mhdrs;      // set by buffer
		size_t          cnt_alloc;	// set by buffer
		bool *          valid;      // set by buffer
		size_t          fsize;      // common frame size
		size_t          cnt_used;   // set by producer
		// called by consumers only
		void put(){
			SERIALIZE_ACCESS();
			ref_cnt--;
			if (!ref_cnt){
				owner->releaseTicket(this);
			}
		}
		bool isContinuous()const {
			return _continuous;
		}
		void reGet(size_t cnt){
			SERIALIZE_ACCESS();
			this->ref_cnt += cnt;
		}

		// called by producers only
		void notifyFill(size_t cnt){
			if (cnt == 0)
				returnEmpty();
			else {
				cnt_used = cnt;
				owner->useTicket(this);
			}
		}
		void returnEmpty(){
			cnt_used = 0;
			owner->releaseTicket(this,true);
		}

		// called by producers, but not to be mixed with the notifyFill/returnEmpty members above
		// *get is called once to set the ref counts
		// *cancel is caled one or more times to indicate the ticket should be reclaimed once all producers are done
		// *put adds to the cnt used, and decreases reference count.
		// If ref count goes to 0, then the ticket is either queued or reclaimed
		// if cancelling, producer must cancel before putting
		void getProducerProxy(size_t peers, bool preservesContinuity){
			SERIALIZE_ACCESS();
			_proxyCancelled=false;
			cnt_used    = 0;
			_continuous = preservesContinuity;
			ref_cnt = peers;
		}
		void cancelProducerProxy(){
			SERIALIZE_ACCESS();
			_proxyCancelled=true;
		}
		void putProducerProxy(size_t _cnt_used){
			SERIALIZE_ACCESS();
			cnt_used += _cnt_used;
			ref_cnt--;
			if (ref_cnt == 0){
				if (_proxyCancelled){
					owner->releaseTicket(this,true);
				} else {
					owner->useTicket(this);
				}
			}
		}



		/*
		// debugging
		void print(){
			cout << "========================== Ticket ===============================\n";
			for (int i=0; i<10; i++){
				cout << "=== record " << i  << " ===" << endl;
				cout << "Frame pointer:    " << hex << (size_t) frames[i] << endl;
				cout << "IOV self pointer: " << hex << (size_t) &iovs[i] << endl;
				cout << "IOV base pointer: " << hex << (size_t) iovs[i].iov_base << endl;
				cout << "IOV length:       " << dec << (size_t) iovs[i].iov_len << endl;
				cout << "MHDR len:         " << dec << (size_t) mhdrs[i].msg_len << endl;
				cout << "MHDR ctrl ptr:    " << hex << (size_t) mhdrs[i].msg_hdr.msg_control    << endl;
				cout << "MHDR ctrl len:    " << dec << (size_t) mhdrs[i].msg_hdr.msg_controllen << endl;
				cout << "MHDR iov ptr:     " << hex << (size_t) mhdrs[i].msg_hdr.msg_iov        << endl;
				cout << "MHDR iov len:     " << dec << (size_t) mhdrs[i].msg_hdr.msg_iovlen     << endl;
				cout << "MHDR name ptr:    " << hex << (size_t) mhdrs[i].msg_hdr.msg_name       << endl;
				cout << "MHDR name len:    " << dec << (size_t) mhdrs[i].msg_hdr.msg_namelen    << endl;
				cout << "MHDR flags:       " << dec << (size_t) mhdrs[i].msg_hdr.msg_flags      << endl;
			}
			cout << "========================== End Ticket ===============================\n";

		}
		*/
	private:
		TicketBuffer*   owner;
		size_t          ref_cnt;
		bool            _continuous;// set by buffer
		bool            _proxyCancelled;
		DECLARE_ACCESS_MUTEX();
		void init(
				TicketBuffer* owner,
				void** frames,
				struct iovec* iovs,
				struct mmsghdr* mhdrs,
				size_t cnt_alloc,
				bool* valid,
				size_t fsize,
				bool continuous){
			this->owner     = owner;
			this->frames    = frames;
			this->iovs      = iovs;
			this->mhdrs     = mhdrs;
			this->cnt_alloc = cnt_alloc;
			this->valid     = valid;
			this->fsize     = fsize;
			this->cnt_used  = 0;
			this->ref_cnt   = 0;
			this->_continuous = continuous;

		}

		// called by parent only
		void get(size_t cnt){this->ref_cnt = cnt;}
	};
/*
	////////////////////////////////////////////////////////
	// The ConsumerProxyTicket enables redistributing the ticket to
	// multiple consumers, where the last to release
	// handles releasing the ticket itself
	// note: there is no extra locking, since a proxy ticket
	// should never be used by more than one thread
	////////////////////////////////////////////////////////
	class ConsumerProxyTicket{
	public:
		ConsumerProxyTicket(Ticket* _t=NULL, void* _privateData=NULL):
			t(_t),released(false),privateData(_privateData){}
		~ConsumerProxyTicket(){}
		Ticket* getTicket(){return t;}
		void put(){	if (t && !released) {t->put();released=true;}}
		void* getPrivateData(){return privateData;}
	private:
		Ticket* t; // the ticket we're latched on to
		bool    released;
		void*   privateData; // convenience reference
	};

	////////////////////////////////////////////////////////
	// The Producer Proxy Ticket enables redistributing the
	// ticket to producers, where the last to release
	// handles pushing the ticket into the buffer
	// again, no extra locking
	////////////////////////////////////////////////////////
	class ProducerProxyTicket{
	public:
		ProducerProxyTicket(Ticket* _t=NULL, void* _privateData=NULL):
			t(_t),released(false),privateData(_privateData){}
		~ProducerProxyTicket(){}
		Ticket* getTicket(){return t;}
		void notifyFill(size_t count){ if (t && !released){ t->putProducerProxy(count);}}
		void returnEmpty(){ if (t && !released){ t->cancelProducerProxy(); t->putProducerProxy(0);}}
		void* getPrivateData(){return privateData;}
	private:
		Ticket* t;           // the ticket we're latched on to
		bool    released;
		void*   privateData; // convenience reference
	};
*/
	////////////////////////////////////////////////////////
	// The Ticket Subscriber does something useful with the
	// data. i.e. is a consumer
	////////////////////////////////////////////////////////
	class Subscriber{
	public:
		Subscriber(string name):sub_name(name){}
		string getSubscriberName()const{return sub_name;}
		virtual ~Subscriber(){
		}
		// derived class can put right away, or queue and put later
		virtual void onTicketAvailable(Ticket* ticket){ticket->put();}
		// derived class must free any tickets not put'd, but may do so asynchronously
		virtual void onReset(){}
		// will be called only once all outstanding tickets have been returned by all co-subscribers
		virtual void onNewGeometry(const size_t& size, const size_t& framesize, const size_t& framesPerTicket){}
		// called when the buffer reaches end of life. subscribers are still
		// responsible for returning tickets and unsubscribing
		virtual void notifyDisconnected(){}
	private :
		string sub_name;
	};




	////////////////////////////////////////////////////////
	//
	// The Ticket Buffer manages data flow between producer(only 1) and consumer(s)
	//
	////////////////////////////////////////////////////////
	friend class Ticket;
	TicketBuffer(string name):
		tb_name(name),
		size(0),	framesize(0),	framesPerTicket(0),	numTickets(0),	numFrames(0),
		dataArea(NULL),	frames(NULL),	iovs(NULL),	mhdrs(NULL), valid(NULL),
		freeTickets(0),	subscribers(0),
		_invalid(true), haveResources(false){
		// nothing to do
	}

	virtual ~TicketBuffer(){
		____freeResources();
	}
	/////////////////////////////////////
	// producer interface
	/////////////////////////////////////
	// must not be called while holding a ticket (use ticket->returnEmpty() first)

	void reset(const TB_Geometry& geometry){
		reset(geometry.size, geometry.framesize, geometry.framesPerTicket);
	}

	virtual void reset(const size_t& size, const size_t& framesize, const size_t& framesPerTicket){
		__invalidate(); // should block until all tickets are returned
		_invalid=!__resize(size, framesize, framesPerTicket);
		// now we're valid
	}
	virtual Ticket* nextFreeTicket(){
		if (_invalid) return NULL;
		SERIALIZE_ACCESS(free_list);
		if (freeTickets.empty()){
			//LOGC(L_FATAL, "_____________BUFFER UNDERFLOW ______________ ", FATAL2_COLORS);
			return NULL;
		}
		Ticket* rv = freeTickets.front();
		if (rv) {
			freeTickets.pop_front();
		}
		//LOGC(L_FATAL, "______________________ Ticket Acquired", FATAL2_COLORS);
		return rv;
	}
	virtual void uninit(){
		__invalidate();
		__freeResources();
	}

	/////////////////////////////////////
	// consumer interface
	/////////////////////////////////////
	virtual void subscribe(Subscriber* who){
		if (!who){
			LOGC(L_FATAL, "Program error: called subscribe with null pointer.", FATAL_COLORS);
		} else {

			LOGC(L_DEBUG, "[TicketBuffer:"+this->getBufferName()+"] <SUBSCRIBE:"+who->getSubscriberName()+">", ACTOR_WARNING_COLORS);
			SERIALIZE_ACCESS(subscribers_list);
			subscribers.push_back(who);
			who->onNewGeometry(size, framesize, framesPerTicket);
		}
	}

	virtual void unsubscribe(Subscriber* who, bool destructorAuto = false){
		LOGC(L_DEBUG, "[TicketBuffer:"+this->getBufferName()+"] ENTER UNSUBSCRIBE >", ACTOR_WARNING_COLORS);
		if (!who){
			LOGC(L_FATAL, "Program error: called unsubscribe with null pointer.", FATAL_COLORS);
		} else {

			SERIALIZE_ACCESS(subscribers_list);
			deque<Subscriber*>::iterator it =subscribers.begin();
			bool erased = false;
			while(it != subscribers.end()){
				if (*it == who){
					LOGC(L_DEBUG, "[TicketBuffer:"+this->getBufferName()+"] <UNSUBSCRIBE:"+who->getSubscriberName()+">", ACTOR_WARNING_COLORS);
					subscribers.erase(it);
					it =subscribers.begin();
					erased = true;
				}
			}
			if (subscribers.empty()){
				LOGC(L_DEBUG, "[TicketBuffer:"+this->getBufferName()+"] Last subscriber unsubscribed", ACTOR_COLORS);
				if (freeTickets.size() < (numTickets-1)){ // will likely be one inbound
					LOGC(L_FATAL, "[TicketBuffer:"+this->getBufferName()+"] No subscribers, but free pool is not full", ACTOR_COLORS);
				}
			}
			if (!erased && !destructorAuto){
				LOGC(L_FATAL, "Program error: called unsubscribe when not a subscriber.", FATAL_COLORS);
			}
		}
		LOGC(L_DEBUG, "[TicketBuffer:"+this->getBufferName()+"] LEAVE UNSUBSCRIBE >", ACTOR_WARNING_COLORS);
	}


	bool haveSubscribers(){
		SERIALIZE_ACCESS(subscribers_list);
		return !subscribers.empty();
	}
	size_t getSubscriberCount(){
		SERIALIZE_ACCESS(subscribers_list);
		return subscribers.size();
	}
	void notifyAllSubscribersDisconnect(){
		LOGC(L_DEBUG, "[TicketBuffer:"+this->getBufferName()+"] begin notifyAllSubscribersDisconnect()", TRACE_COLORS);
		{
			SERIALIZE_ACCESS(subscribers_list);
			foreach(Subscriber* sub, subscribers){
				LOGC(L_DEBUG, "[TicketBuffer:"+this->getBufferName()+"] Notified '"+sub->getSubscriberName()+"'", TRACE_COLORS);
				sub->notifyDisconnected();
			}
		}
		LOGC(L_DEBUG, "[TicketBuffer:"+this->getBufferName()+"] All subscribers notified", TRACE_COLORS);

	}

	virtual bool isValid(){
		return !_invalid;
	}


	int usagePct(){
		size_t unused;
		{
			SERIALIZE_ACCESS(free_list);
			unused=freeTickets.size();
		}
		return (100ll*(numTickets-unused))/numTickets;
	}
	string getBufferName()const{return tb_name;}

private:
	string                    tb_name;
	size_t                    size;
	size_t                    framesize;
	size_t                    framesPerTicket;
	size_t                    numTickets;
	size_t                    numFrames;

	void*                     dataArea;
	void**                    frames;
	struct iovec*             iovs;
	struct mmsghdr*           mhdrs;
	bool*                     valid;
	deque<Ticket*>            freeTickets;
	deque<Subscriber*>        subscribers;
	volatile bool 			  _invalid;
	volatile bool 			  haveResources;

	DECLARE_ACCESS_MUTEX(free_list);
	DECLARE_ACCESS_MUTEX(subscribers_list);



	////////////////////////////////////////////////
	// ticket interface (called by Ticket as needed)
	////////////////////////////////////////////////
	void useTicket(Ticket* which){
		if (_invalid){
			LOGC(L_FATAL, "Program error: called useTicket while _invalid asserted.", FATAL_COLORS);
			exit(-1);
		} else {
			SERIALIZE_ACCESS(subscribers_list);
			if (subscribers.empty()){
				releaseTicket(which);
				if (freeTickets.size() != numTickets){
					LOGC(L_FATAL, "Ticket lost: only have " +LXS(freeTickets.size()) + " out of " + LXS(numTickets) + " tickets", FATAL_COLORS);
				}
			} else {
				//LOGC(L_FATAL, "______________________ Ticket Used", FATAL_COLORS);
				which->get(subscribers.size());
				foreach(Subscriber* sub, subscribers){
					sub->onTicketAvailable(which);
				}
			}
		}
	}

	void releaseTicket(Ticket* which, bool fromReturnEmpty=false){
		//LOGC(L_FATAL, "______________________ Ticket Released" + string(fromReturnEmpty ? " (empty)" : " (no subscribers)"), FATAL_COLORS);
		SERIALIZE_ACCESS(free_list);
		freeTickets.push_back(which);
	}

	////////////////////////////////////////////////
	// internal mgmt.
	////////////////////////////////////////////////
	void __invalidate(){
		if (_invalid)
			return;
		LOGC(L_DEBUG, "[TicketBuffer:"+this->getBufferName()+"] Begin __invalidate()", TRACE_COLORS);
		{
			SERIALIZE_ACCESS(subscribers_list);
			foreach(Subscriber* sub, subscribers){
				sub->onReset();
			}
			_invalid=true;
		}
		bool done=false;
		while (!done){
			SERIALIZE_ACCESS(free_list);
			done = freeTickets.size() == numTickets;
			//LOGC(L_DEBUG, "__invalidate() waiting ... " +LXS(freeTickets.size())+" of "+LXS(numTickets), TRACE_COLORS);
		}
		LOGC(L_DEBUG, "[TicketBuffer:"+this->getBufferName()+"] Finish __invalidate()", TRACE_COLORS);
	}
	bool __resize(const size_t& size, const size_t& framesize, const size_t& framesPerTicket){
		LOGC(L_DEBUG, "[TicketBuffer:"+this->getBufferName()+"] Resizing to "+Storage::humanReadable(size)+" with "+Storage::humanReadable(framesize)+" frames and "+LXS(framesPerTicket)+" frames/ticket", TRACE_COLORS);

		size_t newSize = size;
		size_t newNumFrames    = size/framesize;
		size_t newNumFramesFix = ((((newNumFrames/framesPerTicket)+1)*framesPerTicket)-framesPerTicket);
		size_t newNumTickets = newNumFramesFix/framesPerTicket;
		if (this->size != newSize){
			ALLOC(dataArea, newSize, void);
		}
		if (this->numFrames != newNumFrames){
			ALLOC(frames,  newNumFramesFix * sizeof(void*),          void*   );
			ALLOC(iovs,    newNumFramesFix * sizeof(struct iovec),   struct iovec  );
			ALLOC(mhdrs,   newNumFramesFix * sizeof(struct mmsghdr), struct mmsghdr);
			ALLOC(valid,   newNumFramesFix * sizeof(bool),           bool);
		}

		if (this->numTickets != newNumTickets){
			size_t created = 0;
			size_t destroyed = 0;
			while(freeTickets.size() < newNumTickets){
				created++;
				Ticket* tmp = new Ticket();
				if (!tmp){
					LOGC(L_FATAL, "Can't allocate memory for ticket", FATAL_COLORS);
					return false;
				} else {
					haveResources = true;
					freeTickets.push_back(tmp);
				}
			}
			while(freeTickets.size() > newNumTickets){
				destroyed++;
				Ticket* tmp = freeTickets.front();
				freeTickets.pop_front();
				if (tmp) delete (tmp);
			}
			LOGC(L_DEBUG, "[TicketBuffer:"+this->getBufferName()+"] Destroyed "+LXS(destroyed)+" old tickets", TRACE_COLORS);
			LOGC(L_DEBUG, "[TicketBuffer:"+this->getBufferName()+"] Created "+LXS(created)+" new tickets", TRACE_COLORS);
		}



		bzero((void*) iovs,  newNumFramesFix * sizeof(struct iovec));
		bzero((void*) mhdrs, newNumFramesFix * sizeof(struct mmsghdr));
		for (size_t i=0; i< newNumFramesFix; i++){
			iovs[i].iov_base = (void*)(&(((char*)dataArea)[i*framesize]));
			iovs[i].iov_len  = framesize;
			mhdrs[i].msg_hdr.msg_iov    = &iovs[i];
			mhdrs[i].msg_hdr.msg_iovlen = 1;
			frames[i]=iovs[i].iov_base;
			valid[i] = false;
		}

		size_t i=0;
		foreach(Ticket* t, freeTickets){
			t->init(this,
					&frames[i*framesPerTicket],
					&iovs[i*framesPerTicket],
					&mhdrs[i*framesPerTicket],
					framesPerTicket,
					&valid[i*framesPerTicket],
					framesize,
					true
			);
			i++;
		}

		this->size=size;
		this->framesize=framesize;
		this->framesPerTicket=framesPerTicket;
		this->numTickets=newNumTickets;
		this->numFrames=newNumFramesFix;
		// let all subscribers know the new geometry
		{
			SERIALIZE_ACCESS(subscribers_list);
			foreach(Subscriber* sub, subscribers){
				sub->onNewGeometry(size, framesize, framesPerTicket);
			}
		}
		LOGC(L_DEBUG, "[TicketBuffer:"+this->getBufferName()+"] Resizing complete ", TRACE_COLORS);
		return true;
	}

	void __freeResources(){
		SERIALIZE_ACCESS(free_list);
		SERIALIZE_ACCESS(subscribers_list);
		____freeResources();

	}
	void ____freeResources(){
		if (!haveResources) return;
		LOGC(L_DEBUG, "[TicketBuffer:"+this->getBufferName()+"] Releasing resources", TRACE_COLORS);
		if (dataArea) {free(dataArea); dataArea=NULL; }
		if (frames)   {free(frames);   frames=NULL;   }
		if (iovs)     {free(iovs);     iovs=NULL;     }
		if (mhdrs)    {free(mhdrs);    mhdrs=NULL;    }
		if (valid)    {free(valid);    valid=NULL;    }
		{
			SERIALIZE_ACCESS(free_list);
			while(!freeTickets.empty()){
				Ticket* tmp = freeTickets.front();
				freeTickets.pop_front();
				if (tmp) delete (tmp);
			}
		}
		LOGC(L_DEBUG, "[TicketBuffer:"+this->getBufferName()+"] Resource release complete ", TRACE_COLORS);
		haveResources = false;
	}


};

#endif /* TICKETBUFFER_H_ */

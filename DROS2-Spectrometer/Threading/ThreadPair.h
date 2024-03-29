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



#ifndef THREADPAIR_H_
#define THREADPAIR_H_

#include "GenericThread.h"
class ThreadPair: public GenericThread {
public:
	ThreadPair(string name, PriorityGroup _pg, int _id=0):
		GenericThread(name, _pg, _id),
		t1(NULL),
		t2(NULL){
	}

	bool isInterrupted(){
		return interrupted;
	}
	virtual void start(){
		t1 = new boost::thread(boost::bind(&ThreadPair::__run, this, 0));
		t2 = new boost::thread(boost::bind(&ThreadPair::__run, this, 1));
	}

	virtual void stop(){
		if (t1!=NULL){t1->join();}
		if (t2!=NULL){t2->join();}
	}

	virtual void doAbort(){
		interrupted = true;
	}

	virtual void yield(){
		boost::thread::yield();
	}

	virtual ~ThreadPair(){
		if (t1) delete (t1);
		if (t2) delete (t2);
	}

	virtual void run_master(){

	}
	virtual void run_slave(){

	}

	void sendToMaster(void* o){
		SERIALIZE_ACCESS(s2m);
		if (o!=NULL) s2m.push_back(o);
	}
	void sendToSlave(void* o){
		SERIALIZE_ACCESS(m2s);
		if (o!=NULL) m2s.push_back(o);
	}
	void* getFromMaster(){
		SERIALIZE_ACCESS(m2s);
		if (!m2s.empty()){
			void* rv = m2s.front();
			m2s.pop_front();
			return rv;
		} else return NULL;
	}
	void* getFromSlave(){
		SERIALIZE_ACCESS(m2s);
		if (!s2m.empty()){
			void* rv = s2m.front();
			s2m.pop_front();
			return rv;
		} else return NULL;
	}


protected:
	ThreadPair(){}
	boost::thread* t1;
	boost::thread* t2;
	deque<void*> s2m;
	deque<void*> m2s;

private:
	DECLARE_ACCESS_MUTEX(m2s);
	DECLARE_ACCESS_MUTEX(s2m);

	void __run(int which){
		ThreadManager::getInstance()->announceSelf(AbstractThread::getName()+":"+LXS(which));
		// set cpu affinity
		int cpu = GenericThread::getTargetCpu(pg,id);
		cpu_set_t cpuset;
		CPU_ZERO(&cpuset);
		CPU_SET(cpu,&cpuset);
		if(sched_setaffinity(getpid(),sizeof(cpuset),&cpuset)<0){
			LOGC(L_WARNING, "[ThreadPair] : Can't set CPU affinity : '"+string(strerror(errno))+"'", ACTOR_WARNING_COLORS);
		} else {
			LOGC(L_INFO, "[ThreadPair] : Set CPU affinity : '"+LXS(cpu)+"'", ACTOR_COLORS);
		}
		// set priority
		int priority = GenericThread::getTargetPriority(pg,id);
		int res = setpriority(PRIO_PROCESS, 0, priority);
		if (res<0){
			LOGC(L_WARNING, "[ThreadPair] : Can't set priority : '"+string(strerror(errno))+"'", ACTOR_WARNING_COLORS);
		} else {
			LOGC(L_INFO, "[ThreadPair] : Set priority : '"+LXS(priority)+"'", ACTOR_COLORS);
		}
		if (which==0)
			run_master();
		else
			run_slave();
	}
};


#endif /* THREADPAIR_H_ */

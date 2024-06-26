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

#include "Schedule.h"


OpIdle Schedule::op_idle;

DEFN_ENUM_STR(E_ScS, E_ScS_VALS);
SINGLETON_CLASS_SOURCE(Schedule);

SINGLETON_CLASS_CONSTRUCTOR(Schedule):
	GenericThread("Schedule",PG_MODERATE),
	currentOperation((ScheduledOperation*)&op_idle){
}

SINGLETON_CLASS_DESTRUCTOR(Schedule){
}
void Schedule::doAbort(){
	SERIALIZE_ACCESS();
	ScheduledOperation* op = NULL;

	// cancel queued-but-not-started
	while ((op = opsQueue.pop_front()) != NULL){
		LOGC(L_INFO, "[Schedule] Cancelling unstarted operation: " + op->toString() , ACTOR_COLORS);
		op->doCancel();
		while(!op->isDone());
		LOGC(L_INFO, "[Schedule] completed cancelation: " + op->toString(), SCHEDULE_COLORS);
		delete op;
	}
	// cancel dequeued-and-started
	if (((Operation*)currentOperation != (Operation*)&op_idle)){
		LOGC(L_INFO, "[Schedule] Cancelling running operation: " + currentOperation->toString() , ACTOR_COLORS);
		currentOperation->doCancel();
		while(!currentOperation->isDone());
		LOGC(L_INFO, "[Schedule] completed cancelation: " + currentOperation->toString(), SCHEDULE_COLORS);
		ScheduledOperation* tmp = currentOperation;
		currentOperation = (ScheduledOperation*)&op_idle;
		delete tmp;
	}
	GenericThread::doAbort();
}
void  Schedule::run(){
	static bool announcedUnreachable = false;
	size_t sleepTime;
	LOGC(L_INFO, "[Schedule] started scheduler.", ACTOR_COLORS);
	while(!isInterrupted()){
		switch(getState()){
			case SchBusy:
				sleepTime=doBusy();
				if (sleepTime) usleep(sleepTime);
				break;
			case SchWait:
				sleepTime=doWait();
				if (sleepTime) usleep(sleepTime);
				break;
			case SchIdle:
				usleep(100000); // 100 ms
				break;
			default:
				if (!announcedUnreachable){
					LOGC(L_ERROR,"[Scheduler] Reached unreachable code.", ACTOR_ERROR_COLORS);
					announcedUnreachable = true;
				}
				continue;
		}
	}
	LOGC(L_INFO, "[Schedule] stopped scheduler.", ACTOR_COLORS);
}

#define MAX_WAIT 200000 /* 200 ms*/
#define MIN_WAIT 50000  /* 50  ms*/
static microsecond reasonableWaitTime(TimeStamp nextEvent, TimeStamp now, bool precise=false){
	TimeStamp delta = Time::getTimeStampDelta(now,nextEvent);
	if (delta.MJD != 0)       return 5*MAX_WAIT; // 1s
	if (delta.MPM > MAX_WAIT) return MAX_WAIT;
	if (delta.MPM < MIN_WAIT) return (precise) ? 0 : MIN_WAIT;
	return (precise) ? 0 : delta.MPM * 900ll; // 90% of wait time
}

microsecond Schedule::doWait(){
	SERIALIZE_ACCESS();
	ScheduledOperation* next = opsQueue.front();
	TimeSlot ts = next->getTimeSlot(true);
	TimeStamp opstart = ts.start;
	TimeStamp now = Time::now();
	if (Time::compareTimestamps(opstart,now)<=0){
		currentIsCancelled=false;
		notifiedWindowStart=false;
		notifiedExecutionStart=false;
		notifiedExecutionStop=false;
		notifiedWindowStop=false;
		currentOperation = next;
		opsQueue.pop_front();
		return 0;
	} else {
		return reasonableWaitTime(opstart, now, true);
	}
}

microsecond  Schedule::doBusy(){
	static TimeStamp nextEvent;
	SERIALIZE_ACCESS();
	if (!currentIsCancelled){
		TimeStamp now=Time::now();
		if (!notifiedWindowStart){
			TimeSlot w = currentOperation->getTimeSlot(true);
			nextEvent = w.start;
			if (Time::compareTimestamps(nextEvent,now)<=0){
				LOGC(L_INFO, "[Schedule] window start current operation: " + currentOperation->toString(), SCHEDULE_COLORS);
				TimeSlot e = currentOperation->getTimeSlot(false);
				nextEvent = e.start;
				currentOperation->doWindowStart();
				notifiedWindowStart = true;
				return 0;
			} else {
				return reasonableWaitTime(nextEvent, now, true);
			}
		} else if (!notifiedExecutionStart){
			if (Time::compareTimestamps(nextEvent,now)<=0){
				LOGC(L_INFO, "[Schedule] execution start current operation: " + currentOperation->toString(), SCHEDULE_COLORS);
				currentOperation->doExecutionStart();
				notifiedExecutionStart = true;
				TimeSlot e = currentOperation->getTimeSlot(false);
				if (e.duration == FOREVER){
					notifiedExecutionStop = true;
					notifiedWindowStop = true;
				} else {
					nextEvent = Time::addTime(e.start,e.duration);
				}
				return 0;
			} else {
				return reasonableWaitTime(nextEvent, now, true);
			}
		} else if (!notifiedExecutionStop){
			if (Time::compareTimestamps(nextEvent,now)<=0){
				LOGC(L_INFO, "[Schedule] execution stop current operation: " + currentOperation->toString(), SCHEDULE_COLORS);
				currentOperation->doExecutionStop();
				notifiedExecutionStop = true;
				TimeSlot w = currentOperation->getTimeSlot(true);
				nextEvent = Time::addTime(w.start,w.duration);
				return 0;
			} else {
				return reasonableWaitTime(nextEvent, now, false);
			}
		} else if (!notifiedWindowStop){
			if (Time::compareTimestamps(nextEvent,now)<=0){
				LOGC(L_INFO, "[Schedule] window stop current operation: " + currentOperation->toString(), SCHEDULE_COLORS);
				currentOperation->doWindowStop();
				notifiedWindowStop = true;
				return 0;
			} else {
				return reasonableWaitTime(nextEvent, now, false);
			}
		} else {
			if (currentOperation->isDone()){
					LOGC(L_INFO, "[Schedule] completed as scheduled: " + currentOperation->toString(), SCHEDULE_COLORS);
					ScheduledOperation* tmp = currentOperation;
					currentOperation = (ScheduledOperation*)&op_idle;
					delete tmp;
					//LOGC(L_INFO, "[Schedule] ===========================================", SCHEDULE_COLORS);
					return 0;
			} else{
				return 1000; // 1 ms wait
			}
		}
	} else {

		if (currentOperation->isDone()){
			LOGC(L_INFO, "[Schedule] completed cancellation: " + currentOperation->toString(), SCHEDULE_COLORS);
			ScheduledOperation* tmp = currentOperation;
			currentOperation = (ScheduledOperation*)&op_idle;
			delete tmp;

			return 0;
		} else {
			return 1000; // 1 ms wait
		}
	}
	return 0;
}

bool Schedule::doCancelCurrent(){
	SERIALIZE_ACCESS();
	if ((Operation*)currentOperation != (Operation*)&op_idle){
		LOGC(L_INFO, "[Schedule] started cancellation (current): " + currentOperation->toString(), SCHEDULE_COLORS);
		if(currentOperation->doCancel()){
			LOGC(L_INFO, "[Schedule] doCancel() ==> 'true'  (current) for " + currentOperation->toString(), SCHEDULE_COLORS);
			currentIsCancelled = true;
			return true;
		}
		LOGC(L_INFO, "[Schedule] doCancel() ==> 'false'  (current) for " + currentOperation->toString(), SCHEDULE_COLORS);

	}
	return false;
}

bool Schedule::doCancelQueued(ScheduledOperation* op){
	SERIALIZE_ACCESS();
	LOGC(L_INFO, "[Schedule] started cancellation (queued): " + currentOperation->toString(), SCHEDULE_COLORS);
	bool rv =op->doCancel();
	LOGC(L_INFO, "[Schedule] doCancel() ==> '" + std::string(rv ? "true" : "false") + "' (queued) for " + currentOperation->toString(), SCHEDULE_COLORS);
	return rv;
}


ScheduleState Schedule::getState(){
	SERIALIZE_ACCESS();
	if ((Operation*)currentOperation == (Operation*)&op_idle){
		if (opsQueue.isEmpty()){
			return SchIdle;
		} else {
			return SchWait;
		}
	} else {
		return SchBusy;
	}
}

bool          Schedule::isTimeSlotFree(TimeSlot ts, ScheduledOperation*& conflict_op){
	SERIALIZE_ACCESS();
	ScheduledOperation* tmp = opsQueue.find_conflict(ts);
	if (tmp == NULL){
		//LOGC(L_WARNING, "[Scheduler] No Time conflict", ACTOR_ERROR_COLORS);
		return true;
	} else {
		//LOGC(L_WARNING, "[Scheduler] Time conflict detected: " + tmp->toString(true), ACTOR_ERROR_COLORS);
		conflict_op = tmp;
		return false;
	}
	/*



	bool conflictFound = false;
	OpsQueue temp;
	while (!opsQueue.empty()){
		ScheduledOperation* op = opsQueue.top();

		if (!conflictFound){
			TimeSlot tsop = op->getTimeSlot(true);
			if (Time::overlaps(ts,tsop)){
				conflictFound=true;
				conflict_op=op;
			}
		}
		temp.push(op);
		opsQueue.pop();
	}
	opsQueue=temp;
	return !conflictFound;
	*/
}


bool          Schedule::scheduleOperation(ScheduledOperation* op, ScheduledOperation*& conflict_op){
	SERIALIZE_ACCESS();
	if (op == NULL) {
		LOGC(L_ERROR, "[Scheduler] Tried to schedule with null pointer.", ACTOR_ERROR_COLORS);
		return false;
	}
	if ((Operation*)op == (Operation*)&op_idle){
		LOGC(L_ERROR, "[Scheduler] Tried to schedule the idle operation.", ACTOR_ERROR_COLORS);
		return false;
	}
	/*
	TimeSlot w = op->getTimeSlot();
	if (!isTimeSlotFree(w,conflict_op)){
		LOGC(L_ERROR, "[Scheduler] Tried to schedule operation with conflicting timeslot.", ACTOR_ERROR_COLORS);
		return false;
	}
	*/
	ScheduledOperation* tmp = opsQueue.insert(op);
	if (tmp != NULL){
		LOGC(L_ERROR, "[Scheduler] Tried to schedule operation with conflicting timeslot.", ACTOR_ERROR_COLORS);
		conflict_op = tmp;
		return false;
	} else {
		LOGC(L_INFO, "[Scheduler] Scheduled operation '"+op->toString(), ACTOR_COLORS);
		return true;
	}
}

bool          Schedule::cancelOperation(size_t reference, string& comment){
	SERIALIZE_ACCESS();
	if ((((Operation*)currentOperation != (Operation*)&op_idle)) && (currentOperation->getReference() == reference)){
		if (!doCancelCurrent()){
			comment = "Operation can not be canceled";
			return false;
		} else {
			comment = ""; return true;
		}
	} else {
		/*
		bool opFound = false;
		bool success = false;
		OpsQueue temp;
		while (!opsQueue.empty()){
			ScheduledOperation* op = opsQueue.top(); opsQueue.pop();
			if(op->getReference() == reference){
				opFound = true;
				if(!doCancelQueued(op)){
					comment = "Operation can not be canceled";
					temp.push(op);
					success=false;
				} else {
					comment = "";
					success = true;
				}
			} else {
				temp.push(op);
			}
		}
		opsQueue=temp;
		return opFound && success;
		*/
		ScheduledOperation* tmp = opsQueue.find_by_ref(reference);
		if (tmp == NULL){
			comment = "Operation with reference number " + LXS(reference) + " was not found";
			return false;
		} else {
			if(!doCancelQueued(tmp)){
				comment = "Operation can not be canceled";
				return false;
			} else {
				opsQueue.remove_by_ref(reference);
				comment = "";
				return true;
			}
		}
	}
}

Operation*    Schedule::getCurrentOperation(){
	SERIALIZE_ACCESS();
	return (Operation*) currentOperation;
}
int           Schedule::getScheduleEntryCount(){
	return opsQueue.getSize();
}

string        Schedule::getScheduleEntry(int i, bool full){
	/*
	SERIALIZE_ACCESS();
	if (!i) return "";
	int k=(int)opsQueue.size();
	//LOGC(L_DEBUG, "[Scheduler] k = " + LXS(k), ACTOR_WARNING_COLORS);

	int j=0;
	if (i<0){
		j=k+i;
	} else {
		j=i-1;
	}
	//LOGC(L_DEBUG, "[Scheduler] j = " + LXS(j), ACTOR_WARNING_COLORS);
	if (j<0 || j>k) return "";

	size_t c=0;
	OpsQueue temp;
	ScheduledOperation* op=NULL;
	while (!opsQueue.empty()){
		ScheduledOperation* op_tmp = opsQueue.top();
		opsQueue.pop();
		//LOGC(L_DEBUG, "[Scheduler] c     --> " + LXS(c), ACTOR_WARNING_COLORS);
		//LOGC(L_DEBUG, "[Scheduler] op[c] --> '" + op_tmp->toString(full) + "'", ACTOR_WARNING_COLORS);
		if (c==(size_t)j){
			//LOGC(L_DEBUG, "[Scheduler] match ", ACTOR_WARNING_COLORS);
			op=op_tmp;
		}
		c++;
		temp.push(op_tmp);
	}
	opsQueue=temp;
	if (op==NULL)
		return "";
	else
		return op->toString(full);
	*/

	SERIALIZE_ACCESS();
	int k = (int)opsQueue.getSize();
	int j = (i<0) ? (k+i) : (i-1);
	/*
	LOGC(L_DEBUG, "[Scheduler] k = " + LXS(k), ACTOR_WARNING_COLORS);
	LOGC(L_DEBUG, "[Scheduler] i = " + LXS(i), ACTOR_WARNING_COLORS);
	LOGC(L_DEBUG, "[Scheduler] j = " + LXS(j), ACTOR_WARNING_COLORS);
	*/
	if ( (i == 0) ||  (j < 0)  ||  (j >= k)  )
		return "";
	ScheduledOperation* op = opsQueue.find_by_index(j);
	if (op != NULL){
		///LOGC(L_DEBUG, "[Scheduler] ==> " + op->toString(full), ACTOR_WARNING_COLORS);
		return op->toString(full);
	} else {
		///LOGC(L_DEBUG, "[Scheduler] ==> NULL", ACTOR_WARNING_COLORS);
		return "";
	}
}




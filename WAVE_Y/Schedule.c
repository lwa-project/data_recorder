/*
  MCS-DROS: Monitor and Control System - Data Recorder Operating Software
  Copyright (C) 2009-2010  Virginia Tech, Christopher Wolfe <chwolfe2@vt.edu>

  This file is part of MCS-DROS.

  MCS-DROS is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  MCS-DROS is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with MCS-DROS.  If not, see <http://www.gnu.org/licenses/>.
*/
/*
 * Schedule.c
 *
 *  Created on: Oct 31, 2009
 *      Author: chwolfe2
 */
#include "Schedule.h"
#include "Time.h"
#include "Log.h"
#include "Persistence.h"
#include "Operations.h"
#include "Globals.h"
#include "Profiler.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>

Database scheduleDb;
boolean  isScheduleOpen=false;
boolean firstElementChanged=true;
#define SCHEDULE_DEBUG 1

void 	   Schedule_ConstructEntry(ScheduleEntry* entry, size_t reference, size_t startMJD, size_t startMPM, size_t stopMJD, size_t stopMPM, size_t datalength, char* data){
	assert(entry!=NULL);
	assert(data!=NULL);
	entry->data       = data;
	entry->reference  = reference;
	entry->startMJD   = startMJD;
	entry->startMPM   = startMPM;
	entry->stopMJD    = stopMJD;
	entry->stopMPM    = stopMPM;
	entry->datalength = datalength;
}


boolean _entryIsBeforeNow(ScheduleEntry* entry){
	assert(entry!=NULL);
	TimeStamp now=getTimestamp();
	size_t initMJD;
	size_t initMPM;
	// adjust for 2.5 seconds of init time
	if(entry->startMPM>SchedulingWindowLeadTime){
		initMJD=entry->startMJD;
		initMPM=entry->startMPM-SchedulingWindowLeadTime;
	} else {
		initMJD=entry->startMJD-1l;
		initMPM=entry->startMPM+MILLISECONDS_PER_DAY-SchedulingWindowLeadTime;
	}
	return ((entry->startMJD < 5l) || before(initMJD, initMPM, now.MJD, now.MPM));
}
boolean _evBefore(ScheduleEntry* evA, ScheduleEntry* evB, size_t padding){
	assert(evA!=NULL && evB!=NULL);
	size_t aMJD;
	size_t aMPM;
	size_t bMJD;
	size_t bMPM;

	// adjust for 2.5 seconds of hold time
	if(evA->stopMPM+padding<MILLISECONDS_PER_DAY){
		aMJD=evA->stopMJD;			aMPM=evA->stopMPM + padding;
	} else {
		aMJD=evA->stopMJD+1l;		aMPM=evA->stopMPM + padding - MILLISECONDS_PER_DAY;
	}

	// adjust for 2.5 seconds of setup time
	if(evB->startMPM>padding){
		bMJD=evB->startMJD;			bMPM=evB->startMPM - padding;
	} else {
		bMJD=evB->startMJD-1l;		bMPM=evB->startMPM + MILLISECONDS_PER_DAY - padding;
	}
	return before(aMJD,aMPM,bMJD,bMPM);
}

boolean _evAfter(ScheduleEntry* evA, ScheduleEntry* evB, size_t padding){
	return _evBefore(evB,evA,padding);
}
boolean _evOverlaps(ScheduleEntry* evA, ScheduleEntry* evB, size_t padding){
	assert(evA!=NULL && evB!=NULL);
	size_t astartMJD;
	size_t astartMPM;
	size_t astopMJD;
	size_t astopMPM;
	size_t bstartMJD;
	size_t bstartMPM;
	size_t bstopMJD;
	size_t bstopMPM;

	// adjust for 2.5 seconds of setup time
	if(evA->startMPM>padding){
		astartMJD=evA->startMJD;		astartMPM=evA->startMPM - padding;
	} else {
		astartMJD=evA->startMJD-1l;		astartMPM=evA->startMPM + MILLISECONDS_PER_DAY - padding;
	}
	// adjust for 2.5 seconds of setup time
	if(evB->startMPM>padding){
		bstartMJD=evB->startMJD;		bstartMPM=evB->startMPM - padding;
	} else {
		bstartMJD=evB->startMJD-1l;		bstartMPM=evB->startMPM + MILLISECONDS_PER_DAY - padding;
	}

	// adjust for 2.5 seconds of hold time
	if(evA->stopMPM+padding<MILLISECONDS_PER_DAY){
		astopMJD=evA->stopMJD;			astopMPM=evA->stopMPM + padding;
	} else {
		astopMJD=evA->stopMJD+1l;		astopMPM=evA->stopMPM + padding - MILLISECONDS_PER_DAY;
	}
	// adjust for 2.5 seconds of hold time
	if(evB->stopMPM+padding<MILLISECONDS_PER_DAY){
		bstopMJD=evB->stopMJD;			bstopMPM=evB->stopMPM + padding;
	} else {
		bstopMJD=evB->stopMJD+1l;		bstopMPM=evB->stopMPM + padding - MILLISECONDS_PER_DAY;
	}
	if (before(astartMJD, astartMPM, bstopMJD, bstopMPM) && before(bstartMJD, bstartMPM, astartMJD, astartMPM)) return true;
	if (before(astopMJD,   astopMPM, bstopMJD, bstopMPM) && before(bstartMJD, bstartMPM,  astopMJD,  astopMPM)) return true;
	if (before(bstartMJD, bstartMPM, astopMJD, astopMPM) && before(astartMJD, astartMPM, bstartMJD, bstartMPM)) return true;
	if (before(bstopMJD,   bstopMPM, astopMJD, astopMPM) && before(astartMJD, astartMPM,  bstopMJD,  bstopMPM)) return true;
	if (equal(astartMJD, astartMPM, bstartMJD, bstartMPM)) return true;
	if (equal(astartMJD, astartMPM,  bstopMJD,  bstopMPM)) return true;
	if (equal(astopMJD,   astopMPM, bstartMJD, bstartMPM)) return true;
	if (equal(astopMJD,   astopMPM,  bstopMJD,  bstopMPM)) return true;
	return false;
}




int _Schedule_Count(){
	StatusCode sc;
	int count=0;
	sc = Persistence_List_GetCount(&scheduleDb, "SCHEDULEENTRY",&count);
	if (sc!=SUCCESS) count=-1;
	return count;
}


StatusCode _Schedule_Create(){
	StatusCode sc;
	sc = Persistence_Create(SCHEDULEFILENAME,NULL);					if (sc!=SUCCESS) return sc;
	sc = Persistence_Open(&scheduleDb,SCHEDULEFILENAME);			if (sc!=SUCCESS) return sc;
	sc = Persistence_List_Create(&scheduleDb, "SCHEDULEENTRY");		if (sc!=SUCCESS) return sc;
	sc = Persistence_List_Create(&scheduleDb, "SCHEDULEDATA");		if (sc!=SUCCESS) return sc;
	sc = Persistence_Close(&scheduleDb);
	return sc;
}
void _Schedule_Close(){
	if (isScheduleOpen){
		Persistence_Close(&scheduleDb);
		isScheduleOpen=false;
	}
}
StatusCode _Schedule_Open(){
	StatusCode sc = Persistence_Open(&scheduleDb,SCHEDULEFILENAME);
	if (sc!=SUCCESS){
		StatusCode sc = _Schedule_Create();
		if (sc!=SUCCESS){
			return FAILURE;
		} else {
			sc = Persistence_Open(&scheduleDb,SCHEDULEFILENAME);
			if (sc!=SUCCESS){
				return FAILURE;
			}
		}
	}


	int c =_Schedule_Count();
	if (c==-1) return FAILURE;
	// got to this point, then we're open
	ScheduleEntry entry;
	size_t bytes;
	boolean done=false;
	while (c>0 && !done) {
		sc = Persistence_List_GetItemBinary(&scheduleDb, "SCHEDULEENTRY",POSITION_FRONT,(char*)&entry,&bytes);
		if (sc!=SUCCESS || bytes!=sizeof(ScheduleEntry)) return FAILURE;
		if (_entryIsBeforeNow(&entry)){
			//Log_Add("[SCHEDULER] Deleting expired entry: <ref=%lu, MJD=%lu, MPM=%lu>", entry.reference, entry.startMJD, entry.startMPM);
			sc = Persistence_List_RemoveItem(&scheduleDb, "SCHEDULEENTRY",POSITION_FRONT); if (sc!=SUCCESS) return FAILURE;
			sc = Persistence_List_RemoveItem(&scheduleDb, "SCHEDULEDATA",POSITION_FRONT);if (sc!=SUCCESS) return FAILURE;
			c =_Schedule_Count();			if (c==-1) return FAILURE;
		} else {
			done=true;
		}
	}
	atexit(_Schedule_Close);
	isScheduleOpen=true;
	return SUCCESS;
}

StatusCode Schedule_Open(boolean create){
	if (isScheduleOpen)return FAILURE;
	if (!create){
		return _Schedule_Open();
	} else {
		StatusCode sc = _Schedule_Create();	if (sc!=SUCCESS) return sc;
		return _Schedule_Open();
	}

}
void printentry(ScheduleEntry* entry){
	/*Log_Add("[SCHEDULER] Entry:"
			"[SCHEDULER] \tStart MJD:%lu"
			"[SCHEDULER] \tStart MPM:%lu"
			"[SCHEDULER] \tStop  MJD:%lu"
			"[SCHEDULER] \tStop  MPM:%lu"
			"[SCHEDULER] \tDatalen:  %lu"
			"[SCHEDULER] \tDataPtr:  %lu",
			entry->startMJD,
			entry->startMPM,
			entry->stopMJD,
			entry->stopMPM,
			entry->datalength,
			(size_t)entry->data);
			*/
}

StatusCode _Schedule_Insert(ScheduleEntry* entry, int position){
	StatusCode sc;
	assert(entry!=NULL);
	assert(entry->data!=NULL);
	sc = Persistence_List_AddItemBinary(&scheduleDb, "SCHEDULEENTRY",position,(char*)entry,sizeof(ScheduleEntry));
	if (sc!=SUCCESS) {
		//Log_Add("[SCHEDULER] Scheduling Failure, insertion");
		exit(EXIT_CODE_FAIL);
	}
	sc = Persistence_List_AddItemBinary(&scheduleDb, "SCHEDULEDATA",position,entry->data,entry->datalength);
	if (sc!=SUCCESS) {
		//Log_Add("[SCHEDULER] Scheduling Failure, insertion");
		exit(EXIT_CODE_FAIL);
	}
	if (position<=0 && position!=POSITION_BACK) firstElementChanged=true;
	return SUCCESS;
}



StatusCode Schedule_AddEntry(ScheduleEntry* entry, ScheduleEntry* conflict){
	//printf("AddEntry\n");
	//printf("entryptr %lu\n ",(size_t)entry);printentry(entry);
	assert(isScheduleOpen);
	StatusCode sc;
	int count = _Schedule_Count();
	if ((count==-1) || _entryIsBeforeNow(entry)) {
		memcpy((void *)conflict,(void *)entry,sizeof(ScheduleEntry));
		return FAILURE;
	}
	if (count==0) {
		//case where schedule is empty
		return  _Schedule_Insert(entry,POSITION_FRONT);
	}
	int i=0;
	size_t bytes=0;
	ScheduleEntry ToCompare1;
	ScheduleEntry ToCompare2;
	for(i=0;i<count;i++){
		sc = Persistence_List_GetItemBinary(&scheduleDb, "SCHEDULEENTRY",i,(char*)&ToCompare1,&bytes);
		if (sc!=SUCCESS || bytes!=sizeof(ScheduleEntry)) {
			//Log_Add("[SCHEDULER] Scheduling Failure, entry missing");
			exit(EXIT_CODE_FAIL);
		}
		if (_evOverlaps(entry, &ToCompare1, SchedulingWindowLeadTime)){
			memcpy((void *)conflict,(void *)&ToCompare1,sizeof(ScheduleEntry));
			return FAILURE;
		}
		//printf("AddEntry: i=%d : not overlaps\n",i);
		if (_evBefore(entry, &ToCompare1, SchedulingWindowLeadTime)){
			// am before the next one in the list, so check to make sure we're after the previous item
			if (i==0){
				// case where before the first entry in the list
				return  _Schedule_Insert(entry,POSITION_FRONT);
			} else {
				// not the first, so check previous for after-ness
				sc = Persistence_List_GetItemBinary(&scheduleDb, "SCHEDULEENTRY",i-1,(char*)&ToCompare2,&bytes);
				if (sc!=SUCCESS || bytes!=sizeof(ScheduleEntry)) {
					//Log_Add("[SCHEDULER] Scheduling Failure, entry missing");
					exit(EXIT_CODE_FAIL);
				}
				if (_evAfter(entry, &ToCompare1, SchedulingWindowLeadTime)){
					// case where insertion point is in between i-1 and i
					return  _Schedule_Insert(entry,i);
				}
			}
		}
	}// end  of loop

	// if we got here, then entry is not before any entry, and doesn't conflict with any entry, thus it IS after all entrys, so add it
	// case where insertion point is in between i-1 and i
	return  _Schedule_Insert(entry,POSITION_BACK);
}
int Schedule_getIndexByReference(size_t reference){
	assert(isScheduleOpen);
	StatusCode sc;
	int count = _Schedule_Count();
	if (count==-1) return -1;
	int i=0;
	size_t bytes=0;
	ScheduleEntry temp;
	while (i<count){
		sc = Persistence_List_GetItemBinary(&scheduleDb, "SCHEDULEENTRY",i,(char*)&temp,&bytes);
		if (sc!=SUCCESS || bytes!=sizeof(ScheduleEntry)) {
			//Log_Add("[SCHEDULER] Scheduling Failure, entry missing");
			exit(EXIT_CODE_FAIL);
		}
		if (temp.reference == reference) return i;
		i++;
	}
	return -1;
}
StatusCode Schedule_RemoveEntry(size_t reference){
	assert(isScheduleOpen);
	int idx=Schedule_getIndexByReference(reference);
	if (idx!=-1){
		StatusCode sc;
		sc = Persistence_List_RemoveItemBinary(&scheduleDb, "SCHEDULEDATA",idx);
		if (sc!=SUCCESS) {
			//Log_Add("[SCHEDULER] Scheduling Failure, data missing");
			exit(EXIT_CODE_FAIL);
		}
		sc = Persistence_List_RemoveItemBinary(&scheduleDb, "SCHEDULEENTRY",idx);
		if (sc!=SUCCESS) {
			//Log_Add("[SCHEDULER] Scheduling Failure, entry missing");
			exit(EXIT_CODE_FAIL);
		}
		if (idx==0) firstElementChanged = true;
		return SUCCESS;
	} else {
		return FAILURE;
	}
}

StatusCode Schedule_GetEntry(size_t reference, ScheduleEntry* entry){
	assert(isScheduleOpen);
	assert(entry!=NULL);
	int idx=Schedule_getIndexByReference(reference);
	size_t bytes;
	if (idx!=-1){
		StatusCode sc;
		sc = Persistence_List_GetItemBinary(&scheduleDb, "SCHEDULEENTRY",idx,(char*)entry,&bytes);
		if (sc!=SUCCESS || bytes!=sizeof(ScheduleEntry)) {
			//Log_Add("[SCHEDULER] Scheduling Failure, entry missing");
			exit(EXIT_CODE_FAIL);
		}
		return SUCCESS;
	} else {
		return FAILURE;
	}
}
StatusCode Schedule_GetEntryIndex(int index, ScheduleEntry* entry){
	assert(isScheduleOpen);
	assert(entry!=NULL);
	size_t bytes;
	return  Persistence_List_GetItemBinary(&scheduleDb, "SCHEDULEENTRY",index,(char*)entry,&bytes);
}
StatusCode Schedule_GetEntryDataIndex(int index, char* data, size_t* bytes){
	assert(isScheduleOpen);
	assert(data!=NULL);
	return  Persistence_List_GetItemBinary(&scheduleDb, "SCHEDULEDATA",index,data,bytes);
}

StatusCode Schedule_GetEntryData(size_t reference, char* data, size_t* bytes){
	assert(isScheduleOpen);
	assert(data!=NULL);
	int idx=Schedule_getIndexByReference(reference);
	if (idx!=-1){
		StatusCode sc;
		sc = Persistence_List_GetItemBinary(&scheduleDb, "SCHEDULEDATA",idx,data,bytes);
		if (sc!=SUCCESS) {
			//Log_Add("[SCHEDULER] Scheduling Failure, data missing");
			exit(EXIT_CODE_FAIL);
		}
		return SUCCESS;
	} else {
		return FAILURE;
	}
}

StatusCode Schedule_Close(){
	_Schedule_Close();
	return SUCCESS;
}

boolean Schedule_EntriesEqual(ScheduleEntry* a, ScheduleEntry* b){
	assert(a!=NULL && b!=NULL);
	if (a==b) return true;
	char * c=(char*)a;
	char * d=(char*)b;
	int i;
	for (i=0;i<sizeof(ScheduleEntry);i++){
		if (c[i]!=d[i]) return false;
	}
	return true;
}

boolean Schedule_Changed(){
	if (firstElementChanged){
		firstElementChanged=false;
		return true;
	}
	return false;
}




ScheduleEntry leadEntry;
OperationParameters leadEntryData;
boolean operationStarted=false;
boolean leadEntryWaitToRunDone(){
	TimeStamp now = getTimestamp();
	if (Schedule_Changed()) return false;
	return !before(now.MJD,now.MPM, leadEntry.startMJD,leadEntry.startMPM);
}
ScheduleEntry leadEntry;

#define SCHEDULER_STATE_IDLE       		0
#define SCHEDULER_STATE_WAIT_INIT  		1
#define SCHEDULER_STATE_WAIT_START 		2
#define SCHEDULER_STATE_WAIT_STOP 		3
#define SCHEDULER_STATE_WAIT_COOLDOWN	4
#define SCHEDULER_STATE_DELETE			5
#define SCHEDULER_STATE_ABORT			6
int schedulerState=SCHEDULER_STATE_IDLE;
void Process_AbortCurrent(){
	if (schedulerState!=SCHEDULER_STATE_IDLE){
		schedulerState=SCHEDULER_STATE_ABORT;
	}
}
void Process_TerminateCurrent(){
	if (schedulerState!=SCHEDULER_STATE_IDLE){
		schedulerState=SCHEDULER_STATE_DELETE;
	}
}
extern void recordingPrefire();

void Process_Schedule(){
	//boolean tchanged=false;
	TimeStamp now = getTimestamp();
	/*static size_t lastMPM =0;
	static size_t lastMJD =0;
	if ((lastMPM!=now.MPM) || (lastMJD!=now.MJD)){
		lastMPM=now.MPM;
		lastMJD=now.MJD;
		tchanged=true;
	} else {
		tchanged=false;
	}*/

	size_t tempMPM = 0;
	size_t tempMJD = 0;
	StatusCode sc;

	switch (schedulerState){
		case SCHEDULER_STATE_IDLE       	:
			globals.opData.opType=OP_TYPE_IDLE;
			if (_Schedule_Count()>=1){
				if(Schedule_GetEntryIndex(POSITION_FRONT, &leadEntry)==SUCCESS){
					size_t bytes;
					if(Schedule_GetEntryDataIndex(POSITION_FRONT,(char*) &leadEntryData, &bytes)==SUCCESS || bytes!=sizeof(leadEntryData)){
						//Log_Add("[SCHEDULER] Entered wait phase for operation with reference number %lu",leadEntryData.opReference );
						if (SCHEDULE_DEBUG) printf("(%d) DEBUG: schedulerState => SCHEDULER_STATE_WAIT_INIT\n",__LINE__);
						schedulerState=SCHEDULER_STATE_WAIT_INIT;
						Schedule_Changed(); // gotta check this so future checks will be valid.
					} else {
						//Log_Add("[SCHEDULER] FATAL Error: Non-empty schedule and dequeue failed");
						exit(EXIT_CODE_FAIL);
					}
				} else {
					//Log_Add("[SCHEDULER] FATAL Error: Non-empty schedule and dequeue failed");
					exit(EXIT_CODE_FAIL);
				}
			}
		break;
		case SCHEDULER_STATE_WAIT_INIT  	:
			if (Schedule_Changed()){
				//Log_Add("[SCHEDULER] schedule changed, abandoning operation with reference number %lu, return to idle",leadEntryData.opReference );
				if (SCHEDULE_DEBUG) printf("(%d) DEBUG: schedulerState => SCHEDULER_STATE_IDLE\n",__LINE__);
				schedulerState=SCHEDULER_STATE_IDLE; // go back to start and retrieve 1st scheduled entry
			} else {
				subTime(leadEntry.startMJD,leadEntry.startMPM,SchedulingWindowLeadTime,&tempMJD,&tempMPM);
				if (!before(now.MJD,now.MPM,tempMJD,tempMPM)){
					//if (tchanged) printf("%lu:%lu >= %lu:%lu", now.MJD,now.MPM,tempMJD,tempMPM);
					//Log_Add("[SCHEDULER] Entered initialization phase for operation with reference number %lu, waiting to init",leadEntryData.opReference );
					if(Operation_Init(&leadEntryData)==SUCCESS){
						//Log_Add("[SCHEDULER] Initialized operation with reference number %lu, waiting to start",leadEntryData.opReference );
						schedulerState=SCHEDULER_STATE_WAIT_START;
						if (SCHEDULE_DEBUG) printf("(%d) DEBUG: schedulerState => SCHEDULER_STATE_WAIT_START\n",__LINE__);
					} else {
						//Log_Add("[SCHEDULER] Initialization failure, aborting operation with reference number %lu",leadEntryData.opReference );
						schedulerState=SCHEDULER_STATE_ABORT;
						if (SCHEDULE_DEBUG) printf("(%d) DEBUG: schedulerState => SCHEDULER_STATE_ABORT\n",__LINE__);
					}
				} /*else {
					if (tchanged) printf("%lu:%lu < %lu:%lu\n", now.MJD,now.MPM,tempMJD,tempMPM);
				}*/
			}
		break;
		case SCHEDULER_STATE_WAIT_START 	:
			if (Schedule_Changed()){
				//Log_Add("[SCHEDULER] Schedule changed, abandoning operation with reference number %lu, goto abort",leadEntryData.opReference );
				schedulerState=SCHEDULER_STATE_ABORT; // abort scheduled item
				if (SCHEDULE_DEBUG) printf("(%d) DEBUG: schedulerState => SCHEDULER_STATE_ABORT\n",__LINE__);
			} else {

				if ((globals.opData.opType == OP_TYPE_REC) || (globals.opData.opType == OP_TYPE_SPC)){
					// while waiting to start a record operation, we need to actively read the socket and discard any packets just prior to the start time.
					// otherwise, latency in opening the socket will cause problems.
					recordingPrefire();
				}
				if (!before(now.MJD,now.MPM, leadEntry.startMJD,leadEntry.startMPM)){
					//Log_Add("[SCHEDULER] Entered start phase for operation with reference number %lu",leadEntryData.opReference );
					schedulerState=SCHEDULER_STATE_WAIT_STOP;
					if (SCHEDULE_DEBUG) printf("(%d) DEBUG: schedulerState => SCHEDULER_STATE_WAIT_STOP\n",__LINE__);
					PROFILE_BEGIN();
				}
			}
		break;
		case SCHEDULER_STATE_WAIT_STOP 		:
			if (Schedule_Changed()){ //should only happen when lead entry is deleted!!!
				PROFILE_END();
				//Log_Add("[SCHEDULER] Schedule changed, abandoning operation with reference number %lu, goto abort",leadEntryData.opReference );
				schedulerState=SCHEDULER_STATE_ABORT; // abort scheduled item
				if (SCHEDULE_DEBUG) printf("(%d) DEBUG: schedulerState => SCHEDULER_STATE_ABORT\n",__LINE__);

			} else {
				if (!before(now.MJD,now.MPM, globals.opData.opStopMJD, globals.opData.opStopMPM)){
					PROFILE_END();
					////Log_Add("[SCHEDULER] ---- opstop is %06lu:%09lu              ----now is %06lu:%09lu",globals.opData.opStopMJD, globals.opData.opStopMPM,now.MJD,now.MPM);
					//Log_Add("[SCHEDULER] Entered stop phase for operation with reference number %lu",leadEntryData.opReference );
					if(Operation_CleanUp(&globals.opData)!=SUCCESS){
						//Log_Add("[SCHEDULER] Error: Operation cleanup failure");
						schedulerState=SCHEDULER_STATE_DELETE;
						if (SCHEDULE_DEBUG) printf("(%d) DEBUG: schedulerState => SCHEDULER_STATE_DELETE\n",__LINE__);
					} else {
						schedulerState=SCHEDULER_STATE_WAIT_COOLDOWN;
						if (SCHEDULE_DEBUG) printf("(%d) DEBUG: schedulerState => SCHEDULER_STATE_WAIT_COOLDOWN\n",__LINE__);
					}
				} else {
					sc=Operation_Run(&globals.opData);
					if(sc==FAILURE){
						PROFILE_END();
						//Log_Add("[SCHEDULER] Error: Operation execution failure, aborting ");
						schedulerState=SCHEDULER_STATE_ABORT;
						if (SCHEDULE_DEBUG) printf("(%d) DEBUG: schedulerState => SCHEDULER_STATE_ABORT\n",__LINE__);
					} else if (sc==SUCCESS){
						PROFILE_END();
						//Log_Add("[SCHEDULER] Operation completed, wrapping up... ");
						schedulerState=SCHEDULER_STATE_DELETE;
						if (SCHEDULE_DEBUG) printf("(%d) DEBUG: schedulerState => SCHEDULER_STATE_DELETE\n",__LINE__);
					} // else NOT_READY, don't change state
				}
			}
		break;
		case SCHEDULER_STATE_WAIT_COOLDOWN	:
			if (Schedule_Changed()){
				//Log_Add("[SCHEDULER] Schedule changed, abandoning operation with reference number %lu, goto delete",leadEntryData.opReference );
				schedulerState=SCHEDULER_STATE_DELETE;
				if (SCHEDULE_DEBUG) printf("(%d) DEBUG: schedulerState => SCHEDULER_STATE_DELETE\n",__LINE__);

			} else {
				addTime(globals.opData.opStopMJD, globals.opData.opStopMPM,SchedulingWindowLeadTime,&tempMJD,&tempMPM);
				if (!before(now.MJD,now.MPM, tempMJD, tempMPM)){
					//Log_Add("[SCHEDULER] Cooldown ended for operation with reference number %lu, goto delete",leadEntryData.opReference );
					schedulerState=SCHEDULER_STATE_DELETE;
					if (SCHEDULE_DEBUG) printf("(%d) DEBUG: schedulerState => SCHEDULER_STATE_DELETE\n",__LINE__);
				}
			}
		break;
		case SCHEDULER_STATE_DELETE			:{
			//Log_Add("[SCHEDULER] Clean-up for operation with reference number %lu ",leadEntryData.opReference );
			Operation_CleanUp(&globals.opData);
			//Log_Add("[SCHEDULER] Deleting operation with reference number %lu from schedule, return to idle",leadEntryData.opReference );
			Schedule_RemoveEntry(leadEntry.reference);
			schedulerState=SCHEDULER_STATE_IDLE;
			if (SCHEDULE_DEBUG) printf("(%d) DEBUG: schedulerState => SCHEDULER_STATE_IDLE\n",__LINE__);

		}
		break;
		case SCHEDULER_STATE_ABORT			:
			//Log_Add("[SCHEDULER] Clean-up for operation with reference number %lu ",leadEntryData.opReference );
			Operation_CleanUp(&globals.opData);
			//Log_Add("[SCHEDULER] Abort and delete operation with reference number %lu from schedule",leadEntryData.opReference );
			Schedule_RemoveEntry(leadEntry.reference);
			schedulerState=SCHEDULER_STATE_IDLE;
			if (SCHEDULE_DEBUG) printf("(%d) DEBUG: schedulerState => SCHEDULER_STATE_IDLE\n",__LINE__);
		break;

		default:
			//Log_Add("[SCHEDULER] FATAL Error: Schedule FSM in undefined state");
			exit (EXIT_CODE_FAIL);
		break;
	}
}


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
 * Schedule.h
 *
 *  Created on: Oct 31, 2009
 *      Author: chwolfe2
 */

#ifndef SCHEDULE_H_
#define SCHEDULE_H_
#define SchedulingWindowLeadTime 2500l
#include "Defines.h"
#include <stdlib.h>

#define  SCHEDULEFILENAME "/LWA/database/scheduleDb.gdbm"

#define NO_STOP_MJD 999999l
#define NO_STOP_MPM 999999999l

typedef int EntryCode;
#define EVENT_NONE			0
#define EVENT_ENTERED_INIT	1
#define EVENT_ENTERED_RUN	2
#define EVENT_ENTERED_STOP	3
#define EVENT_EXPIRED		4



typedef struct __ScheduleEntry{
	size_t 		reference;
	size_t 		startMJD;
	size_t 		startMPM;
	size_t 		stopMJD;
	size_t 		stopMPM;
	size_t 		datalength;
	char*  		data;
} ScheduleEntry;


void 		Schedule_ConstructEntry(ScheduleEntry* entry, size_t reference, size_t startMJD, size_t startMPM, size_t stopMJD, size_t stopMPM, size_t datalength, char* data);

boolean 	_entryIsBeforeNow(ScheduleEntry* entry);
boolean 	_evBefore(ScheduleEntry* evA, ScheduleEntry* evB, size_t padding);
boolean 	_evAfter(ScheduleEntry* evA, ScheduleEntry* evB, size_t padding);
boolean 	_evOverlaps(ScheduleEntry* evA, ScheduleEntry* evB, size_t padding);
int 	    _Schedule_Count();
int		    Schedule_getIndexByReference(size_t reference);
void 	    _Schedule_Close();
StatusCode  _Schedule_Create();
StatusCode  _Schedule_Open();

StatusCode  Schedule_Open(boolean create);
StatusCode  Schedule_AddEntry(ScheduleEntry* entry, ScheduleEntry* conflict);
StatusCode  Schedule_RemoveEntry(size_t reference);
StatusCode  Schedule_GetEntry(size_t reference, ScheduleEntry* entry);
StatusCode  Schedule_GetEntryData(size_t reference, char* data, size_t* bytes);
StatusCode  Schedule_Close();

boolean     Schedule_Changed();
int 		_Schedule_Count();
boolean 	Schedule_EntriesEqual(ScheduleEntry* a, ScheduleEntry* b);
StatusCode 	Schedule_GetEntryIndex(int index, ScheduleEntry* entry);
StatusCode 	Schedule_GetEntryDataIndex(int index, char* data, size_t* bytes);
void Process_Schedule();
void Process_AbortCurrent();
void Process_TerminateCurrent();
boolean leadEntryWaitToRunDone(); // implemented in Schedule.c

#endif /* SCHEDULE_H_ */

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
 * Globals.h
 *
 *  Created on: Nov 1, 2009
 *      Author: chwolfe2
 */

#ifndef GLOBALS_H_
#define GLOBALS_H_
//#include "Message.h"
#include "Disk.h"
#include "Time.h"
#include "Log.h"
#include "FileSystem.h"
#include "Socket.h"
#include "RingQueue.h"
#include "MessageQueue.h"
//#include "Persistence.h"
//#include "Schedule.h"
#include "DataFormats.h"
#include "Operations.h"
#include "LiveCapture.h"

#define UP   0
#define DOWN 1

typedef struct __Globals{
	Disk* 					disk;
	FileSystem* 			fileSystem;
	Socket* 				rxMsgSocket;
	Socket* 				txMsgSocket;
	Socket* 				rxSocket;
	RingQueue* 				rxQueue;
	MessageQueue*			messageQueue;
	MessageQueue*			responseQueue;
	unsigned short			MessageInPort;
	unsigned short			MessageOutPort;
	unsigned short			DataInPort;
	char					MessageOutURL[1024];
	boolean					exitRequested;
	boolean					shutdownRequested;
	boolean					scramRequested;
	boolean					rebootRequested;
	boolean					flushConfigRequested;
	boolean					flushDataRequested;
	boolean					flushLogRequested;
	boolean					flushScheduleRequested;
	char					MyReferenceDesignator[4];
	char					MySerialNumber[1024];
	char					Version[1024];
	char 					ntpServer[1024];
	OperationData			opData;
	int						upStatus;
	unsigned short			arraySelect;
	/*
	char 					arraySelectByName[1024];
	boolean                 useArraySelectByName;
	*/
	unsigned short			numArraysFound;
	Disk*					arrays;
	boolean					LocalEchoMessaging;
} Globals;

#ifndef __IN_MAIN__
	extern Globals globals;
#else
	Globals globals;
#endif

#endif /* GLOBALS_H_ */

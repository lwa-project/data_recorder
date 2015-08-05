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
 * LiveCapture.h
 *
 *  Created on: Feb 17, 2010
 *      Author: chwolfe2
 */

#ifndef LIVECAPTURE_H_
#define LIVECAPTURE_H_
#include <stdlib.h>
#include "DataFormats.h"
#include "Operations.h"

#define TRIGGER_TIME					0
#define TRIGGER_INDEX					1

#define WIDTH_TIME						0
#define WIDTH_INDEX						1

#define RETRIGGER_NONE					0
#define RETRIGGER_ALWAYS				1

#define ORDER_BEFORE					0
#define ORDER_AFTER						1

#define POST_BEHAVIOUR_DELETE			0
#define POST_BEHAVIOUR_HOLD				1
#define POST_BEHAVIOUR_HOLD_RESTRICT	2



typedef struct __LiveCaptureParameters{
	int triggerType;
	int widthType;
	int retriggerType;
	int order;
	int postBehaviour;
	size_t triggerValue;
	size_t widthValue;
	size_t retriggerValue;
}LiveCaptureParameters;


typedef struct __LiveCaptureInfo{
	size_t 					opReference;
	size_t 				 	opStartMJD;
	size_t 					opStartMPM;
}LiveCaptureInfo;

typedef struct __LiveCaptureBuffer{
	LiveCaptureParameters params;
	LiveCaptureInfo	info;
	char*  buffer[2];
	size_t packetSize;
	double packetRate;

	size_t Tt0;		// first Trigger (in packets)
	size_t Tc;		// current Time (in packets)
	size_t Tw;		// width (in packets)
	size_t Trt;		// retrigger period (in packets)

	size_t Tt[2];		// time of triggering for this buffer
	size_t Pp[2];		// packets present
	size_t Pr[2];		// packets retrieved
	int    currentBuffer;

}LiveCaptureBuffer;

extern char * LiveCaptureMnemonics[];
extern char * trigTypes[];
extern char * behaviours[];
extern char * postBehaviours[];


void   LiveCapture_setParameters(LiveCaptureParameters* lci);
void   LiveCapture_Event_RecordingStart(OperationData* opData);
int    isCapBufEmpty(int which);
int    isCapBufRetrievalStarted(int which);
void   LiveCapture_CapturePacket(char* packet, size_t packetSize, size_t pos);
void   LiveCapture_Event_Packet(char* packet, size_t packetSize);
void   LiveCapture_Event_PacketBefore(char* packet, size_t packetSize);
void   LiveCapture_Event_PacketAfter(char* packet, size_t packetSize);
void   LiveCapture_Event_RecordingStop();
// size_t LiveCapture_whichBuffer();
// size_t LiveCapture__getRetrievableCount(size_t which);
size_t LiveCapture_getRetrievableCount();
size_t LiveCapture_getReference();
size_t LiveCapture_getMJD();
size_t LiveCapture_getMPM();
size_t LiveCapture_getPacketSize();
//size_t LiveCapture_getPacketTime();
double LiveCapture_getPacketTime();

int	LiveCapture_getPacket(char* destination);



void   LiveCapture_release();





#endif /* LIVECAPTURE_H_ */

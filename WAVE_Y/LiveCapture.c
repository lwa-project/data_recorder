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
 * LiveCapture.c
 *
 *  Created on: Feb 17, 2010
 *      Author: chwolfe2
 */

#include "LiveCapture.h"
#include "Log.h"
#include "Time.h"
#include <assert.h>
#include <string.h>


boolean					captureParamsSet = false;
LiveCaptureParameters	captureParams;
boolean					captureBufferCreated = false;
boolean					captureBufferActive = false;
LiveCaptureBuffer		captureBuffer;



char * LiveCaptureMnemonics[]={"TI","TI","NA","BA","DHR"," "};
char * trigTypes[]   = {	"ms",	"packets" };
//char * fieldTypes[] = { "","unsigned char","unsigned short","","unsigned int","","","","unsigned long"};
//char * endianNames[] = { "Little-endian", "Big-endian"};
char * postBehaviours[]={
		"destroy buffer, reuse settings",
		"keep buffer, reuse settings",
		"keep buffer, prevent reuse"
};



#define DEBUG_LIVE_CAP 1

void	LiveCapture_setParameters(LiveCaptureParameters* lci){
	assert(lci!=NULL);
	captureParams=(*lci);
	captureParamsSet=true;
}
void 		LiveCapture_Event_RecordingStart(OperationData* opData){
	//if (DEBUG_LIVE_CAP){ printf("\nENTER LiveCapture_Event_RecordingStart\n"); fflush(stdout);}
	assert(opData!=NULL);
	//if (DEBUG_LIVE_CAP){ printf("[LIVE BUFFER] Notice: EVENT REC START\n"); fflush(stdout);}

	//check if live capture has been set up, otherwise return
	if (! captureParamsSet) {
		//if (DEBUG_LIVE_CAP){ printf("\nEXIT LiveCapture_Event_RecordingStart\n"); fflush(stdout);}
		return;
	}
	//if (DEBUG_LIVE_CAP) printf("[LIVE BUFFER] Notice: parameters were set\n");

	//check if we have old, unpurgeable data. If so, return
	if (captureBufferCreated && captureBuffer.params.postBehaviour == POST_BEHAVIOUR_HOLD_RESTRICT) {
		//if (DEBUG_LIVE_CAP){ printf("\nEXIT LiveCapture_Event_RecordingStart\n"); fflush(stdout);}
		return;
	}
	//if (DEBUG_LIVE_CAP) printf("[LIVE BUFFER] Notice: buffer wasn't created or behavior was not restrict\n");
	// get rid of old buffer if we have one
	if(captureBufferCreated){
		//if (DEBUG_LIVE_CAP) printf("[LIVE BUFFER] Notice: buffer was created; freeing...\n");

		free(captureBuffer.buffer[0]);
		free(captureBuffer.buffer[1]);
	}
	//clear struct, copy params, allocate buffers
	bzero(&captureBuffer,sizeof(LiveCaptureBuffer));
	captureBuffer.params = captureParams;
	captureBuffer.packetSize=opData->dataFormat->payloadSize;

	assert(captureBuffer.packetSize);

	if (captureBuffer.params.widthType != WIDTH_TIME)
		captureBuffer.Tw = captureBuffer.params.widthValue;
	else
		captureBuffer.Tw = (opData->dataFormat->rawRate * captureBuffer.params.widthValue) / (opData->dataFormat->payloadSize * 1000);

	captureBuffer.packetRate =(double)( ((long double)(opData->dataFormat->rawRate)) /((long double) (opData->dataFormat->payloadSize * 1000)));


	captureBuffer.buffer[0] = malloc(captureBuffer.Tw * captureBuffer.packetSize);
	if (!captureBuffer.buffer[0]){
		Log_Add("[LIVE CAPTURE] Failed to allocate live capture buffer 0 for current recording '%s'",opData->opTag);
		captureBufferCreated=false;
		captureBufferActive = false;
		//if (DEBUG_LIVE_CAP){ printf("\nEXIT LiveCapture_Event_RecordingStart\n"); fflush(stdout);}
		return;
	}
	captureBuffer.buffer[1] = malloc(captureBuffer.Tw * captureBuffer.packetSize);
	if (!captureBuffer.buffer[1]){
		Log_Add("[LIVE CAPTURE] Failed to allocate live capture buffer 1 for current recording '%s'",opData->opTag);
		free(captureBuffer.buffer[0]);
		captureBufferCreated=false;
		captureBufferActive = false;
		//if (DEBUG_LIVE_CAP){ printf("\nEXIT LiveCapture_Event_RecordingStart\n"); fflush(stdout);}
		return;
	}
	//if (DEBUG_LIVE_CAP) printf("[LIVE BUFFER] Notice: new buffers now created...\n");
	captureBufferCreated = true;
	captureBufferActive = true;
	if (captureBuffer.params.triggerType==TRIGGER_TIME){
		captureBuffer.Tt0 = (opData->dataFormat->rawRate * captureBuffer.params.triggerValue) / (opData->dataFormat->payloadSize * 1000);
		captureBuffer.Trt  = (opData->dataFormat->rawRate * captureBuffer.params.retriggerValue) / (opData->dataFormat->payloadSize * 1000);
	} else {
		captureBuffer.Tt0 = captureBuffer.params.triggerValue;
		captureBuffer.Trt = captureBuffer.params.retriggerValue;
	}
	captureBuffer.Tc  = 0;
	captureBuffer.info.opReference = opData->opReference;
	captureBuffer.info.opStartMJD  = opData->opStartMJD;
	captureBuffer.info.opStartMPM  = opData->opStartMPM;
	captureBuffer.currentBuffer=0;
	//if (DEBUG_LIVE_CAP){ printf("\nEXIT LiveCapture_Event_RecordingStart\n"); fflush(stdout);}

}

int isCapBufEmpty(int which){
	assert( which==0 || which==1);
	return !captureBuffer.Pp[which];
}


int isCapBufRetrievalStarted(int which){
	assert( which==0 || which==1);
	return (captureBuffer.Pp[which] && captureBuffer.Pr[which]);
}



void 		LiveCapture_CapturePacket(char* packet, size_t packetSize, size_t pos){
	//if (DEBUG_LIVE_CAP){ printf("\nENTER LiveCapture_CapturePacket\n"); fflush(stdout);}

	//if (DEBUG_LIVE_CAP) printf("[LIVE BUFFER] Capture Packet\n");
	if (!pos){ // first (trigger) packet of width-sized capture

		if (!isCapBufEmpty(captureBuffer.currentBuffer)){
			//if (DEBUG_LIVE_CAP) printf("[LIVE BUFFER] Trigger Packet on non-empty current buf\n");
			if(isCapBufRetrievalStarted(captureBuffer.currentBuffer)){
				//if (DEBUG_LIVE_CAP) printf("[LIVE BUFFER] Notice: Retrieval had been previously started, switching buffers\n");
				captureBuffer.currentBuffer ^= 1;
				//printf("[LIVE BUFFER] Trigger. (Retrieval started, selecting buffer %d) %s\n", captureBuffer.currentBuffer ,HumanTime());
			}else{
				;//printf("[LIVE BUFFER] Trigger. (Retrieval not started, overwriting) %s\n",HumanTime());
			}
		} else {
			;//printf("[LIVE BUFFER] Trigger. (CurrentBufferEmpty) %s\n",HumanTime());
		}
		//if (DEBUG_LIVE_CAP) printf("[LIVE BUFFER] Notice: current buffer: %d; Trigger time: %lu\n",captureBuffer.currentBuffer,captureBuffer.Tc);
		captureBuffer.Pp[captureBuffer.currentBuffer]=0;
		captureBuffer.Pr[captureBuffer.currentBuffer]=0;
		captureBuffer.Tt[captureBuffer.currentBuffer]=captureBuffer.Tc;
	}

	if (packetSize != captureBuffer.packetSize){
		Log_Add("[LIVE CAPTURE] FATAL ERROR: Packet size does not match expected packet size\n");
		exit(EXIT_CODE_FAIL);
	}

	if (captureBuffer.Pp[captureBuffer.currentBuffer] >= captureBuffer.Tw){
		Log_Add("[LIVE CAPTURE] FATAL ERROR: Tried to capture past the end of the buffer\n");
		exit(EXIT_CODE_FAIL);
	}
	//if (DEBUG_LIVE_CAP) printf("[LIVE BUFFER] Notice: capture of packet %lu complete\n",captureBuffer.Tc);

	memcpy ( &captureBuffer.buffer[captureBuffer.currentBuffer][captureBuffer.Pp[captureBuffer.currentBuffer] * captureBuffer.packetSize], packet, captureBuffer.packetSize);
	captureBuffer.Pp[captureBuffer.currentBuffer]++;

//	if (DEBUG_LIVE_CAP){ printf("\nEXIT LiveCapture_CapturePacket\n"); fflush(stdout);}

}

void 		LiveCapture_Event_Packet(char* packet, size_t packetSize){
	size_t T_phase;
	size_t T_shifted;

	//printf("\n\n\n\nLiveCapture_Event_Packet : Tc=%lu;   Tt0=%lu\n", captureBuffer.Tc, captureBuffer.Tt0);fflush(stdout);
	if (captureBuffer.Tc < captureBuffer.Tt0){
		captureBuffer.Tc++;
		return;
	}
	T_shifted = captureBuffer.Tc - captureBuffer.Tt0;
	//printf("T_shifted=%lu;\n",T_shifted);fflush(stdout);
	if (captureBuffer.params.retriggerType == RETRIGGER_NONE){
		if (T_shifted < captureBuffer.Tw){
			//if (DEBUG_LIVE_CAP) printf("[LIVE BUFFER] Notice: sending pos=%lu\n",T_phase);
			LiveCapture_CapturePacket(packet,packetSize, T_shifted);
		}
	} else { // captureBuffer.params.retriggerType == RETRIGGER_ALWAYS
		//printf("Trt=%lu;\n",captureBuffer.Trt);fflush(stdout);

		T_phase =  (T_shifted % captureBuffer.Trt);
		//printf("T_phase=%lu;\n",T_phase);fflush(stdout);
		if (T_phase < captureBuffer.Tw){
			//if (DEBUG_LIVE_CAP) printf("[LIVE BUFFER] Notice: sending pos=%lu\n",T_phase);
			LiveCapture_CapturePacket(packet,packetSize, T_phase);
		}
	}
	captureBuffer.Tc++;

}

void		LiveCapture_Event_PacketBefore(char* packet, size_t packetSize){
	if (!captureParamsSet || !captureBufferCreated || !packet || ! packetSize || !captureBufferActive) return;
	if (captureBuffer.params.order != ORDER_BEFORE) return;
	LiveCapture_Event_Packet(packet,packetSize);
	//if (DEBUG_LIVE_CAP) printf("[LIVE BUFFER] Notice: EVENT BEFORE; psize=%lu\n",packetSize);
}

void		LiveCapture_Event_PacketAfter(char* packet, size_t packetSize){
	if (!captureParamsSet || !captureBufferCreated || !packet || ! packetSize || !captureBufferActive) return;
	if (captureBuffer.params.order != ORDER_AFTER) return;
	LiveCapture_Event_Packet(packet,packetSize);
	//if (DEBUG_LIVE_CAP) printf("[LIVE BUFFER] Notice: EVENT AFTER; psize=%lu\n",packetSize);
}

void		LiveCapture_Event_RecordingStop(){
	if (!captureParamsSet || !captureBufferCreated || !captureBufferActive) return;
	//if (DEBUG_LIVE_CAP) printf("[LIVE BUFFER] Notice: EVENT REC STOP\n");
	captureBufferActive = false;
	switch (captureBuffer.params.postBehaviour){
		case POST_BEHAVIOUR_DELETE:
			if(captureBufferCreated){
				free(captureBuffer.buffer[0]);
				free(captureBuffer.buffer[1]);
			}
			bzero(&captureBuffer,sizeof(LiveCaptureBuffer));
			captureBufferCreated = false;
			break;
		case POST_BEHAVIOUR_HOLD:
		case POST_BEHAVIOUR_HOLD_RESTRICT:
		default:
			break;
	}
}

size_t LiveCapture_whichBuffer(){
	if (isCapBufEmpty(0)){
		if (isCapBufEmpty(1))
			return 2;
		else
			return 1;
	} else {
		if (isCapBufEmpty(1))
			return 0;
		else
			if (captureBuffer.Tt[0] <= captureBuffer.Tt[1])
				return 0;
			else
				return 1;
	}
}

size_t LiveCapture__getRetrievableCount(size_t which){
	if (which>1) return 0; //only 2 buffers
	if (isCapBufEmpty(which)){
		return 0;
	} else {
		return captureBuffer.Pp[which]-captureBuffer.Pr[which];
	}
}

size_t LiveCapture_getRetrievableCount(){
	if (!captureParamsSet || !captureBufferCreated) return 0;
	size_t which = LiveCapture_whichBuffer();
	return LiveCapture__getRetrievableCount(which);
}

size_t LiveCapture_getReference(){
	if (!captureParamsSet || !captureBufferCreated) return 0;
	return captureBuffer.info.opReference;
}

size_t LiveCapture_getMJD(){
	if (!captureParamsSet || !captureBufferCreated) return 0;
	return captureBuffer.info.opStartMJD;
}

size_t LiveCapture_getMPM(){
	if (!captureParamsSet || !captureBufferCreated) return 0;
	return captureBuffer.info.opStartMPM;
}


size_t LiveCapture_getPacketSize(){
	if (!captureParamsSet || !captureBufferCreated) return 0;
	return captureBuffer.packetSize;
}

double LiveCapture_getPacketTime(){
	size_t which =LiveCapture_whichBuffer();
	if (which > 1) return 0.0;
	/*return (captureBuffer.Tt[which] + captureBuffer.Pr[which]);
	/*/
	size_t samples = captureBuffer.Tt[which] + captureBuffer.Pr[which];
	return ((double) samples)/captureBuffer.packetRate;
	//*/
}

int	LiveCapture_getPacket(char* destination){
	size_t which =LiveCapture_whichBuffer();
	if (which > 1) return 0;
	memcpy(destination,
		   &captureBuffer.buffer[which][captureBuffer.packetSize * captureBuffer.Pr[which]],
		   captureBuffer.packetSize
	);
	captureBuffer.Pr[which]++;
	if (captureBuffer.Pr[which] >= captureBuffer.Tw){ //retrieved the last packet in the buffer
		captureBuffer.Pr[which]=0;
		captureBuffer.Pp[which]=0;
		captureBuffer.Tt[which]=0;
	}
	return captureBuffer.packetSize;
}

void		LiveCapture_release(){
	// get rid of old buffer if we have one
	if(captureBufferCreated){
		free(captureBuffer.buffer[0]);
		free(captureBuffer.buffer[1]);
	}
	bzero(&captureBuffer,sizeof(LiveCaptureBuffer));
	bzero(&captureParams,sizeof(LiveCaptureParameters));
	captureParamsSet = false;
	captureBufferCreated = false;
	captureBufferActive = false;
}

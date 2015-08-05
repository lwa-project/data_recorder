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
 * Message.c
 *
 *  Created on: Oct 30, 2009
 *      Author: chwolfe2
 */

#include "Message.h"
#include "Config.h"
#include "Log.h"
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include "Globals.h"

#include "Time.h"
const char * ErrorStrings[]={
		"An unknown error has occurred",
		"Operation not permitted",
		"Invalid name",
		"Invalid size",
		"Invalid rate",
		"Already up",
		"Already down",
		"Not detected",
		"Cannot start",
		"Invalid storage ID",
		"File not found",
		"Invalid filename %s",
		"Invalid position",
		"Invalid range",
 	    "Not scheduled",
		"Already stopped",
		"Invalid time",
		"Time conflict: '%s'",
		"Unknown format: '%s'",
		"Insufficient drive space",
		"Component not available: '%s'",
		"Unknown MIB entry '%s'",
		"Unknown command '%s'",
		"File already exists '%s'",
		"Illegal characters in message body",
		"Illegal FFT size",
		"Illegal integration count",
		"Illegal output frame size"
};



StatusCode Message_GenerateResponse(SafeMessage * message, SafeMessage * response, boolean accepted, char* data, size_t dataSize){
	bzero(response,sizeof(SafeMessage));
	TimeStamp now=getTimestamp();
	strncpy(response->type,message->type,3);
	strncpy(response->destination,message->sender,3);
	strncpy(response->reference,message->reference,9);
	strncpy(response->sender,globals.MyReferenceDesignator,3);
	sprintf(response->dataLength, "%04lu", dataSize+8);
	sprintf(&response->MJD[0], "%06lu", now.MJD);
	sprintf(&response->MPM[0], "%09lu", now.MPM);

	response->data.responseData.r_accept[0]= (accepted ? 'A' : 'R');
	if (globals.shutdownRequested)
		strncpy(response->data.responseData.r_status,"SHUTDWN",7);
	else
		strncpy(response->data.responseData.r_status,"NORMAL ",7);
	size_t bytes=(dataSize<RESPONSE_FIELDSIZE_DATA)? dataSize : RESPONSE_FIELDSIZE_DATA;

	if(dataSize)strncpy(response->data.responseData.r_comment,data,bytes);
	response->data.responseData.r_comment[RESPONSE_FIELDSIZE_DATA-1]='\0';
	response->ignored[0]=' ';
	//if (globals.LocalEchoMessaging) printf("[MESSAGING] Received: '%s %s %s %s %s %s %s %s'\n",message->sender, message->destination, message->type, message->reference, message->MJD, message->MPM, message->dataLength, message->data.messageData );
	if (globals.LocalEchoMessaging) printf("[MESSAGING] Sent: '%s %s %s %s %s %s %s %s %s %s'\n",response->sender, response->destination, response->type, response->reference, response->MJD, response->MPM, response->dataLength, response->data.responseData.r_accept, response->data.responseData.r_status, response->data.responseData.r_comment);
	return SUCCESS;

}
StatusCode Message_GenerateResponseBinary(SafeMessage * message, SafeMessage * response, boolean accepted, char* data, size_t dataSize){
	bzero(response,sizeof(SafeMessage));
	TimeStamp now=getTimestamp();
	strncpy(response->type,message->type,3);
	strncpy(response->destination,message->sender,3);
	strncpy(response->reference,message->reference,9);
	strncpy(response->sender,globals.MyReferenceDesignator,3);
	sprintf(response->dataLength, "%04lu", dataSize+8);
	sprintf(&response->MJD[0], "%06lu", now.MJD);
	sprintf(&response->MPM[0], "%09lu", now.MPM);

	response->data.responseData.r_accept[0]= (accepted ? 'A' : 'R');
	if (globals.shutdownRequested)
		strncpy(response->data.responseData.r_status,"SHUTDWN",7);
	else
		strncpy(response->data.responseData.r_status,"NORMAL ",7);
	if(dataSize)memcpy((void*)response->data.responseData.r_comment,(void*)data,dataSize);
	response->ignored[0]=' ';
	//if (globals.LocalEchoMessaging) printf("[MESSAGING] Received: '%s %s %s %s %s %s %s %s'\n",message->sender, message->destination, message->type, message->reference, message->MJD, message->MPM, message->dataLength, message->data.messageData );
	if (globals.LocalEchoMessaging) printf("[MESSAGING] Sent: '%s %s %s %s %s %s %s %s %s %s'\n",response->sender, response->destination, response->type, response->reference, response->MJD, response->MPM, response->dataLength, response->data.responseData.r_accept, response->data.responseData.r_status, "<BINARY DATA OMITTED>");
	return SUCCESS;

}

size_t Message_TransmitSize(BaseMessage * response){
	char buffer[8192];
	strncpy(buffer,&response->dataLength[0],4);
	buffer[4]='\0';
	//printf("Determining Transmit Size as 38 + %lu or %lu\n",strtoul(buffer,NULL,10),(MESSAGE_MINIMUM_SIZE + strtoul(buffer,NULL,10)));
	return (MESSAGE_MINIMUM_SIZE + strtoul(buffer,NULL,10));
}
boolean Message_Verify(BaseMessage * message, size_t bytesReceived){
	//size_t bytesReceived;
	message->ignored[0]=' ';
	int i=0;
	for(i=0;i<bytesReceived;i++){
		if (!isprint(((char*)message)[i]) && ((char*)message)[i]!='\0')
			return false;
	}
	return true;
}

StatusCode GenerateErrorResponse(SafeMessage* message, SafeMessage* response, int errorCode, char* detail){
	if (errorCode>MAX_ERROR_CODE || errorCode<0) errorCode = ERROR_CODE_UNKNOWN_ERROR;
	char buffer[8192];
	sprintf(buffer, ErrorStrings[errorCode], detail);
	return Message_GenerateResponse(message, response, false, buffer, strlen(buffer));
}

void MessageConvertBaseSafe(BaseMessage* base, SafeMessage* safe){
	memcpy((void*)&(safe->reference[0]),        (void*)&(base->reference[0]),          9l); safe->reference[9]  ='\0';
	memcpy((void*)&(safe->MJD[0]),              (void*)&(base->MJD[0]),                6l); safe->MJD[6]        ='\0';
	memcpy((void*)&(safe->MPM[0]),              (void*)&(base->MPM[0]),                9l); safe->MPM[9]        ='\0';
	memcpy((void*)&(safe->dataLength[0]),       (void*)&(base->dataLength[0]),         4l); safe->dataLength[4] ='\0';
	memcpy((void*)&(safe->destination[0]),      (void*)&(base->destination[0]),        3l); safe->destination[3]='\0';
	memcpy((void*)&(safe->sender[0]),           (void*)&(base->sender[0]),             3l); safe->sender[3]     ='\0';
	memcpy((void*)&(safe->type[0]),             (void*)&(base->type[0]),               3l); safe->type[3]       ='\0';
	memcpy((void*)&(safe->ignored[0]),          (void*)" ",                            1l); safe->ignored[1]    ='\0';
	memcpy((void*)&(safe->data.messageData[0]), (void*)&(base->data.messageData[0]),   MESSAGE_FIELDSIZE_DATA); safe->data.messageData[MESSAGE_FIELDSIZE_DATA]  ='\0';
}
void MessageConvertSafeBase(SafeMessage* safe, BaseMessage* base){
	memcpy((void*)&(base->reference[0]),        (void*)&(safe->reference[0]),          9l);
	memcpy((void*)&(base->MJD[0]),              (void*)&(safe->MJD[0]),                6l);
	memcpy((void*)&(base->MPM[0]),              (void*)&(safe->MPM[0]),                9l);
	memcpy((void*)&(base->dataLength[0]),       (void*)&(safe->dataLength[0]),         4l);
	memcpy((void*)&(base->destination[0]),      (void*)&(safe->destination[0]),        3l);
	memcpy((void*)&(base->sender[0]),           (void*)&(safe->sender[0]),             3l);
	memcpy((void*)&(base->type[0]),             (void*)&(safe->type[0]),               3l);
	memcpy((void*)&(base->ignored[0]),          (void*)" ",                            1l);
	memcpy( (void*)&(base->data.responseData.r_accept[0]),
			(void*)&(safe->data.responseData.r_accept[0]),   1l);
	memcpy( (void*)&(base->data.responseData.r_status[0]),
			(void*)&(safe->data.responseData.r_status[0]),   7l);
	memcpy( (void*)&(base->data.responseData.r_comment[0]),
			(void*)&(safe->data.responseData.r_comment[0]),   RESPONSE_FIELDSIZE_DATA);
}

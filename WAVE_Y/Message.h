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
 * Message.h
 *
 *  Created on: Oct 18, 2009
 *      Author: chwolfe2
 */
#ifndef MESSAGE_H_
#define MESSAGE_H_

#include "Defines.h"
#include <stdlib.h>

#pragma pack(1)

typedef int ErrorCode;

#define ERROR_CODE_UNKNOWN_ERROR              0
#define ERROR_CODE_OPERATION_NOT_PERMITTED 	  1
#define ERROR_CODE_INVALID_NAME            	  2
#define ERROR_CODE_INVALID_SIZE			   	  3
#define ERROR_CODE_INVALID_RATE			   	  4
#define ERROR_CODE_ALREADY_UP				  5
#define ERROR_CODE_ALREADY_DOWN				  6
#define ERROR_CODE_NOT_DETECTED				  7
#define ERROR_CODE_CANNOT_START				  8
#define ERROR_CODE_INVALID_STORAGE_ID		  9
#define ERROR_CODE_FILE_NOT_FOUND			 10
#define ERROR_CODE_INVALID_FILENAME			 11
#define ERROR_CODE_INVALID_POSITION			 12
#define ERROR_CODE_INVALID_RANGE			 13
#define ERROR_CODE_NOT_SCHEDULED			 14
#define ERROR_CODE_ALREADY_STOPPED			 15
#define ERROR_CODE_INVALID_TIME				 16
#define ERROR_CODE_TIME_CONFLICT			 17
#define ERROR_CODE_UNKNOWN_FORMAT			 18
#define ERROR_CODE_INSUFFICIENT_SPACE		 19
#define ERROR_CODE_COMPONENT_NOT_AVAILABLE	 20
#define ERROR_CODE_UNKNOWN_MIB_ENTRY         21
#define ERROR_CODE_UNKNOWN_COMMAND           22
#define ERROR_CODE_FILE_ALREADY_EXISTS       23
#define ERROR_CODE_ILLEGAL_CHARACTERS        24
#define ERROR_CODE_ILLEGAL_FFT_SIZE       	 25
#define ERROR_CODE_ILLEGAL_INTEGRATION_COUNT 26
#define ERROR_CODE_ILLEGAL_FRAME_SIZE     	 27
#define MAX_ERROR_CODE						ERROR_CODE_ILLEGAL_FRAME_SIZE



#define MESSAGE_MAXIMUM_SIZE			  8192l
#define MESSAGE_FIELDSIZE_DESTINATION     3l
#define MESSAGE_FIELDSIZE_SENDER          3l
#define MESSAGE_FIELDSIZE_TYPE            3l
#define MESSAGE_FIELDSIZE_REFERENCE       9l
#define MESSAGE_FIELDSIZE_DATALENGTH      4l
#define MESSAGE_FIELDSIZE_MJD             6l
#define MESSAGE_FIELDSIZE_MPM             9l
#define MESSAGE_FIELDSIZE_IGNORED         1l
#define MESSAGE_MINIMUM_SIZE              (MESSAGE_FIELDSIZE_DESTINATION + MESSAGE_FIELDSIZE_SENDER + MESSAGE_FIELDSIZE_TYPE + MESSAGE_FIELDSIZE_REFERENCE + MESSAGE_FIELDSIZE_DATALENGTH + MESSAGE_FIELDSIZE_MJD + MESSAGE_FIELDSIZE_MPM + MESSAGE_FIELDSIZE_IGNORED)
#define MESSAGE_FIELDSIZE_DATA            (MESSAGE_MAXIMUM_SIZE - MESSAGE_MINIMUM_SIZE)
//R-RESPONSE
#define RESPONSE_FIELDSIZE_ACCEPT         1l
//R-STATUS
#define RESPONSE_FIELDSIZE_STATUS         7l
#define RESPONSE_MINIMUM_SIZE             (RESPONSE_FIELDSIZE_DESTINATION + RESPONSE_FIELDSIZE_SENDER + RESPONSE_FIELDSIZE_TYPE + RESPONSE_FIELDSIZE_REFERENCE + RESPONSE_FIELDSIZE_DATALENGTH + RESPONSE_FIELDSIZE_MJD + RESPONSE_FIELDSIZE_MPM + RESPONSE_FIELDSIZE_IGNORED + RESPONSE_FIELDSIZE_ACCEPT + RESPONSE_FIELDSIZE_STATUS)
//R-COMMENT
#define RESPONSE_FIELDSIZE_DATA           (MESSAGE_FIELDSIZE_DATA - (RESPONSE_FIELDSIZE_ACCEPT+RESPONSE_FIELDSIZE_STATUS))

typedef struct __basemessage {
		char destination [MESSAGE_FIELDSIZE_DESTINATION];
		char sender      [MESSAGE_FIELDSIZE_SENDER];
		char type        [MESSAGE_FIELDSIZE_TYPE];
		char reference   [MESSAGE_FIELDSIZE_REFERENCE];
		char dataLength  [MESSAGE_FIELDSIZE_DATALENGTH];
		char MJD         [MESSAGE_FIELDSIZE_MJD];
		char MPM         [MESSAGE_FIELDSIZE_MPM];
		char ignored     [MESSAGE_FIELDSIZE_IGNORED];
		union {
			char messageData        [MESSAGE_FIELDSIZE_DATA];
			struct {
				char r_accept  [RESPONSE_FIELDSIZE_ACCEPT];
				char r_status  [RESPONSE_FIELDSIZE_STATUS];
				char r_comment [RESPONSE_FIELDSIZE_DATA];
			} responseData;
		} data;
} BaseMessage;
typedef struct __safemessage {
		char destination [MESSAGE_FIELDSIZE_DESTINATION+1];
		char sender      [MESSAGE_FIELDSIZE_SENDER+1];
		char type        [MESSAGE_FIELDSIZE_TYPE+1];
		char reference   [MESSAGE_FIELDSIZE_REFERENCE+1];
		char dataLength  [MESSAGE_FIELDSIZE_DATALENGTH+1];
		char MJD         [MESSAGE_FIELDSIZE_MJD+1];
		char MPM         [MESSAGE_FIELDSIZE_MPM+1];
		char ignored     [MESSAGE_FIELDSIZE_IGNORED+1];
		struct {
			char messageData        [MESSAGE_FIELDSIZE_DATA+1];
			struct {
				char r_accept  [RESPONSE_FIELDSIZE_ACCEPT+1];
				char r_status  [RESPONSE_FIELDSIZE_STATUS+1];
				char r_comment [RESPONSE_FIELDSIZE_DATA+1];
			} responseData;
		} data;
} SafeMessage;
//typedef SafeMessage Message;
//#define MessageSize sizeof(BaseMessage)



StatusCode GenerateErrorResponse(SafeMessage* message, SafeMessage* response, ErrorCode errorCode, char* detail);
StatusCode Message_GenerateResponse(SafeMessage * message, SafeMessage * response, boolean accepted, char* data, size_t dataSize);
StatusCode Message_GenerateResponseBinary(SafeMessage * message, SafeMessage * response, boolean accepted, char* data, size_t dataSize);
size_t Message_TransmitSize(BaseMessage * response);
boolean Message_Verify(BaseMessage * message, size_t bytesRecieved);
void MessageConvertBaseSafe(BaseMessage* base, SafeMessage* safe);
void MessageConvertSafeBase(SafeMessage* safe, BaseMessage* base);

#endif /* MESSAGE_H_ */

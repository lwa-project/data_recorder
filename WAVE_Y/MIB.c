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
 * MIB.c
 *
 *  Created on: Oct 29, 2009
 *      Author: chwolfe2
 */
#include "MIB.h"
#include <stdlib.h>
#include <string.h>

#include "Globals.h"
#include "HostInterface.h"
char * NotImplemented = "NOT IMPLEMENTED";

StatusCode _MIB_Get_Operation(SafeMessage* message, SafeMessage* response){
	char buffer[MESSAGE_FIELDSIZE_DATA];
	char result[MESSAGE_FIELDSIZE_DATA];
	strncpy(buffer,message->data.messageData, MESSAGE_FIELDSIZE_DATA);
	buffer[MESSAGE_FIELDSIZE_DATA-1]='\0';
	if 	 	  (strncmp(buffer, "OP-TYPE", 		   7) == 0) {
		sprintf(result,"%s",Operation_TypeName(globals.opData.opType));
		return Message_GenerateResponse(message, response, true, result, strlen(result));
	} else if (strncmp(buffer, "OP-START", 		   8) == 0) {
		sprintf(result,"%06lu %09lu",globals.opData.opStartMJD,globals.opData.opStartMPM);
		return Message_GenerateResponse(message, response, true, result, strlen(result));
	} else if (strncmp(buffer, "OP-STOP", 	  	   7) == 0) {
		sprintf(result,"%06lu %09lu",globals.opData.opStopMJD,globals.opData.opStopMPM);
		return Message_GenerateResponse(message, response, true, result, strlen(result));
	} else if (strncmp(buffer, "OP-REFERENCE",    12) == 0) {
		sprintf(result,"%09lu",globals.opData.opReference);
		return Message_GenerateResponse(message, response, true, result, strlen(result));
	} else if (strncmp(buffer, "OP-ERRORS", 	   9) == 0) {
		sprintf(result,"%015lu %015lu",globals.opData.errorCount,globals.opData.warningCount);
		return Message_GenerateResponse(message, response, true, result, strlen(result));
	} else if (strncmp(buffer, "OP-TAG", 		   6) == 0) {
		snprintf(result,17,"%s",globals.opData.opTag);
		return Message_GenerateResponse(message, response, true, result, strlen(result));
	} else if (strncmp(buffer, "OP-FORMAT", 	   9) == 0) {
		snprintf(result,DATA_FORMAT_MAX_NAME_LENGTH+1,"%s",globals.opData.opFormat);
		return Message_GenerateResponse(message, response, true, result, strlen(result));
	} else if (strncmp(buffer, "OP-FILEPOSITION", 15) == 0) {
		sprintf(result,"%015lu %015lu %015lu",globals.opData.opFileCpyDmpStartPos,globals.opData.opFileCpyDmpLength,globals.opData.opFileCpyDmpCurrentPos);
		return Message_GenerateResponse(message, response, true, result, strlen(result));
	} else if (strncmp(buffer, "OP-FILENAME", 	  11) == 0) {
		sprintf(result,"%-*s %-*s",MAX_DEVICE_NAME_LENGTH,globals.opData.extdisk->deviceName,EXT_FILE_MAX_NAME_LENGTH,globals.opData.opFileName);
		return Message_GenerateResponse(message, response, true, result, strlen(result));
	} else if (strncmp(buffer, "OP-FILEINDEX", 	  12) == 0) {
		sprintf(result,"%09lu",globals.opData.opFileDmpCurrentIndex);
		return Message_GenerateResponse(message, response, true, result, strlen(result));
	} else return GenerateErrorResponse(message, response, ERROR_CODE_UNKNOWN_MIB_ENTRY, buffer);
}

StatusCode _MIB_Get_Schedule(SafeMessage* message, SafeMessage* response){
	char buffer[MESSAGE_FIELDSIZE_DATA];
	strncpy(buffer,message->data.messageData, MESSAGE_FIELDSIZE_DATA);
	buffer[MESSAGE_FIELDSIZE_DATA-1]='\0';
	char opp_string[RESPONSE_FIELDSIZE_DATA];
	OperationParameters opp;
	ScheduleEntry entry;
	char result[40];
	size_t bytes=0;

	//char result[MESSAGE_FIELDSIZE_DATA];
	if 	 	  (strncmp(buffer, "SCHEDULE-COUNT",   14) == 0) {
		int x= _Schedule_Count();
		if (x==-1){
			return Message_GenerateResponse(message, response, true, "000000", strlen("000000"));
		} else {
			char result[40];
			sprintf(result,"%06d",x);
			return Message_GenerateResponse(message, response, true, result, strlen(result));
		}
	} else if (strncmp(buffer, "SCHEDULE-ENTRY-",  15) == 0) {
		strncpy(result,&buffer[15],6); result[6]='\0';
		int index=atoi(result);
		if (!index) return  GenerateErrorResponse(message, response, ERROR_CODE_UNKNOWN_MIB_ENTRY, buffer);
		index--;
		if (Schedule_GetEntryIndex(index,&entry)!=SUCCESS) return GenerateErrorResponse(message, response, ERROR_CODE_UNKNOWN_MIB_ENTRY, buffer);
		if (Schedule_GetEntryDataIndex(index,(char*)&opp, &bytes)!=SUCCESS) return GenerateErrorResponse(message, response, ERROR_CODE_UNKNOWN_MIB_ENTRY, buffer);
		sprintf(opp_string,"%11.11s %09lu %06lu %09lu %06lu %09lu %32.32s", Operation_TypeName(opp.opType), entry.reference, entry.startMJD, entry.startMPM, entry.stopMJD, entry.stopMPM, opp.opFormat);
		return Message_GenerateResponse(message, response, true, opp_string, strlen(opp_string));
	} else return GenerateErrorResponse(message, response, ERROR_CODE_UNKNOWN_MIB_ENTRY, buffer);
}

StatusCode _MIB_Get_Directory(SafeMessage* message, SafeMessage* response){
	char buffer[MESSAGE_FIELDSIZE_DATA];
	char num[32];
	FileMetaData* fm;
	strncpy(buffer,message->data.messageData, MESSAGE_FIELDSIZE_DATA);
	buffer[MESSAGE_FIELDSIZE_DATA-1]='\0';
	char result[RESPONSE_FIELDSIZE_DATA];
	if (globals.upStatus==DOWN)		return GenerateErrorResponse(message, response, ERROR_CODE_COMPONENT_NOT_AVAILABLE, "DRSU");
	if 	 	  (strncmp(buffer, "DIRECTORY-COUNT",   15) == 0) {
		size_t count;
		count = FileSystem_GetFileCount(globals.fileSystem);
		sprintf(result,"%06lu",count);
		return Message_GenerateResponse(message, response, true, result, strlen(result));
	} else if (strncmp(buffer, "DIRECTORY-ENTRY-",  16) == 0) {
		strncpy(num, &message->data.messageData[16], 6);
		num[6]='\0';
		size_t filecount=FileSystem_GetFileCount(globals.fileSystem);
		size_t index=0;
		ssize_t relindex=strtol(num,NULL,10);
		if (relindex<0){
			relindex=relindex + ((ssize_t) 1l) + ((ssize_t)filecount);
		}
		index = (size_t) relindex;
		if (index<1 || index>FileSystem_GetFileCount(globals.fileSystem))
			return  GenerateErrorResponse(message, response, ERROR_CODE_UNKNOWN_MIB_ENTRY, buffer);
		FileSystem_GetFileMetaData(globals.fileSystem,index,&fm);
		sprintf(result,"%16s %6s %9s %6s %9s %32s %15s %15s %3s",
			fm->tag,
			fm->StartMJD,
			fm->StartMPM,
			fm->StopMJD,
			fm->StopMPM,
			fm->format,
			fm->size,
			fm->usage,
			fm->complete
		);
		return Message_GenerateResponse(message, response, true, result, strlen(result));
	} else return GenerateErrorResponse(message, response, ERROR_CODE_UNKNOWN_MIB_ENTRY, buffer);
}


StatusCode _MIB_Get_Devices(SafeMessage* message, SafeMessage* response){
	char buffer[MESSAGE_FIELDSIZE_DATA];
	char result[RESPONSE_FIELDSIZE_DATA];
	int devid;
	//char * endptr;
	char* restxt;
	strncpy(buffer,message->data.messageData, MESSAGE_FIELDSIZE_DATA);
	buffer[MESSAGE_FIELDSIZE_DATA-1]='\0';
	if 	 	  (strncmp(buffer, "DEVICE-COUNT",      12) == 0) {
		sprintf(result,"%d",DiskGetUsableCount());
		return Message_GenerateResponse(message, response, true, result, strlen(result));
	} else if (strncmp(buffer, "DEVICE-ID-",        10) == 0) {
		devid=atoi(&buffer[10]);
		if (!devid) return  GenerateErrorResponse(message, response, ERROR_CODE_UNKNOWN_MIB_ENTRY, buffer);
		devid--;

		if (devid >= 0  && devid < DiskGetUsableCount()){
			restxt=DiskGetUsableName(devid);
			if (restxt){
				return Message_GenerateResponse(message, response, true, restxt, strlen(restxt));
			}
		}
		return GenerateErrorResponse(message, response, ERROR_CODE_UNKNOWN_MIB_ENTRY, buffer);
	} else if (strncmp(buffer, "DEVICE-STORAGE-",   15) == 0) {
		devid=atoi(&buffer[15]);
		if (!devid) return  GenerateErrorResponse(message, response, ERROR_CODE_UNKNOWN_MIB_ENTRY, buffer);
		devid--;
		if (devid >= 0  && devid < DiskGetUsableCount()){
			sprintf(result,"%lu",DiskGetUsableFreeSpace(devid));
			return Message_GenerateResponse(message, response, true, result, strlen(result));
		}
		return GenerateErrorResponse(message, response, ERROR_CODE_UNKNOWN_MIB_ENTRY, buffer);
	} else if (strncmp(buffer, "TOTAL-STORAGE",     13) == 0) {
		if (globals.fileSystem){
			sprintf(result,"%lu",globals.fileSystem->fileSystemHeaderData->filesystemInfo.diskSize);
			return Message_GenerateResponse(message, response, true, result, strlen(result));
		} else {
			return Message_GenerateResponse(message, response, true, "0", 1);
		}
	} else if (strncmp(buffer, "REMAINING-STORAGE", 17) == 0) {
			if (globals.fileSystem){
			sprintf(result,"%lu",FileSystem_GetFreeSpace(globals.fileSystem));
			return Message_GenerateResponse(message, response, true, result, strlen(result));
		} else {
			return Message_GenerateResponse(message, response, true, "0", 1);
		}
	} else if (strncmp(buffer, "CONTIGUOUS-STORAGE", 18) == 0) {
		if (globals.fileSystem){
			sprintf(result,"%lu",FileSystem_GetFreeSpaceContiguous(globals.fileSystem));
			return Message_GenerateResponse(message, response, true, result, strlen(result));
		} else {
			return Message_GenerateResponse(message, response, true, "0", 1);
		}
	} else return GenerateErrorResponse(message, response, ERROR_CODE_UNKNOWN_MIB_ENTRY, buffer);
	return GenerateErrorResponse(message, response, ERROR_CODE_UNKNOWN_ERROR, "");
}
StatusCode _MIB_Get_DRSUINFO(SafeMessage* message, SafeMessage* response){
	char buffer[MESSAGE_FIELDSIZE_DATA];
	char result[RESPONSE_FIELDSIZE_DATA];
	int arrayNum;
	strncpy(buffer,message->data.messageData, MESSAGE_FIELDSIZE_DATA);
	buffer[MESSAGE_FIELDSIZE_DATA-1]='\0';
	if 	 	  (strncmp(buffer, "DRSU-COUNT",      10) == 0) {
		sprintf(result,"%hu", globals.numArraysFound);
		return Message_GenerateResponse(message, response, true, result, strlen(result));
	} else if (strncmp(buffer, "DRSU-SELECTED", 13) == 0) {
		sprintf(result,"%hu", globals.arraySelect+1);
		return Message_GenerateResponse(message, response, true, result, strlen(result));
	} else if (strncmp(buffer, "DRSU-INFO-", 10) == 0) {
		arrayNum=atoi(&buffer[10]);
		if (!arrayNum) return  GenerateErrorResponse(message, response, ERROR_CODE_UNKNOWN_MIB_ENTRY, buffer);
		arrayNum--;
		if (arrayNum < 0 || arrayNum >= globals.numArraysFound)
			return GenerateErrorResponse(message, response, ERROR_CODE_UNKNOWN_MIB_ENTRY, buffer);
		sprintf(result,"DRSU%02hu: %64s %lu", arrayNum+1, globals.arrays[arrayNum].deviceName, globals.arrays[arrayNum].bytesTotal);
		return Message_GenerateResponse(message, response, true, result, strlen(result));
	} else if (strncmp(buffer, "DRSU-BARCODE", 12) == 0) {
		char* bc = globals.arrays[globals.arraySelect].barcode;
		if (strlen(bc))
			return Message_GenerateResponse(message, response, true, bc, strlen(bc));
		else
			return Message_GenerateResponse(message, response, false, "Barcode data unavailable", strlen("Barcode data unavailable"));
	} else {
		return GenerateErrorResponse(message, response, ERROR_CODE_UNKNOWN_MIB_ENTRY, buffer);
	}
}
StatusCode _MIB_Get_CPU(SafeMessage* message, SafeMessage* response){
	char buffer[MESSAGE_FIELDSIZE_DATA];
	char command[8192];
	char respText[RESPONSE_FIELDSIZE_DATA];

	size_t CoreNumber;
	size_t CoresCount;
	strncpy(buffer,message->data.messageData, MESSAGE_FIELDSIZE_DATA);
	buffer[MESSAGE_FIELDSIZE_DATA-1]='\0';
	if 	 	  (strncmp(buffer, "CPU-COUNT",      9) == 0) {
		if (ShellGetNumber("grep processor /proc/cpuinfo | tail -n 1 | nawk '{print $3}'",&CoresCount)==SUCCESS){
			CoresCount++;
			sprintf(respText,"%lu",CoresCount);
			return Message_GenerateResponse(message, response, true, respText , strlen(respText));
		}

	} else if (strncmp(buffer, "CPU-TEMP-",      9) == 0) {
		if (ShellGetNumber("grep processor /proc/cpuinfo | tail -n 1 | nawk '{print $3}'",&CoresCount)==SUCCESS){
			CoresCount++;
			respText[0]='\0';
			char * endptr;
			CoreNumber=strtoul(&buffer[9],&endptr,0);
			if (!CoreNumber) return  GenerateErrorResponse(message, response, ERROR_CODE_UNKNOWN_MIB_ENTRY, buffer);
			CoreNumber--;
			if (!( (((size_t)endptr - (size_t)(&buffer[9]))>=1) && (CoreNumber<CoresCount)))
				return GenerateErrorResponse(message, response, ERROR_CODE_UNKNOWN_MIB_ENTRY, buffer);

			sprintf(command,
					"sensors | "							// get lm-sensors data
					"grep -Ee '(^CPU.+°C)|(^Core)' | "		// find either CPU line for T1500, or Core xx lines for XPS 435MT models
					"head -n %lu | "						// limit output, for 435's this means get indiv. core temp line
					"tail -n 1 | "							// limit to one line, either global temp for T1500, or indiv for 435s
					"cut -d : -f 2 | "						// process the line; use cut since temp field is diff. for diff models
					"cut -d '(' -f 1 | "					// more processing, strip trailing part
					"nawk '{print $1}' | "					// leaves temp with sign and units as 1st/only item on the line
					"tr -d \"°+C\n\""						// remove + sign and units (all are in °C anyway)
					,CoreNumber+1);							// use corenumber + 1 b/c line counts are 1-based core is 0-based
			if (ShellGetString(command,respText,MESSAGE_FIELDSIZE_DATA)==SUCCESS){
				return Message_GenerateResponse(message, response, true, respText , strlen(respText));
			}
		}
	} else return GenerateErrorResponse(message, response, ERROR_CODE_UNKNOWN_MIB_ENTRY, buffer);
	return GenerateErrorResponse(message, response, ERROR_CODE_UNKNOWN_ERROR, "");
}

StatusCode _MIB_Get_HDD(SafeMessage* message, SafeMessage* response){
	char buffer[MESSAGE_FIELDSIZE_DATA];
	char command[8192];
	char respText[RESPONSE_FIELDSIZE_DATA];
	size_t DriveNumber;
	size_t DrivesCount;
	strncpy(buffer,message->data.messageData, MESSAGE_FIELDSIZE_DATA);
	buffer[MESSAGE_FIELDSIZE_DATA-1]='\0';
	if 	 	  (strncmp(buffer, "HDD-COUNT",      9) == 0) {
		if (ShellGetNumber("grep -e s[dg][a-z]$ /proc/partitions | awk '{print $4}' | wc -l",&DrivesCount)==SUCCESS){
			sprintf(respText,"%lu",DrivesCount);
			return Message_GenerateResponse(message, response, true, respText , strlen(respText));
		}

	} else if (strncmp(buffer, "HDD-TEMP-",      9) == 0) {
		if (ShellGetNumber("grep -e s[dg][a-z]$ /proc/partitions | awk '{print $4}' | wc -l",&DrivesCount)==SUCCESS){
			respText[0]='\0';
			char * endptr;
			DriveNumber=strtoul(&buffer[9],&endptr,0);
			if (!DriveNumber) return  GenerateErrorResponse(message, response, ERROR_CODE_UNKNOWN_MIB_ENTRY, buffer);
			DriveNumber--;
			if (!( (((size_t)endptr - (size_t)(&buffer[9]))>=1) && (DriveNumber<DrivesCount)))
				return GenerateErrorResponse(message, response, ERROR_CODE_UNKNOWN_MIB_ENTRY, buffer);
			sprintf(command, "dev=`grep s[dg][a-z]$ /proc/partitions | nawk '{if (NR==%lu) print $4}'`; "
					         "sudo smartctl -A /dev/$dev | grep Temperature_Celsius | nawk '{print $4}' | bc | tr -d '\\n'",DriveNumber+1);
			if (ShellGetString(command,respText,MESSAGE_FIELDSIZE_DATA)==SUCCESS){
				return Message_GenerateResponse(message, response, true, respText , strlen(respText));
			}
		}
	} else return GenerateErrorResponse(message, response, ERROR_CODE_UNKNOWN_MIB_ENTRY, buffer);
	return GenerateErrorResponse(message, response, ERROR_CODE_UNKNOWN_ERROR, "");
}

/*
StatusCode DataFormats_GetCount			(int* count);
StatusCode DataFormats_GetName			(int index, char * formatName);
StatusCode DataFormats_GetSpec			(int index, char * spec);
StatusCode DataFormats_GetPayloadSize	(int index, size_t * payloadSize);
StatusCode DataFormats_GetRate			(int index, size_t * rate);
*/

StatusCode _MIB_Get_Formats(SafeMessage* message, SafeMessage* response){
	char buffer[MESSAGE_FIELDSIZE_DATA];
	strncpy(buffer,message->data.messageData, MESSAGE_FIELDSIZE_DATA);
	buffer[RESPONSE_FIELDSIZE_DATA]='\0';
	char result[RESPONSE_FIELDSIZE_DATA];
	if 	 	  (strncmp(buffer, "FORMAT-COUNT",    12) == 0) {
		int count;
		StatusCode sc = DataFormats_GetCount(&count);
		if (sc==SUCCESS){
			sprintf(result,"%06d", count);
			return Message_GenerateResponse(message, response, true, result, strlen(result));
		} else {
			return GenerateErrorResponse(message, response, ERROR_CODE_UNKNOWN_ERROR, "");
		}
	} else if (strncmp(buffer, "FORMAT-NAME-",    12) == 0) {
		strncpy(result, &message->data.messageData[12], 6);
		result[6]='\0';
		int index=atoi(result);
		if (!index) return  GenerateErrorResponse(message, response, ERROR_CODE_UNKNOWN_MIB_ENTRY, buffer);
		index--;
		StatusCode sc = DataFormats_GetName(index,result);
		if (sc==SUCCESS){
			return Message_GenerateResponse(message, response, true, result, strlen(result));
		} else {
			return GenerateErrorResponse(message, response, ERROR_CODE_UNKNOWN_MIB_ENTRY, buffer);
		}
	} else if (strncmp(buffer, "FORMAT-PAYLOAD-", 15) == 0) {
		strncpy(result, &message->data.messageData[15], 6);
		result[6]='\0';
		int index=atoi(result);
		if (!index) return  GenerateErrorResponse(message, response, ERROR_CODE_UNKNOWN_MIB_ENTRY, buffer);
		index--;
		size_t PayloadSize;
		StatusCode sc = DataFormats_GetPayloadSize(index,&PayloadSize);
		if (sc==SUCCESS){
			sprintf(result,"%04lu",PayloadSize);
			return Message_GenerateResponse(message, response, true, result, strlen(result));
		} else {
			return GenerateErrorResponse(message, response, ERROR_CODE_UNKNOWN_MIB_ENTRY, buffer);
		}
	} else if (strncmp(buffer, "FORMAT-RATE-",    12) == 0) {
		strncpy(result, &message->data.messageData[12], 6);
		result[6]='\0';
		int index=atoi(result);
		if (!index) return  GenerateErrorResponse(message, response, ERROR_CODE_UNKNOWN_MIB_ENTRY, buffer);
		index--;
		size_t rate;
		StatusCode sc = DataFormats_GetRate(index,&rate);
		if (sc==SUCCESS){
			sprintf(result,"%015lu",rate);
			return Message_GenerateResponse(message, response, true, result, strlen(result));
		} else {
			return GenerateErrorResponse(message, response, ERROR_CODE_UNKNOWN_MIB_ENTRY, buffer);
		}
	} else if (strncmp(buffer, "FORMAT-SPEC-",    12) == 0) {
		strncpy(result, &message->data.messageData[12], 6);
		result[6]='\0';
		int index=atoi(result);
		if (!index) return  GenerateErrorResponse(message, response, ERROR_CODE_UNKNOWN_MIB_ENTRY, buffer);
		index--;
		StatusCode sc = DataFormats_GetSpec(index,result);
		if (sc==SUCCESS){
			return Message_GenerateResponse(message, response, true, result, strlen(result));
		} else {
			return GenerateErrorResponse(message, response, ERROR_CODE_UNKNOWN_MIB_ENTRY, buffer);
		}
	} else return GenerateErrorResponse(message, response, ERROR_CODE_UNKNOWN_MIB_ENTRY, buffer);
}
StatusCode _MIB_Get_Timetag(SafeMessage* message, SafeMessage* response){
	char buffer[MESSAGE_FIELDSIZE_DATA];
	char result[MESSAGE_FIELDSIZE_DATA];
	strncpy(buffer,message->data.messageData, MESSAGE_FIELDSIZE_DATA);
	buffer[MESSAGE_FIELDSIZE_DATA-1]='\0';
	if 	 	  (strncmp(buffer, "TT-LAG-INITIAL", 		14) == 0) {
		if (globals.opData.lag_measure_initialized) {
			ssize_t time_first_rx    = (ssize_t) globals.opData.local_time_uspe_first;
			ssize_t timetag_first_rx = (ssize_t) globals.opData.rx_time_uspe_first;
			ssize_t tdelta           = time_first_rx - timetag_first_rx;
			sprintf(result,"%ld",tdelta);
			return Message_GenerateResponse(message, response, true, result, strlen(result));
		} else {
			return Message_GenerateResponse(message, response, false, "No Lag Data Available", strlen("No Lag Data Available"));
		}

	} else if (strncmp(buffer, "TT-LAG", 	             6) == 0) {
		if (globals.opData.lag_measure_initialized) {
			ssize_t time_rx    = (ssize_t) globals.opData.local_time_uspe_recent;
			ssize_t timetag_rx = (ssize_t) globals.opData.rx_time_uspe_recent;
			ssize_t tdelta     = time_rx - timetag_rx;
			sprintf(result,"%ld",tdelta);
			return Message_GenerateResponse(message, response, true, result, strlen(result));
		} else {
			return Message_GenerateResponse(message, response, false, "No Lag Data Available", strlen("No Lag Data Available"));
		}

	} else {
		return GenerateErrorResponse(message, response, ERROR_CODE_UNKNOWN_MIB_ENTRY, buffer);
	}
}

StatusCode _MIB_Get_Log(SafeMessage* message, SafeMessage* response){
	int count,entry;
	char * endptr;
	char buffer[MESSAGE_FIELDSIZE_DATA];
	char result[RESPONSE_FIELDSIZE_DATA];
	strncpy(buffer,message->data.messageData, MESSAGE_FIELDSIZE_DATA);
	buffer[MESSAGE_FIELDSIZE_DATA-1]='\0';
	if 	 	  (strncmp(buffer, "LOG-COUNT",      9) == 0) {
		if (Log_GetCount(&count)==SUCCESS){
			sprintf(result,"%d",count);
			return Message_GenerateResponse(message, response, true, result, strlen(result));
		}
	} else if (strncmp(buffer, "LOG-ENTRY-",    10) == 0) {
		entry=strtol(&buffer[10],&endptr,0);
		if (!entry) return  GenerateErrorResponse(message, response, ERROR_CODE_UNKNOWN_MIB_ENTRY, buffer);
		entry--;
		if ( ( (size_t)endptr - (size_t)(&buffer[10]) >=1)  &&  Log_GetEntry(entry,result)==SUCCESS){
			return Message_GenerateResponse(message, response, true, result, strlen(result));
		}
		return GenerateErrorResponse(message, response, ERROR_CODE_UNKNOWN_MIB_ENTRY, buffer);
	} else return GenerateErrorResponse(message, response, ERROR_CODE_UNKNOWN_MIB_ENTRY, buffer);
	return GenerateErrorResponse(message, response, ERROR_CODE_UNKNOWN_ERROR, "");
}


StatusCode _MIB_Get_Buffer(SafeMessage* message, SafeMessage* response){
	char buffer[MESSAGE_FIELDSIZE_DATA];
	char result[RESPONSE_FIELDSIZE_DATA];
	size_t available;
	size_t fit;
	size_t psize;
	size_t gotten;
	size_t i;
	const size_t TextFieldWidth=10l + 7l + 10l + 19l + 5l + 4l;
	strncpy(buffer,message->data.messageData, MESSAGE_FIELDSIZE_DATA);
	buffer[MESSAGE_FIELDSIZE_DATA-1]='\0';
	if 	 	 (strncmp(buffer, "BUFFER",               6) == 0) {
		available=LiveCapture_getRetrievableCount();
		if (available){
			psize=LiveCapture_getPacketSize();
			fit = (RESPONSE_FIELDSIZE_DATA - TextFieldWidth) / psize;
			if (!fit) return Message_GenerateResponse(message, response, false, "Data too large for response format", strlen("Data too large for response format"));
			if (strncmp(&buffer[6], "-RESTRICT", 15) == 0) fit=1;
			if (available < fit) fit=available;
			sprintf(result,"%09lu %06lu %09lu %18.9lf %04lu %04lu",
					LiveCapture_getReference(),
					LiveCapture_getMJD(),
					LiveCapture_getMPM(),
					LiveCapture_getPacketTime(),
					LiveCapture_getPacketSize(),
					fit);
			for (i=0;i<fit;i++){
				gotten = LiveCapture_getPacket(&result[TextFieldWidth + (i * psize)]);
				if (gotten!=psize) Log_Add("[Live BUFFER] Error: packet retrieved was not the correct length.");
			}
			return Message_GenerateResponseBinary(message,response,true,result,TextFieldWidth + (fit * psize));

		} else {
			return Message_GenerateResponse(message, response, false, "No data available", strlen("No data available"));
		}

	} else return GenerateErrorResponse(message, response, ERROR_CODE_UNKNOWN_MIB_ENTRY, buffer);
}


StatusCode MIB_Get(SafeMessage* message, SafeMessage* response){
	if ((message==NULL) || (response==NULL) ) return FAILURE;
	char buffer[MESSAGE_FIELDSIZE_DATA];
	char result[MESSAGE_FIELDSIZE_DATA];
	strncpy(buffer,message->data.messageData, MESSAGE_FIELDSIZE_DATA);
	buffer[MESSAGE_FIELDSIZE_DATA-1]='\0';
	// a quick 2-character hash for speed
	if 	 	  (strncmp(buffer, "OP", 2) == 0)   return _MIB_Get_Operation(message,response);
	else   if (strncmp(buffer, "SC", 2) == 0)   return _MIB_Get_Schedule(message,response);
	else   if (strncmp(buffer, "DI", 2) == 0)   return _MIB_Get_Directory(message,response);
	else   if (strncmp(buffer, "CP", 2) == 0)   return _MIB_Get_CPU(message,response);
	else   if (strncmp(buffer, "HD", 2) == 0)   return _MIB_Get_HDD(message,response);
	else   if (strncmp(buffer, "FO", 2) == 0)   return _MIB_Get_Formats(message,response);
	else   if (strncmp(buffer, "LO", 2) == 0)   return _MIB_Get_Log(message,response);
	else   if (strncmp(buffer, "DE", 2) == 0)   return _MIB_Get_Devices(message,response);
	else   if (strncmp(buffer, "TO", 2) == 0)   return _MIB_Get_Devices(message,response);
	else   if (strncmp(buffer, "RE", 2) == 0)   return _MIB_Get_Devices(message,response);
	else   if (strncmp(buffer, "CO", 2) == 0)   return _MIB_Get_Devices(message,response);
	else   if (strncmp(buffer, "DR", 2) == 0)   return _MIB_Get_DRSUINFO(message,response);
	else   if (strncmp(buffer, "BU", 2) == 0)   return _MIB_Get_Buffer(message,response);
	else   if (strncmp(buffer, "TT", 2) == 0)   return _MIB_Get_Timetag(message,response);
	// MIB branch 1 here
	else   if (strncmp(buffer, "SUMMARY", 7) == 0)   {
		if (globals.shutdownRequested)
			return Message_GenerateResponse(message, response, true,"SHUTDWN",7);
		else
			if (globals.upStatus==DOWN)
				return Message_GenerateResponse(message, response, true,"WARNING",7);
			else
				return Message_GenerateResponse(message, response, true,"NORMAL ",7);
	}
	else   if (strncmp(buffer, "INFO", 4) == 0)   {
		if (globals.upStatus==DOWN)
			return Message_GenerateResponse(message, response, true,"!Component not available: 'DRSU'",strlen("!Component not available: 'DRSU'"));
		else
			return Message_GenerateResponse(message, response, true,"No warning or error to report",strlen("No warning or error to report"));
	}
	else   if (strncmp(buffer, "LASTLOG", 7) == 0)   {
		if (Log_GetEntry(0,result)==SUCCESS){
			return Message_GenerateResponse(message, response, true, result, strlen(result));
		} else {
			return Message_GenerateResponse(message, response, false,"Component not available: 'LOG'",strlen("Component not available: 'LOG'"));
		}
	}
	else   if (strncmp(buffer, "SUBSYSTEM", 9) == 0)   {
		return Message_GenerateResponse(message, response, true,globals.MyReferenceDesignator,3);
	}
	else   if (strncmp(buffer, "SERIALNO", 8) == 0)   {
		return Message_GenerateResponse(message, response, true,globals.MySerialNumber,strlen(globals.MySerialNumber));
	}
	else   if (strncmp(buffer, "VERSION", 7) == 0)   {
		return Message_GenerateResponse(message, response, true,globals.Version,strlen(globals.Version));
	}
	else   return GenerateErrorResponse(message, response, ERROR_CODE_UNKNOWN_MIB_ENTRY, buffer);
}

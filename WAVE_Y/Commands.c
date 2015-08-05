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
 * Commands.c
 *
 *  Created on: Oct 30, 2009
 *      Author: chwolfe2
 */

#include "Commands.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/mman.h>
#include "Globals.h"
#include "Config.h"
#include "MIB.h"
#include "HostInterface.h"
#include "LiveCapture.h"

char packetbuf[8192];
char filename[8192];
MyFile tempfile;

boolean vfypacket(char* packet, size_t size, boolean isLast);

StatusCode Command_Process(SafeMessage* message, SafeMessage* response){
	assert(message!=NULL && response!=NULL);
	if (globals.LocalEchoMessaging) printf("[MESSAGING] Received: '%s %s %s %s %s %s %s %s'\n",message->sender, message->destination, message->type, message->reference, message->MJD, message->MPM, message->dataLength, message->data.messageData );

	char commandType[4];
	int i=0;for(i=0;i<3;i++)commandType[i]=message->type[i];commandType[3]='\0';
	if 		(strncmp("PNG",commandType,3)==0) return Command_PNG(message,response);
	else if (strncmp("RPT",commandType,3)==0) return Command_RPT(message,response);
	else if (strncmp("SHT",commandType,3)==0) return Command_SHT(message,response);
	else if (strncmp("INI",commandType,3)==0) return Command_INI(message,response);
	else if (strncmp("DEL",commandType,3)==0) return Command_DEL(message,response);
	else if (strncmp("STP",commandType,3)==0) return Command_STP(message,response);
	else if (strncmp("GET",commandType,3)==0) return Command_GET(message,response);
	else if (strncmp("DWN",commandType,3)==0) return Command_DWN(message,response);
	else if (strncmp("UP ",commandType,3)==0) return Command_UP(message,response);
	else if (strncmp(" UP",commandType,3)==0) return Command_UP(message,response);
	else if (strncmp("FRC",commandType,3)==0) return Command_FakeREC(message,response);
	else if (strncmp("REC",commandType,3)==0) return Command_REC(message,response);
	else if (strncmp("TST",commandType,3)==0) return Command_TST(message,response);
	else if (strncmp("VFY",commandType,3)==0) return Command_VFY(message,response);
	else if (strncmp("CPY",commandType,3)==0) return Command_CPY(message,response);
	else if (strncmp("DMP",commandType,3)==0) return Command_DMP(message,response);
	//else if (strncmp("JUP",commandType,3)==0) return Command_JUP(message,response);
	else if (strncmp("SPC",commandType,3)==0) return Command_SPC(message,response);
	else if (strncmp("XCP",commandType,3)==0) return Command_XCP(message,response);
	else if (strncmp("FMT",commandType,3)==0) return Command_FMT(message,response);
	else if (strncmp("SYN",commandType,3)==0) return Command_SYN(message,response);
	else if (strncmp("SEL",commandType,3)==0) return Command_SEL(message,response);
	else if (strncmp("EXT",commandType,3)==0) return Command_EXT(message,response);
	else if (strncmp("BUF",commandType,3)==0) return Command_BUF(message,response);
	else if (strncmp("ECH",commandType,3)==0) return Command_ECH(message,response);
	else if (strncmp("DVN",commandType,3)==0) return Command_DVN(message,response);
	else {
		return GenerateErrorResponse(message, response, ERROR_CODE_UNKNOWN_COMMAND, commandType);
	}
}

#define BAD_FORMAT() return Message_GenerateResponse(message,response,false, "Bad format", strlen("Bad format"));

// Instant Commands
StatusCode Command_PNG(SafeMessage* message, SafeMessage* response){
	return Message_GenerateResponse(message, response, true, "", 0);
}
StatusCode Command_RPT(SafeMessage* message, SafeMessage* response){
	return  MIB_Get(message,response);
}

StatusCode Command_SHT(SafeMessage* message, SafeMessage* response){
	globals.exitRequested=true;
	globals.shutdownRequested=true;
	if (strstr(message->data.messageData,"SCRAM")!=NULL) globals.scramRequested=true;
	if (strstr(message->data.messageData,"REBOOT")!=NULL) globals.rebootRequested=true;
	return Message_GenerateResponse(message,response,true,"",0);
}
StatusCode Command_ECH(SafeMessage* message, SafeMessage* response){
	if (strstr(message->data.messageData,"OFF")!=NULL){
		globals.LocalEchoMessaging = false;
		return Message_GenerateResponse(message,response,true,"Messaging local echo disabled",strlen("Messaging local echo disabled"));
	} else if (strstr(message->data.messageData,"ON")!=NULL) {
		globals.LocalEchoMessaging = true;
		return Message_GenerateResponse(message,response,true,"Messaging local echo enabled",strlen("Messaging local echo enabled"));
	} else {
		if (globals.LocalEchoMessaging==true){
			globals.LocalEchoMessaging=false;
			return Message_GenerateResponse(message,response,true,"Messaging local echo disabled",strlen("Messaging local echo disabled"));
		} else {
			globals.LocalEchoMessaging=true;
			return Message_GenerateResponse(message,response,true,"Messaging local echo enabled",strlen("Messaging local echo enabled"));
		}
	}

}

StatusCode Command_INI(SafeMessage* message, SafeMessage* response){
	globals.exitRequested=true;
	globals.shutdownRequested=false;
	globals.flushConfigRequested=true;
	globals.flushScheduleRequested=true;
	globals.flushLogRequested = false;
	globals.flushDataRequested = false;
	if (strstr(message->data.messageData,"L")!=NULL) globals.flushLogRequested = true;
	if (strstr(message->data.messageData,"D")!=NULL) globals.flushDataRequested = true;
	if (strstr(message->data.messageData,"--flush-log")!=NULL) globals.flushLogRequested = true;
	if (strstr(message->data.messageData,"--flush-data")!=NULL) globals.flushDataRequested = true;
	return Message_GenerateResponse(message,response,true,"",0);
}

StatusCode Command_DVN(SafeMessage* message, SafeMessage* response){
	if ( (globals.upStatus==DOWN) || (_Schedule_Count()!=0) )
		return GenerateErrorResponse(message, response, ERROR_CODE_OPERATION_NOT_PERMITTED, "");
	int slen = strlen(message->data.messageData);
	if (slen >= FILE_MAX_NAME_LENGTH)
		return Message_GenerateResponse(message,response,true,"Volume name exceeds allowable length",strlen("Volume name exceeds allowable length"));
	StatusCode sc = FileSystem_SetVolumeName(globals.fileSystem,message->data.messageData);
	if (sc == SUCCESS){
		globals.exitRequested=true;
		globals.shutdownRequested=false;
		globals.flushConfigRequested=false;
		globals.flushScheduleRequested=true;
		globals.flushLogRequested = false;
		globals.flushDataRequested = false;
		return Message_GenerateResponse(message,response,true,"",0);
	} else {
		return Message_GenerateResponse(message,response,false,"Filesystem reported error in setting file name",strlen("Filesystem reported error in setting file name"));
	}
}

int min (int a, int b){
	if (a<b)
		return a;
	else
		return b;
}
StatusCode Command_SEL(SafeMessage* message, SafeMessage* response){
	char arrayNameTemp[128];
	char temp[128];
	StatusCode sc;
	int scheduleCount;
	unsigned short arraySelect=0;
	int stateWasUp=0;
	if (message->data.messageData[0] == '#'){
		// specify DRSU by barcode
		char* barcode = &message->data.messageData[1];
		int bcl = min(strlen(barcode),MAX_BARCODE_LENGTH);
		if (bcl<13){
			BAD_FORMAT();
		}
		short ai = 0;
		for (ai=0; ai<globals.numArraysFound; ai++){
			if (strncmp(barcode,globals.arrays[ai].barcode,bcl)==0){
				// matching barcode found
				arraySelect = ((unsigned short) ai) +1;
				break;
			}
		}
		if (arraySelect == 0){
			return Message_GenerateResponse(message,response,false, "DRSU with specified barcode is not attached", strlen("DRSU with specified barcode is not attached"));
		}
	}else {
		// specify DRSU by array number
		if (sscanf(message->data.messageData,"%hu",&arraySelect)!=1){
			BAD_FORMAT();
		}
	}

	scheduleCount=_Schedule_Count();
	if (scheduleCount!=0){
		return GenerateErrorResponse(message, response, ERROR_CODE_OPERATION_NOT_PERMITTED, "");
	}
	if (!arraySelect){
		sprintf(arrayNameTemp,"DRSU%hu",arraySelect);
		return GenerateErrorResponse(message,response, ERROR_CODE_COMPONENT_NOT_AVAILABLE,arrayNameTemp);
	}
	arraySelect--;


	if (arraySelect == globals.arraySelect){
		return Message_GenerateResponse(message,response,false, "Already Selected", strlen("Already Selected"));
	}
	if (arraySelect>=globals.numArraysFound){
		sprintf(arrayNameTemp,"DRSU%hu",arraySelect);
		return GenerateErrorResponse(message,response, ERROR_CODE_COMPONENT_NOT_AVAILABLE,arrayNameTemp);
	}
	sprintf(temp,"%hu",arraySelect);
	sc = Config_Set("ArraySelect",temp);
	if (sc!=SUCCESS){
		Log_Add("[COMMAND SEL] Error: failed to update configuration database.\n");
		return GenerateErrorResponse(message,response, ERROR_CODE_UNKNOWN_ERROR,"");
	}
	Log_Add("[COMMAND SEL] Notice: Set default DRSU to DRSU%hu.\n", arraySelect);






	// if the system is UP, bring it down
	if (globals.upStatus!=DOWN){
		globals.upStatus=DOWN;
		stateWasUp=1;
		if (globals.fileSystem && globals.fileSystem->filesystemIsOpen){
			FileSystem_Close(globals.fileSystem);
		}
	}

	//switch DRSUs
	memcpy((void*)globals.disk,(void*)&(globals.arrays[arraySelect]),sizeof(Disk));
	globals.arraySelect=arraySelect;
	if ( ! FileSystem_TestDiskForLWAFS(&globals.arrays[arraySelect])){
		Log_Add("[COMMAND SEL] Notice: Selected DRSU%hu is not formatted, now formatting.\n", arraySelect+1);
		sc = FileSystem_Create(&globals.arrays[arraySelect]);
		if (sc!=SUCCESS){
			Log_Add("[COMMAND SEL] Warning: DRSU  file system creation did not complete successfully.\n");
			return Message_GenerateResponse(message,response,true,"Warning: File System Creation Failure",strlen("Warning: File System Creation Failure"));
		}
		return Message_GenerateResponse(message,response,true,"DRSU Formatted",strlen("DRSU Formatted"));
	}




	// if the system was UP before the command was received, bring it back up
	if (stateWasUp){
		globals.upStatus=UP;
		if (FileSystem_Open(globals.fileSystem,globals.disk)!=SUCCESS){
			globals.upStatus=DOWN;
			return GenerateErrorResponse(message, response, ERROR_CODE_UNKNOWN_ERROR, "");
		}
	}
	return Message_GenerateResponse(message,response,true,"",0);
}

StatusCode Command_EXT(SafeMessage* message, SafeMessage* response){
	globals.exitRequested=true;
	globals.shutdownRequested=false;
	globals.flushConfigRequested=false;
	globals.flushScheduleRequested=false;
    globals.flushLogRequested = false;
	globals.flushDataRequested = false;
	return Message_GenerateResponse(message,response,true,"",0);
}


StatusCode Command_DEL(SafeMessage* message, SafeMessage* response){
	char filename[17];
	size_t opReference;
	size_t mjd;
	int findex=-1;
	int schedule_index=-1;
	if (globals.upStatus==DOWN) return GenerateErrorResponse(message, response, ERROR_CODE_OPERATION_NOT_PERMITTED, "");
	strncpy(filename,message->data.messageData,17);
	filename[16]='\0';
	if (sscanf(message->data.messageData,"%6lu_%9lu",&mjd,&opReference)!=2){
		BAD_FORMAT();
	}
	if (_Schedule_Count()!=0){
		return GenerateErrorResponse(message, response, ERROR_CODE_OPERATION_NOT_PERMITTED, "");
	}
	findex=_FileSystem_GetFileIndex(globals.fileSystem,filename);
	if (findex!=-1){// if file exists....
		//check if any pending operations require the file, or if file is open
		schedule_index = Schedule_getIndexByReference(opReference);
		if ((globals.fileSystem->fileSystemHeaderData->fileInfo[findex].isOpen) || (schedule_index != -1))
			return GenerateErrorResponse(message,response, ERROR_CODE_OPERATION_NOT_PERMITTED,"Delete Open File");
		// delete the file
		if (FileSystem_DeleteFile(globals.fileSystem,findex)!=SUCCESS)
			return GenerateErrorResponse(message,response, ERROR_CODE_UNKNOWN_ERROR,"Can't delete file");
		else
			return Message_GenerateResponse(message, response, true, "File Deleted", strlen("File Deleted"));
	} else {
		return GenerateErrorResponse(message,response, ERROR_CODE_FILE_NOT_FOUND,filename);
	}
}

StatusCode Command_STP(SafeMessage* message, SafeMessage* response){
	char filename[20];
	size_t opReference;
	size_t mjd;
	int schedule_index=-1;
	StatusCode sc;
	if (globals.upStatus==DOWN) return GenerateErrorResponse(message, response, ERROR_CODE_OPERATION_NOT_PERMITTED, "");
	strncpy(filename,message->data.messageData,20);
	filename[17]='\0';
	if (sscanf(filename,"%lu_%lu",&mjd,&opReference)!=2)
		BAD_FORMAT();
	//opReference=strtoul(filename,NULL,10);
	if (globals.opData.opReference == opReference){
		Process_AbortCurrent();
		return Message_GenerateResponse(message, response, true, "", 0);
	} else {
		schedule_index = Schedule_getIndexByReference(opReference);
		if (schedule_index!=-1){
			sc=Schedule_RemoveEntry(opReference);
			if (sc!=SUCCESS){
				Log_Add("[COMMAND STOP] Error: failed to remove item %d with reference %lu from schedule.\n",schedule_index,opReference);
				exit(EXIT_CODE_FAIL);
			}
			return Message_GenerateResponse(message, response, true, "", 0);
		} else {
			if (FileSystem_FileExists(globals.fileSystem,filename)){
				return GenerateErrorResponse(message,response, ERROR_CODE_ALREADY_STOPPED,"");
			} else {
				char specname[1024];
				sprintf(specname,"/LWADATA/%06lu_%09lu_spectrometer.dat",mjd,opReference);
				if (!access(specname,F_OK)){
					return GenerateErrorResponse(message,response, ERROR_CODE_ALREADY_STOPPED,"");
				}
				return GenerateErrorResponse(message,response, ERROR_CODE_NOT_SCHEDULED,"");
			}
		}
	}
}

StatusCode Command_GET(SafeMessage* message, SafeMessage* response){
	char *rdbuffer = NULL;
	char filename[17];
	size_t opReference;
	size_t mjd;
	size_t start;
	size_t length;
	ssize_t bytesRead;
	size_t fileStartPos;
	size_t chunksize=globals.fileSystem->fileSystemHeaderData->filesystemInfo.chunkSize;
	StatusCode sc;
	if (globals.upStatus==DOWN) return GenerateErrorResponse(message, response, ERROR_CODE_OPERATION_NOT_PERMITTED, "");
	tempfile.index=-1;
	tempfile.isOpen=false;
	rdbuffer = (char*) mmap(NULL, chunksize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_POPULATE | MAP_LOCKED, -1, 0);
	strncpy(filename,message->data.messageData,17);
	filename[16]='\0';

	if (sscanf(message->data.messageData,"%6lu_%9lu %15lu %15lu",&mjd,&opReference,&start, &length)!=4){
		munmap((void*)rdbuffer,chunksize);
		BAD_FORMAT();
	}
	//printf("input good mjd=%lu  ref=%lu  start=%lu  len=%lu\n",mjd,opReference,start, length);
	if (length>8146){
		munmap((void*)rdbuffer,chunksize);
		return GenerateErrorResponse(message,response, ERROR_CODE_INVALID_RANGE,"");
	}
	//printf("range good\n");
	if (FileSystem_FileExists(globals.fileSystem,filename)){
		//printf("File Exists\n");
		if (FileSystem_OpenFile(globals.fileSystem,&tempfile,filename,READ)!=SUCCESS){
			munmap((void*)rdbuffer,chunksize);
			return GenerateErrorResponse(message,response, ERROR_CODE_UNKNOWN_ERROR,"");
		}
		//printf("File is opened\n");
		if (length+start-1 > globals.fileSystem->fileSystemHeaderData->fileInfo[tempfile.index].size){
			munmap((void*)rdbuffer,chunksize);
			return GenerateErrorResponse(message,response, ERROR_CODE_INVALID_RANGE,"");
		}
		//printf("File size ok\n");
		fileStartPos=globals.fileSystem->fileSystemHeaderData->fileInfo[tempfile.index].startPosition + chunksize ;
		// may require two reads if boundary overlaps chunk size
		if ( ((length+start-1) & ~(chunksize-1)) == (start & ~(chunksize-1))){
			//printf("File get in 1 read\n");
			if (_FileSystem_SynchronousRead(globals.fileSystem,(void*)rdbuffer,fileStartPos+(start & ~(chunksize-1)),chunksize,&bytesRead)!=SUCCESS || (bytesRead!=chunksize)){
				Log_Add("[COMMAND GET] File _FileSystem_SynchronousRead fail\n");
				FileSystem_CloseFile(&tempfile);
				munmap((void*)rdbuffer,chunksize);
				return GenerateErrorResponse(message,response, ERROR_CODE_UNKNOWN_ERROR,"");
			} else {
				//printf("File read #1 \n");
				memcpy((void*)packetbuf, (void*)&(rdbuffer[(start % chunksize)]),length);
			}
		} else {
			//printf("File get in 2 reads\n");
			if (_FileSystem_SynchronousRead(globals.fileSystem,(void*)rdbuffer,fileStartPos+(start & ~(chunksize-1)),chunksize,&bytesRead)!=SUCCESS || (bytesRead!=chunksize)){
				Log_Add("[COMMAND GET] File _FileSystem_SynchronousRead fail\n");
				FileSystem_CloseFile(&tempfile);
				munmap((void*)rdbuffer,chunksize);
				return GenerateErrorResponse(message,response, ERROR_CODE_UNKNOWN_ERROR,"");
			} else {
				//printf("File read #1 \n");
				memcpy((void*)packetbuf, (void*)&(rdbuffer[(start % chunksize)]),chunksize-(start % chunksize));
			}
			if (_FileSystem_SynchronousRead(globals.fileSystem,(void*)rdbuffer,fileStartPos+(start & ~(chunksize-1))+chunksize,chunksize,&bytesRead)!=SUCCESS || (bytesRead!=chunksize)){
				Log_Add("[COMMAND GET] File _FileSystem_SynchronousRead fail\n");
				FileSystem_CloseFile(&tempfile);
				munmap((void*)rdbuffer,chunksize);
				return GenerateErrorResponse(message,response, ERROR_CODE_UNKNOWN_ERROR,"");
			} else {
				//printf("File read #2 \n");
				memcpy((void*)&(packetbuf[(chunksize-(start % chunksize))]), (void*)&(rdbuffer[0]),length-(chunksize-(start % chunksize)));
			}
		}
		//printf("File read done \n");
		if(FileSystem_CloseFile(&tempfile)!=SUCCESS){
			Log_Add("[COMMAND GET] File close fail\n");
			munmap((void*)rdbuffer,chunksize);
			return GenerateErrorResponse(message,response, ERROR_CODE_UNKNOWN_ERROR,"");
		} else{
			//printf("File close success\n");
			sc = Message_GenerateResponseBinary(message,response,true,packetbuf,length);
			munmap((void*)rdbuffer,chunksize);;
			return sc;
		}
	} else {
		//printf("File not found\n");
		munmap((void*)rdbuffer,chunksize);
		return GenerateErrorResponse(message,response, ERROR_CODE_FILE_NOT_FOUND,filename);
	}
}






StatusCode Command_BUF(SafeMessage* message, SafeMessage* response){
	int i=0;
	char* loc=NULL;
	int pos;
	int readcount;
	LiveCaptureParameters lci;

	char a[200];
	char b[200];
	char c[200];
	char d[200];
	char f[200];
	char h[200];
	char buffer[2000];


	if (globals.upStatus==DOWN) return GenerateErrorResponse(message, response, ERROR_CODE_OPERATION_NOT_PERMITTED, "");
	for (i=0;i<6;i++){
		loc = strchr(LiveCaptureMnemonics[i],message->data.messageData[i]);
		if (loc==NULL){
			return Message_GenerateResponse(message,response,false, "Bad format: invalid mnemonics", strlen("Bad format: invalid mnemonics"));
		}
		pos=(int)((size_t)loc - (size_t)LiveCaptureMnemonics[i]);
		switch (i){
			case 0: lci.triggerType = pos; break;
			case 1: lci.widthType = pos; break;
			case 2: lci.retriggerType = pos; break;
			case 3: lci.order = pos; break;
			case 4: lci.postBehaviour = pos; break;
			case 5: /*do nothing*/ break;
		}
	}

	readcount = sscanf(&message->data.messageData[6],"%lu %lu %lu",
			&lci.triggerValue,
			&lci.widthValue,
			&lci.retriggerValue
	);

	if (readcount!=3) return Message_GenerateResponse(message,response,false, "Bad format: parameter count", strlen("Bad format: parameter count"));
	LiveCapture_setParameters(&lci);

	sprintf(a,"Triggered: %lu %s.\n",lci.triggerValue, trigTypes[lci.triggerType]);
	sprintf(b,"Capture for: %lu %s\n",lci.widthValue, trigTypes[lci.widthType]);
	sprintf(c,"every %lu %s",lci.retriggerValue, trigTypes[lci.triggerType]);
	sprintf(d,"Retrigger: %s\n",((lci.retriggerType) ? c : "NEVER"));
	sprintf(f,"Capture: %s formatting\n",((lci.order) ? "AFTER" : "BEFORE"));
	sprintf(h,"End recording: %s\n",postBehaviours[lci.postBehaviour]);
	sprintf(buffer, "[COMMAND BUFFER] Set on-the-fly capture buffer:\n %s %s %s %s %s",a,b,d,f,h);
	Log_Add("%s",buffer);
	return Message_GenerateResponse(message,response,true, buffer, strlen(buffer));

}
StatusCode Command_FakeREC(SafeMessage* message, SafeMessage* response){
	OperationParameters opp,conflictOpp;
	ScheduleEntry temp;
	size_t size,size2;
	DataFormat format;
	/*CompiledDataFormat compiledFormat;*/
	bzero((void*)&opp,sizeof(OperationParameters));
	bzero((void*)&conflictOpp,sizeof(OperationParameters));

	size_t length;
	size_t fakelength;
	if (globals.upStatus==DOWN) return GenerateErrorResponse(message, response, ERROR_CODE_OPERATION_NOT_PERMITTED, "");
	int readcount = sscanf(message->data.messageData,"%6lu %9lu %9lu %9lu %32s",&opp.opStartMJD, &opp.opStartMPM, &length, &fakelength, opp.opFormat);
	if (readcount!=5){
		return GenerateErrorResponse(message, response, ERROR_CODE_UNKNOWN_FORMAT, opp.opFormat);
	} else {
		opp.opStopMJD = opp.opStartMJD;
		opp.opStopMPM = opp.opStartMPM + length;
		if (opp.opStopMPM > MILLISECONDS_PER_DAY){
			opp.opStopMPM -= MILLISECONDS_PER_DAY;
			opp.opStopMJD++;
		}
		sprintf(opp.opDeviceName,"%s",globals.disk->deviceName);
		char buffer[10]={0,0,0,0,0,0,0,0,0,0};
		strncpy(buffer,message->reference,9);
		opp.opReference=strtoul(buffer,NULL,10);
		buffer[9]='\0';
		sprintf(opp.opTag,"%06lu_%09lu",opp.opStartMJD,opp.opReference);
		opp.opType=OP_TYPE_REC;
		// now make sure we have the space
		//printf("Message: \n'%s'\n '%s'\n '%s'\n '%s'\n '%s'\n '%s'\n '%s'\n '%s'\n",message->sender, message->destination, message->type, message->reference, message->MJD, message->MPM, message->dataLength, message->data.messageData );
		if (DataFormats_Get(opp.opFormat, &format)!=SUCCESS)
			return GenerateErrorResponse(message, response, ERROR_CODE_UNKNOWN_FORMAT, opp.opFormat);
/*		if (DataFormats_Compile(&format, &compiledFormat)!=SUCCESS)*/
/*			return GenerateErrorResponse(message, response, ERROR_CODE_UNKNOWN_FORMAT, opp.opFormat);*/
		size =  (format.rawRate * length) / 1000l;
		size2 =  (format.rawRate * fakelength) / 1000l;
		if (size2>size) size2=size;
		if (!FileSystem_IsFileCreatable(globals.fileSystem,opp.opTag,size,false)){
			if (FileSystem_FileExists(globals.fileSystem,opp.opTag)){
				return GenerateErrorResponse(message, response, ERROR_CODE_FILE_ALREADY_EXISTS, opp.opTag);
			} else {
				return GenerateErrorResponse(message, response, ERROR_CODE_INSUFFICIENT_SPACE, "");
			}
		}
		temp.reference	= opp.opReference;
		temp.startMJD   = opp.opStartMJD;
		temp.startMPM   = opp.opStartMPM;
		temp.stopMJD    = opp.opStopMJD;
		temp.stopMPM    = opp.opStopMPM;
		temp.data       = (char*)&opp;
		temp.datalength = sizeof(OperationParameters);
		int findex=0;
		if (FileSystem_CreateFile(globals.fileSystem,opp.opTag,size,false,&findex)!=SUCCESS){
			return GenerateErrorResponse(message, response, ERROR_CODE_UNKNOWN_ERROR, "");
		}
		FileSystem_SetFileMetaData(globals.fileSystem, findex, opp.opStartMPM, opp.opStartMJD, opp.opReference, opp.opStopMPM, opp.opStopMJD, opp.opFormat);
		MyFile filp;
		bzero((void*)&filp,sizeof(MyFile));
		filp.index=-1;
		if (FileSystem_OpenFile(globals.fileSystem,&filp,opp.opTag,WRITE)!=SUCCESS){
			return GenerateErrorResponse(message, response, ERROR_CODE_UNKNOWN_ERROR, "");
		}
		globals.fileSystem->fileSystemHeaderData->fileInfo[findex].currentPosition=size2;
		FileSystem_SetFileMetaDataIsComplete(globals.fileSystem, findex,1);
		if (FileSystem_CloseFile(&filp)!=SUCCESS){
			return GenerateErrorResponse(message, response, ERROR_CODE_UNKNOWN_ERROR, "");
		}
		return Message_GenerateResponse(message, response, true, opp.opTag, strlen(opp.opTag));
	}

}
bool relayedFromREC = false;
// Schedulable Commands (Operations)
StatusCode Command_REC(SafeMessage* message, SafeMessage* response){
	char conflictString[8192];
	OperationParameters opp,conflictOpp;
	ScheduleEntry temp;
	ScheduleEntry conflict;
	StatusCode sc;
	size_t size;
	DataFormat format;
	/*CompiledDataFormat compiledFormat;*/
	bzero((void*)&opp,sizeof(OperationParameters));
	bzero((void*)&conflictOpp,sizeof(OperationParameters));

	size_t __mjd, __mpm, __length, __tl, __ic, __mf, __hw;
	int rc_temp = sscanf(message->data.messageData,"%6lu %9lu %9lu %6lu %8lu %8lu %4lu",&__mjd, &__mpm, &__length, &__tl, &__ic, &__mf, &__hw);
	if (rc_temp >= 5){
		return Command_SPC(message, response);
	}


	size_t length;
	if (globals.upStatus==DOWN) return GenerateErrorResponse(message, response, ERROR_CODE_OPERATION_NOT_PERMITTED, "");
	int readcount = sscanf(message->data.messageData,"%6lu %9lu %9lu %32s",&opp.opStartMJD, &opp.opStartMPM, &length, opp.opFormat);
	if (readcount<4){
		return GenerateErrorResponse(message, response, ERROR_CODE_UNKNOWN_FORMAT, opp.opFormat);
	} else {
		opp.opStopMJD = opp.opStartMJD;
		opp.opStopMPM = opp.opStartMPM + length;
		while (opp.opStopMPM > MILLISECONDS_PER_DAY){
			opp.opStopMPM -= MILLISECONDS_PER_DAY;
			opp.opStopMJD++;
		}
		sprintf(opp.opDeviceName,"%s",globals.disk->deviceName);
		char buffer[10]={0,0,0,0,0,0,0,0,0,0};
		strncpy(buffer,message->reference,9);
		opp.opReference=strtoul(buffer,NULL,10);
		buffer[9]='\0';
		sprintf(opp.opTag,"%06lu_%09lu",opp.opStartMJD,opp.opReference);
		opp.opType=OP_TYPE_REC;
		// now make sure we have the space
		//printf("Message: \n'%s'\n '%s'\n '%s'\n '%s'\n '%s'\n '%s'\n '%s'\n '%s'\n",message->sender, message->destination, message->type, message->reference, message->MJD, message->MPM, message->dataLength, message->data.messageData );
		if (DataFormats_Get(opp.opFormat, &format)!=SUCCESS){
			return GenerateErrorResponse(message, response, ERROR_CODE_UNKNOWN_FORMAT, opp.opFormat);
		}

		/*if (DataFormats_Compile(&format, &compiledFormat)!=SUCCESS)*/
			/*return GenerateErrorResponse(message, response, ERROR_CODE_UNKNOWN_FORMAT, opp.opFormat);*/
		size =  (((format.rawRate * length) / 1000l)* 101l)/100l;
		if (!FileSystem_IsFileCreatable(globals.fileSystem,opp.opTag,size,false)){
			if (FileSystem_FileExists(globals.fileSystem,opp.opTag)){
				return GenerateErrorResponse(message, response, ERROR_CODE_FILE_ALREADY_EXISTS, opp.opTag);
			} else {
				return GenerateErrorResponse(message, response, ERROR_CODE_INSUFFICIENT_SPACE, "");
			}
		}
		temp.reference	= opp.opReference;
		temp.startMJD   = opp.opStartMJD;
		temp.startMPM   = opp.opStartMPM;
		temp.stopMJD    = opp.opStopMJD;
		temp.stopMPM    = opp.opStopMPM;
		temp.data       = (char*)&opp;
		temp.datalength = sizeof(OperationParameters);
		if (Schedule_AddEntry(&temp,&conflict)!=SUCCESS){
			if (Schedule_EntriesEqual(&temp,&conflict)){
				return GenerateErrorResponse(message, response, ERROR_CODE_INVALID_TIME, "");
			} else {

				size_t bytes;
				sc = Schedule_GetEntryData(conflict.reference,(char*)&conflictOpp,&bytes);
				if (sc!=SUCCESS || bytes!= sizeof(OperationParameters)){
					Log_Add("[COMMAND RECORD] Error retrieving conflict data\nExitting\n");
					exit(EXIT_CODE_FAIL);
				}
				sprintf(conflictString,"%11.11s %09lu %06lu %09lu %06lu %09lu %32.32s", Operation_TypeName(conflictOpp.opType), conflict.reference, conflict.startMJD, conflict.startMPM, conflict.stopMJD, conflict.stopMPM, conflictOpp.opFormat);
				return GenerateErrorResponse(message, response, ERROR_CODE_TIME_CONFLICT, conflictString);
			}
		} else {
			int findex=0;
			if (FileSystem_CreateFile(globals.fileSystem,opp.opTag,size,false,&findex)!=SUCCESS){
				return GenerateErrorResponse(message, response, ERROR_CODE_UNKNOWN_ERROR, "");
			}
			FileSystem_SetFileMetaData(globals.fileSystem, findex, opp.opStartMPM, opp.opStartMJD, opp.opReference, opp.opStopMPM, opp.opStopMJD, opp.opFormat);
			return Message_GenerateResponse(message, response, true, opp.opTag, strlen(opp.opTag));
		}
	}
}
void dumppacket(char* packet, size_t size, size_t hlStart, size_t hlStop);

StatusCode Command_TST(SafeMessage* message, SafeMessage* response){
	char *rdbuffer =  (char*) mmap(NULL, 1048576, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_POPULATE | MAP_LOCKED, -1, 0);
	//char *rdbuffer = (char*)globals.rxQueue->data;
	//char rdbuffer[262144];
	if (globals.upStatus==DOWN) return GenerateErrorResponse(message, response, ERROR_CODE_OPERATION_NOT_PERMITTED, "");
	size_t bytes_in_rdbuf=0;
	size_t bytes_in_pbuf=0;
	size_t bytesToCpy=0;
	size_t frameSize=200;
	size_t numFrames=50;
	size_t startFrame=0;
	size_t FrameCount=0;

	//char * next=NULL;
	//startFrame = strtoul(message->data.messageData, &next ,10);
	//numFrames = strtoul(next, &next ,10);
	//frameSize = strtoul(next, &next ,10);
	//strcpy(filename,"055142_000000208");
	//strncpy(filename,next+1,16);
	//filename[16]='\0';

	int count=sscanf(message->data.messageData,"%lu %lu %lu %16s", &startFrame, &numFrames, &frameSize, filename);
	//printf("data='%s'\n",message->data.messageData);
	Log_Add("[COMMAND TEST] framedata: '%lu %lu %lu %s'\n", startFrame, numFrames, frameSize, filename);

	if (count!=4) return Message_GenerateResponse(message, response, false, "Bad Format", strlen("Bad Format"));

	tempfile.index=-1;
	tempfile.isOpen=false;
	StatusCode sc = FileSystem_OpenFile(globals.fileSystem,&tempfile,filename,READ);
	if (sc==SUCCESS){
		//printf("(first) reof %d, frm %lu, bytes_in_pbuf %lu, bytes_in_rdbuf %lu\n",myfile.reof, numFrames,bytes_in_pbuf, bytes_in_rdbuf);
		while (!tempfile.reof && numFrames){
			//printf("reof %d, frm %lu, bytes_in_pbuf %lu, bytes_in_rdbuf %lu\n",myfile.reof, numFrames,bytes_in_pbuf, bytes_in_rdbuf);
			if (bytes_in_pbuf && bytes_in_rdbuf) {
				bytesToCpy=(bytes_in_rdbuf<(frameSize-bytes_in_pbuf)) ? bytes_in_rdbuf : (frameSize-bytes_in_pbuf);
				if ((bytesToCpy + bytes_in_pbuf) > 8192) bytesToCpy = 8192-bytes_in_pbuf;
				memcpy( (void *) (&(packetbuf[bytes_in_pbuf])), (void *) &rdbuffer[262144-bytes_in_rdbuf], bytesToCpy);
				bytes_in_pbuf+=bytesToCpy;
				if (bytes_in_pbuf>=frameSize){
					if (++FrameCount>startFrame) dumppacket(packetbuf,frameSize,0,0);
					numFrames--;
					bytes_in_pbuf-=frameSize;
					if (bytes_in_pbuf!=0){Log_Add("[COMMAND TEST] Algorithm error\n"); exit(EXIT_CODE_FAIL);}
				}
				bytes_in_rdbuf-=bytesToCpy;
			} else if (bytes_in_rdbuf) {
				bytesToCpy=(bytes_in_rdbuf<frameSize) ? bytes_in_rdbuf : frameSize;
				if ((bytesToCpy + bytes_in_pbuf) > 8192) bytesToCpy = 8192-bytes_in_pbuf;
				memcpy( (void *) (&(packetbuf[bytes_in_pbuf])), (void *) &rdbuffer[262144-bytes_in_rdbuf], bytesToCpy);
				bytes_in_pbuf+=bytesToCpy;
				if (bytes_in_pbuf>=frameSize){
					if (++FrameCount>startFrame) dumppacket(packetbuf,frameSize,0,0);
					numFrames--;
					bytes_in_pbuf-=frameSize;
					if (bytes_in_pbuf!=0){Log_Add("[COMMAND TEST] Algorithm error\n"); exit(EXIT_CODE_FAIL);}
				}
				bytes_in_rdbuf-=bytesToCpy;
			} else {
				sc=FileSystem_AsynchronousReadStart(&tempfile,rdbuffer,262144);
				if (sc!=SUCCESS){Log_Add("[COMMAND TEST] File Read Failure\n"); exit(EXIT_CODE_FAIL);}
				while ((sc=FileSystem_IsAsynchronousReadDone(&tempfile))==NOT_READY){
					usleep(10000);//wait
				}
				if (sc!=SUCCESS){Log_Add("[COMMAND TEST] File Read Failure\n"); exit(EXIT_CODE_FAIL);}
				if(FileSystem_GetAsynchronousBytesRead(&tempfile,&bytes_in_rdbuf)!=SUCCESS){Log_Add("[COMMAND TEST] File Read Failure\n"); exit(EXIT_CODE_FAIL);}
			}
		}
		if(FileSystem_CloseFile(&tempfile)!=SUCCESS){Log_Add("[COMMAND TEST] File Close Failure\n"); exit(EXIT_CODE_FAIL);}
		Log_Add("[COMMAND TEST] Finish dump\n"); fflush(stdout);
	} else {
		munmap((void*)rdbuffer , 1048576);
		return Message_GenerateResponse(message, response, false, "File Open Failure",strlen("File Open Failure") );
	}
	munmap((void*)rdbuffer , 1048576);
	return Message_GenerateResponse(message, response, false, "", 0);

}

StatusCode Command_VFY(SafeMessage* message, SafeMessage* response){
	char *rdbuffer =  (char*) mmap(NULL, 1048576, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_POPULATE | MAP_LOCKED, -1, 0);
	if (rdbuffer == (char  *) MAP_FAILED){
		printf("[VERIFY] MAP FAILED\n");
		exit(-1);
	}
	size_t bytes_in_rdbuf=0;
	size_t bytes_in_pbuf=0;
	size_t bytesToCpy=0;
	size_t frameSize=200;
	size_t numFrames=50;
	size_t startFrame=0;
	size_t FrameCount=0;
	if (globals.upStatus==DOWN) return GenerateErrorResponse(message, response, ERROR_CODE_OPERATION_NOT_PERMITTED, "");
	int count=sscanf(message->data.messageData,"%lu %lu %lu %16s", &startFrame, &numFrames, &frameSize, filename);
	if (count!=4) return Message_GenerateResponse(message, response, false, "Bad Format", strlen("Bad Format"));
	tempfile.index=-1;
	tempfile.isOpen=false;
	StatusCode sc = FileSystem_OpenFile(globals.fileSystem,&tempfile,filename,READ);
	if (sc==SUCCESS){
		while (!tempfile.reof && numFrames){
			if (bytes_in_pbuf && bytes_in_rdbuf) {
				bytesToCpy=(bytes_in_rdbuf<(frameSize-bytes_in_pbuf)) ? bytes_in_rdbuf : (frameSize-bytes_in_pbuf);
				//if ((bytesToCpy + bytes_in_pbuf) > 8192) bytesToCpy = 8192-bytes_in_pbuf;
				memcpy( (void *) (&(packetbuf[bytes_in_pbuf])), (void *) &rdbuffer[262144-bytes_in_rdbuf], bytesToCpy);
				bytes_in_pbuf+=bytesToCpy;
				if (bytes_in_pbuf>=frameSize){
					if (++FrameCount>startFrame)
						if (!vfypacket(packetbuf,frameSize,(numFrames==0))){
							munmap((void*)rdbuffer , 1048576);
							return Message_GenerateResponse(message, response, false, "File Verification Failed",strlen("File Verification Failed"));
						}

					numFrames--;
					bytes_in_pbuf-=frameSize;
				}
				bytes_in_rdbuf-=bytesToCpy;
			} else if (bytes_in_rdbuf) {
				bytesToCpy=(bytes_in_rdbuf<frameSize) ? bytes_in_rdbuf : frameSize;
				//if ((bytesToCpy + bytes_in_pbuf) > 8192) bytesToCpy = 8192-bytes_in_pbuf;
				memcpy( (void *) (&(packetbuf[bytes_in_pbuf])), (void *) &rdbuffer[262144-bytes_in_rdbuf], bytesToCpy);
				bytes_in_pbuf+=bytesToCpy;
				if (bytes_in_pbuf>=frameSize){
					if (++FrameCount>startFrame)
						if (!vfypacket(packetbuf,frameSize,(numFrames==0))){
							munmap((void*)rdbuffer , 1048576);
							return Message_GenerateResponse(message, response, false, "File Verification Failed",strlen("File Verification Failed"));
						}
					numFrames--;
					bytes_in_pbuf-=frameSize;
				}
				bytes_in_rdbuf-=bytesToCpy;
			} else {
				sc=FileSystem_AsynchronousReadStart(&tempfile,rdbuffer,262144);
				if (sc!=SUCCESS){Log_Add("[COMMAND VERIFY] File Read Failure\n"); exit(EXIT_CODE_FAIL);}
				while ((sc=FileSystem_IsAsynchronousReadDone(&tempfile))==NOT_READY){
					usleep(1000);//wait
				}
				if (sc!=SUCCESS){Log_Add("[COMMAND VERIFY] File Read Failure\n"); exit(EXIT_CODE_FAIL);}
				if(FileSystem_GetAsynchronousBytesRead(&tempfile,&bytes_in_rdbuf)!=SUCCESS){Log_Add("[COMMAND VERIFY] File Read Failure\n"); exit(EXIT_CODE_FAIL);}
			}
		}
		if(FileSystem_CloseFile(&tempfile)!=SUCCESS){
			Log_Add("[COMMAND VERIFY] File Close Failure\n");
			exit(EXIT_CODE_FAIL);
		}
		Log_Add("[COMMAND VERIFY] Finish verify\n");
		fflush(stdout);
		munmap((void*)rdbuffer , 1048576);
		return Message_GenerateResponse(message, response, true, "File Verification Succeeded",strlen("File Verification Succeeded"));
	} else {
		munmap((void*)rdbuffer , 1048576);
		return Message_GenerateResponse(message, response, false, "File Open Failure",strlen("File Open Failure"));
	}
	munmap((void*)rdbuffer , 1048576);
	return Message_GenerateResponse(message, response, false, "", 0);
}

// Non-instant, Non-schedulable Commands (Operations)
StatusCode Command_SYN(SafeMessage* message, SafeMessage* response){
	StatusCode sc;
	char command[8192];
	sprintf(command, "ntpdate -u %s | tr -d '\\n'", globals.ntpServer);
	char result[8192];
	sc= ShellGetString(command, result, 8192);
	if (sc!=SUCCESS){
		Log_Add("[COMMAND NTP SYNCH] Error: NTP synchronization process failed\n");
		return GenerateErrorResponse(message,response, ERROR_CODE_UNKNOWN_ERROR,"");
	} else {
		return Message_GenerateResponse(message, response, true, result, strlen(result));
	}
}

StatusCode Command_CPY(SafeMessage* message, SafeMessage* response){
	char conflictString[8192];
	OperationParameters opp,conflictOpp;
	ScheduleEntry temp;
	ScheduleEntry conflict;
	int diskindex;
	TimeStamp now;
	int fileIndex;
	bzero((void*)&opp,sizeof(OperationParameters));
	StatusCode sc;
	char * token;

	if (globals.upStatus==DOWN) return GenerateErrorResponse(message, response, ERROR_CODE_OPERATION_NOT_PERMITTED, "");
	token = strtok(message->data.messageData," ");  if (!token) BAD_FORMAT();
	strncpy(opp.opTag,token,16);	opp.opTag[16]='\0';

	token = strtok(NULL," "); 						if (!token) BAD_FORMAT();
	opp.opFileCpyDmpStartPos = strtoul(token,NULL,10);

	token = strtok(NULL," "); 						if (!token) BAD_FORMAT();
	opp.opFileCpyDmpLength = strtoul(token,NULL,10);

	token = strtok(NULL," ");  if (!token) BAD_FORMAT();
	strncpy(opp.opDeviceName,token,64);		opp.opDeviceName[63]='\0';

	token = strtok(NULL," ");  if (!token) BAD_FORMAT();
	strncpy(opp.opFileName,token,128);		opp.opFileName[127]='\0';


	fileIndex = _FileSystem_GetFileIndex(globals.fileSystem,opp.opTag);
	if (fileIndex != -1){
		if ((opp.opFileCpyDmpStartPos + opp.opFileCpyDmpLength-1) > globals.fileSystem->fileSystemHeaderData->fileInfo[fileIndex].size){
			return GenerateErrorResponse(message,response, ERROR_CODE_INVALID_RANGE,"");
		}
		if (strspn(opp.opFileName,"0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_.")!=strlen(opp.opFileName)){
			return GenerateErrorResponse(message,response, ERROR_CODE_INVALID_FILENAME,opp.opFileName);
		}
		if (strstr(opp.opFileName,"..")!=NULL){
			return GenerateErrorResponse(message,response, ERROR_CODE_INVALID_FILENAME,opp.opFileName);
		}
		now = getTimestamp();
		addTime(now.MJD, now.MPM, 3000, &opp.opStartMJD, &opp.opStartMPM);
		opp.opStopMJD = 999999l;
		opp.opStopMPM = 999999999l;
		diskindex = DiskFindIndexByName(opp.opDeviceName);
		if (diskindex == -1){
			return GenerateErrorResponse(message, response, ERROR_CODE_INVALID_STORAGE_ID, opp.opDeviceName);
		} else {
			if (DiskGetType(diskindex)!=DRIVETYPE_EXTERNAL_PARTITION){
				return GenerateErrorResponse(message, response, ERROR_CODE_INVALID_STORAGE_ID, opp.opDeviceName);
			} else {
				if (DiskGetFreeSpace(diskindex)<(opp.opFileCpyDmpLength*11/10)){
					return GenerateErrorResponse(message, response, ERROR_CODE_INSUFFICIENT_SPACE,"");
				} else {
					opp.opFileDmpBlockSize=opp.opFileCpyDmpLength;
					if (FileSystem_GetFileFormat(globals.fileSystem,fileIndex,opp.opFormat)!=SUCCESS)
						strcpy(opp.opFormat,"<UNKNOWN-FORMAT>");
					opp.opReference=strtoul(message->reference,NULL,10);
					opp.opType=OP_TYPE_CPY;
					temp.reference	= opp.opReference;
					temp.startMJD   = opp.opStartMJD;
					temp.startMPM   = opp.opStartMPM;
					temp.stopMJD    = opp.opStopMJD;
					temp.stopMPM    = opp.opStopMPM;
					temp.data       = (char*)&opp;
					temp.datalength = sizeof(OperationParameters);
					if (Schedule_AddEntry(&temp,&conflict)!=SUCCESS){
						if (Schedule_EntriesEqual(&temp,&conflict)){
							return GenerateErrorResponse(message, response, ERROR_CODE_OPERATION_NOT_PERMITTED, "");
						} else {
							size_t bytes;
							sc = Schedule_GetEntryData(conflict.reference,(char*)&conflictOpp,&bytes);
							if (sc!=SUCCESS || bytes!= sizeof(OperationParameters)){
								Log_Add("[COMMAND COPY] Error retrieving conflict data\nExitting\n");
								exit(EXIT_CODE_FAIL);
							}
							sprintf(conflictString,"%11.11s %09lu %06lu %09lu %06lu %09lu %32.32s", Operation_TypeName(conflictOpp.opType), conflict.reference, conflict.startMJD, conflict.startMPM, conflict.stopMJD, conflict.stopMPM, conflictOpp.opFormat);
							return GenerateErrorResponse(message, response, ERROR_CODE_TIME_CONFLICT, conflictString);
						}
					} else {
						printf("COMMAND COPY: %s %lu %lu %s %s\n", opp.opTag, opp.opFileCpyDmpStartPos, opp.opFileCpyDmpLength, opp.opDeviceName, opp.opFileName);
						return Message_GenerateResponse(message, response, true, "", 0);
					}
				}
			}
		}
	} else {
		return GenerateErrorResponse(message,response, ERROR_CODE_FILE_NOT_FOUND,opp.opTag);
	}
}

StatusCode Command_JUP(SafeMessage* message, SafeMessage* response){
	char conflictString[8192];
	OperationParameters opp,conflictOpp;
	ScheduleEntry temp;
	ScheduleEntry conflict;
	int diskindex;
	TimeStamp now;
	int fileIndex;
	bzero((void*)&opp,sizeof(OperationParameters));
	StatusCode sc;
	char * token;

	if (globals.upStatus==DOWN) return GenerateErrorResponse(message, response, ERROR_CODE_OPERATION_NOT_PERMITTED, "");
	token = strtok(message->data.messageData," ");  if (!token) BAD_FORMAT();
	strncpy(opp.opTag,token,16);	opp.opTag[16]='\0';

	token = strtok(NULL," "); 						if (!token) BAD_FORMAT();
	opp.TransformLength = strtoul(token,NULL,10);

	token = strtok(NULL," "); 						if (!token) BAD_FORMAT();
	opp.IntegrationCount = strtoul(token,NULL,10);

	token = strtok(NULL," ");  if (!token) BAD_FORMAT();
	strncpy(opp.opDeviceName,token,64);		opp.opDeviceName[63]='\0';

	token = strtok(NULL," ");  if (!token) BAD_FORMAT();
	strncpy(opp.opFileName,token,128);		opp.opFileName[127]='\0';

	fileIndex = _FileSystem_GetFileIndex(globals.fileSystem,opp.opTag);
	if (fileIndex != -1){
		opp.opFileCpyDmpStartPos = 0;
		opp.opFileCpyDmpLength = globals.fileSystem->fileSystemHeaderData->fileInfo[fileIndex].size * 16l / opp.IntegrationCount;
		if (strspn(opp.opFileName,"0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_.")!=strlen(opp.opFileName)){
			return GenerateErrorResponse(message,response, ERROR_CODE_INVALID_FILENAME,opp.opFileName);
		}
		if (strstr(opp.opFileName,"..")!=NULL){
			return GenerateErrorResponse(message,response, ERROR_CODE_INVALID_FILENAME,opp.opFileName);
		}
		now = getTimestamp();
		addTime(now.MJD, now.MPM, 3000, &opp.opStartMJD, &opp.opStartMPM);
		opp.opStopMJD = 999999l;
		opp.opStopMPM = 999999999l;
		diskindex = DiskFindIndexByName(opp.opDeviceName);
		if (diskindex == -1){
			return GenerateErrorResponse(message, response, ERROR_CODE_INVALID_STORAGE_ID, opp.opDeviceName);
		} else {
			if (DiskGetType(diskindex)!=DRIVETYPE_EXTERNAL_PARTITION){
				return GenerateErrorResponse(message, response, ERROR_CODE_INVALID_STORAGE_ID, opp.opDeviceName);
			} else {
				if (DiskGetFreeSpace(diskindex)<(opp.opFileCpyDmpLength*11/10)){
					return GenerateErrorResponse(message, response, ERROR_CODE_INSUFFICIENT_SPACE,"");
				} else {
					opp.opFileDmpBlockSize=opp.opFileCpyDmpLength;
					if (FileSystem_GetFileFormat(globals.fileSystem,fileIndex,opp.opFormat)!=SUCCESS)
						strcpy(opp.opFormat,"<UNKNOWN-FORMAT>");
					opp.opReference=strtoul(message->reference,NULL,10);
					opp.opType=OP_TYPE_JUP;
					temp.reference	= opp.opReference;
					temp.startMJD   = opp.opStartMJD;
					temp.startMPM   = opp.opStartMPM;
					temp.stopMJD    = opp.opStopMJD;
					temp.stopMPM    = opp.opStopMPM;
					temp.data       = (char*)&opp;
					temp.datalength = sizeof(OperationParameters);
					if (Schedule_AddEntry(&temp,&conflict)!=SUCCESS){
						if (Schedule_EntriesEqual(&temp,&conflict)){
							return GenerateErrorResponse(message, response, ERROR_CODE_OPERATION_NOT_PERMITTED, "");
						} else {
							size_t bytes;
							sc = Schedule_GetEntryData(conflict.reference,(char*)&conflictOpp,&bytes);
							if (sc!=SUCCESS || bytes!= sizeof(OperationParameters)){
								Log_Add("[COMMAND JUP] Error retrieving conflict data\nExitting\n");
								return GenerateErrorResponse(message, response, ERROR_CODE_TIME_CONFLICT, "Conflict data not available");

							}
							sprintf(conflictString,"%11.11s %09lu %06lu %09lu %06lu %09lu %32.32s", Operation_TypeName(conflictOpp.opType), conflict.reference, conflict.startMJD, conflict.startMPM, conflict.stopMJD, conflict.stopMPM, conflictOpp.opFormat);
							return GenerateErrorResponse(message, response, ERROR_CODE_TIME_CONFLICT, conflictString);
						}
					} else {
						printf("COMMAND JUP: %s %u %u %s %s\n", opp.opTag, opp.TransformLength, opp.IntegrationCount, opp.opDeviceName, opp.opFileName);
						return Message_GenerateResponse(message, response, true, "", 0);
					}
				}
			}
		}
	} else {
		return GenerateErrorResponse(message,response, ERROR_CODE_FILE_NOT_FOUND,opp.opTag);
	}
}
StatusCode Command_SPC(SafeMessage* message, SafeMessage* response){
	char conflictString[8192];
	OperationParameters opp,conflictOpp;
	ScheduleEntry temp;
	ScheduleEntry conflict;
	bzero((void*)&opp,sizeof(OperationParameters));
	StatusCode sc;
	char * token;
	size_t length;

	if (globals.upStatus==DOWN) return GenerateErrorResponse(message, response, ERROR_CODE_OPERATION_NOT_PERMITTED, "");

	// get start day
	token = strtok(message->data.messageData," ");  if (!token) BAD_FORMAT();
	opp.opStartMJD = strtoul(token,NULL,10);

	// get start MPM
	token = strtok(NULL," "); 	if (!token) BAD_FORMAT();
	opp.opStartMPM = strtoul(token,NULL,10);

	// get length
	token = strtok(NULL," "); 	if (!token) BAD_FORMAT();
	length = strtoul(token,NULL,10);
	// compute stop time
	opp.opStopMJD = opp.opStartMJD;
	opp.opStopMPM = opp.opStartMPM + length;
	while (opp.opStopMPM > MILLISECONDS_PER_DAY){
		opp.opStopMPM -= MILLISECONDS_PER_DAY;
		opp.opStopMJD++;
	}

	// get transform length(num bins)
	token = strtok(NULL," "); 	if (!token) BAD_FORMAT();
	opp.TransformLength = strtoul(token,NULL,10);
	if (!__isPowerOf2(opp.TransformLength) || (opp.TransformLength<2) || (opp.TransformLength>4096) ){
		return GenerateErrorResponse(message, response, ERROR_CODE_ILLEGAL_FFT_SIZE, "");
	}

	// get integration count
	token = strtok(NULL," "); 	if (!token) BAD_FORMAT();
	opp.IntegrationCount = strtoul(token,NULL,10);
	if ((opp.IntegrationCount < 2) ||  (0 != ((opp.IntegrationCount * opp.TransformLength) % DRX_SAMPLES_PER_FRAME))){
		return GenerateErrorResponse(message, response, ERROR_CODE_ILLEGAL_INTEGRATION_COUNT, "");
	}

	// optional params min fill and highWater will be selected automatically
	opp.minFill   = opp.IntegrationCount / 2;
	opp.highWater = 8;
	// get minimum fill (below which partial frames will be dropped
	token = strtok(NULL," ");
	if (token){
		opp.minFill = strtoul(token,NULL,10);
		// get high water mark... in blocks
		token = strtok(NULL," "); 	if (token){
			opp.highWater = strtoul(token,NULL,10);
		}
	}
	if (opp.minFill < 1)
		opp.minFill = 1;
	if (opp.minFill > opp.IntegrationCount * opp.TransformLength)
		opp.minFill = opp.IntegrationCount * opp.TransformLength;
	if (opp.highWater < 2)
		opp.highWater = 2;
	if (opp.highWater < NUM_BLOCKS-2)
		opp.highWater = NUM_BLOCKS-2;


	opp.opReference=strtoul(message->reference,NULL,10);
	opp.opType		= OP_TYPE_SPC;
	temp.reference	= opp.opReference;
	temp.startMJD   = opp.opStartMJD;
	temp.startMPM   = opp.opStartMPM;
	temp.stopMJD    = opp.opStopMJD;
	temp.stopMPM    = opp.opStopMPM;
	temp.data       = (char*)&opp;
	temp.datalength = sizeof(OperationParameters);
	sprintf(opp.opTag,"%06lu_%09lu",opp.opStartMJD,opp.opReference);

	if (Schedule_AddEntry(&temp,&conflict)!=SUCCESS){
		if (Schedule_EntriesEqual(&temp,&conflict)){
			return GenerateErrorResponse(message, response, ERROR_CODE_OPERATION_NOT_PERMITTED, "");
		} else {
			size_t bytes;
			sc = Schedule_GetEntryData(conflict.reference,(char*)&conflictOpp,&bytes);
			if (sc!=SUCCESS || bytes!= sizeof(OperationParameters)){
				Log_Add("[COMMAND SPC] Error retrieving conflict data\nExitting\n");
				return GenerateErrorResponse(message, response, ERROR_CODE_TIME_CONFLICT, "Conflict data not available");
			}
			sprintf(conflictString,"%11.11s %09lu %06lu %09lu %06lu %09lu %32.32s", Operation_TypeName(conflictOpp.opType), conflict.reference, conflict.startMJD, conflict.startMPM, conflict.stopMJD, conflict.stopMPM, conflictOpp.opFormat);
			return GenerateErrorResponse(message, response, ERROR_CODE_TIME_CONFLICT, conflictString);
		}
	} else {
		return Message_GenerateResponse(message, response, true, opp.opTag, strlen(opp.opTag));
	}
}
StatusCode Command_XCP(SafeMessage* message, SafeMessage* response){
	char conflictString[8192];
	OperationParameters opp,conflictOpp;
	ScheduleEntry temp;
	ScheduleEntry conflict;
	bzero((void*)&opp,sizeof(OperationParameters));
	StatusCode sc;
	char * token;
	size_t length;

	if (globals.upStatus==DOWN) return GenerateErrorResponse(message, response, ERROR_CODE_OPERATION_NOT_PERMITTED, "");

	// get start day
	token = strtok(message->data.messageData," ");  if (!token) BAD_FORMAT();
	opp.opStartMJD = strtoul(token,NULL,10);

	// get start MPM
	token = strtok(NULL," "); 	if (!token) BAD_FORMAT();
	opp.opStartMPM = strtoul(token,NULL,10);

	// get length
	token = strtok(NULL," "); 	if (!token) BAD_FORMAT();
	length = strtoul(token,NULL,10);
	// compute stop time
	opp.opStopMJD = opp.opStartMJD;
	opp.opStopMPM = opp.opStartMPM + length;
	while (opp.opStopMPM > MILLISECONDS_PER_DAY){
		opp.opStopMPM -= MILLISECONDS_PER_DAY;
		opp.opStopMJD++;
	}

	// get #samples per output frame (stored in the existing transform length field)
	token = strtok(NULL," "); 	if (!token) BAD_FORMAT();
	opp.TransformLength = strtoul(token,NULL,10);
	if (!__isPowerOf2(opp.TransformLength) || (opp.TransformLength<2) || (opp.TransformLength>4096) ){
		return GenerateErrorResponse(message, response, ERROR_CODE_ILLEGAL_FFT_SIZE, "");
	}

	// get integration count
	token = strtok(NULL," "); 	if (!token) BAD_FORMAT();
	opp.IntegrationCount = strtoul(token,NULL,10);
	if ((opp.IntegrationCount < 2) ||  (0 != ((opp.IntegrationCount * opp.TransformLength) % DRX_SAMPLES_PER_FRAME))){
		return GenerateErrorResponse(message, response, ERROR_CODE_ILLEGAL_INTEGRATION_COUNT, "");
	}

	// optional params min fill and highWater will be selected automatically
	opp.minFill   = opp.IntegrationCount / 2;
	opp.highWater = 8;
	// get minimum fill (below which partial frames will be dropped
	token = strtok(NULL," ");
	if (token){
		opp.minFill = strtoul(token,NULL,10);
		// get high water mark... in blocks
		token = strtok(NULL," "); 	if (token){
			opp.highWater = strtoul(token,NULL,10);
		}
	}
	if (opp.minFill < 1)
		opp.minFill = 1;
	if (opp.minFill > opp.IntegrationCount * opp.TransformLength)
		opp.minFill = opp.IntegrationCount * opp.TransformLength;
	if (opp.highWater < 2)
		opp.highWater = 2;
	if (opp.highWater < NUM_BLOCKS-2)
		opp.highWater = NUM_BLOCKS-2;


	opp.opReference=strtoul(message->reference,NULL,10);
	opp.opType		= OP_TYPE_XCP;
	temp.reference	= opp.opReference;
	temp.startMJD   = opp.opStartMJD;
	temp.startMPM   = opp.opStartMPM;
	temp.stopMJD    = opp.opStopMJD;
	temp.stopMPM    = opp.opStopMPM;
	temp.data       = (char*)&opp;
	temp.datalength = sizeof(OperationParameters);
	sprintf(opp.opTag,"%06lu_%09lu",opp.opStartMJD,opp.opReference);

	if (Schedule_AddEntry(&temp,&conflict)!=SUCCESS){
		if (Schedule_EntriesEqual(&temp,&conflict)){
			return GenerateErrorResponse(message, response, ERROR_CODE_OPERATION_NOT_PERMITTED, "");
		} else {
			size_t bytes;
			sc = Schedule_GetEntryData(conflict.reference,(char*)&conflictOpp,&bytes);
			if (sc!=SUCCESS || bytes!= sizeof(OperationParameters)){
				Log_Add("[COMMAND SPC] Error retrieving conflict data\nExitting\n");
				return GenerateErrorResponse(message, response, ERROR_CODE_TIME_CONFLICT, "Conflict data not available");
			}
			sprintf(conflictString,"%11.11s %09lu %06lu %09lu %06lu %09lu %32.32s", Operation_TypeName(conflictOpp.opType), conflict.reference, conflict.startMJD, conflict.startMPM, conflict.stopMJD, conflict.stopMPM, conflictOpp.opFormat);
			return GenerateErrorResponse(message, response, ERROR_CODE_TIME_CONFLICT, conflictString);
		}
	} else {
		return Message_GenerateResponse(message, response, true, opp.opTag, strlen(opp.opTag));
	}
}

StatusCode Command_DMP(SafeMessage* message, SafeMessage* response){
	char conflictString[8192];
	OperationParameters opp,conflictOpp;
	ScheduleEntry temp;
	ScheduleEntry conflict;
	int diskindex;
	TimeStamp now;
	int fileIndex;
	bzero((void*)&opp,sizeof(OperationParameters));
	StatusCode sc;

	/**
	 * <Tag>'<Start Byte>'<Length>'<Block Size>'<Device ID>'<Filename>
	 */
	if (globals.upStatus==DOWN) return GenerateErrorResponse(message, response, ERROR_CODE_OPERATION_NOT_PERMITTED, "");
	char * token;
	token = strtok(message->data.messageData," ");  if (!token) BAD_FORMAT();
	strncpy(opp.opTag,token,16);	opp.opTag[16]='\0';

	token = strtok(NULL," "); 						if (!token) BAD_FORMAT();
	opp.opFileCpyDmpStartPos = strtoul(token,NULL,10);

	token = strtok(NULL," "); 						if (!token) BAD_FORMAT();
	opp.opFileCpyDmpLength = strtoul(token,NULL,10);

	token = strtok(NULL," "); 						if (!token) BAD_FORMAT();
	opp.opFileDmpBlockSize = strtoul(token,NULL,10);

	token = strtok(NULL," ");  if (!token) BAD_FORMAT();
	strncpy(opp.opDeviceName,token,64);		opp.opDeviceName[63]='\0';

	token = strtok(NULL," ");  if (!token) BAD_FORMAT();
	strncpy(opp.opFileName,token,128);		opp.opFileName[127]='\0';


	fileIndex = _FileSystem_GetFileIndex(globals.fileSystem,opp.opTag);
	if (fileIndex != -1){
		if ((opp.opFileCpyDmpStartPos + opp.opFileCpyDmpLength-1) > globals.fileSystem->fileSystemHeaderData->fileInfo[fileIndex].size){
			return GenerateErrorResponse(message,response, ERROR_CODE_INVALID_RANGE,"");
		}
		if (strspn(opp.opFileName,"0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_.")!=strlen(opp.opFileName)){
			return GenerateErrorResponse(message,response, ERROR_CODE_INVALID_FILENAME,opp.opFileName);
		}
		if (strstr(opp.opFileName,"..")!=NULL){
			return GenerateErrorResponse(message,response, ERROR_CODE_INVALID_FILENAME,opp.opFileName);
		}
		now = getTimestamp();
		addTime(now.MJD, now.MPM, 3000, &opp.opStartMJD, &opp.opStartMPM);
		opp.opStopMJD = 999999l;
		opp.opStopMPM = 999999999l;
		diskindex = DiskFindIndexByName(opp.opDeviceName);
		if (diskindex == -1){
			return GenerateErrorResponse(message, response, ERROR_CODE_INVALID_STORAGE_ID, opp.opDeviceName);
		} else {
			if (DiskGetType(diskindex)!=DRIVETYPE_EXTERNAL_PARTITION){
				return GenerateErrorResponse(message, response, ERROR_CODE_INVALID_STORAGE_ID, opp.opDeviceName);
			} else {
				if (DiskGetFreeSpace(diskindex)<(opp.opFileCpyDmpLength*11/10)){
					return GenerateErrorResponse(message, response, ERROR_CODE_INSUFFICIENT_SPACE,"");
				} else {
					if (FileSystem_GetFileFormat(globals.fileSystem,fileIndex,opp.opFormat)!=SUCCESS)
						strcpy(opp.opFormat,"<UNKNOWN-FORMAT>");
					opp.opReference=strtoul(message->reference,NULL,10);
					opp.opType=OP_TYPE_DMP;
					temp.reference	= opp.opReference;
					temp.startMJD   = opp.opStartMJD;
					temp.startMPM   = opp.opStartMPM;
					temp.stopMJD    = opp.opStopMJD;
					temp.stopMPM    = opp.opStopMPM;
					temp.data       = (char*)&opp;
					temp.datalength = sizeof(OperationParameters);
					if (Schedule_AddEntry(&temp,&conflict)!=SUCCESS){
						if (Schedule_EntriesEqual(&temp,&conflict)){
							return GenerateErrorResponse(message, response, ERROR_CODE_OPERATION_NOT_PERMITTED, "");
						} else {
							size_t bytes;
							sc = Schedule_GetEntryData(conflict.reference,(char*)&conflictOpp,&bytes);
							if (sc!=SUCCESS || bytes!= sizeof(OperationParameters)){
								Log_Add("[COMMAND DUMP] Error retrieving conflict data\nExitting\n");
								exit(EXIT_CODE_FAIL);
							}
							sprintf(conflictString,"%11.11s %09lu %06lu %09lu %06lu %09lu %32.32s", Operation_TypeName(conflictOpp.opType), conflict.reference, conflict.startMJD, conflict.startMPM, conflict.stopMJD, conflict.stopMPM, conflictOpp.opFormat);
							return GenerateErrorResponse(message, response, ERROR_CODE_TIME_CONFLICT, conflictString);
						}
					} else {
						return Message_GenerateResponse(message, response, true, "", 0);
					}
				}
			}
		}
	} else {
		return GenerateErrorResponse(message,response, ERROR_CODE_FILE_NOT_FOUND,opp.opTag);
	}
}

StatusCode Command_FMT(SafeMessage* message, SafeMessage* response){
	char conflictString[8192];
	OperationParameters opp,conflictOpp;
	ScheduleEntry temp;
	ScheduleEntry conflict;
	int diskindex;
	TimeStamp now;
	bzero((void*)&opp,sizeof(OperationParameters));
	StatusCode sc;
	int diskType;

	/**
	 * optional...
	 * <Device ID>
	 */
	if (globals.upStatus==DOWN && strlen(message->data.messageData)==0) return GenerateErrorResponse(message, response, ERROR_CODE_OPERATION_NOT_PERMITTED, "");
	char * token;
	token = strtok(message->data.messageData," ");
	if (token){
		strncpy(opp.opDeviceName,token,64);
		opp.opDeviceName[63]='\0';
	} else {
		if (strlen(message->data.messageData)!=0){
			BAD_FORMAT();
		} else {
			strncpy(opp.opDeviceName,globals.fileSystem->diskInfo->deviceName,64);
			opp.opDeviceName[63]='\0';
		}
	}
	now = getTimestamp();
	addTime(now.MJD, now.MPM, 3000, &opp.opStartMJD, &opp.opStartMPM);
	opp.opStopMJD = 999999l;
	opp.opStopMPM = 999999999l;
	diskindex = DiskFindIndexByName(opp.opDeviceName);
	if (diskindex == -1){
		return GenerateErrorResponse(message, response, ERROR_CODE_INVALID_STORAGE_ID, opp.opDeviceName);
	} else {
		diskType = DiskGetType(diskindex);
		if (diskType!=DRIVETYPE_EXTERNAL_PARTITION && diskType!=DRIVETYPE_INTERNAL_MULTIDISK_DEVICE){
			return GenerateErrorResponse(message, response, ERROR_CODE_INVALID_STORAGE_ID, opp.opDeviceName);
		} else {
			opp.opReference=strtoul(message->reference,NULL,10);
			opp.opType=OP_TYPE_FMT;
			temp.reference	= opp.opReference;
			temp.startMJD   = opp.opStartMJD;
			temp.startMPM   = opp.opStartMPM;
			temp.stopMJD    = opp.opStopMJD;
			temp.stopMPM    = opp.opStopMPM;
			temp.data       = (char*)&opp;
			temp.datalength = sizeof(OperationParameters);
			if (Schedule_AddEntry(&temp,&conflict)!=SUCCESS){
				if (Schedule_EntriesEqual(&temp,&conflict)){
					return GenerateErrorResponse(message, response, ERROR_CODE_OPERATION_NOT_PERMITTED, "");
				} else {
					size_t bytes;
					sc = Schedule_GetEntryData(conflict.reference,(char*)&conflictOpp,&bytes);
					if (sc!=SUCCESS || bytes!= sizeof(OperationParameters)){
						Log_Add("[COMMAND FORMAT] Error retrieving conflict data\nExitting\n");
						exit(EXIT_CODE_FAIL);
					}
					sprintf(conflictString,"%11.11s %09lu %06lu %09lu %06lu %09lu %32.32s", Operation_TypeName(conflictOpp.opType), conflict.reference, conflict.startMJD, conflict.startMPM, conflict.stopMJD, conflict.stopMPM, conflictOpp.opFormat);
					return GenerateErrorResponse(message, response, ERROR_CODE_TIME_CONFLICT, conflictString);
				}
			} else {
				return Message_GenerateResponse(message, response, true, "", 0);
			}
		}
	}
}

StatusCode Command_DWN(SafeMessage* message, SafeMessage* response){
	int scheduleCount;
	if (globals.upStatus!=UP)
		return GenerateErrorResponse(message, response, ERROR_CODE_ALREADY_DOWN, "");
	scheduleCount=_Schedule_Count();
	if (scheduleCount!=0)	return GenerateErrorResponse(message, response, ERROR_CODE_OPERATION_NOT_PERMITTED, "");
	if (globals.fileSystem && globals.fileSystem->filesystemIsOpen){
		FileSystem_Close(globals.fileSystem);
	}
	globals.upStatus=DOWN;
	return Message_GenerateResponse(message, response, true, "System Now Offline", strlen("System Now Offline"));
}
StatusCode Command_UP (SafeMessage* message, SafeMessage* response){
	if (globals.upStatus!=DOWN)
		return GenerateErrorResponse(message, response, ERROR_CODE_ALREADY_UP, "");
	if (!globals.fileSystem->filesystemIsOpen){
		if (FileSystem_Open(globals.fileSystem,globals.disk)!=SUCCESS)
			return GenerateErrorResponse(message, response, ERROR_CODE_COMPONENT_NOT_AVAILABLE, "Internal Storage");
	}
	globals.upStatus=UP;
	return Message_GenerateResponse(message, response, true, "System Now Online", strlen("System Now Online"));
}







//ANSI color console text ===================================================================================================
#define COLOR_NORMAL "\033[1;37m"
#define COLOR_RED "\033[0;31m"
#define COLOR_GREEN "\033[0;32m"
#define COLOR_GREY "\033[1;30m"
#define HIGHLIGHT_GOOD(a)(COLOR_GREEN a COLOR_NORMAL)
#define HIGHLIGHT_BAD(a)(COLOR_RED a COLOR_NORMAL)

void dumppacket(char* packet, size_t size, size_t hlStart, size_t hlStop){
	printf(COLOR_NORMAL);
	printf(" __________________________________________________________________________________\n");
	printf(">MPM   = %lu\n",*((size_t*)packet));
	printf(HIGHLIGHT_GOOD(">frame = %lu\n"),((size_t*)packet)[1]);
	printf("              ____________________________________________________________________ \n");
	printf("              |  00  01  02  03  04  05  06  07  08  09  0A  0B  0C  0D  0E  0F   |\n");
	printf(" _____________|___________________________________________________________________|\n");
	size_t row=0;
	size_t col=0;
	for(row=0;row<((size/16)+1);row++){
		printf(COLOR_NORMAL);
	    printf("|%12lX |  ",row);
		for (col=row*16; ((col<((row*16)+16)) ) ;col++){
			if (col<hlStart) printf(COLOR_GREEN);
			if (col>=hlStart) printf(COLOR_RED);
			if (col>=hlStop) printf(COLOR_GREY);
			if (col<size)
				printf("%.2x  ",((char)packet[col])&0xff);
			else
				printf("    ");

		}
		printf(COLOR_NORMAL);
		printf(" | ");
		for (col=row*16; ((col<((row*16)+16)) ) ;col++){
			if (col<size && isprint(packet[col]))
				printf("%c",packet[col]);
			else
				printf(".");
		}
		printf("\n");
	}
	printf(COLOR_NORMAL);
	printf("|_____________|___________________________________________________________________|\n");
}

size_t lastsid=0;
size_t sid=0;

boolean vfypacket(char* packet, size_t size, boolean isLast){
	unsigned int i;
	if (sid == 0){
		lastsid=(((size_t*)packet)[1]);
		sid=lastsid;
		Log_Add("[COMMAND VERIFY] Beginning verification.\n First packet serial id is %lu",lastsid);
	} else {
		sid=(((size_t*)packet)[1]);
		if (sid != lastsid+1){
			Log_Add("[COMMAND VERIFY] Verification error: Dropped packets between %lu and %lu,\n",lastsid,sid);
			return false;
		}
	}

	for (i = (3 * sizeof(size_t)); i<size; i++ ){
		if (packet[i]!=(char)(i&0xff)){
			Log_Add("[COMMAND VERIFY] Verification error: incorrect data (packet # %lu , byte # %u ) 0x%x != 0x%x\n",sid,i,(int)packet[i],(char)(i&0xff));
			return false;
		}
	}
	lastsid=sid;

	if (isLast){
		Log_Add("[COMMAND VERIFY] Verification complete.\n");
		sid=0;
		lastsid=0;
	}
	return true;
}

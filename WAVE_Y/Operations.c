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
 * Operations.c
 *
 *  Created on: Oct 31, 2009
 *      Author: chwolfe2
 */

#include <stdio.h>
#include <stdlib.h>
#include	<sys/stat.h>
#include	<unistd.h>
#include	<fcntl.h>
#include	<errno.h>
#include 	<string.h>
#include    <assert.h>
#include <sys/mman.h>
#include <linux/fs.h>
#include <sys/ioctl.h>
#include <ctype.h>
#include	<aio.h>
#include <time.h>
#include <sys/time.h>
#include <sys/statvfs.h>

#include "Operations.h"
#include "Globals.h"
#include "HostInterface.h"
#include "LiveCapture.h"
#include "Schedule.h" // for boolean leadEntryWaitToRunDone();
#include "ConsoleColors.h"
#include "Time.h"
#include "OpSpec.h"
#include "OpXcp.h"
#include "OpJup.h"
#include "DRX.h"

inline void receiveDiscard();
inline void receiveEnqueue();
inline void dequeueWrite();
inline void recordingPrefire();

char * TYPE_NAMES[]={
		"Idle       ",
		"Record     ",
		"Format     ",
		"Copy       ",
		"Dump       ",
		"Hot-Jupiter",
		"Spectrometr",
		"BeamCorrl8r"
};



#define debugCopyDump 0

#define CDS_INIT						0
#define CDS_CREATE_BUFFER				1
#define CDS_DESTROY_BUFFER_CONTINUE		2
#define CDS_DESTROY_BUFFER_ABORT		3
#define CDS_CHECK_WRITTEN_ENOUGH		4
#define CDS_OPEN_DESTINATION			5
#define CDS_CLOSE_DESTINATION_CONTINUE	6
#define CDS_CLOSE_DESTINATION_ABORT		7
#define CDS_CHECK_DONE_CURRENT			8
#define CDS_CHECK_BUFFER_EMPTY			9
#define CDS_CHECK_SOURCE_EOF			10
#define CDS_READ_START					11
#define CDS_READ_WAIT					12
#define CDS_READ_DONE					13
#define CDS_WRITE_START					14
#define CDS_WRITE_WAIT					15
#define CDS_WRITE_DONE					16
#define CDS_END_SUCCESS					17
#define CDS_END_FAIL					18

boolean	formatApplied 		= false;
boolean	dataReceived 		= false;
boolean	insertionAvailable 	= false;
boolean writeComplete 		= false;
boolean	removalAvailable 	= false;
boolean	writeInProgress 	= false;
boolean prefireStarted  	= false;
boolean preFireComplete 	= false;


size_t TagFileSizeLimit;
size_t TotalProcessedBytesReceived=0;
size_t notifiedOverflow=0;



char * Operation_TypeName(OpType type){
	return TYPE_NAMES[type];
}

// fill out the Operation Data struct from saved (in the schedule) params.
StatusCode Operation_Init(OperationParameters* params){
	char command[4096];
	char * buffer;
	FileMetaData* fmd;
	globals.opData.initialParameters		= params;
	globals.opData.errorCount				= 0;
	globals.opData.warningCount				= 0;
	globals.opData.opFileCpyDmpLength		= params->opFileCpyDmpLength;
	globals.opData.opFileDmpBlockSize		= params->opFileDmpBlockSize;
	globals.opData.opFileCpyDmpStartPos 	= params->opFileCpyDmpStartPos;
	globals.opData.opFileCpyDmpCurrentPos	= 0;
	globals.opData.opFileDmpCurrentIndex 	= 0;
	globals.opData.opStartMJD 				= params->opStartMJD;
	globals.opData.opStartMPM 				= params->opStartMPM;
	globals.opData.opStopMJD 				= params->opStopMJD;
	globals.opData.opStopMPM 				= params->opStopMPM;
	globals.opData.opReference 				= params->opReference;
	globals.opData.extFileHandle 			= -1;
	globals.opData.opStatus 				= OP_STATUS_NULL;
	globals.opData.opType 					= params->opType;
	globals.opData.opCleanupComplete 		= false;
	globals.opData.TransformLength			= params->TransformLength;
	globals.opData.IntegrationCount			= params->IntegrationCount;
	globals.opData.minFill					= params->minFill;
	globals.opData.highWater				= params->highWater;

	strncpy(globals.opData.opDeviceName,params->opDeviceName,MAX_DEVICE_NAME_LENGTH+1);
	strncpy(globals.opData.opFormat,params->opFormat,DATA_FORMAT_MAX_NAME_LENGTH+1);
	strncpy(globals.opData.opFileName,params->opFileName,EXT_FILE_MAX_NAME_LENGTH+1);
	strncpy(globals.opData.opTag,params->opTag,16+1);



	MyFile*      file   = NULL;
	FileSystem*  fs     = NULL;
	int          findex = -1;
	FileInfo*    fi     = NULL;
	size_t       chunk  = 0;
	size_t       start  = 0;
	size_t       stop   = 0;

	switch(params->opType){
		case OP_TYPE_IDLE		:
			return SUCCESS;
		case OP_TYPE_REC		:
			formatApplied 		= false;
			dataReceived 		= false;
			insertionAvailable 	= false;
			writeComplete 		= false;
			removalAvailable 	= false;
			writeInProgress 	= false;
			globals.opData.rxSocketOpen = false;
			globals.opData.tagfile->index=-1;
			if (DataFormats_Get(params->opFormat,globals.opData.dataFormat)==SUCCESS){
/*				if (DataFormats_Compile(globals.opData.dataFormat, globals.opData.compiledFormat)==SUCCESS){*//*COMPILE_DATA_FORMAT:REMOVED*/
					if (FileSystem_OpenFile(globals.fileSystem,globals.opData.tagfile,params->opTag,WRITE)==SUCCESS){
						if(RingQueue_Reset(globals.rxQueue)==SUCCESS){
							file   = globals.opData.tagfile;
							fs     = file->fs;
							findex = file->index;
							fi     = &(fs->fileSystemHeaderData->fileInfo[findex]);
							chunk  = fs->fileSystemHeaderData->filesystemInfo.chunkSize;
							start  = fi->startPosition;
							stop   = fi->stopPosition;
							TagFileSizeLimit    = ((stop - start) - (chunk*2) - globals.opData.dataFormat->payloadSize);
							TotalProcessedBytesReceived = 0;
							notifiedOverflow=0;
							Log_Add("[DEbUG]  [OPERATION RECORD] TagFileSizeLimit: %lu",TagFileSizeLimit);
							globals.opData.opStatus =OP_STATUS_INITIALIZED;
							prefireStarted  	= false;
							preFireComplete 	= false;
							return SUCCESS;
						} else {// reset ring queue
							Log_Add("[OPERATION RECORD] Error: RingQueue could not be flushed.");
						}
					} else {// open tag file
						Log_Add("[OPERATION RECORD] Error: Tag file could not be opened.");
					}
/*				} else {// compile format
					Log_Add("[OPERATION RECORD] Error: Data format could not be compiled.");
				}*/
			} else {// get recording format
				Log_Add("[OPERATION RECORD] Error: Data format could not be identified.");
			}

			return FAILURE;
		case OP_TYPE_FMT		:
			if (strncmp(globals.opData.opDeviceName, "/dev/sda",8)==0){ // should never happen since command processor should prevent this, but just in case.
				Log_Add("[OPERATION FORMAT] -------------------- DANGER DANGER DANGER --------------------");
				Log_Add("[OPERATION FORMAT] DANGER: ATTEMPTED TO FORMAT SYSTEM DRIVE.");
				Log_Add("[OPERATION FORMAT] -------------------- DANGER DANGER DANGER --------------------");
				return FAILURE;
			}
			sprintf(command, "mke2fs -q -t ext2 %s 2>/dev/null ; echo $?", globals.opData.opDeviceName);
			buffer = malloc(16);
			if (!buffer) {
				printf ("[OPERATION FORMAT] Allocation Failure: Format command result buffer.");
				return FAILURE;
			}
			switch( LaunchProcess(command, 1, 16, buffer, ((ProcessHandle**)(&globals.opData.opExtraData)))){
				case SUCCESS:
					globals.opData.opStatus = OP_STATUS_INITIALIZED;
					return SUCCESS;
				default:
					globals.opData.opStatus = OP_STATUS_ERROR;
					return FAILURE;
				}
			break;
		case OP_TYPE_CPY		://fall through
		case OP_TYPE_DMP		:
			globals.opData.tagfile->index=-1;
			if (FileSystem_OpenFile(globals.fileSystem,globals.opData.tagfile,params->opTag,READ)!=SUCCESS){
				Log_Add("[OPERATION COPY / DUMP] Error: Tag file could not be opened.");
				return FAILURE;
			}
			if(Disk_GetDiskInfo(params->opDeviceName,globals.opData.extdisk)!=SUCCESS){
				Log_Add("[OPERATION COPY / DUMP] Error: External Storage device could not be identified.");
				FileSystem_CloseFile(globals.opData.tagfile);
				return FAILURE;
			}
			if(!globals.opData.extdisk->mounted){
				if(Disk_Mount(globals.opData.extdisk)!=SUCCESS){
					Log_Add("[OPERATION COPY / DUMP] Error: External Storage device could not be mounted.");
					FileSystem_CloseFile(globals.opData.tagfile);
					return FAILURE;
				}
			}
			/*
			sprintf(derivedFileName,"/LWA_EXT%s/%s",params->opDeviceName,params->opFileName);
			globals.opData.extFileHandle = open(derivedFileName, O_WRONLY | O_EXCL | O_CREAT | O_NONBLOCK, DEFFILEMODE);
			if (globals.opData.extFileHandle < 0){
				Log_Add("[OPERATION COPY / DUMP] Error: Destination file could not be opened.");
				return FAILURE;
			}*/
			globals.opData.opStatus = OP_STATUS_INITIALIZED;
			FileSystem_GetFileMetaData(globals.fileSystem,globals.opData.tagfile->index,&fmd);
			strncpy(globals.opData.opFormat,fmd->format,DATA_FORMAT_MAX_NAME_LENGTH+1);
			return SUCCESS;
		case OP_TYPE_JUP		:
			return OpJupInit();
		case OP_TYPE_SPC		:
			return OpSpecInit();
		case OP_TYPE_XCP		:
			return OpXcpInit();
		default :
			Log_Add("[OPERATION MANAGER] Unsupported operation type.");
			return FAILURE;
	}
}




int numDigits(size_t num){
	if (num<10) return 1;
	int digs=1;
	size_t i=10;
	while (num>i && digs<=20){
		i*=10;
		digs++;
	}
	return digs;
}

/*char * CpyDmpBuffer;
char * buf2;
size_t CpyDmpSizeOfBuffer;
size_t CpyDmpBytesInBuffer;
size_t CpyDmpBufferPos;
size_t maxWriteSize;
size_t bytesToWrite;
*/


static inline size_t min(size_t a, size_t b){
	return (((a)<(b))?(a):(b));
}




char * CpyDmpBuffer = NULL;
size_t CpyDmpSizeOfBuffer = 0;
size_t CpyDmpBufferSOD = 0;
size_t CpyDmpBufferEOD = 0;
int    CpyDmpNumDigits = 0;
struct aiocb waiocbinfo;

int CopyDumpState=CDS_INIT;

int CDSM_CreateBuffer(int StateIfSucceed, int StateIfFail){
	CpyDmpSizeOfBuffer = globals.opData.tagfile->fs->fileSystemHeaderData->filesystemInfo.chunkSize;
	CpyDmpBuffer = mmap(0, CpyDmpSizeOfBuffer, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS | MAP_POPULATE | MAP_LOCKED, -1, 0);
	return (CpyDmpBuffer!=MAP_FAILED) ? StateIfSucceed : StateIfFail;
}

int CDSM_DestroyBuffer(int nextState){
	munmap(CpyDmpBuffer,CpyDmpSizeOfBuffer);
	CpyDmpSizeOfBuffer=0;
	CpyDmpBufferSOD = 0;
	CpyDmpBufferEOD = 0;
	return nextState;
}

int CDSM_SourceEOF(int StateIfYes, int StateIfNo){
	if (globals.opData.tagfile->reof) Log_Add("[OPERATION COPY / DUMP] Read source past or to EOF.");
	return (globals.opData.tagfile->reof) ? StateIfYes : StateIfNo;
}

int CDSM_WrittenEnough(int StateIfYes, int StateIfNo){
	return (globals.opData.opFileCpyDmpCurrentPos >= globals.opData.opFileCpyDmpLength) ? StateIfYes : StateIfNo;
}
int CDSM_OpenDestination(int StateIfSucceed, int StateIfFail){
	char mountPoint[EXT_FILE_MAX_NAME_LENGTH+1];
	struct statvfs statinfoSystem;
	bzero(&waiocbinfo,sizeof(struct aiocb));
	globals.opData.opFileDmpCurrentIndex = globals.opData.opFileCpyDmpCurrentPos / globals.opData.opFileDmpBlockSize;
	//stat the filesystem and see if we can open a file of the appropriate size
	sprintf(mountPoint,"/LWA_EXT%s",globals.opData.opDeviceName);
	if (statvfs(mountPoint,&statinfoSystem)){
		Log_Add("[OPERATION COPY / DUMP] Can't Stat FileSystem.");
		return StateIfFail;
	}
	if (statinfoSystem.f_frsize * statinfoSystem.f_bfree < min( (globals.opData.opFileCpyDmpLength - globals.opData.opFileCpyDmpCurrentPos), globals.opData.opFileDmpBlockSize)){
		Log_Add("[OPERATION COPY / DUMP] Insufficient storage space for copy/dump operation.");
		return StateIfFail;
	}

	if (globals.opData.opType==OP_TYPE_CPY)
		sprintf(globals.opData.opCurrentFileName,"%s/%s",mountPoint,globals.opData.opFileName);
	else{
		sprintf(globals.opData.opCurrentFileName,"%s/%s.%.*lu",mountPoint,globals.opData.opFileName,CpyDmpNumDigits,globals.opData.opFileDmpCurrentIndex);
	}
	// now we have a file name to open...
	globals.opData.extFileHandle = open(globals.opData.opCurrentFileName, O_RDWR | O_CREAT | O_TRUNC, S_IFREG | S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH);
	if (globals.opData.extFileHandle < 0){
		Log_Add("[OPERATION COPY / DUMP] Error: Destination file '%s' could not be opened.",globals.opData.opFileName);
		return StateIfFail;
	} else {
		if (debugCopyDump) Log_Add("[OPERATION COPY / DUMP] Opened '%s'",globals.opData.opCurrentFileName);
		return StateIfSucceed;
	}
}

int CDSM_CloseDestination(int nextState){
	if (globals.opData.extFileHandle>=0){
		aio_cancel(globals.opData.extFileHandle,NULL);
		close(globals.opData.extFileHandle);
		globals.opData.extFileHandle=-1;
		if (debugCopyDump)  Log_Add("[OPERATION COPY / DUMP] Closed '%s'",globals.opData.opCurrentFileName);
	}
	return nextState;
}

int CDSM_DoneCurrentFile(int StateIfYes, int StateIfNo){
	size_t numWholeBlocks  = globals.opData.opFileCpyDmpLength / globals.opData.opFileDmpBlockSize;
	size_t indexOfNextByte = globals.opData.opFileCpyDmpCurrentPos / globals.opData.opFileDmpBlockSize;
	if (globals.opData.opFileDmpCurrentIndex < numWholeBlocks){
		return (indexOfNextByte > globals.opData.opFileDmpCurrentIndex) ? StateIfYes : StateIfNo;
	} else {
		return (globals.opData.opFileCpyDmpCurrentPos >= globals.opData.opFileCpyDmpLength) ? StateIfYes : StateIfNo;
	}
}

int CDSM_BufferIsEmpty(int StateIfYes, int StateIfNo){
	return (CpyDmpBufferSOD >= CpyDmpBufferEOD) ? StateIfYes : StateIfNo;
}

int CDSM_ReadSourceStart(int StateIfNoChange, int StateIfSucceed, int StateIfFail){
	//CpyDmpSizeOfBuffer;
	size_t file_offset = globals.opData.opFileCpyDmpCurrentPos + globals.opData.opFileCpyDmpStartPos;
	size_t file_chunk_offset = (file_offset / CpyDmpSizeOfBuffer) * CpyDmpSizeOfBuffer;
	if (FileSystem_Seek(globals.opData.tagfile,file_chunk_offset)!=file_chunk_offset)
		return StateIfFail;
	switch(FileSystem_AsynchronousReadStart(globals.opData.tagfile,CpyDmpBuffer,CpyDmpSizeOfBuffer)){
		case NOT_READY: return StateIfNoChange;
		case SUCCESS:   return StateIfSucceed;
		case FAILURE:   //fall through
		default:
			Log_Add("[OPERATION COPY / DUMP] Couldn't start read operation.");
			return StateIfFail;
	}
}
int CDSM_ReadSourceWait(int StateIfNoChange, int StateIfSucceed, int StateIfFail){
	switch (FileSystem_IsAsynchronousReadDone(globals.opData.tagfile)){
		case NOT_READY: return StateIfNoChange;
		case SUCCESS:   return StateIfSucceed;
		case FAILURE:   //fall through
		default:
			Log_Add("[OPERATION COPY / DUMP] Couldn't check read operation status.");
			return StateIfFail;
	}
}
int CDSM_ReadSourceFinish(int StateIfSucceed, int StateIfFail){
	size_t bytesRead;
	switch (FileSystem_GetAsynchronousBytesRead(globals.opData.tagfile,&bytesRead)){
		case SUCCESS:
			CpyDmpBufferSOD = ((globals.opData.opFileCpyDmpCurrentPos + globals.opData.opFileCpyDmpStartPos) % CpyDmpSizeOfBuffer);
			CpyDmpBufferEOD = bytesRead;
			if ((globals.opData.opFileCpyDmpCurrentPos / CpyDmpSizeOfBuffer) == (globals.opData.opFileCpyDmpLength / CpyDmpSizeOfBuffer))
				CpyDmpBufferEOD = min (CpyDmpBufferEOD , ((globals.opData.opFileCpyDmpLength + globals.opData.opFileCpyDmpStartPos) % CpyDmpSizeOfBuffer));
			if (debugCopyDump) printf("CDSM_ReadSourceFinish ---------------------------------\n");
			if (debugCopyDump) printf("CpyDmpBufferSOD = %lu\n",CpyDmpBufferSOD);
			if (debugCopyDump) printf("CpyDmpBufferEOD = %lu\n",CpyDmpBufferEOD);

			return StateIfSucceed;
		case NOT_READY: //fall through
			Log_Add("[OPERATION COPY / DUMP] State machine in undefined state.");
		case FAILURE:   //fall through
		default:
			Log_Add("[OPERATION COPY / DUMP] Couldn't finish read operation.");
			return StateIfFail;
	}
}

int CDSM_WriteDestStart(int StateIfNoChange, int StateIfSucceed, int StateIfFail){
	size_t maxWritable = globals.opData.opFileDmpBlockSize - (globals.opData.opFileCpyDmpCurrentPos % globals.opData.opFileDmpBlockSize);
	waiocbinfo.aio_buf    = &CpyDmpBuffer[CpyDmpBufferSOD];
	waiocbinfo.aio_fildes = globals.opData.extFileHandle;
	waiocbinfo.aio_nbytes = min((CpyDmpBufferEOD - CpyDmpBufferSOD), maxWritable);
	waiocbinfo.aio_offset = globals.opData.opFileCpyDmpCurrentPos % globals.opData.opFileDmpBlockSize;
	if (debugCopyDump) printf("CDSM_WriteDestStart --------------------------\n");
	if (debugCopyDump) printf("waiocbinfo.aio_buf    = 0x%lx\n",(size_t) waiocbinfo.aio_buf);
	if (debugCopyDump) printf("waiocbinfo.aio_fildes = %d\n", waiocbinfo.aio_fildes);
	if (debugCopyDump) printf("waiocbinfo.aio_nbytes = %lu\n", waiocbinfo.aio_nbytes);
	if (debugCopyDump) printf("waiocbinfo.aio_offset = 0x%lx\n",waiocbinfo.aio_offset);
	if (debugCopyDump) printf("max Writable = %lu\n",maxWritable);
	int result = aio_write(&waiocbinfo);
	if (result == 0)
		return StateIfSucceed;
	if (result == -1)
		if (errno == EAGAIN)
			return StateIfNoChange;
	Log_Add("[OPERATION COPY / DUMP] Couldn't start write operation : '%s'.",strerror(errno));
	return StateIfFail;
}

int CDSM_WriteDestWait(int StateIfNoChange, int StateIfSucceed, int StateIfFail){
	int result = aio_error(&waiocbinfo);
	if (result == EINPROGRESS)
		return StateIfNoChange;
	if (result == 0)
		return StateIfSucceed;
	Log_Add("[OPERATION COPY / DUMP] Couldn't check write operation : '%s'.",strerror(errno));
	return StateIfFail;
}
int CDSM_WriteDestFinish(int StateIfSucceed, int StateIfFail){
	ssize_t aioResult = aio_return(&waiocbinfo);
	if (aioResult < 0){
		Log_Add("[OPERATION COPY / DUMP] Couldn't check write operation : '%s'.",strerror(errno));
		return StateIfFail;
	} else {
		if ((size_t)aioResult != waiocbinfo.aio_nbytes){
			if (debugCopyDump) printf("[OPERATION COPY / DUMP] Wrote less than requested : %lu < %lu.",aioResult, waiocbinfo.aio_nbytes);
			return StateIfFail;
		}
		CpyDmpBufferSOD += (size_t)aioResult;
		globals.opData.opFileCpyDmpCurrentPos += (size_t)aioResult;
		return StateIfSucceed;
	}
}

int lastState  = -9999;
char * StateNames[]={ 	"CDS_INIT",
						"CDS_CREATE_BUFFER",
						"CDS_DESTROY_BUFFER_CONTINUE",
						"CDS_DESTROY_BUFFER_ABORT",
						"CDS_CHECK_WRITTEN_ENOUGH",
						"CDS_OPEN_DESTINATION",
						"CDS_CLOSE_DESTINATION_CONTINUE",
						"CDS_CLOSE_DESTINATION_ABORT",
						"CDS_CHECK_DONE_CURRENT",
						"CDS_CHECK_BUFFER_EMPTY",
						"CDS_CHECK_SOURCE_EOF",
						"CDS_READ_START",
						"CDS_READ_WAIT",
						"CDS_READ_DONE",
						"CDS_WRITE_START",
						"CDS_WRITE_WAIT",
						"CDS_WRITE_DONE",
						"CDS_END_SUCCESS",
						"CDS_END_FAIL"};

StatusCode CopyDumpFSM(){
	if (CopyDumpState!=lastState){
		lastState = CopyDumpState;
		if (debugCopyDump) printf("->('%s')\n",StateNames[CopyDumpState]);
	}
	switch(CopyDumpState){
		case CDS_INIT						:
			CpyDmpNumDigits = numDigits(globals.opData.opFileCpyDmpLength / globals.opData.opFileDmpBlockSize);
			if ((globals.opData.opFileCpyDmpLength % globals.opData.opFileDmpBlockSize) != 0)
				CpyDmpNumDigits++;
			CopyDumpState = CDS_CREATE_BUFFER;
			return NOT_READY;
		case CDS_CREATE_BUFFER				:
			CopyDumpState = CDSM_CreateBuffer(CDS_CHECK_WRITTEN_ENOUGH,CDS_END_FAIL);
			return NOT_READY;
		case CDS_DESTROY_BUFFER_CONTINUE	:
			CopyDumpState = CDSM_DestroyBuffer(CDS_END_SUCCESS);
			return NOT_READY;
		case CDS_DESTROY_BUFFER_ABORT		:
			CopyDumpState = CDSM_DestroyBuffer(CDS_END_FAIL);
			return NOT_READY;
		case CDS_CHECK_WRITTEN_ENOUGH		:
			CopyDumpState = CDSM_WrittenEnough(CDS_DESTROY_BUFFER_CONTINUE,CDS_OPEN_DESTINATION);
			return NOT_READY;
		case CDS_OPEN_DESTINATION			:
			CopyDumpState = CDSM_OpenDestination(CDS_CHECK_DONE_CURRENT,CDS_DESTROY_BUFFER_ABORT);
			return NOT_READY;
		case CDS_CLOSE_DESTINATION_CONTINUE	:
			CopyDumpState = CDSM_CloseDestination(CDS_CHECK_WRITTEN_ENOUGH);
			return NOT_READY;
		case CDS_CLOSE_DESTINATION_ABORT	:
			CopyDumpState = CDSM_CloseDestination(CDS_DESTROY_BUFFER_ABORT);
			return NOT_READY;
		case CDS_CHECK_DONE_CURRENT			:
			CopyDumpState = CDSM_DoneCurrentFile(CDS_CLOSE_DESTINATION_CONTINUE , CDS_CHECK_BUFFER_EMPTY);
			return NOT_READY;
		case CDS_CHECK_BUFFER_EMPTY			:
			CopyDumpState = CDSM_BufferIsEmpty(CDS_CHECK_SOURCE_EOF , CDS_WRITE_START);
			return NOT_READY;
		case CDS_CHECK_SOURCE_EOF			:
			CopyDumpState = CDSM_SourceEOF(CDS_CLOSE_DESTINATION_ABORT , CDS_READ_START);
			return NOT_READY;
		case CDS_READ_START					:
			CopyDumpState = CDSM_ReadSourceStart(CDS_READ_START,CDS_READ_WAIT,CDS_CLOSE_DESTINATION_ABORT);
			return NOT_READY;
		case CDS_READ_WAIT					:
			CopyDumpState = CDSM_ReadSourceWait(CDS_READ_WAIT,CDS_READ_DONE,CDS_CLOSE_DESTINATION_ABORT);
			return NOT_READY;
		case CDS_READ_DONE					:
			CopyDumpState = CDSM_ReadSourceFinish(CDS_WRITE_START,CDS_CLOSE_DESTINATION_ABORT);
			return NOT_READY;
		case CDS_WRITE_START				:
			CopyDumpState = CDSM_WriteDestStart(CDS_WRITE_START,CDS_WRITE_WAIT,CDS_CLOSE_DESTINATION_ABORT);
			return NOT_READY;
		case CDS_WRITE_WAIT					:
			CopyDumpState = CDSM_WriteDestWait(CDS_WRITE_WAIT, CDS_WRITE_DONE, CDS_CLOSE_DESTINATION_ABORT);
			return NOT_READY;
		case CDS_WRITE_DONE					:
			CopyDumpState = CDSM_WriteDestFinish(CDS_CHECK_DONE_CURRENT, CDS_CLOSE_DESTINATION_ABORT);
			return NOT_READY;
		case CDS_END_SUCCESS				:
			// excess bytes issue
			if ((globals.opData.opFileCpyDmpLength % globals.opData.opFileDmpBlockSize)!=0){
				if (truncate(globals.opData.opCurrentFileName, (globals.opData.opFileCpyDmpLength % globals.opData.opFileDmpBlockSize) )!=0){
					Log_Add("[OPERATION COPY / DUMP] Truncation of last block failed");
				}
			}
			CopyDumpState = CDS_INIT;
			return SUCCESS;
		case CDS_END_FAIL					:
			CopyDumpState = CDS_INIT;
			return FAILURE;
		default :
			Log_Add("[OPERATION COPY / DUMP] State machine in undefined state.");
			CopyDumpState = CDS_INIT;
			return FAILURE;
	}

}
// Do the work of the operation. returns NOT_READY until complete, and then SUCCESS. FAILURE means FAILURE.
StatusCode Operation_Run(){
	switch(globals.opData.opType){
		case OP_TYPE_IDLE		:
			return SUCCESS;
		case OP_TYPE_REC		:
			// check to make sure we have initialized recording properly
			// very unlikely to have not started the prefire, but just in case:
			if (!prefireStarted || !preFireComplete){
				recordingPrefire();
			} else{
				receiveEnqueue();
				dequeueWrite();
			}
			return NOT_READY; // never terminates automatically, must be terminated by scheduler
		case OP_TYPE_CPY		: //fall through; copy and dump use the same FSM
		case OP_TYPE_DMP		: return CopyDumpFSM();
		case OP_TYPE_FMT		: return CheckProcessDone((ProcessHandle*)globals.opData.opExtraData);
		case OP_TYPE_JUP		: return OpJupRun();
		case OP_TYPE_SPC		: return OpSpecRun();
		case OP_TYPE_XCP		: return OpXcpRun();

		default :
			Log_Add("[OPERATION MANAGER] Unsupported operation type.");
		break;
	}
	return FAILURE;
}

// Do the cleanup required of the operation (close files, etc). when complete data will be in idle state
StatusCode Operation_CleanUp(){
	if (!globals.opData.opCleanupComplete){
		char * result;
		switch(globals.opData.opType){
			case OP_TYPE_IDLE		:
				globals.opData.opCleanupComplete = true;
				return SUCCESS;
			case OP_TYPE_REC		:
				if (	globals.opData.opStatus>=OP_STATUS_RUNNING){
					TimeStamp ts = getTimestamp();
					double stopDrift =	(((double) (ts.MJD - globals.opData.opStopMJD)) * ((double) MILLISECONDS_PER_DAY)) + ((double) ts.MPM) - ((double) globals.opData.opStopMPM) ;
					printf( HLR(">>>>>>>> " HLG("Cleanup") HLR(" @ stop drift = %f ms, buffer=%lu\n")), stopDrift, globals.rxQueue->bytesUsed);
					globals.rxQueue->allowUndersizeRemoval=true;
					while (globals.rxQueue->bytesUsed){
						dequeueWrite();
					}
				}
				if (globals.opData.rxSocketOpen){
					if (Socket_Close(globals.rxSocket)!=SUCCESS){
						Log_Add("[OPERATION RECORD] Error: couldn't close data socket.");
						exit(EXIT_CODE_FAIL);
					}
					globals.opData.rxSocketOpen = false;
				}
				if (globals.opData.tagfile->isOpen){
					FileSystem_SetFileMetaDataIsComplete(globals.fileSystem,globals.opData.tagfile->index,1);
					if (FileSystem_CloseFile(globals.opData.tagfile)!=SUCCESS){
						Log_Add("[OPERATION RECORD] Error: couldn't close tag file.");
						return FAILURE;
					}
				}
				LiveCapture_Event_RecordingStop();
				globals.opData.opStatus =OP_STATUS_COMPLETE;
				globals.opData.opCleanupComplete = true;
				return SUCCESS;
			case OP_TYPE_FMT		:
				result=((ProcessHandle*)globals.opData.opExtraData)->resultBuffer;
				if (result){
					int formatSuccess=(strtoul(result,NULL,10) == 0);
					free(result);
					ReleaseProcessHandle((ProcessHandle*)globals.opData.opExtraData);
					if (formatSuccess){
						//TODO: re-enumerate devices;
						globals.opData.opCleanupComplete = true;
						return SUCCESS;
					}else{
						Log_Add("[OPERATION FORMAT] Error: format failed.");
						return FAILURE;
					}
				} else {
					Log_Add("[OPERATION FORMAT] Error: unspecified result pointer (FMT command cleanup).");
					return FAILURE;
				}
			case OP_TYPE_CPY		: //fall through
			case OP_TYPE_DMP		:
				if (globals.opData.tagfile->isOpen){
					FileSystem_CloseFile(globals.opData.tagfile);
				}
				if (globals.opData.extdisk && globals.opData.extdisk->mounted){
					Disk_Unmount(globals.opData.extdisk);
				}
				Disk_UpdateFreeSpace(globals.opData.extdisk);
				globals.opData.opCleanupComplete = true;
				return SUCCESS;
			case OP_TYPE_JUP		: return OpJupCleanup();
			case OP_TYPE_SPC		: return OpSpecCleanup();
			case OP_TYPE_XCP		: return OpXcpCleanup();

			default :
				Log_Add("[OPERATION MANAGER] Unsupported operation type.");
				return FAILURE;
		}
	}
	return SUCCESS;
}

/*
boolean Operation_ParamsEqual(OperationParameters* a,OperationParameters* b){
	assert(a!=NULL && b!=NULL);
	if (a==b) return true;
	char * c=(char*)a;
	char * d=(char*)b;
	int i;
	for (i=0;i<sizeof(OperationParameters);i++){
		if (c[i]!=d[i]) return false;
	}
	return true;
}

*/



size_t bytesReceived;
char * inPtr=NULL;      // pointer to message waiting to be enqueued
size_t bytesWritten;
char * outPtr=NULL;      // pointer to queued data waiting to be written

void recordingPrefire(){
	if (!prefireStarted){
		Log_Add("[OPERATION RECORD] Opening socket on port %d",globals.DataInPort);
		if (Socket_OpenRecieve(globals.rxSocket,globals.DataInPort)!=SUCCESS){
			printf("Error: couldn't open data socket.");
			exit(EXIT_CODE_FAIL);
		} else {
			globals.opData.rxSocketOpen = true;
		}
		prefireStarted = true;
		printf( HLG("@@@@@@ Started Prefire @@@@@@ %s\n"), HumanTime());
	}
	if (prefireStarted){
		if (!preFireComplete){
			if (leadEntryWaitToRunDone()) {
				TimeStamp ts = getTimestamp();
				double startDrift =	(((double) (ts.MJD - globals.opData.opStartMJD)) * ((double) MILLISECONDS_PER_DAY)) + ((double) ts.MPM) - ((double) globals.opData.opStartMPM) ;
				preFireComplete = true;
				LiveCapture_Event_RecordingStart(&globals.opData);
				globals.opData.opStatus = OP_STATUS_RUNNING;
				printf( HLG("@@@@@@ Finished Prefire @@@@@@ %s\n"), HumanTime());
				printf( HLB("########  Recording Drift : %f ms\n"), startDrift);
				globals.rxSocket->bytes=0;
			}
		}
		if (!preFireComplete){
			receiveDiscard();
		}
	}


}

char ignoredBuff[8192];
size_t ignoredCount;
inline void receiveDiscard(){
	Socket_Recieve(globals.rxSocket,ignoredBuff,&ignoredCount);
	//printf( HLB("@") );
}
/*
void receiveEnqueue(){
	int overFlow=0;
	StatusCode sc;
	static size_t rptcntr=0;
	if (++rptcntr>300000l){
		printf("[OPERATION RECORD] Bytes Received = %15lu; Ring Queue Occupancy=%15lu; Live Capture PacketsAvailable=%15lu; Time=%s\n",globals.rxSocket->bytes,globals.rxQueue->bytesUsed, LiveCapture_getRetrievableCount(), HumanTime());
		rptcntr = 0;
	}

// ----------------------------------------- receive into data buffer
	if ((TotalProcessedBytesReceived ) < TagFileSizeLimit){
		if (!insertionAvailable){
			if(RingQueue_NextInsertionPoint(globals.rxQueue,&inPtr)==SUCCESS){
				assert (inPtr!=NULL);
				insertionAvailable=true;
			}
		}
	} else { // we've run out of space, so until the recording ends, just dump packets
		overFlow=1;
		globals.rxQueue->allowUndersizeRemoval=true;
	}
	if (notifiedOverflow==0){
		if (insertionAvailable && !dataReceived){
			if ((sc=Socket_Recieve(globals.rxSocket,inPtr,&bytesReceived))==SUCCESS){
				dataReceived=true;
				//printf("received %lu\n",bytesReceived);
				if (bytesReceived!=globals.opData.compiledFormat->rawPacketSize){
					globals.opData.warningCount++;
				}
			} else {
				//printf("recv says %s\n", (sc==NOT_READY ? "NOT_READY" : "FAILURE"));
			}
		}
		if(dataReceived && !formatApplied){
			LiveCapture_Event_PacketBefore(inPtr, globals.opData.compiledFormat->rawPacketSize);
			if (DataFormats_Apply(globals.opData.compiledFormat, inPtr)==SUCCESS){
				//printf("format applied %lu\n",globals.opData.compiledFormat->finalPacketSize);
				formatApplied=true;
			} else {
				Log_Add("[OPERATION RECORD] Error: data formatting failed.");
				exit(EXIT_CODE_FAIL);
			}
		}
		if(formatApplied){
			LiveCapture_Event_PacketAfter(inPtr, globals.opData.compiledFormat->finalPacketSize);
			if (RingQueue_NotifyInsertionComplete(globals.rxQueue,globals.opData.compiledFormat->finalPacketSize)!=SUCCESS){
				Log_Add("[OPERATION RECORD] Error: data buffering failed.");
				exit(EXIT_CODE_FAIL);
			} else {
				//printf("recv cycle complete, %lu bytes total in queue\n",globals.rxQueue->bytesUsed);
				TotalProcessedBytesReceived+=globals.opData.compiledFormat->finalPacketSize;
				formatApplied 		= false;
				dataReceived 		= false;
				insertionAvailable 	= false;
			}
		}
	}
	if ((overFlow==1) && (insertionAvailable==false)){
		if (notifiedOverflow==0){
			Log_Add("[OPERATION RECORD] Error: Recording exceeded allocated filesize. Dropping remaining packets");
			//Log_Add("[OPERATION RECORD] cur=%lu, max=%lu, fps=%lu",TotalProcessedBytesReceived,TagFileSizeLimit,globals.opData.compiledFormat->finalPacketSize);
			notifiedOverflow=1;
		}
	}

}
*/
size_t unusedBytesInsertable=0;
void receiveEnqueue(){
	int overFlow=0;
	StatusCode sc;
	static size_t rptcntr=0;
	if (++rptcntr>300000l){
		printf("[OPERATION RECORD] Bytes Received = %15lu; Ring Queue Occupancy=%15lu; Live Capture PacketsAvailable=%15lu; Time=%s\n",globals.rxSocket->bytes,globals.rxQueue->bytesUsed, LiveCapture_getRetrievableCount(), HumanTime());
		rptcntr = 0;
	}

// ----------------------------------------- receive into data buffer
	if ((TotalProcessedBytesReceived ) < TagFileSizeLimit){
		if (!insertionAvailable){
			if(RingQueue_NextInsertionPoint(globals.rxQueue,&inPtr,globals.opData.dataFormat->payloadSize,&unusedBytesInsertable)==SUCCESS){
				assert (inPtr!=NULL);
				insertionAvailable=true;
			}
		}
	} else { // we've run out of space, so until the recording ends, just dump packets
		overFlow=1;
		globals.rxQueue->allowUndersizeRemoval=true;
	}
	if (notifiedOverflow==0){
		if (insertionAvailable && !dataReceived){
			if ((sc=Socket_Recieve(globals.rxSocket,inPtr,&bytesReceived))==SUCCESS){
				dataReceived=true;
				/*****************************************************/
				// TT-LAG measurements
				if (bytesReceived == 4128){
					globals.opData.rx_time_uspe_recent    =  getMicrosSinceEpoch(__builtin_bswap64(((struct __DrxFrame *) inPtr)->header.timeTag));
					globals.opData.local_time_uspe_recent = getMicrosSinceEpoch(0);
					if (!globals.opData.lag_measure_initialized){
						globals.opData.lag_measure_initialized = 1;
						globals.opData.rx_time_uspe_first    = globals.opData.rx_time_uspe_recent;
						globals.opData.local_time_uspe_first = globals.opData.local_time_uspe_recent;
					}
				}
				/*****************************************************/


				//printf("received %lu\n",bytesReceived);
				if (bytesReceived!=globals.opData.dataFormat->payloadSize){
					globals.opData.warningCount++;
				}
			} else {
				//printf("recv says %s\n", (sc==NOT_READY ? "NOT_READY" : "FAILURE"));
			}
		}
		if(dataReceived){
			LiveCapture_Event_PacketBefore(inPtr, globals.opData.dataFormat->payloadSize);
			RingQueue_NotifyInsertionComplete(globals.rxQueue,globals.opData.dataFormat->payloadSize);
			//printf("recv cycle complete, %lu bytes total in queue\n",globals.rxQueue->bytesUsed);
			TotalProcessedBytesReceived+=globals.opData.dataFormat->payloadSize;
/*				formatApplied 		= false;*/
			dataReceived 		= false;
			insertionAvailable 	= false;
		}
	}
	if ((overFlow==1) && (insertionAvailable==false)){
		if (notifiedOverflow==0){
			Log_Add("[OPERATION RECORD] Error: Recording exceeded allocated filesize. Dropping remaining packets");
			TimeStamp ts = getTimestamp();
			double stopDrift =	(((double) (ts.MJD - globals.opData.opStopMJD)) * ((double) MILLISECONDS_PER_DAY)) + ((double) ts.MPM) - ((double) globals.opData.opStopMPM) ;
			printf( HLR("%%%%%%%%%%%%%%%%> ") HLB(" Overflow ") HLR("Hit at Stop Drift : %f ms\n"), stopDrift);
			//Log_Add("[OPERATION RECORD] cur=%lu, max=%lu, fps=%lu",TotalProcessedBytesReceived,TagFileSizeLimit,globals.opData.compiledFormat->finalPacketSize);
			notifiedOverflow=1;
		}
	}

}
size_t bytesRemovable=0;
void dequeueWrite(){
// ----------------------------------------- write from buffer into file
	if (!writeInProgress){
		if (!removalAvailable){
			if (RingQueue_NextRemovalPoint(globals.rxQueue,&outPtr, globals.fileSystem->fileSystemHeaderData->filesystemInfo.chunkSize, &bytesRemovable)==SUCCESS){
			//	printf("Removing...\n");
				assert (outPtr!=NULL);
				removalAvailable=true;
			}
		}
		if(removalAvailable){
			if (bytesRemovable){
				StatusCode sc=FileSystem_AsynchronousWriteStart(globals.opData.tagfile,outPtr,bytesRemovable);
				if (sc==SUCCESS){
				//	printf("write started ...\n");
					writeInProgress=true;
				} else {
					Log_Add("[OPERATION RECORD] Warning: cannot write remaining bytes (%lu) to tagfile. Discarding.",globals.rxQueue->bytesUsed);
					RingQueue_Reset(globals.rxQueue);
					Process_TerminateCurrent();
				}
			} else {
				RingQueue_NotifyRemovalComplete(globals.rxQueue, 0);
			}
		}
	}
	if (writeInProgress){
		if (! writeComplete){
			if (FileSystem_IsAsynchronousWriteDone(globals.opData.tagfile)!=NOT_READY){
				//printf("write complete ...\n");
				if (FileSystem_GetAsynchronousBytesWritten(globals.opData.tagfile,&bytesWritten)!=SUCCESS || bytesWritten!=bytesRemovable){
					globals.opData.errorCount++;
				}
				writeComplete=true;
			}
		}
		if (writeComplete){
			if (RingQueue_NotifyRemovalComplete(globals.rxQueue, bytesWritten)!=SUCCESS){
				Log_Add("[OPERATION RECORD] Error: data buffering failed.");
				exit(EXIT_CODE_FAIL);
			}
			//printf("removal complete ...\n");
			writeComplete = false;
			removalAvailable = false;
			writeInProgress = false;
		}

	}
}




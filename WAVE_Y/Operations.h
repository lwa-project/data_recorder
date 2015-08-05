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
 * Operations.h
 *
 *  Created on: Oct 31, 2009
 *      Author: chwolfe2
 */

#ifndef OPERATIONS_H_
#define OPERATIONS_H_
#include <stdlib.h>
#include "Defines.h"
#include "Message.h"
#include "Disk.h"
#include "Time.h"
#include "Log.h"
#include "FileSystem.h"
#include "Socket.h"
#include "RingQueue.h"
#include "MessageQueue.h"
#include "Persistence.h"
#include "Schedule.h"
#include "DataFormats.h"




#define OP_STATUS_NULL		  0
#define OP_STATUS_INITIALIZED 1
#define OP_STATUS_RUNNING	  2
#define OP_STATUS_WARNING	  3
#define OP_STATUS_ERROR		  4
#define OP_STATUS_COMPLETE	  5

typedef int OpStatus;

#define OP_TYPE_IDLE	0
#define OP_TYPE_REC		1
#define OP_TYPE_FMT		2
#define OP_TYPE_CPY		3
#define OP_TYPE_DMP		4
#define OP_TYPE_JUP		5
#define OP_TYPE_SPC		6
#define OP_TYPE_XCP     7


typedef int OpType;

typedef struct __OperationParameters{
	OpType opType;
	size_t opStartMJD;
	size_t opStartMPM;
	size_t opStopMJD;
	size_t opStopMPM;
	size_t opReference;
	char opTag[16+1];
	char opFormat[DATA_FORMAT_MAX_NAME_LENGTH+1];
	char opFileName[EXT_FILE_MAX_NAME_LENGTH+1];
	char opDeviceName[MAX_DEVICE_NAME_LENGTH];
	size_t opFileCpyDmpStartPos;
	size_t opFileCpyDmpLength;
	size_t opFileDmpBlockSize;
	uint32_t TransformLength;
	uint32_t IntegrationCount;
	uint32_t highWater;
	uint32_t minFill;

} OperationParameters;



typedef struct __OperationData{
	OperationParameters* 	initialParameters;
	OpType 				 	opType;
	size_t 				 	opStartMJD;
	size_t 					opStartMPM;
	size_t 					opStopMJD;
	size_t 					opStopMPM;
	size_t 					opReference;
	size_t 					opFileCpyDmpStartPos;
	size_t 					opFileCpyDmpCurrentPos;
	size_t 					opFileCpyDmpLength;
	size_t 					opFileDmpBlockSize;
	size_t 					opFileDmpCurrentIndex;

	boolean 				opCleanupComplete;
	// operation status
	OpStatus				opStatus;
	size_t 					errorCount;
	size_t					warningCount;

	// internal storage related
	char 					opTag[16+1];
	MyFile *    			tagfile;

	// external storage related
	char 					opFileName[EXT_FILE_MAX_NAME_LENGTH+1];
	char 					opCurrentFileName[EXT_FILE_MAX_NAME_LENGTH+1];
	char 					opDeviceName[MAX_DEVICE_NAME_LENGTH+1];
	Disk* 					extdisk;
	int						extFileHandle;

	// data format
	char opFormat[DATA_FORMAT_MAX_NAME_LENGTH+1];
	DataFormat* 			dataFormat;
/*	CompiledDataFormat* 	compiledFormat;*/

	// data socket
	boolean 				rxSocketOpen;

	//spectrometer options
	uint32_t 				TransformLength;
	uint32_t 				IntegrationCount;
	uint32_t 				highWater;
	uint32_t 				minFill;

	// timetag lag measurements, in micro seconds (so multiply time tags by 1,000)
	boolean             	lag_measure_initialized;
	uint64_t 				rx_time_uspe_first;
	uint64_t 				rx_time_uspe_recent;
	uint64_t 				local_time_uspe_first;
	uint64_t 				local_time_uspe_recent;

	//
	void *					opExtraData;


} OperationData;

// allocate files, check available space, that kind of thing. done at time of command issuance
ErrorCode Operation_Register(OperationParameters* params);

// fill out the Operation Data struct from saved params (saved in the schedule) .
StatusCode Operation_Init(OperationParameters* params);
// Do the work of the operation. returns NOT_READY until complete, and then SUCCESS. FAILURE means FAILURE.
StatusCode Operation_Run();
// Do the cleanup required of the operation (close files, etc). when complete data will be in idle state
StatusCode Operation_CleanUp();

// compare params for equality
boolean Operation_ParamsEqual(OperationParameters* a,OperationParameters* b);

char * Operation_TypeName(OpType type);


#endif /* OPERATIONS_H_ */

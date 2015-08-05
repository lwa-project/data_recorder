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
 * DataFormats.c
 *
 *  Created on: Oct 31, 2009
 *      Author: chwolfe2
 */

#include "DataFormats.h"
#include "Persistence.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>
#include "Log.h"
#include "ConsoleColors.h"
Database formatsDb;
boolean isFormatsOpen=false;

void Formats_Close(){
	if (isFormatsOpen){
		Persistence_Close(&formatsDb);
		isFormatsOpen=false;
	}
}
StatusCode DataFormats_Initialize(boolean reloadConfig){
	if (!isFormatsOpen){
		//printf("%s\n",(reloadConfig?"true":"false"));
		if (!reloadConfig){
			StatusCode sc = Persistence_Open(&formatsDb,FORMATSFILENAME);
			if (sc!=SUCCESS){
				sc = Persistence_Create(FORMATSFILENAME,FORMATSDEFAULTSFILENAME);
				if (sc!=SUCCESS){
					return sc;
				} else {
					sc=Persistence_Open(&formatsDb,FORMATSFILENAME);
					if (sc==SUCCESS){
						atexit(Formats_Close);
						isFormatsOpen=true;
					}
					return sc;
				}
			} else {
				atexit(Formats_Close);
				isFormatsOpen=true;
				return sc;
			}
		} else {
			StatusCode sc = Persistence_Create(FORMATSFILENAME,FORMATSDEFAULTSFILENAME);
			if (sc!=SUCCESS){
				return sc;
			} else {
				sc=Persistence_Open(&formatsDb,FORMATSFILENAME);
				if (sc==SUCCESS){
					atexit(Formats_Close);
					isFormatsOpen=true;
				}
				return sc;
			}
		}
	} else {
		return FAILURE;
	}
}
int _DataFormats_GetCount(){
	assert(isFormatsOpen);
	int count=0;
	StatusCode sc;
	sc = Persistence_Get_Int(&formatsDb,"FORMAT-COUNT",&count);
	if (sc!=SUCCESS || count==0) count=-1;
	return count;
}

int _DataFormats_GetIndex(char * formatName){
	assert(isFormatsOpen);
	assert(formatName != NULL && *formatName);
	int count = _DataFormats_GetCount();
	if (count==-1) return -1;
	int iter;
	char buffer[2048];
	char resData[8192];
	StatusCode sc;
	for (iter=0; iter<count; iter++){
		sprintf(buffer,"FORMAT-NAME-%d",iter);
		sc = Persistence_Get_String(&formatsDb,buffer,resData);
		if (sc!=SUCCESS) return FAILURE;
		printf(HLR("Comparing '%s' with entry %d: '%s'\n"), formatName, iter, &resData[0]);
		if (strcmp(formatName,resData)==0) {
			return iter;
		}
	}
	return -1;
}
StatusCode DataFormats_GetCount(int* count){
	assert(isFormatsOpen);
	*count=_DataFormats_GetCount();
	if (*count != -1) return SUCCESS; else { *count=0; return FAILURE;}
}

StatusCode DataFormats_GetName(int index, char * formatName){
	assert(index!=-1);
	assert(isFormatsOpen);
	assert(formatName != NULL);
	char buffer[2048];
	sprintf(buffer,"FORMAT-NAME-%d",index);
	return Persistence_Get_String(&formatsDb,buffer,formatName);
}

StatusCode DataFormats_GetSpec(int index, char * spec){
	assert(index!=-1);
	assert(isFormatsOpen);
	assert(spec != NULL);
	char buffer[2048];
	sprintf(buffer,"FORMAT-SPEC-%d",index);
	return Persistence_Get_String(&formatsDb,buffer,spec);
}

StatusCode DataFormats_GetPayloadSize(int index, size_t * payloadSize){
	assert(index!=-1);
	assert(isFormatsOpen);
	assert(payloadSize != NULL);
	char buffer[2048];
	sprintf(buffer,"FORMAT-PAYLOAD-%d",index);
	return Persistence_Get_UnsignedLong(&formatsDb,buffer,payloadSize);
}

StatusCode DataFormats_GetRate(int index, size_t * rate){
	assert(index!=-1);
	assert(isFormatsOpen);
	assert(rate != NULL);
	char buffer[2048];
	sprintf(buffer,"FORMAT-RATE-%d",index);
/*	printf("#################################################\n");
	printf("%s\n",buffer);
	printf("#################################################\n");*/
	return Persistence_Get_UnsignedLong(&formatsDb,buffer,rate);
}


StatusCode DataFormats_Get(char * formatName, DataFormat * result){
	assert(isFormatsOpen);
	assert (formatName!=NULL &&  result!=NULL);
	int formatIndex = _DataFormats_GetIndex(formatName);
	if (formatIndex==-1) return FAILURE;
	strcpy(result->name, formatName);
	StatusCode sc;
	/*
	sc=DataFormats_GetSpec(formatIndex,result->specification);
	if (sc!=SUCCESS) return FAILURE;
	*/
	sc=DataFormats_GetPayloadSize(formatIndex,&result->payloadSize);
	if (sc!=SUCCESS) return FAILURE;
	return DataFormats_GetRate(formatIndex,&result->rawRate);
}
/*
StatusCode DataFormats_Compile(DataFormat * format, CompiledDataFormat* result){
	assert (format!=NULL &&  result!=NULL);
	result->rawRate = format->rawRate;
	result->rawPacketSize = format->payloadSize;
	result->subSelectCount = 0;
	int slen = strlen(format->specification);
	int pos=0;
	int dropcount=0;
	while( pos < slen && format->specification[pos]){
		if (format->specification[pos]=='K'){
			result->subSelectCount++;
		} else if (format->specification[pos]=='D') {
			// legal characters, but don't affect sub select count, do nothing
			dropcount++;
		}else if (strchr("0123456789",format->specification[pos])){
			// legal characters, but don't affect sub select count, do nothing
			;
		} else {
			// illegal characters in specification
			return FAILURE;
		}
		pos++;
	}
	// special case for empty strings or one-term stings, default behaviour is to keep whole packet
	// note is not legal to drop the whole packet
	if (result->subSelectCount==0 || (result->subSelectCount==1 && dropcount==0)){
		result->subSelectCount=0;
		result->finalPacketSize=result->rawPacketSize;
		result->finalRate=result->rawRate;
		result->subSelect=NULL;
		return SUCCESS;
	}
	result->subSelect = (SubSelect*) malloc(result->subSelectCount * sizeof(SubSelect));
	pos=0;
	// parse the format  Kxxx means keep xxx bytes, and Dxxx means drop xxx bytes
	size_t curPosInFormat = 0;
	size_t curReadPosInPacket = 0;
	size_t curWritePosInPacket = 0;
	size_t curSubSelectIndex=0;
	size_t value;
	char * offsetptr;
	while(curPosInFormat<slen && format->specification[curPosInFormat]){
		if (format->specification[curPosInFormat]=='K'){//keep
			value = strtoul(&(format->specification[curPosInFormat+1]),&offsetptr,10);
//			printf("KEEP : %lu (FP=%lu, w=%lu; Rp=%lu, Wp=%lu)\n",value,curPosInFormat,(size_t) offsetptr - (size_t) &(format->specification[curPosInFormat+1]),curReadPosInPacket,curWritePosInPacket);
			result->subSelect[curSubSelectIndex].length=value;
			result->subSelect[curSubSelectIndex].position=curReadPosInPacket;
			result->subSelect[curSubSelectIndex].newPosition=curWritePosInPacket;
			curPosInFormat 		+= (size_t) offsetptr - (size_t) &(format->specification[curPosInFormat+1]);
			curReadPosInPacket 	+= value;
			curWritePosInPacket += value;
			curSubSelectIndex	++;
		} else if (format->specification[curPosInFormat]=='D'){//drop
			value = strtoul(&(format->specification[curPosInFormat+1]),&offsetptr,10);
			curPosInFormat 		+= (size_t) offsetptr - (size_t) &(format->specification[curPosInFormat+1]);
			curReadPosInPacket 	+= value;
		} else {
			Log_Add("[FORMAT COMPILER] Bad data format: '%s' at '%c'\n",format->specification, format->specification[curPosInFormat]);
			free(result->subSelect);
			result->subSelect=NULL;
			result->subSelectCount=0;
			return FAILURE;
		}
		curPosInFormat++;
	}
	if (curReadPosInPacket!=format->payloadSize || curPosInFormat!=slen) {
		free(result->subSelect);
		result->subSelect=NULL;
		result->subSelectCount=0;
		return FAILURE;
	}
	result->finalPacketSize = curWritePosInPacket;
	result->finalRate		= (curWritePosInPacket*result->rawRate)/result->rawPacketSize;
	return SUCCESS;
}

StatusCode DataFormats_Release(CompiledDataFormat* compiledFormat){
	if (compiledFormat != NULL && compiledFormat->subSelect!=NULL){
		free(compiledFormat->subSelect);
		compiledFormat->subSelect=NULL;
		compiledFormat->subSelectCount=0;
	}
	return SUCCESS;
}
*/
StatusCode DataFormats_Finalize(){
	Formats_Close();
	return SUCCESS;
}
/*
void printCompiledFormat(CompiledDataFormat* cf){
	int i;
	Log_Add("[FORMAT COMPILER] Compiled Format Start------------------\n");
	Log_Add("[FORMAT COMPILER] \tRaw Rate:        %lu\n", cf->rawRate);
	Log_Add("[FORMAT COMPILER] \tRaw payload:     %lu\n", cf->rawPacketSize);
	Log_Add("[FORMAT COMPILER] \tFinal Rate:      %lu\n", cf->finalRate);
	Log_Add("[FORMAT COMPILER] \tFinal payload:   %lu\n", cf->finalPacketSize);
	Log_Add("[FORMAT COMPILER] \tNumber MemMoves: %lu\n", cf->subSelectCount);
	for (i=0; i<cf->subSelectCount; i++ ){
		//void * memmove ( void * destination, const void * source, size_t num );
		Log_Add("[FORMAT COMPILER] \t [%03d]: memmove((void*)&packet[%05lu], (void*)&packet[%05lu], %05lu);\n",
				i,
				cf->subSelect[i].newPosition,
				cf->subSelect[i].position,
				cf->subSelect[i].length
		);
	}
	Log_Add("[FORMAT COMPILER] Compiled Format End--------------------\n");
}
StatusCode DataFormats_Apply			(CompiledDataFormat* compiledFormat, char* memoryLocation){
	assert (compiledFormat!=NULL && memoryLocation!=NULL);
	int i=0;
	while(i<compiledFormat->subSelectCount){
		memmove(
				(void*)&memoryLocation[compiledFormat->subSelect[i].newPosition],
				(void*)&memoryLocation[compiledFormat->subSelect[i].position],
				compiledFormat->subSelect[i].length
		);
		i++;
	}
	return SUCCESS;
}
*/

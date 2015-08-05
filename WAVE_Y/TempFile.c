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

#include <unistd.h>
#include <sys/stat.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "Config.h"
#include "HostInterface.h"
#include "TempFile.h"

boolean tempFileExists(char* name, size_t* fileMPM, size_t* fileMJD){
	char fn[256];
	sprintf(fn,"%s%s.temp",TEMP_FILE_LOCATION,name);
	struct stat filestats;
	if (stat(fn,&filestats)){
		return false;
	} else {
		// get file modification time and convert to MJD+MPM
		struct tm t;
		gmtime_r(&filestats.st_mtim.tv_sec, &t);
		// compute MPM
			(*fileMPM) =  (t.tm_hour*3600 + t.tm_min*60 + t.tm_sec)*1000 + (filestats.st_mtim.tv_sec/1000);
			size_t a = (14 - (t.tm_mon +1)) / 12;
			size_t y = (t.tm_year +1900) + 4800 - a;
			size_t m = (t.tm_mon +1) + (12 * a) - 3;
			size_t p = t.tm_mday + (((153 * m) + 2) / 5) + (365 * y);
			size_t q = (y / 4) - (y / 100) + (y / 400) - 32045;
			// compute MJD
			(*fileMJD) = ( (p+q) - 2400000.5);
		return true;
	}
}

void writeTempFile(char* name, char* data, size_t size){
	char fn[256];
	char cmd[8192];
	char res[8192];
	sprintf(fn,"%s%s.temp",TEMP_FILE_LOCATION,name);
	sprintf(cmd,"rm -f %s",fn); issueShellCommand(cmd,res,8192);
	FILE* fd = fopen(fn,"wb");
	if (!fd){
		printf("Error: can't create temp file '%s'\n",fn);
		return;
	}
	if (data && size){
		if (fwrite(data,size,1,fd)!=1){
			printf("Error: can't write temp file '%s'\n",fn);
			fclose(fd);
			issueShellCommand(cmd,res,8192);
		}
	}
	fclose(fd);
}

boolean readTempFile(char* name, char** data, size_t* size){
	char fn[256];
	sprintf(fn,"%s%s.temp",TEMP_FILE_LOCATION,name);
	FILE * fd;
	size_t fsize;
	char* oldData=(*data);
	size_t oldSize=(*size);

	// open
	fd = fopen (fn, "rb");
	if (fd==NULL) {
		printf("Error: can't open temp file '%s'\n",fn);
		return false;
	}
	// check size
	fseek (fd , 0 , SEEK_END);
	fsize = ftell (fd);
	rewind (fd);
	if (!fsize){
		fclose(fd);
		(*data)=NULL;
		(*size)=0;
		return true;
	}
	// allocate mem
	(*data) = (char*) malloc (sizeof(char)*fsize);
	if (!(*data)) {
		printf("Error: can't allocate memory to read temp file '%s'\n",fn);
		(*data)=oldData;
		(*size)=oldSize;
		fclose(fd);
		return false;
	}
	// read
	(*size) = fread ((*data),1,fsize,fd);
	if ((*size) != fsize) {
		printf("Error: reading '%s' error: read %lu bytes when expecting %lu\n",fn,(*size),fsize);
		fclose(fd);
		free((*data));
		(*data)=oldData;
		(*size)=oldSize;
		return false;
	}

	fclose (fd);
	return true;
}
void    deleteTempFile(char* name){
	char fn[256];
	sprintf(fn,"%s%s.temp",TEMP_FILE_LOCATION,name);
	remove(fn);
}

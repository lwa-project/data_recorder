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
 * DataFormats.h
 *
 *  Created on: Oct 31, 2009
 *      Author: chwolfe2
 */

#ifndef DATAFORMATS_H_
#define DATAFORMATS_H_
#include "Defines.h"
#include <stdlib.h>
#define FORMATSFILENAME "/LWA/database/formatDb.gdbm"
#define FORMATSDEFAULTSFILENAME "/LWA/config/formats.cfg"

#define DATA_FORMAT_MAX_NAME_LENGTH 32
#define DATA_FORMAT_MAX_SPEC_LENGTH 256

typedef struct __DataFormat{
	char 	name[DATA_FORMAT_MAX_NAME_LENGTH];
	size_t 	payloadSize;
	size_t	rawRate;
	//char    specification[DATA_FORMAT_MAX_SPEC_LENGTH];
} DataFormat;

typedef struct __SubSelect{
	size_t position;
	size_t length;
	size_t newPosition;
} SubSelect;

/*
typedef struct __CompiledDataFormat{
	size_t 		rawRate;
	size_t 		finalRate;
	size_t		rawPacketSize;
	size_t		finalPacketSize;
	size_t 		subSelectCount;
	SubSelect * subSelect;
} CompiledDataFormat;
*/


// to open
StatusCode DataFormats_Initialize		(boolean reloadConfig);
// for MIB access
StatusCode DataFormats_GetCount			(int* count);
StatusCode DataFormats_GetName			(int index, char * formatName);
StatusCode DataFormats_GetSpec			(int index, char * spec);
StatusCode DataFormats_GetPayloadSize	(int index, size_t * payloadSize);
StatusCode DataFormats_GetRate			(int index, size_t * rate);
// for General usage
StatusCode DataFormats_Get				(char * formatName, DataFormat * result);
/*StatusCode DataFormats_Compile			(DataFormat * format, CompiledDataFormat* result);*/
/*StatusCode DataFormats_Apply			(CompiledDataFormat* compiledFormat, char* memoryLocation);*/
/*StatusCode DataFormats_Release			(CompiledDataFormat* compiledFormat);*/
/*void printCompiledFormat(CompiledDataFormat* compiledFormat);*/
// to close
StatusCode DataFormats_Finalize();



#endif /* DATAFORMATS_H_ */

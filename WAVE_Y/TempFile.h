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
#ifndef TEMPFILE_H_
#define TEMPFILE_H_
#define TEMP_FILE_LOCATION "/LWA/runtime/"

boolean tempFileExists(char* name, size_t* fileMPM, size_t* fileMJD);
void    writeTempFile(char* name, char* data, size_t size);
boolean readTempFile(char* name, char** data, size_t* size);
void    deleteTempFile(char* name);

#endif /* TEMPFILE_H_ */

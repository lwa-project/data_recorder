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
 * HostInterface.c
 *
 *  Created on: Aug 13, 2009
 *      Author: chwolfe2
 */

#include "HostInterface.h"
#include <stdio.h>
#include <string.h>

int issueShellCommand(char * command, char* result, size_t resultSizeLimit){
	if (command==NULL || result==NULL || resultSizeLimit==0) return -1;
	FILE *pipedOutput = popen(command, "r");
	size_t count=0;
	if (pipedOutput){
		while (!feof(pipedOutput)){
			result[count]=fgetc ( pipedOutput );
			if (result[count]==EOF) {
				result[count]=0;
				break;
			}
			count++;
			if (count == resultSizeLimit-1){
				while (!feof(pipedOutput)) fgetc ( pipedOutput ); // dump any remaining characters
			}
		}
	}
	result[count]='\0';
	return pclose(pipedOutput);

}
StatusCode ShellGetNumber(char * command, size_t* number){
	char buffer[2048];
		if (issueShellCommand(command, buffer, 2048) == -1) {
			return FAILURE;
		}
		if (strlen(buffer)==0){
			return FAILURE;
		}
		*number=strtoul(buffer,NULL,10);
		return SUCCESS;
}

StatusCode ShellGetString(char * command, char* result, size_t resultSizeLimit){
	if (issueShellCommand(command, result, resultSizeLimit) == -1)
		return FAILURE;
	else
		return SUCCESS;
}




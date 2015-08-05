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
 * Config.c
 *
 *  Created on: Nov 1, 2009
 *      Author: chwolfe2
 */

#include "DataFormats.h"
#include "Persistence.h"
#include "Config.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>


Database configDb;
boolean isConfigOpen=false;


void Config_Close(){
	if (isConfigOpen){
		Persistence_Close(&configDb);
		isConfigOpen=false;
	}
}

StatusCode Config_Initialize(boolean reloadConfig){
	if (!isConfigOpen){
		//printf("%s\n",(reloadConfig?"true":"false"));
		if (!reloadConfig){
			StatusCode sc = Persistence_Open(&configDb,CONFIGFILENAME);
			if (sc!=SUCCESS){
				sc = Persistence_Create(CONFIGFILENAME,CONFIGDEFAULTSFILENAME);
				if (sc!=SUCCESS){
					return sc;
				} else {
					sc=Persistence_Open(&configDb,CONFIGFILENAME);
					if (sc==SUCCESS){
						atexit(Config_Close);
						isConfigOpen=true;
					}
					return sc;
				}
			} else {
				atexit(Config_Close);
				isConfigOpen=true;
				return sc;
			}
		} else {
			StatusCode sc = Persistence_Create(CONFIGFILENAME,CONFIGDEFAULTSFILENAME);
			if (sc!=SUCCESS){
				return sc;
			} else {
				sc=Persistence_Open(&configDb,CONFIGFILENAME);
				if (sc==SUCCESS){
					atexit(Config_Close);
					isConfigOpen=true;
				}
				return sc;
			}
		}
	} else {
		return FAILURE;
	}
}

StatusCode Config_Get(char * param, char * result){
	assert(isConfigOpen);
	assert (param!=NULL &&  result!=NULL);
	return Persistence_Get_String(&configDb,param,result);
}

StatusCode Config_Set		 (char * param, char * value){
	assert(isConfigOpen);
	assert (param!=NULL &&  value!=NULL);
	return Persistence_Set_String(&configDb,param,value);
}

StatusCode Config_Finalize(){
	Config_Close();
	return SUCCESS;
}

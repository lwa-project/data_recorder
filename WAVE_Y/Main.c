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
 * Main.c
 *
 *  Created on: Oct 17, 2009
 *      Author: chwolfe2
 */
//#include <gdbm.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
//#include <sys/mman.h>
#include "Message.h"
#include "Commands.h"
//#include "Defines.h"
//#include "Disk.h"
#include "Time.h"
#include "Log.h"
//#include "FileSystem.h"
//#include "Socket.h"
//#include "RingQueue.h"
//#include "MessageQueue.h"
#include "Persistence.h"
#include "Schedule.h"
#include "Config.h"
#include "DataFormats.h"
#include "HostInterface.h"
#include "TempFile.h"

#define __IN_MAIN__
#include "Globals.h"
#undef  __IN_MAIN__


void Process_Messaging();
int isMessagingDone();

// general boot options
boolean BootOption_DeleteLog=false;			// -flushLog
boolean BootOption_DeleteSchedule=false;	// -flushSchedule
boolean BootOption_DeleteData=false;		// -flushData
boolean BootOption_ResetConfigData=false;	// -flushConfig
boolean ShutdownOption_Scram = false;
boolean ShutdownOption_Reboot = true;


typedef struct __statusInfo{
	boolean bootStarted;		size_t bootStartMJD;		size_t bootStartMPM;
	boolean bootComplete;		size_t bootCompleteMJD;		size_t bootCompleteMPM;
	boolean shutdownStarted;	size_t shutdownStartMJD;	size_t shutdownStartMPM;
	boolean shutdownComplete;	size_t shutdownCompleteMJD;	size_t shutdownCompleteMPM;
	boolean bootConsistent;
	boolean shutdownConsistent;
	boolean runtimeConsistent;
	boolean improperShutdownDetected;
}statusInfo;

// info regarding previous process invocation
statusInfo status_info;


void readStatusInfo(){
	status_info.bootStarted       = tempFileExists("BootStart",        &status_info.bootStartMJD,        &status_info.bootStartMPM);
	status_info.bootComplete      = tempFileExists("BootComplete",     &status_info.bootCompleteMJD,     &status_info.bootCompleteMPM);
	status_info.shutdownStarted   = tempFileExists("ShutdownStart",    &status_info.shutdownStartMJD,    &status_info.shutdownStartMPM);
	status_info.shutdownComplete  = tempFileExists("ShutdownComplete", &status_info.shutdownCompleteMJD, &status_info.shutdownCompleteMPM);
	status_info.bootConsistent    = (
			compareDates(
			 status_info.bootStartMJD, status_info.bootStartMPM,
			 status_info.bootCompleteMJD, status_info.bootCompleteMPM
			)<1
	);
	status_info.shutdownConsistent= (
			compareDates(
			 status_info.shutdownStartMJD, status_info.shutdownStartMPM,
			 status_info.shutdownCompleteMJD, status_info.shutdownCompleteMPM
			)<1
	);
	status_info.runtimeConsistent = (
			compareDates(
			 status_info.bootCompleteMJD, status_info.bootCompleteMPM,
			 status_info.shutdownStartMJD, status_info.shutdownStartMPM
			)<1
	);
	status_info.improperShutdownDetected = (
			!status_info.bootStarted ||
			!status_info.bootComplete ||
			!status_info.shutdownStarted ||
			!status_info.shutdownComplete ||
			!status_info.bootConsistent ||
			!status_info.shutdownConsistent ||
			!status_info.runtimeConsistent
	);
}

void clearTempFiles(){
	char cmd[8192]; char res[8192];
	sprintf(cmd,"rm -f %s*.temp",TEMP_FILE_LOCATION); issueShellCommand(cmd,res,8192);
	issueShellCommand(cmd,res,8192);
}

void setMarker(char * name){
	writeTempFile(name,NULL,0);
}
void clearMarker(char * name){
	deleteTempFile(name);
}


void ParseBootOptions(int argc, char* argv[]){
	// check last process invocation status
	readStatusInfo();
	clearTempFiles();
	// do command line argument parse
	int i = 1;
	while (i<argc){
		Log_Add("[BOOT] Processing option '%s'",argv[i]);
		if 			(strcmp(argv[i],"-flushLog")==0) 		BootOption_DeleteLog = true;
		else if 	(strcmp(argv[i],"-flushSchedule")==0) 	BootOption_DeleteSchedule = true;
		else if 	(strcmp(argv[i],"-flushData")==0) 		BootOption_DeleteData = true;
		else if 	(strcmp(argv[i],"-flushConfig")==0) 	BootOption_ResetConfigData = true;
		i++;
	}
}

int main(int argc, char* argv[]){

/* code to test process hosting
	const int sz=200;
	ProcessHandle* ph;
	StatusCode sc;
	char* comand = "ls -l /home/chwolfe2";
	char ls_output[sz];
	s =  LaunchProcess(comand, 1, sz, ls_output, &ph);
	if (s!=SUCCESS){
		printf("Can't launch process '%s'\n",comand);
	}
	while ((s= CheckProcessDone(ph))==NOT_READY){
		//wait...
	}
	if (s!=SUCCESS){
		printf("Process exited abnormally.\n");
	} else {
		printf("Process output for '%s'\n'%s'\n",comand,ls_output);
	}
	ReleaseProcessHandle(ph);
	exit(0);
//*/


	//////////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////////
	//
	//        @@@@@    @@@@   @@@@  @@@@@@
	//        @@  @@  @@  @@ @@  @@   @@
	//        @@@@@   @@  @@ @@  @@   @@
	//        @@  @@  @@  @@ @@  @@   @@
	//        @@@@@    @@@@   @@@@    @@
	//
	//////////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////////

	StatusCode sc;
	Disk 			disk;
	FileSystem 		fileSystem;
	MessageQueue 	messageQueue;
	MessageQueue 	responseQueue;
	RingQueue 		rxQueue;
	Socket 			rxMsgSocket;
	Socket 			txMsgSocket;
	Socket 			rxSocket;
	bzero((void *)&disk,			sizeof(Disk));
	bzero((void *)&fileSystem,		sizeof(FileSystem));
	bzero((void *)&messageQueue,	sizeof(MessageQueue));
	bzero((void *)&responseQueue,	sizeof(MessageQueue));
	bzero((void *)&rxQueue,			sizeof(RingQueue));
	bzero((void *)&rxMsgSocket,		sizeof(Socket));
	bzero((void *)&txMsgSocket,		sizeof(Socket));
	bzero((void *)&rxSocket,		sizeof(Socket));

	bzero((void*)&globals,sizeof(Globals));
	bzero((void *)&status_info,			sizeof(statusInfo));

	globals.disk			= &disk;
	globals.fileSystem		= &fileSystem;
	globals.messageQueue	= &messageQueue;
	globals.responseQueue	= &responseQueue;
	globals.rxQueue			= &rxQueue;
	globals.rxMsgSocket		= &rxMsgSocket;
	globals.txMsgSocket		= &txMsgSocket;
	globals.rxSocket		= &rxSocket;
	globals.flushConfigRequested 	= false;
	globals.flushDataRequested 		= false;
	globals.flushLogRequested 		= false;
	globals.flushScheduleRequested 	= false;
	globals.exitRequested			= false;
	globals.shutdownRequested 		= false;
	globals.rebootRequested 		= false;
	globals.scramRequested 			= false;


	DataFormat 			df;
/*	CompiledDataFormat 	cdf;*/
	Disk 				extdisk;
	MyFile				tagfile;
	bzero((void *)&df,			sizeof(DataFormat));
/*	bzero((void *)&cdf,			sizeof(CompiledDataFormat));*/
	bzero((void *)&extdisk,		sizeof(Disk));
	bzero((void *)&tagfile,		sizeof(MyFile));
/*	globals.opData.compiledFormat 		= &cdf;*/
	globals.opData.dataFormat			= &df;
	globals.opData.extdisk				= &extdisk;
	globals.opData.tagfile				= &tagfile;
	globals.opData.initialParameters	= NULL;
	globals.opData.errorCount 			= 0l;
	globals.opData.warningCount 		= 0l;


	globals.LocalEchoMessaging			= true;


	//parse command line options (and verify option/lock files)
	ParseBootOptions(argc,argv);
	setMarker("BootStart");
	Log_Add("[BOOT] Loading system configuration.");






	// load configuration
	sc=Config_Initialize(BootOption_ResetConfigData);
	if (sc!=SUCCESS){
		Log_Add("[BOOT] Fatal Error: Could not load system configuration. Exiting...");
		return EXIT_CODE_FAIL;
	}
	char temp[8192];
	sc=Config_Get("MessageInPort",temp);
	if (sc!=SUCCESS){Log_Add("[BOOT] Fatal Error: Could not determine configuration value for '%s'. Exiting...","MessageInPort");return EXIT_CODE_FAIL;}
	globals.MessageInPort = (unsigned short) strtoul(temp,NULL,10);

	Config_Get("MessageOutPort",temp);
	if (sc!=SUCCESS){Log_Add("[BOOT] Fatal Error: Could not determine configuration value for '%s'. Exiting...","MessageOutPort");return EXIT_CODE_FAIL;}
	globals.MessageOutPort = (unsigned short) strtoul(temp,NULL,10);

	Config_Get("DataInPort",temp);
	if (sc!=SUCCESS){Log_Add("[BOOT] Fatal Error: Could not determine configuration value for '%s'. Exiting...","DataInPort");return EXIT_CODE_FAIL;}
	globals.DataInPort = (unsigned short) strtoul(temp,NULL,10);

	Config_Get("MessageOutURL",globals.MessageOutURL);
	if (sc!=SUCCESS){Log_Add("[BOOT] Fatal Error: Could not determine configuration value for '%s'. Exiting...","MessageOutURL");return EXIT_CODE_FAIL;}

	Config_Get("MyReferenceDesignator", globals.MyReferenceDesignator);
	if (sc!=SUCCESS){Log_Add("[BOOT] Fatal Error: Could not determine configuration value for '%s'. Exiting...","MyReferenceDesignator");return EXIT_CODE_FAIL;}

	Config_Get("MySerialNumber", globals.MySerialNumber);
	if (sc!=SUCCESS){Log_Add("[BOOT] Fatal Error: Could not determine configuration value for '%s'. Exiting...","MySerialNumber");return EXIT_CODE_FAIL;}

	Config_Get("Version", globals.Version);
	if (sc!=SUCCESS){Log_Add("[BOOT] Fatal Error: Could not determine configuration value for '%s'. Exiting...","Version");return EXIT_CODE_FAIL;}

	Config_Get("TimeAuthority",globals.ntpServer);
	if (sc!=SUCCESS){Log_Add("[BOOT] Fatal Error: Could not determine configuration value for '%s'. Exiting...","TimeAuthority");return EXIT_CODE_FAIL;}

	Config_Get("ArraySelect",temp);
	if (sc!=SUCCESS){
		Log_Add("[BOOT] Fatal Error: Could not determine configuration value for '%s'. Default to 0...","ArraySelect");
		globals.arraySelect = 0;
	} else
		globals.arraySelect = (unsigned short) strtoul(temp,NULL,10);
	/********************************************************************************************************************************
	 ********************************************************************************************************************************
	  not ready to implement yet
	Config_Get("ArraySelectByName",globals.arraySelectByName);
	if (sc!=SUCCESS){
		Log_Add("[BOOT] Warning: Could not determine configuration value for 'ArraySelectByName'. Using integer value of ArraySelect to determine primary DRSU");
		Log_Add("[BOOT] Info: DRSU search order is 1st: Array specified by integer ArraySelect, and last: the first DRSU found");
		globals.useArraySelectByName=false;
	} else {
		Log_Add("[BOOT] Info: DRSU search order is 1st: DRSU with volume name '%s', 2nd: Array specified by integer ArraySelect, and last: the first DRSU found");
		globals.useArraySelectByName=true;
	}
	 ********************************************************************************************************************************
	 ********************************************************************************************************************************/



	char command[8192];
	char result[8192];
	//issueShellCommand("killall ntpd", result, 8192);
	sprintf(command, "ntpdate %s", globals.ntpServer);
	sc= ShellGetString(command, result, 8192);
	if (sc!=SUCCESS){
		Log_Add("[BOOT] Error: NTP synchronization process failed");
	} else {
		Log_Add("[BOOT] NTP synchronization result: '%s'",result);
	}


	// initialize the log
	Log_Add("[BOOT] Initializing the system log.");
	sc = Log_Initialize(BootOption_DeleteLog);
	if (sc!=SUCCESS){
		Log_Add("[BOOT] Fatal Error: Could not initialize system log. Exiting...");
		Config_Finalize();
		return EXIT_CODE_FAIL;
	}
	Log_Add("[BOOT] Opened System Log.");

	// report the status of last process invocation
	Log_Add("[BOOT] Inspecting last runtime status.");
	if (status_info.improperShutdownDetected){
		Log_Add("[BOOT] Improper shutdown detected");
		Log_Add("[BOOT]    boot started?           %s",((status_info.bootStarted) ? "yes" : "no"));
		Log_Add("[BOOT]    boot completed?         %s",((status_info.bootComplete) ? "yes" : "no"));
		Log_Add("[BOOT]    shutdown started?       %s",((status_info.shutdownStarted) ? "yes" : "no"));
		Log_Add("[BOOT]    shutdown completed?     %s",((status_info.shutdownComplete) ? "yes" : "no"));
		Log_Add("[BOOT]    boot tag time skew?     %s",((!status_info.bootConsistent) ? "yes" : "no"));
		Log_Add("[BOOT]    runtime tag time skew?  %s",((!status_info.runtimeConsistent) ? "yes" : "no"));
		Log_Add("[BOOT]    shutdown tag time skew? %s",((!status_info.shutdownConsistent) ? "yes" : "no"));
	}



	// initialize the drive devices
	Log_Add("[BOOT] Initializing the system's storage devices.");
	sc = Disk_IdentifyAll();
	if (sc!=SUCCESS){
		Log_Add("[BOOT] Fatal Error: Could not enumerate system storage devices.Exiting...");
		Log_Close();
		Config_Finalize();
		return EXIT_CODE_FAIL;
	}
	Log_Add("[BOOT] Initialized system's storage devices.");

	// find internal storage looking for /dev/md0, then /dev/md1, upto /dev/md5
	Log_Add("[BOOT] Locating internal storage among detected devices.");
	int arrayNum=0;
	boolean arrayFound=false;
	char arrayDeviceName[128];

	char arrayNames[10][128];
	unsigned short numArraysFound=0;
	Disk arrays[10];
	while (arrayNum < 6){
		sprintf(arrayDeviceName, "/dev/md%d", arrayNum);
		sc = Disk_GetDiskInfo(arrayDeviceName, &arrays[numArraysFound]);
		if (sc==SUCCESS){
			/*****************************************************************
			New detection semantics: external storage may be identified as /dev/mdX,
			So check to make sure the disk was identified as an internal storage device
			(actual categorization happens during enumeration, we only check category here)
			Note: old logic did not discriminate since md0..md6 were reserved for DRSUs
			*****************************************************************/
			if (arrays[numArraysFound].type == DRIVETYPE_INTERNAL_MULTIDISK_DEVICE){
				arrayFound=true;
				strncpy(arrayNames[numArraysFound],arrayDeviceName,128);
				numArraysFound++;
			} else{
				// do nothing
			}
		}
		arrayNum++;
	}
	Log_Add("[BOOT] Information: Located %hu internal storage devices (DRSUs).",numArraysFound);

	if (!arrayFound){
		Log_Add("[BOOT] Fatal Error: Could not locate internal storage among detected devices. Exiting...");
		Log_Close();
		Config_Finalize();
		return EXIT_CODE_FAIL;
	} else {
		globals.numArraysFound=numArraysFound;
	 /********************************************************************************************************************************
	 ********************************************************************************************************************************
		not ready to implement

		if (globals.useArraySelectByName){
			unsigned short tempArraySelect = findArrayIndexByLWAFSVolumeName(arrays,globals.numArraysFound,globals.arraySelectByName);
			if (tempArraySelect < globals.numArraysFound){
				Log_Add("[BOOT] Information: Located DRSU by volume name: '%s'.",globals.arraySelectByName);
				globals.arraySelect = tempArraySelect;
			} else {
				Log_Add("[BOOT] Warning: Could not locate DRSU by volume name: '%s'; defaulting to integer method of selection.",globals.arraySelectByName);
			}
		}
	  ********************************************************************************************************************************
	  ********************************************************************************************************************************/
		printf("Global array select is %d\n",globals.arraySelect);
		if (globals.arraySelect>=globals.numArraysFound){
			Log_Add("[BOOT] Error: boot config requested DRSU %hu, but only found %hu DRSUs. Defaulting to DRSU 1.",globals.arraySelect,globals.numArraysFound);
			globals.arraySelect=0;
		}
		globals.arrays = arrays;
		memcpy(&disk,&arrays[globals.arraySelect],sizeof(Disk));
		Log_Add("[BOOT] Identified internal storage device as '%s'",disk.deviceName);
	}

	// possibly create filesystem
	if (BootOption_DeleteData){
		Log_Add("[BOOT] Creating internal storage file system.");
		sc = FileSystem_Create(&disk);
		if (sc!=SUCCESS){
			Log_Add("[BOOT] Fatal Error: Could not create internal storage file system. Exiting...");
			Log_Close();
			Config_Finalize();
			return EXIT_CODE_FAIL;
		} else {
			Log_Add("[BOOT] File system created, all data deleted!!!");
		}
	}

	// open filesystem
	Log_Add("[BOOT] Opening internal storage file system.");
	sc = FileSystem_Open(&fileSystem, &disk);
	if (sc!=SUCCESS){
		Log_Add("[BOOT] Fatal Error: Could not open internal storage file system. Exiting...");
		Log_Close();
		Config_Finalize();
		return EXIT_CODE_FAIL;
	}
	Log_Add("[BOOT] Opened internal storage file system.");
	/*printf("globals.fileSystem = %lu \n",(size_t) globals.fileSystem);
	printf("globals.fileSystem->fileSystemHeaderData = %lu \n",(size_t) globals.fileSystem->fileSystemHeaderData);
	printf("globals.fileSystem->fileSystemHeaderData->filesystemInfo.chunkSize = %lu \n",(size_t) globals.fileSystem->fileSystemHeaderData->filesystemInfo.chunkSize);
*/
	// initialize data formats
	Log_Add("[BOOT] Opening data formats database.");
	sc = DataFormats_Initialize(BootOption_ResetConfigData);
	if (sc!=SUCCESS){
		Log_Add("[BOOT] Fatal Error: Could not read data formats database. Exiting...");
		FileSystem_Close(&fileSystem);
		Log_Close();
		Config_Finalize();
		return EXIT_CODE_FAIL;
	}
	Log_Add("[BOOT] Read data formats database.");


	// create messaging receive socket
	Log_Add("[BOOT] Creating messaging receive socket.");
	sc = Socket_Create(&rxMsgSocket);
	if (sc!=SUCCESS){
		Log_Add("[BOOT] Fatal Error: Could not create messaging receive socket. Exiting...");
		DataFormats_Finalize();
		FileSystem_Close(&fileSystem);
		Log_Close();
		Config_Finalize();
		return EXIT_CODE_FAIL;
	}
	Log_Add("[BOOT] Created messaging receive socket.");

	// create messaging send socket
	Log_Add("[BOOT] Creating messaging send socket.");
	sc = Socket_Create(&txMsgSocket);
	if (sc!=SUCCESS){
		Log_Add("[BOOT] Fatal Error: Could not create messaging send socket. Exiting...");
		Socket_Destroy(&rxMsgSocket);
		DataFormats_Finalize();
		FileSystem_Close(&fileSystem);
		Log_Close();
		Config_Finalize();
		return EXIT_CODE_FAIL;
	}
	Log_Add("[BOOT] Created messaging send socket.");

	// open messaging receive socket
	Log_Add("[BOOT] Opening messaging receive socket.");
	sc = Socket_OpenRecieve(&rxMsgSocket,globals.MessageInPort);
	if (sc!=SUCCESS){
		Log_Add("[BOOT] Fatal Error: Could not open messaging receive socket. Exiting...");
		Socket_Destroy(&txMsgSocket);
		Socket_Destroy(&rxMsgSocket);
		DataFormats_Finalize();
		FileSystem_Close(&fileSystem);
		Log_Close();
		Config_Finalize();
		return EXIT_CODE_FAIL;
	}
	Log_Add("[BOOT] Opened messaging receive socket.");

	// open messaging send socket
	Log_Add("[BOOT] Opening messaging send socket.");
	sc = Socket_OpenSend(&txMsgSocket,globals.MessageOutURL,globals.MessageOutPort);
	if (sc!=SUCCESS){
		Log_Add("[BOOT] Fatal Error: Could not open messaging send socket. Exiting...");fflush(stdout);
		Socket_Close(&rxMsgSocket);
		Socket_Destroy(&txMsgSocket);
		Socket_Destroy(&rxMsgSocket);
		DataFormats_Finalize();
		FileSystem_Close(&fileSystem);
		Log_Close();
		Config_Finalize();
		return EXIT_CODE_FAIL;
	}
	Log_Add("[BOOT] Opened messaging send socket.");fflush(stdout);

	// create messaging receive queue
	Log_Add("[BOOT] Creating messaging receive queue.");fflush(stdout);
	sc = MessageQueue_Create(&messageQueue,"/message");
	if (sc!=SUCCESS){
		Log_Add("[BOOT] Fatal Error: Could not create messaging receive queue. Exiting...");
		Socket_Close(&txMsgSocket);
		Socket_Close(&rxMsgSocket);
		Socket_Destroy(&txMsgSocket);
		Socket_Destroy(&rxMsgSocket);
		DataFormats_Finalize();
		FileSystem_Close(&fileSystem);
		Log_Close();
		Config_Finalize();
		return EXIT_CODE_FAIL;
	}
	Log_Add("[BOOT] Created messaging receive queue.");

	// create messaging send queue
	Log_Add("[BOOT] Creating messaging send queue.");
	sc = MessageQueue_Create(&responseQueue,"/response");
	if (sc!=SUCCESS){
		Log_Add("[BOOT] Fatal Error: Could not create messaging send queue. Exiting...");
		MessageQueue_Destroy(&messageQueue);
		Socket_Close(&txMsgSocket);
		Socket_Close(&rxMsgSocket);
		Socket_Destroy(&txMsgSocket);
		Socket_Destroy(&rxMsgSocket);
		DataFormats_Finalize();
		FileSystem_Close(&fileSystem);
		Log_Close();
		Config_Finalize();
		return EXIT_CODE_FAIL;
	}
	Log_Add("[BOOT] Created messaging send queue.");


	// create data receive socket
	Log_Add("[BOOT] Creating data receive socket.");
	sc = Socket_Create(&rxSocket);
	if (sc!=SUCCESS){
		Log_Add("[BOOT] Fatal Error: Could not create data receive socket. Exiting...");
		MessageQueue_Destroy(&responseQueue);
		MessageQueue_Destroy(&messageQueue);
		Socket_Close(&txMsgSocket);
		Socket_Close(&rxMsgSocket);
		Socket_Destroy(&txMsgSocket);
		Socket_Destroy(&rxMsgSocket);
		DataFormats_Finalize();
		FileSystem_Close(&fileSystem);
		Log_Close();
		Config_Finalize();
		return EXIT_CODE_FAIL;
	}
	Log_Add("[BOOT] Created data receive socket.");

	// create data receive queue
	Log_Add("[BOOT] Creating data receive queue.");
	sc = RingQueue_Create(&rxQueue,RECEIVE_QUEUE_SIZE);
	if (sc!=SUCCESS){
		Log_Add("[BOOT] Fatal Error: Could not create data receive queue. Exiting...");
		Socket_Destroy(&rxSocket);
		MessageQueue_Destroy(&responseQueue);
		MessageQueue_Destroy(&messageQueue);
		Socket_Close(&txMsgSocket);
		Socket_Close(&rxMsgSocket);
		Socket_Destroy(&txMsgSocket);
		Socket_Destroy(&rxMsgSocket);
		DataFormats_Finalize();
		FileSystem_Close(&fileSystem);
		Log_Close();
		Config_Finalize();
		return EXIT_CODE_FAIL;
	}
	Log_Add("[BOOT] Created data receive queue.");



	// open schedule
	Log_Add("[BOOT] Reading system schedule.");
	sc = Schedule_Open(BootOption_DeleteSchedule);
	if (sc!=SUCCESS){
		Log_Add("[BOOT] Fatal Error: Read system schedule. Exiting...");
		RingQueue_Destroy(&rxQueue);
		Socket_Destroy(&rxSocket);
		MessageQueue_Destroy(&responseQueue);
		MessageQueue_Destroy(&messageQueue);
		Socket_Close(&txMsgSocket);
		Socket_Close(&rxMsgSocket);
		Socket_Destroy(&txMsgSocket);
		Socket_Destroy(&rxMsgSocket);
		DataFormats_Finalize();
		FileSystem_Close(&fileSystem);
		Log_Close();
		Config_Finalize();
		return EXIT_CODE_FAIL;
	}

	Log_Add("[BOOT] Finished reading system schedule.");
	Log_Add("[BOOT] Booting Complete.");
	globals.upStatus=UP;


	setMarker("BootComplete");
	//////////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////////
	//
	//        @@@@  @@@@   @@@@@   @@@@@@  @@   @@      @@     @@@@   @@@@  @@@@
	//        @@ @@@@ @@  @@   @@    @@    @@@  @@      @@    @@  @@ @@  @@ @@ @@
	//        @@  @@  @@  @@@@@@@    @@    @@ @ @@      @@    @@  @@ @@  @@ @@@@
	//        @@      @@  @@   @@    @@    @@  @@@      @@    @@  @@ @@  @@ @@
	//        @@      @@  @@   @@  @@@@@@  @@   @@      @@@@@  @@@@   @@@@  @@
	//
	//////////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////////

	// Main control loop
	//globals.exitRequested=true;
	while(!globals.exitRequested || (globals.exitRequested && !isMessagingDone())){
		Process_Messaging();
		Process_Schedule();
	}

	//////////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////////
	//
	//      @@@@@@ @@   @@ @@   @@ @@@@@@      @@@@@   @@@@  @@   @@ @@   @@
	//      @@     @@   @@ @@   @@   @@        @@  @@ @@  @@ @@   @@ @@@  @@
	//      @@@@@@ @@@@@@@ @@   @@   @@        @@  @@ @@  @@ @@ @ @@ @@ @ @@
	//          @@ @@   @@ @@@ @@@   @@        @@  @@ @@  @@ @@@@@@@ @@  @@@
	//      @@@@@@ @@   @@  @@@@@    @@        @@@@@   @@@@   @@ @@  @@   @@
	//
	//////////////////////////////////////////////////////////////////////////////////
	//////////////////////////////////////////////////////////////////////////////////

	// close schedule
	setMarker("ShutdownStart");
	Log_Add("[SHUTDOWN] Closing system schedule.");
	sc = Schedule_Close();
	if (sc!=SUCCESS){
		Log_Add("[SHUTDOWN] Fatal Error: close system schedule. Exiting...");
		RingQueue_Destroy(&rxQueue);
		Socket_Destroy(&rxSocket);
		MessageQueue_Destroy(&responseQueue);
		MessageQueue_Destroy(&messageQueue);
		Socket_Close(&txMsgSocket);
		Socket_Close(&rxMsgSocket);
		Socket_Destroy(&txMsgSocket);
		Socket_Destroy(&rxMsgSocket);
		DataFormats_Finalize();
		FileSystem_Close(&fileSystem);
		Log_Close();
		Config_Finalize();
		return EXIT_CODE_FAIL;
	}
	Log_Add("[SHUTDOWN] Closed system schedule.");

	if (!globals.scramRequested){

		// destroy data receive queue
		Log_Add("[SHUTDOWN] Destroying data receive queue.");
		sc = RingQueue_Destroy(&rxQueue);
		if (sc!=SUCCESS){
			Log_Add("[SHUTDOWN] Fatal Error: Could not destroy data receive queue. Exiting...");
			Socket_Destroy(&rxSocket);
			MessageQueue_Destroy(&responseQueue);
			MessageQueue_Destroy(&messageQueue);
			Socket_Close(&txMsgSocket);
			Socket_Close(&rxMsgSocket);
			Socket_Destroy(&txMsgSocket);
			Socket_Destroy(&rxMsgSocket);
			DataFormats_Finalize();
			FileSystem_Close(&fileSystem);
			Log_Close();
			Config_Finalize();
			return EXIT_CODE_FAIL;
		}
		Log_Add("[SHUTDOWN] Destroyed data receive queue.");

		// destroy data receive socket
		Log_Add("[SHUTDOWN] Destroying data receive socket.");
		sc = Socket_Destroy(&rxSocket);
		if (sc!=SUCCESS){
			Log_Add("[SHUTDOWN] Fatal Error: Could not destroy data receive socket. Exiting...");
			MessageQueue_Destroy(&responseQueue);
			MessageQueue_Destroy(&messageQueue);
			Socket_Close(&txMsgSocket);
			Socket_Close(&rxMsgSocket);
			Socket_Destroy(&txMsgSocket);
			Socket_Destroy(&rxMsgSocket);
			DataFormats_Finalize();
			FileSystem_Close(&fileSystem);
			Log_Close();
			Config_Finalize();
			return EXIT_CODE_FAIL;
		}
		Log_Add("[SHUTDOWN] Destroyed data receive socket.");

		// destroy messaging send queue
		Log_Add("[SHUTDOWN] Destroying messaging send queue.");
		sc = MessageQueue_Destroy(&responseQueue);
		if (sc!=SUCCESS){
			Log_Add("[SHUTDOWN] Fatal Error: Could not destroy messaging send queue. Exiting...");
			MessageQueue_Destroy(&messageQueue);
			Socket_Close(&txMsgSocket);
			Socket_Close(&rxMsgSocket);
			Socket_Destroy(&txMsgSocket);
			Socket_Destroy(&rxMsgSocket);
			DataFormats_Finalize();
			FileSystem_Close(&fileSystem);
			Log_Close();
			Config_Finalize();
			return EXIT_CODE_FAIL;
		}
		Log_Add("[SHUTDOWN] Destroyed messaging send queue.");

		// destroy messaging receive queue
		Log_Add("[SHUTDOWN] Destroying messaging receive queue.");
		sc = MessageQueue_Destroy(&messageQueue);
		if (sc!=SUCCESS){
			Log_Add("[SHUTDOWN] Fatal Error: Could not destroy messaging receive queue. Exiting...");
			Socket_Close(&txMsgSocket);
			Socket_Close(&rxMsgSocket);
			Socket_Destroy(&txMsgSocket);
			Socket_Destroy(&rxMsgSocket);
			DataFormats_Finalize();
			FileSystem_Close(&fileSystem);
			Log_Close();
			Config_Finalize();
			return EXIT_CODE_FAIL;
		}
		Log_Add("[SHUTDOWN] Destroyed messaging receive queue.");

		// close messaging send socket
		Log_Add("[SHUTDOWN] Closing messaging send socket.");
		sc = Socket_Close(&txMsgSocket);
		if (sc!=SUCCESS){
			Log_Add("[SHUTDOWN] Fatal Error: Could not close messaging send socket. Exiting...");
			Socket_Close(&rxMsgSocket);
			Socket_Destroy(&txMsgSocket);
			Socket_Destroy(&rxMsgSocket);
			DataFormats_Finalize();
			FileSystem_Close(&fileSystem);
			Log_Close();
			Config_Finalize();
			return EXIT_CODE_FAIL;
		}
		Log_Add("[SHUTDOWN] Closed messaging send socket.");

		// close messaging receive socket
		Log_Add("[SHUTDOWN] Closing messaging receive socket.");
		sc = Socket_Close(&rxMsgSocket);
		if (sc!=SUCCESS){
			Log_Add("[SHUTDOWN] Fatal Error: Could not close messaging receive socket. Exiting...");
			Socket_Destroy(&txMsgSocket);
			Socket_Destroy(&rxMsgSocket);
			DataFormats_Finalize();
			FileSystem_Close(&fileSystem);
			Log_Close();
			Config_Finalize();
			return EXIT_CODE_FAIL;
		}
		Log_Add("[SHUTDOWN] Closed messaging receive socket.");

		// destroy messaging send socket
		Log_Add("[SHUTDOWN] Destroying messaging send socket.");
		sc = Socket_Destroy(&txMsgSocket);
		if (sc!=SUCCESS){
			Log_Add("[SHUTDOWN] Fatal Error: Could not destroy messaging send socket. Exiting...");
			Socket_Destroy(&rxMsgSocket);
			DataFormats_Finalize();
			FileSystem_Close(&fileSystem);
			Log_Close();
			Config_Finalize();
			return EXIT_CODE_FAIL;
		}
		Log_Add("[SHUTDOWN] Destroyed messaging send socket.");

		// destroy messaging send socket
		Log_Add("[SHUTDOWN] Destroying messaging receive socket.");
		sc = Socket_Destroy(&rxMsgSocket);
		if (sc!=SUCCESS){
			Log_Add("[SHUTDOWN] Fatal Error: Could not destroy messaging receive socket. Exiting...");
			DataFormats_Finalize();
			FileSystem_Close(&fileSystem);
			Log_Close();
			Config_Finalize();
			return EXIT_CODE_FAIL;
		}
		Log_Add("[SHUTDOWN] Destroyed messaging receive socket.");

		// close data formats database
		Log_Add("[SHUTDOWN] Closing data formats database.");
		sc = DataFormats_Finalize();
		if (sc!=SUCCESS){
			Log_Add("[SHUTDOWN] Fatal Error: Could not close data formats database. Exiting...");
			FileSystem_Close(&fileSystem);
			Log_Close();
			Config_Finalize();
			return EXIT_CODE_FAIL;
		}
		Log_Add("[SHUTDOWN] Closed messaging receive socket.");
	} // if "SCRAM"


	// close file system
	Log_Add("[SHUTDOWN] Closing internal storage file system.");
	sc = FileSystem_Close(&fileSystem);
	if (sc!=SUCCESS){
		Log_Add("[SHUTDOWN] Fatal Error: Could not close internal storage file system. Exiting...");
		Log_Close();
		Config_Finalize();
		return EXIT_CODE_FAIL;
	}
	Log_Add("[SHUTDOWN] Closed internal storage file system.");

	// close system log
	Log_Add("[SHUTDOWN] Closing system log.");
	Log_Close();


	// close config database
	Log_Add("[SHUTDOWN] Closing config database. ");
	sc = Config_Finalize();
	if (sc!=SUCCESS){
		Log_Add("[SHUTDOWN] Fatal Error: Could not close config database. Exiting... ");
		return EXIT_CODE_FAIL;
	}
	setMarker("ShutdownComplete");
	#define MAX_OPTS_COUNT 10
	char * opts[MAX_OPTS_COUNT];
	bzero((void*)opts,MAX_OPTS_COUNT*sizeof(char*));
	int argc_n=0;
	if (true)							opts[argc_n++] = argv[0];
	if (globals.flushConfigRequested) 	opts[argc_n++] = "-flushConfig";
	if (globals.flushDataRequested) 	opts[argc_n++] = "-flushData";
	if (globals.flushLogRequested) 		opts[argc_n++] = "-flushLog";
	if (globals.flushScheduleRequested) opts[argc_n++] = "-flushSchedule";
	if (!globals.shutdownRequested || (globals.shutdownRequested && globals.rebootRequested)){
		Log_Add("[SHUTDOWN]  Shutdown? %s\tReboot? %s\tScram? %s",
				((globals.shutdownRequested)? "Yes" : " No"),
				((globals.rebootRequested)? "Yes" : " No"),
				((globals.scramRequested)? "Yes" : " No")
		);
		execv(argv[0],opts);
		Log_Add("[SHUTDOWN] Fatal Error: could not exec next DROS process, but INI, or SHT+reboot was specified");
	} else {
		Log_Add("[SHUTDOWN] Shutdown w/o reboot was specified, shutting down DROS");
	}
	return -1;
}

//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
//
//        @@@@@  @@@@@   @@@@   @@@@@  @@@@@@ @@@@@@ @@@@@@ @@@@@@ @@@@@@
//        @@  @@ @@  @@ @@  @@ @@@  @@ @@     @@     @@     @@     @@
//        @@@@@  @@@@@  @@  @@ @@      @@@@   @@@@@@ @@@@@@ @@@@   @@@@@@
//        @@     @@  @@ @@  @@ @@@  @@ @@         @@     @@ @@         @@
//        @@     @@  @@  @@@@   @@@@@  @@@@@@ @@@@@@ @@@@@@ @@@@@@ @@@@@@
//
//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
static BaseMessage messageJustIn;
static BaseMessage messageReadyToProcess;
static SafeMessage messageReadyToProcessConv;
static SafeMessage responseUnConv;
static BaseMessage response;
static BaseMessage responseReadyToSend;
static boolean processed = false;
static boolean busy_messageJustIn = false;
static boolean busy_messageReadyToProcess = false;
static boolean busy_responseReadyToSend = false;
static size_t bytesReceived;
static size_t bytesToSend;

void Process_Messaging(){
	StatusCode sc;

	if (!busy_messageJustIn){
		sc=Socket_Recieve(globals.rxMsgSocket,(char*)&messageJustIn, &bytesReceived);
		if (sc==SUCCESS){
			if ( (strncmp(messageJustIn.destination,globals.MyReferenceDesignator,MESSAGE_FIELDSIZE_DESTINATION)==0) ||
				 (strncmp(messageJustIn.destination,"ALL",MESSAGE_FIELDSIZE_DESTINATION)==0)){
				busy_messageJustIn=true;
			}
		}
	}
	if (busy_messageJustIn){
		sc=MessageQueue_Enqueue(globals.messageQueue,&messageJustIn);
		if (sc==SUCCESS){
			bzero((void*)&messageJustIn,sizeof(messageJustIn));
			busy_messageJustIn=false;
		} /*else {
			printf("sc= %s\n", ((sc==FAILURE)?"FAILURE":"NOT_READY"));
		}*/
	}
	if (!busy_messageReadyToProcess){
		sc=MessageQueue_Dequeue(globals.messageQueue,&messageReadyToProcess);
		if (sc==SUCCESS){
			//((char*)&messageReadyToProcess)[8191]='\0';
			//printf("%s\n",(char*)&messageReadyToProcess);
			busy_messageReadyToProcess=true;
		}
	}

	if (busy_messageReadyToProcess){
		if (!processed){
			MessageConvertBaseSafe(&messageReadyToProcess,&messageReadyToProcessConv);
			//printf("RefSafe %s  vs.   RefBase %s",messageReadyToProcessConv.reference,messageReadyToProcess.reference);
			//if (Message_Verify(&messageReadyToProcess)){
				sc = Command_Process(&messageReadyToProcessConv,&responseUnConv);
				//printf("Command_Process(&messageReadyToProcessConv,&responseUnConv) = %s\n",statusString(sc));fflush(stdout);
				if (sc!=SUCCESS){
					Log_Add("MESSAGING: Error: could not process message.");
					//TODO: when messaging fails
				}
				processed=true;
				//printf("Processed\n");
			/*} else {
				sc = GenerateErrorResponse(&messageReadyToProcess,&response,ERROR_CODE_ILLEGAL_CHARACTERS,"");
				if (sc!=SUCCESS){
					Log_Add("MESSAGING: Error: could not process message.");
					//TODO: when messaging fails
				}
				processed=true;
			}*/
		}
		MessageConvertSafeBase(&responseUnConv,&response);
		sc=MessageQueue_Enqueue(globals.responseQueue,&response);
		if (sc==SUCCESS){
			bzero((void*)&messageReadyToProcess,sizeof(messageReadyToProcess));
			bzero((void*)&response,sizeof(response));
			busy_messageReadyToProcess=false;
			processed=false;
		}

	}
	if (!busy_responseReadyToSend){
		sc=MessageQueue_Dequeue(globals.responseQueue,&responseReadyToSend);
		if (sc==SUCCESS){
			busy_responseReadyToSend=true;
		}
	}
	if (busy_responseReadyToSend){
		bytesToSend = Message_TransmitSize(&responseReadyToSend);
		sc=Socket_Send(globals.txMsgSocket, (char *)&responseReadyToSend, bytesToSend);
		if (sc==SUCCESS){
			char buffer[256];
			snprintf(buffer,255,"%s",(char*)&responseReadyToSend);
			buffer[255]='\0';
			//Log_Add("MESSAGING: Responded with: '%s'",buffer);
			busy_responseReadyToSend=false;
		}
	}
}
int isMessagingDone(){
	//printf ("%d   %d   %d\n", busy_messageJustIn , busy_messageReadyToProcess , busy_responseReadyToSend);
	return !(busy_messageJustIn || busy_messageReadyToProcess || busy_responseReadyToSend);
}








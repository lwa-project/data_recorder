/*
  MCS-DROS: Monitor and Control System - Data Recorder Operating Software
  Copyright (C) 2009-2011  Virginia Tech, Christopher Wolfe <chwolfe2@vt.edu>

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
Software: MCS-DR Operating Software
Author:    C.N. Wolfe
Version:  DROS-1.5.0-Beta-Release
Date:     2012-06-22

Introduction
============

 This archive contains the MCS-DR Operating Software (MCS-DROS or DROS). DROS is designed to 
 accept commands from MCS to record and later retrieve data provided by DP. In this tarball
 exists all of the souce for the executable, as well as make files needed to build it. 
 
 The application is developed in C and is mostly ANSI C compliant barring the occasional 
 mixing of declarations and code. The final release version will be 100% ANSI C compliant.
 The application is developed under Eclipse 3.4 (Ganymede), and this archive includes the 
 .project and .cproject files required to import the project back into eclipse. 
 
 DROS is designed exclusively for 64-bit linux architectures, and will not compile for 32-bit 
 targets. This design is targeted for Ubuntu Linux 9.04 x86_64, Desktop, and no other targets
 have been tested. Although the software should build for any 64-bit linux target, this is 
 not guaranteed. 
 
 DROS requires two libraries to be installed to build: libgdbm and librt. libgdbm provides
 basic database-like capabilities and fast hashed lookup of associative key-value pairs, and 
 librt provides asynchronous disk access. Under Ubuntu, "apt-get install libgdbm3 libgdbm-dev"
 installs the required libgdbm, and librt is installed by default.
 
 The DROS contained in this tarball is not the same as the code used to test & verify in 
 LWA memo 165. This software differs in that a good portion of the ICD has been implemented 
 on top of the existing data and message processing mechanisms. Further, the software has 
 been cleaned to remove portions only useful in testing and development.



Limitations
===========
 DROS is currently under development, and the version provided here is 'beta'. Remaining tasks 
 are mostly cosmetic. Version DROS-1.5.0-Beta-Release is ready with the following exceptions:
  <<none>>
 
 
Build instructions
==================
  The DROS executable may be built by untarring, entering the Release folder, and issuing make:
    tar -xvf ./>>>ARCHIVE-NAME<<<.tar.gz 
    cd WAVE_Y/Release/
    touch ../*
    make
  The DROS executable may be also be built by untarring, importing the project into Eclipse (Ganymede),
  and choosing "Build Project" from eclipse's menu system.
  
Usage Instructions
==================
    For detailed usage instructions, please see ExtendedUsersGuide.txt in the DROS-1.5.0-Beta-Release tarball.
    
    Minimally, extract and build the tarball, and start the DROS software by:
    >sudo /LWA/bin/WAVE_Y -flushLog -flushData -flushSchedule -flushConfig

    and you should see something similar to:
        055329-068181529 : [BOOT] Processing option '-flushLog'
        055329-068181529 : [BOOT] Processing option '-flushData'
        055329-068181529 : [BOOT] Processing option '-flushSchedule'
        055329-068181529 : [BOOT] Processing option '-flushConfig'
        055329-068181529 : [BOOT] Loading system configuration.
        055329-068181564 : [DATABASE] Database File Created: '/LWA/database/configDb.gdbm'
        055329-068181564 : [DATABASE] Opening default values file: '/LWA/config/defaults.cfg'
        055329-068181564 : [DATABASE] Now filling database with default values.
        055329-068181568 : [DATABASE] Finished initializing database.   Processed 30 lines   Added 10 records
        055329-068181568 : [DATABASE] Database File Opened: '/LWA/database/configDb.gdbm'
        ntpd: no process killed
        055329-068182089 : [BOOT] NTP synchronization result: '13 May 14:56:22 ntpdate[5931]: adjust time server 91.189.94.4 offset -0.001385 sec'
        055329-068182089 : [BOOT] Initializing the system log.
        055329-068182090 : [DATABASE] Database File Created: '/LWA/database/logDb.gdbm'
        055329-068182090 : [DATABASE] Database File Opened: '/LWA/database/logDb.gdbm'
        055329-068182090 : [BOOT] Opened System Log.
        055329-068182090 : [BOOT] Initializing the system's storage devices.
        
        [ Additional output removed ]
        
        055329-068182354 : [BOOT] Created messaging send socket.
        055329-068182355 : [BOOT] Opening messaging receive socket.
        055329-068182355 : [BOOT] Opened messaging receive socket.
        055329-068182355 : [BOOT] Opening messaging send socket.
        055329-068182356 : [BOOT] Opened messaging send socket.
        055329-068182356 : [BOOT] Creating messaging receive queue.
        055329-068182357 : [BOOT] Created messaging receive queue.
        055329-068182357 : [BOOT] Creating messaging send queue.
        055329-068182358 : [BOOT] Created messaging send queue.
        055329-068182358 : [BOOT] Creating data receive socket.
        055329-068182359 : [BOOT] Created data receive socket.
        055329-068182359 : [BOOT] Creating data receive queue.
        055329-068182762 : [BOOT] Created data receive queue.
        055329-068182762 : [BOOT] Reading system schedule.
        055329-068182764 : [DATABASE] Database File Created: '/LWA/database/scheduleDb.gdbm'
        055329-068182765 : [DATABASE] Database File Opened: '/LWA/database/scheduleDb.gdbm'
        055329-068182770 : [DATABASE] Database File Opened: '/LWA/database/scheduleDb.gdbm'
        055329-068182770 : [BOOT] Finished reading system schedule.
        055329-068182771 : [BOOT] Booting Complete.
    
    With the last line indicating that the MCS-DR software is up and running.     
 

Additional Files required
=========================
 
 DROS expects to be installed to a specific path, and expects to find additional support files 
 at specified locations. 
 
 /LWA/scripts		Contains the launch.sh host script. see sec. "Host Script"
 /LWA/config		Contains defaults.cfg, and formats.cfg
 /LWA/bin		Contains the DROS binary "WAVE_Y" 
 /LWA/database           Folder to hold runtime information such as log and scheduling databases.
                         Runtime files are created automatically if missing.
 
   defaults.cfg
   ============
     This file is used to configure the MCS-DROS on first boot and/or at the issuance of an INI command
     The following is a sample config file. Note that the values are those used in development,
     and not suitable for the LWA shelter network environment.
     
     /-----------------------------------------------------------------------------------------
     |    # system IP
     |    SelfIP 192.168.1.20
     |    
     |    # system reference Identifier
     |    MyReferenceDesignator MD1
     |    
     |    # udp port to listen for messages
     |    MessageInPort 5001
     |    
     |    # udp port to send responses to (presumably MCS)
     |    MessageOutPort 5000
     |    
     |    # URL to send responses to (presumably MCS)
     |    MessageOutURL 192.168.1.20
     |    
     |    # udp port to listen for data
     |    DataInPort 6002
     |    
     |    # ntp time server ip address or URL
     |    TimeAuthority ntp.ubuntu.com
     |    
     |    # DROS software version number
     |    Version 1.0(beta)
     |    
     |    # MCS-DR serial number (currently just a place holder until devices are assigned)
     |    MySerialNumber XPS-435-MT-(Development-Machine-0)
     \-----------------------------------------------------------------------------------------
     
  
  formats.cfg
  ===========
     This file is used to describe the supported data formats of the MCS-DROS on first boot and/or 
     at the issuance of an INI command. The following is a sample config file. Note that the values 
     are those used in development, and not necessarily the final values. (i.e., this is to demonstrate
     the format, not specify content)
     /-----------------------------------------------------------------------------------------
     |                                                                                         
     | # number of formats                                                                     
     | FORMAT-COUNT            3                                                               
     |                                                                                         
     | # format 0: default full TBN                                                            
     | FORMAT-NAME-0           DEFAULT_TBN                                                     
     | FORMAT-RATE-0           117440512                                                       
     | FORMAT-PAYLOAD-0        1024                                                            
     |                                                                                         
     | # format 1: default full TBW                                                            
     | FORMAT-NAME-1           DEFAULT_TBW                                                     
     | FORMAT-RATE-1           104857600                                                       
     | FORMAT-PAYLOAD-1        1224                                                            
     |                                                                                         
     \-----------------------------------------------------------------------------------------
 

File Inventory
==============
 readme.txt              This file.
 
 .project                Eclipse-specific project settings file
 .cproject               Eclipse-specific project settings file
 /Release/makefile       makefile
 /Release/objects.mk     file included by makefile during make
 /Release/sources.mk     file included by makefile during make
 /Release/subdir.mk      file included by makefile during make
 
 Main.c                  performs initialization, main loop, message rx/tx loop, shutdown
 Defines.h               defines several constants and macros used throughout
 Globals.h               defines several globals which are frequently required
 
 Message.c               Wrapper functions and data structure definitions 
 MessageQueue.c          for POSIX features and data buffering mechanisms
 Socket.c
 RingQueue.c
 Message.h
 MessageQueue.h
 RingQueue.h
 Socket.h
 
 Persistence.c           Wrappers for libgdbm interface for associative arrays
 Persistence.h           and persistent data
 
 
 DataFormats.h           persistent data formats records
 DataFormats.c
 Log.h                   persistent system log
 Log.c
 Config.h                persistent configuration data
 Config.c
 Schedule.h              persistent scheduling, and schedule processing
 Schedule.c
 
 Commands.c              command message processing (includes RPT, which is passed to MIB.c)
 Commands.h
 
 Operations.c            handles the execution of commands which must be scheduled or 
 Operations.h            can't be executed in the 3 seconds allotted to message-response 
                         time. (REC,CPY,DMP,TST,SYN,FMT, etc.)
 
 MIB.c                   monitor and control points (MIB) processing
 MIB.h
 
 Disk.c                  partition recognition, formatting, mounting
 Disk.h
 
 FileSystem.c            internal storage file system and file functions
 FileSystem.h
 
 Time.c                  functions for dealing with and calculating MJD and MPM from system time,
 Time.h                  as well as various timing mechanisms and utility functions.
 
 HostInterface.h         functions for interacting with the host operating system via
 HostInterface.c         child process execution
    
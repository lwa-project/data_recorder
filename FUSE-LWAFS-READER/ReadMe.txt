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

Software: MCS-DR Operating Software utility: DRSU Reader Software (LWAFS volume mounter)
Author:    C.N. Wolfe
Version:  DROS-1.5.0-Beta-Release
Date:     2012-06-22

Introduction
============
  The FUSE-LWAFS-READER is a userspace tool to access a DRSU's contents from the linux command line.
  FUSE-LWAFS-READER mounts a given DRSU to a mountpoint similar to the way an ext2 partitions is mounted.
  This allows common command-line utilities and scripts to access previously made recordings. The 
  primary purpose of this utility is to allow off-line processing of recorded data. This utility cannot 
  be used while DROS is running on the same machine as this would generate contention and synchronization 
  errors.   
  

Assumptions
===========

  To use this utility, you must have built and installed the FUSE-LWAFS-READER software. Also,
  you must have prepared a DRSU in compliance with LWA memo 0019 and created an array in
  accordance with the QuickStart.txt guide. Also, if you wish to transfer files, you must have 
  a suitable additional storage device attached/formatted/mounted. 

NOTICE
======
  This tool cannot be used on an MCS-DR without interfering with DROS. 
  This tool cannot be used on 32-bit platforms, and not on distros other than Ubuntu Desktop x86_64 <9.04 (most likely). 
  

Build instructions
==================
  This utility is automatically built and installed by in the process of building and installing the tarball. 
  To build just this package, the LWAFS reader executable may be built by untarring, entering the Release 
  folder, and issuing make:
    >tar -xvf ./<Tarballl Filename>.tar.gz 
    >cd FUSE-LWAFS-READER/Release/
    >touch ../*
    >make
  The LWAFS reader executable may be also be built by untarring, importing the project into Eclipse (Ganymede),
  and choosing "Build Project" from eclipse's menu system. 

	
Usage and Limitations
=====================
    
    Mounting the DRSU
    =================
    
     To invoke the tool, specify the following at the command line:
         >sudo /LWA/bin/FUSE-LWAFS-READER -o direct_io,max_read=262144,max_write=262144,sync_read,device=<DRSU> -s -f <Mount Point>
     
     Where:
         <DRSU>          is the raid array block device of the DRSU. (Typically "/dev/md0")
         <Mount Point>   is an empty folder in which to mount the DRSU
                         The preferred location of this mountpoint is "/LWA/mounts/DRSUxxx" on MCS-DRs, but 
                         any empty directory entry will suffice.
     
     This will launch the filesystem mounter in foreground mode, which is necessary to view debugging output. 
     Running the application in background (remove the "-f" flag) mode should work, but hasn't been tested, and
     reduces visibility in the event that an error occurs. 
 
    
    Accessing DRSU Contents
    =======================
    
     To use the application, open ***another*** terminal, and use appropriate command line tools to navigate
     and operate on DRSU recordings. For instance, to list the files in the DRSU
	    >ls -la <Mount Point>
	    
     Output (not shown) should be a standard file listing, all filenames being of the format XXXXXX_YYYYYYYYY
     where X is the MJD the recording was scheduled on, and Y is the reference number of the command which 
     requested the scheduling of the recording.
    
    Disconnecting / Un-mounting
    ===========================
    
    To unmount the DRSU, in another terminal:
       >umount <Mount Point>
    Pressing CTRL+C in the terminal used to mount the DRSU or sending the kill signal to FUSE-LWAFS-READER will 
    also work, but may result in data loss or corruption. 
    
    
    Other files and file types
    ==========================
    
    The FUSE-LWAFS-READER is capable of creating arbitrary files, and being used as you would any other file system.
    However, adding additional files to the file system is not recommended, as the DROS software
    expects certain meta-data to be present. Since many of the files involved will be hundreds of gigabytes, or 
    even terabytes, using the free space of a DRSU for temporary storage while processing may be required. In such
    circumstances, be sure to remove all non-recordings before using the DRSU with an MCS-DR. Also note that 
    copies of recordings will not bear the metadata associated with the original file. In the future, this may be
    changed, but for the present, the only way to retrieve file metadata is in-situ connected to an MCS-DR via the
    ICD interface  
    
    Caveats and Performance Issues
    ==============================
    
    * only supports 8191 files.
    * no folders, all files exist in the root directory of the DRSU
    * nonblocking calls silently default to blocking, so glibc-based apps should be aware of this
    * file allocations/resizing are geared for large files, so performance is degraded for small files.
    * overall performance of filesystem is ~20% of the recording speed ( about 20-30 MiB/s ).
    * opening and closing files is expensive, so speed-conscious applications should "prefer" to keep files open.
    

File Inventory
==============
ReadMe.txt              This file.

.project                Eclipse-specific project settings file
.cproject               Eclipse-specific project settings file
/Release/makefile       makefile
/Release/objects.mk     file included by makefile during make
/Release/sources.mk     file included by makefile during make
/Release/subdir.mk      file included by makefile during make

fuse-lwafs.c            performs initialization, main loop, message rx/tx loop, shutdown
Defines.h               defines several constants and macros used throughout

Disk.c                  partition recognition, formatting, mounting
Disk.h

FileSystem.c            internal storage file system and file functions
FileSystem.h

Time.c                  functions for dealing with and calculating MJD and MPM from system time,
Time.h                  as well as various timing mechanisms and utility functions.

HostInterface.h         functions for interacting with the host operating system via
HostInterface.c         child process execution
       

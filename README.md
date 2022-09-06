
		========================================
		Quick Start User's guide for MCS-DR/DRSU
		========================================

====================================================================================
Version: DROS-2.3.0.1-RELEASE
Date:    2013-06-23
Author:  Christopher Wolfe
====================================================================================

DROS Versions and ICD Version
=============================
  This quick start applies to DROS-2.3.0.1-RELEASE. For additional information about versions and 
  features, please refer to the extended user's guide 'ExtendedUsersGuide.txt'.

Prerequisites
=============
  1) MCS-DR PC with x86_64 Ubuntu 12.04 
  2) the following packages are installed: libgdbm3, libgdbm-dev, libfuse2, 
      libfuse-dev, lmsensors, smartmontools, mdadm, libfftw3-dev, fftw3,
      libboost-all-dev, gnuplot
  3) gnu C/C++ compiler (stdlib, pthreads, librt)
  4) DRSU (assembled IAW "LWA Engineering Memo MCS0019")
  5) Drivers for the Myricom 10GbE adapter have been built and installed


Files and Organization
======================
  The top level of this tar-ball contains this file 'QuickStart.txt', an extended user's 
  guide 'ExtendedUsersGuide.txt', and a 'build' folder.  Each folder under 'build' contains 
  the source and makefile for each of the binaries included in this tar-ball. For more 
  information on the all of the files and organization, see the extended user's guide.


Step 0: Root Access
===================
  You must be root for most of the tasks outlined here. The DROS software needs 
  root permission to work directly with partitions, so 'sudo su' before doing 
  anything is easiest. Note that building the software can be done in user space, 
  but installation requires root privileges. Consequently, you will need login 
  information:
  The default username and password for MCS-DR PCs is 
	mcsdr / mcsdrrdscm
  but may be 
	chwolfe2 / cnw112177.
  on earlier machines (first PC on-site at LWA1 and PC sent to JPL)
  
  At the bash prompt, type:
	>sudo su
  and enter the appropriate password.
  
  For any additional terminals you open, repeat this step.
	


Step 1: Installation
====================
  Note: Building the 'install' target will overwrite all files in the /LWA folder,
        so be sure to backup configuration files before building.

  At the bash prompt, type:
	>tar -xvf <tar-ball-file-name>
	>cd <tar-ball-file-name-minus-'.tar.gz'>/build
	>make all && make install

  This should generate a lot of output ending with:

	################################################################
	# Notice:
	# 
	# MCS-DR software has been installed to:
	#       /LWA
	# 
	# The default configuration files are installed to:
	#       /LWA/config/
	# 
	# You must modify defaults.cfg.example to reflect your network environment.
	# 
	# To launch the software, execute:
	#       /LWA/bin/DROS
	# 
	# To install the software to run on-boot, execute:
	#       cd /LWA/scripts; ./installStartupScript.sh /LWA/scripts/StartDROS.sh
	# 
	################################################################



Step 2: System Configuration
============================
  A) Determine which linux device is the 10Gbe Adapter, and which is 1GbE
	>cat /etc/udev/rules.d/70-persistent-net.rules:
     The device with module name 'myri10Ge' is the 10GbE adapter, others are 1GbE
     
  B) Edit /etc/network/interfaces and /etc/resolv.conf to suit site network environment.
  	>nano /etc/network/interfaces
	>nano /etc/resolv.conf

  C) Edit /LWA/config/defaults_v2.cfg to reflect configuraiton in step 2 section B (above), as well.
     as machine reference designator, serial number, etc.

Step 3: DRSU preparation
========================
  This command will create a DRSU array for the most likely partition naming. For
  more complete description, see the extended user's guide.
  A) Create the array:
     Note: where b-f are your root partitions to add to the array.
	>sudo mdadm -C /dev/md0 -C 256 -l 0 -n 5 /dev/sd{b,c,d,e,f}     
     
     You should see something along the lines of 
	mdadm: /dev/md0 has been started with 5 drives.
     
     Next:
	>cp /etc/mdadm/mdadm.conf /etc/mdadm/mdadm.conf.QS.bak
  	>mdadm --examine --scan > /etc/mdadm.conf
  
  B) Format the DRSU
   	>/LWA/scripts/StorageControl.sh format /dev/md0 MyDrsuBarcode

     You should see something similar to:
	mke2fs 1.42 (29-Nov-2011)
	Filesystem label='myDrsuBarcode'
	OS type: Linux
	Block size=4096 (log=2)
	Fragment size=4096 (log=2)
	Stride=64 blocks, Stripe width=320 blocks
	1192336 inodes, 2441893120 blocks
	122094656 blocks (5.00%) reserved for the super user
	First data block=0
	Maximum filesystem blocks=4294967296
	74521 block groups
	32768 blocks per group, 32768 fragments per group
	16 inodes per group
	Superblock backups stored on blocks: 
		32768, 98304, 163840, 229376, 294912, 819200, 884736, 1605632, 2654208, 
		4096000, 7962624, 11239424, 20480000, 23887872, 71663616, 78675968, 
		102400000, 214990848, 512000000, 550731776, 644972544, 1934917632

	Allocating group tables: done                            
	Writing inode tables: done                            
	Writing superblocks and filesystem accounting information: 
	Complete!!!
	To mount the new DRSU, type 'mount -t ext4 -o defaults,data=writeback,noatime,barrier=0 /dev/md0 <mountpoint>' Or Use this script to enable use with DROS.


Step 4: First Run
=================
  Normally, the DROS binary is launched from a rc.d script, but the first
  time it is run, I recommend launching it directly. 
  So, at the command line type:
	>/LWA/bin/DROS

  The following will be displayed:
	  ========================== Logfile rotated ==========================
	  ========================== Logfile opened ==========================
	  FATAL: Error inserting f71882fg (/lib/modules/3.2.0-30-generic/kernel/drivers/hwmon/f71882fg.ko): No such device
	  2012-11-09 23:09:24.415 [I] message originating in      ../System/System.cpp:134
				  [I] [System          ] ======================= Active Thread Report =======================
				  [I] [System          ] [   0]: tid = 13279
				  [I] [System          ] ===================== End Active Thread Report =====================
	  2012-11-09 23:09:24.416 [I] [System          ] [System] Booting Start
	  2012-11-09 23:09:24.416 [I] [System          ] [System] Hardware CPU count: 8
	  2012-11-09 23:09:24.416 [I] [System          ] [System] Read config...
	  2012-11-09 23:09:24.416 [I] [System          ] Configuration data loaded...
	  2012-11-09 23:09:24.416 [D] message originating in      ../System/Config.cpp:98
				  [D] [System          ] MyReferenceDesignator         DR1
				  [D] [System          ] SelfIp                        192.168.1.20
				  [D] [System          ] MessageInPort                 5000
				  [D] [System          ] MessageOutPort                5001
				  [D] [System          ] TimeAuthority                 Time.Ubuntu.com
				  [D] [System          ] SerialNumber                  0
				  [D] [System          ] ArraySelect                   1
				  [D] [System          ] DataInPort                    16180
	  2012-11-09 23:09:24.417 [I] [System          ] [System] Scan storage devices...
	  2012-11-09 23:09:24.417 [I] [System          ] Beginning storage subsystem initialization.

< will pause here for a few minutes >
	  
...
	  2012-11-09 23:10:23.589 [D] [System          ] [WaitQueueManager] Registered queue 'MessageProcessor'.
	  2012-11-09 23:10:23.589 [I] [System          ] [System] Start message receiver...
	  2012-11-09 23:10:23.589 [I] [System          ] [MessageListener] : Messaging socket opened on port5000
	  2012-11-09 23:10:23.589 [I] [System          ] [System] Determining system status...
	  2012-11-09 23:10:23.589 [I] [System          ] [System] system is 'NORMAL '
	  2012-11-09 23:10:23.589 [I] [System          ] [System] Booting Complete

Step 5: Test Messaging
======================
  At this point, the system is up and listening for command messages. 
  To test messaging, use the included Msender utility. 

  In a new terminal:
	>/LWA/bin/Msender -v -Source "MCS" -Destination "DR1" -Type "PNG" -ReferenceNumber 0 -DestinationIpAddress localhost -DestinationPort 5001 -ResponseListenPort 5000    

  On the original terminal you should see:
	  2012-11-09 23:16:03.934 [I] [MessageListener ] >> Message(56240:83763934, MCS, DR1, 1, PNG, 0, '' ) 127.0.0.1:39352
	  2012-11-09 23:16:03.935 [I] [ResponseSender  ] << Response(56240:83763935, DR1, MCS, 1, PNG, 8, Accept, NORMAL , '', '' ) 127.0.0.1:5001

  On the new terminal you should see:
	[send]Message :
	[send]	Sender:                     MCS
	[send]	Destination:                DR1
	[send]	Type:                       PNG
	[send]	Reference:                  0
	[send]	Modified Julian Day:        56240
	[send]	Milliseconds Past Midnight: 83763934
	[send]	Data Length:                0
	[send]	Data:                       No Data Specified

	[recv]Response :
	[recv]	Sender:                     DR1
	[recv]	Destination:                MCS
	[recv]	Type:                       PNG
	[recv]	Reference:                  0
	[recv]	Modified Julian Day:        56240
	[recv]	Milliseconds Past Midnight: 83763935
	[recv]	Accept/Reject:              Accepted
	[recv]	General Status:             NORMAL 
	[recv]	Comment:                    No Comment Specified
	[recv]	Data Length:                9
	[recv]	Data: 
	[recv]		<   0>:   41 4e 4f 52 4d 41 4c 20  0 

Step 6: Exit First Run
======================
  In the new terminal from step 5:
	>/LWA/bin/Msender -v -Source "MCS" -Destination "DR1" -Type "SHT" -ReferenceNumber 0 -DestinationIpAddress localhost -DestinationPort 5001 -ResponseListenPort 5000    
  In the DROS terminal you should see 
	...
	2012-11-09 23:22:42.869 [I] message originating in      ../System/Storage.cpp:142
				[I] [System          ] ============================================================
				[I] [System          ] ==       Storage Subsystem Report                         ==
				[I] [System          ] ============================================================
				[I] [System          ] ==                  -- Offline --                         ==
				[I] [System          ] ============================================================
				[I] [System          ] ==       End Report                                       ==
				[I] [System          ] ============================================================
	2012-11-09 23:22:42.869 [I] [System          ] [System] Shutting down config...
	2012-11-09 23:22:42.870 [I] [System          ] Configuration data saved...
	2012-11-09 23:22:42.870 [I] [System          ] [System] Shutdown Complete
	========================== Logfile closed ==========================
  
  and the DROS program will exit, returning control of the original terminal to you.

 
Step 7: Normal operation mode
=============================
  To install the DROS software to start on system boot, execute the following:
	>cd /LWA/scripts	
	>./installStartupScript.sh StartDROS.sh

  Terminal output should be as follows:
	update-rc.d: warning: /etc/init.d/StartDROS.sh missing LSB information
	update-rc.d: see <http://wiki.debian.org/LSBInitScripts>
	 Adding system startup for /etc/init.d/StartDROS.sh ...
	   /etc/rc0.d/K99StartDROS.sh -> ../init.d/StartDROS.sh
	   /etc/rc1.d/K99StartDROS.sh -> ../init.d/StartDROS.sh
	   /etc/rc6.d/K99StartDROS.sh -> ../init.d/StartDROS.sh
	   /etc/rc2.d/S99StartDROS.sh -> ../init.d/StartDROS.sh
	   /etc/rc3.d/S99StartDROS.sh -> ../init.d/StartDROS.sh
	   /etc/rc4.d/S99StartDROS.sh -> ../init.d/StartDROS.sh
	   /etc/rc5.d/S99StartDROS.sh -> ../init.d/StartDROS.sh


  Reboot, and verify DROS is responding to messaging as in Step 5. 
  The "Terminal" output of DROS can be found in '/LWA/runtime/runtime.log'
  For troubleshooting and more information, see the Extended User's Guide
  The "Terminal" output should be similar to the initial launch (in step 4).


Step 8: Scheduling a recording
==============================
  The format of scheduling a recording is as follows:
  	>/LWA/bin/Msender 
		-Source MCS 
		-Destination DR1 
		-Type REC 
		-ReferenceNumber 123456789 
		-DestinationIpAddress localhost 
		-DestinationPort 5000 
		-ResponseListenPort 5001 
		-Data "xxxxxx yyyyyyyyy zzzzzzzzz FFFFFFFFFFFFFF...FFF"
  which will request that a recording of 
	zzzzzzzzz milliseconds in length 
	starting on the day xxxxxx         xxxxxx is Modified Julian Day 
	at yyyyyyyyy milliseconds past midnight
	using data format specified by FFFFFFFFFFFFFF...FFF. 
  
  In order to process the request, it must be received at least 5-10 seconds before the 
  scheduled start time of the recording. 

  Select appropriate values for xxxxxx, yyyyyyyyy, zzzzzzzzz, and FFFFFFFFFFFFFF...FFF
  Note the FFFFFFFFFFFFFF...FFF values are defined in 'formats.cfg'

  In the new window from step 5, enter the following* (line breaks added for clarity):
	>/LWA/bin/Msender -s MCS -d DR1 -r 4 -I 127.0.0.1 -po 5000 -pi 5001 -v -t "REC" -D "MJD MPM+3000 30000 DEFAULT_TBN"
  
  In the second terminal, you should see:
	...
	[recv]	Modified Julian Day:        56240
	[recv]	Milliseconds Past Midnight: 13276752
	[recv]	Accept/Reject:              Accepted
	[recv]	General Status:             NORMAL 
	[recv]	Comment:                    055240_000000001
	[recv]	Data Length:                25
	[recv]	Data: 
	[recv]		<   0>:   41 4e 4f 52 4d 41 4c 20 30 35 35 32 34 30 5f 30 
	[recv]		<  10>:   30 30 30 30 30 30 30 31  0 
  
  This notifies us that the command was received and the the file 055240_000000001 will
  be used to hold the data.	

  In '/LWA/runtime/runtime.log', you should see:
	2012-11-09 23:54:23.979 [I] [MessageListener ] >> Message(56240:86063978, MCS, DR1, 4, REC, 34, '056240 086066978 30000 DEFAULT_TBN' ) 127.0.0.1:47029
	2012-11-09 23:54:23.980 [D] [Folder Watcher  ] Watched folder '/LWA_STORAGE/Internal/0/DROS/Rec' select returned 1
	2012-11-09 23:54:23.980 [D] [Folder Watcher  ] Watched folder re-read '/LWA_STORAGE/Internal/0/DROS/Rec'
	2012-11-09 23:54:23.980 [I] [MessageProcessor] [Scheduler] Scheduled operation 'Record      000000004 056240 086066978  056240 086096978  DEFAULT_TBN                     
	2012-11-09 23:54:23.981 [I] [ResponseSender  ] << Response(56240:86063980, DR1, MCS, 4, REC, 24, Accept, NORMAL , '056240 086066978 30000 DEFAULT_TBN', '056240_000000004' ) 127.0.0.1:5001
	2012-11-09 23:54:24.478 [I] [Schedule        ] [Schedule] window start current operation: Record      000000004 056240 086066978  056240 086096978  DEFAULT_TBN                     
	2012-11-09 23:54:24.478 [I] [Schedule        ] [RECORD] Starting recorder
	2012-11-09 23:54:24.478 [I] [Schedule        ] [RECORD] Recorder started
	2012-11-09 23:54:24.478 [I] [FileWriter:/ ...] [GenericThread] : Set CPU affinity : '2'
	2012-11-09 23:54:24.478 [I] [FileWriter:/ ...] [GenericThread] : Set priority : '-2'
	2012-11-09 23:54:24.478 [I] [FileWriter:/ ...] [RECORD] FileWriter thread started
	2012-11-09 23:54:24.478 [D] [FileWriter:/ ...] [TicketBuffer:MasterReceiveBuffer] <SUBSCRIBE:FileWriter:/LWA_STORAGE/Internal/0/DROS/Rec/056240_000000004>
	2012-11-09 23:54:26.978 [I] [Schedule        ] [Schedule] execution start current operation: Record      000000004 056240 086066978  056240 086096978  DEFAULT_TBN                     
	2012-11-09 23:54:26.978 [I] [FileWriter:/ ...] [RECORD] FileWriter record started
  Sometime later you should see:
	2012-11-09 23:54:50.924 [I] [Receiver        ] [Receiver] Buffer usage: [________________________________________] (  0%)
	2012-11-09 23:54:50.925 [I] [Receiver        ] [Receiver] Receive rate: [________________________________________] (  0%)  0.00000 B/s  
	2012-11-09 23:54:50.925 [I] [Receiver        ] [Receiver] Subscribers 1
	2012-11-09 23:54:56.997 [I] [Schedule        ] [Schedule] execution stop current operation: Record      000000004 056240 086066978  056240 086096978  DEFAULT_TBN                     
	2012-11-09 23:54:56.997 [I] [Schedule        ] [RECORD] Waiting on recorder cleanup
	2012-11-09 23:54:56.997 [I] [FileWriter:/ ...] [RECORD] FileWriter record finished
	2012-11-09 23:54:56.997 [D] [FileWriter:/ ...] [TicketBuffer:MasterReceiveBuffer] ENTER UNSUBSCRIBE >
	2012-11-09 23:54:56.997 [D] [FileWriter:/ ...] [TicketBuffer:MasterReceiveBuffer] <UNSUBSCRIBE:FileWriter:/LWA_STORAGE/Internal/0/DROS/Rec/056240_000000004>
	2012-11-09 23:54:56.998 [D] [FileWriter:/ ...] [TicketBuffer:MasterReceiveBuffer] Last subscriber unsubscribed
	2012-11-09 23:54:56.998 [D] [FileWriter:/ ...] [TicketBuffer:MasterReceiveBuffer] LEAVE UNSUBSCRIBE >
	2012-11-09 23:54:56.998 [I] [FileWriter:/ ...] [RECORD] FileWriter thread finished
	2012-11-09 23:54:57.497 [I] [Schedule        ] [RECORD] Recorder cleanup complete
	2012-11-09 23:54:59.504 [I] [Schedule        ] [Schedule] window stop current operation: Record      000000004 056240 086066978  056240 086096978  DEFAULT_TBN                     
	2012-11-09 23:54:59.504 [I] [Schedule        ] [Schedule] completed as scheduled: Record      000000004 056240 086066978  056240 086096978  DEFAULT_TBN                     
	2012-11-09 23:54:59.504 [I] [Schedule        ] [Schedule] ===========================================

  In this example, the recording was for 30 seconds, but no data source was attached,
  consequently, Receive rate is always 0. If a data source is connected and configured 
  to send to the MCS-DR, these numbers will be non-zero. 
  


  Now, to verify that the recording was made, issue a DIRECTORY-ENTRY-X MIB request:
	>/LWA/bin/Msender -s MCS -d DR1 -r 4 -I 127.0.0.1 -po 5000 -pi 5001 -v -t "RPT" -D "DIRECTORY-ENTRY-1"

  On the second terminal you should see:
	...
	[recv]	Modified Julian Day:        56240
	[recv]	Milliseconds Past Midnight: 15455364
	[recv]	Accept/Reject:              Accepted
	[recv]	General Status:             NORMAL 
	[recv]	Comment:                    065240_000000004 055240 013500000 055240 013510000                      DEFAULT_TBN 000000000000000 000000000004096 YES
	[recv]	Data Length:                128
	...


Step 8: Retrieving recorded data
================================
  The get command takes arguments "<File name> <Start position> <length>", and length must 
  fit within the the data field of a response message, so about 8000 bytes or less at a a time. 
  Obviously, retrieving data this way is not going to be as fast as the recording rate. The 
  following is an example of retrieving data for visual inspection using the Msender utility:

	>/LWA/bin/Msender -s MCS -d DR1 -r 4 -I 127.0.0.1 -po 5000 -pi 5001 -v -t "GET" -D "065240_000000004 000000000 4096"
	

  This would retrieve the 4096 bytes starting at position 0 from the file 056240_000000001.
  Note: Recording file names are constructed automatically from the MJD and reference number of 
  the REC command. The -v flag tells Msender to display both the outgoing message and response. 
  In our example, no data was recorded, so MCS-DR will respond with an 'Invalid range' because
  while the file exists, it has a 0 length (length is updated to reflect the actual recorded amount
  of data when a recording completes). Had there been any recorded data, this would show the
  message/response session and the data from the recording would be returned in the response's 
  data field -- and consequently displayed in hex by the Msender utility. 
 
  Additional methods of accessing data are outlined in the extended user's guide.
 


Additional Testing and trouble-shooting
=======================================
  The extended user's guide contains more informaiton on setting up tests to capture real data,
  as well as trouble shooting tips and suggestions. For anything not covered by these two 
  documents, please feel free to contact me directly by email chwolfe2@vt.edu.
	  
	




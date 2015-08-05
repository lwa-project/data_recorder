#!/bin/bash
if [ "$1" != "I-HAVE-READ-THE-COMMENTS" ]; then
	echo "This script requires that you invoke it with the parameter \"I-HAVE-READ-THE-COMMENTS\""
	echo "please read the comments before invoking this script."
	exit -1
fi

######################################################################################################
#
#
#  This script runs through several interactions with the MCS-DR to test ICD compliance. 
#  You must have the following:
#    0) Know that this test is a lab tool, and not meant to be used outside of the lab
#    1) An MCS-DR set up IAW the quickstart, running in another terminal
#    2) A DRSU with drives sdb through sdg
#    3) An external sotrage device (drive) as /dev/sdg with
#       a) 1 partition /dev/sdg1
#       b) /dev/sdg1 formatted ext2 (initially)
#    4) you must edit the variables below to reflect your configuration
#    5) Understanding that execution times might cause this script to hang or issue 
#       requests prematurely. If hung, CTRL+C, edit the appropriate delay time, and restart
#    6) always after every execution, do (inserting appropriate device id if not sdg1):
#      	  >cat /proc/mounts > /etc/mtab; umount /LWA_EXT/dev/sdg1
#    7) If all else fails, email me, Christopher Wolfe: chwolfe2@vt.edu 
#    8) This script probably has errors.
#
# The subsystem reference designator that the computer running this script is assigned
src=MCS
# The subsystem reference designator that the is assigned to the MCS-DR
dst=MD1
# ip address of the MCS-DR (localhost ok)
dia=localhost
# destination port for messaging
dp=5001
# response listen port to listen for the response from MCS-DR
rlp=5000
# data port number for the MCS-DR
ddp=6002


# Don't edit anything other than delay times below here this line    -----------------------------------

ref=0
echo "0" > ./refNum
remDevice=/dev/sdg1

function incRef
{
ref=$(echo "`cat ./refNum` + 1" | bc)
echo "$ref" > ./refNum
}

function getMIB
{
/LWA/bin/Msender -Source $src -Destination $dst -ReferenceNumber $ref -Type RPT -DestinationIpAddress $dia -DestinationPort $dp -ResponseListenPort $rlp -Data "$1" | sed -e 's/No Comment Specified//'
incRef
}

function getShowMIB
{
echo "MIB '$1' => '`getMIB "$1"`'"
incRef
}

function sendCommand
{
/LWA/bin/Msender -v -Source $src -Destination $dst -ReferenceNumber $ref -Type $1 -DestinationIpAddress $dia -DestinationPort $dp -ResponseListenPort $rlp -Data "$2" | sed -e 's/No Comment Specified//'
incRef
}

function getRemoteMJD
{
/LWA/bin/Msender -v -Source $src -Destination $dst -Type PNG -ReferenceNumber $ref -DestinationIpAddress $dia -DestinationPort $dp -ResponseListenPort $rlp | grep 'recv.*Modified' | nawk '{print $5}'
incRef
}

function getRemoteMPM
{
/LWA/bin/Msender -v -Source $src -Destination $dst -Type PNG -ReferenceNumber $ref -DestinationIpAddress $dia -DestinationPort $dp -ResponseListenPort $rlp | grep 'recv.*Milliseconds' | nawk '{print $5}'
incRef
}

function relativeMJD
{
echo "`getRemoteMJD` + $1" | bc
}

function relativeMPM
{
echo "`getRemoteMPM` + $1" | bc
}


function getShowBranchOperation
{
getShowMIB "OP-TYPE"
getShowMIB "OP-START"
getShowMIB "OP-STOP"
getShowMIB "OP-REFERENCE"
getShowMIB "OP-ERRORS"
getShowMIB "OP-TAG"
getShowMIB "OP-FORMAT"
getShowMIB "OP-FILEPOSITION"
getShowMIB "OP-FILENAME"
getShowMIB "OP-FILEINDEX"
}


function getShowBranchSchedule
{
getShowMIB "SCHEDULE-COUNT"
Scount=$(getMIB "SCHEDULE-COUNT")
for (( i=1 ; i<=$Scount ; i=i+1 )); do
	getShowMIB "SCHEDULE-ENTRY-$i"
done
}

function getShowBranchDirectory
{
getShowMIB "DIRECTORY-COUNT"
Dcount=$(getMIB "DIRECTORY-COUNT")
for (( i=1 ; i<=$Dcount ; i=i+1 )); do
	getShowMIB "DIRECTORY-ENTRY-$i"
done
}

function getShowBranchDevices
{
getShowMIB "DEVICE-COUNT"
Dvcount=$(getMIB "DEVICE-COUNT")
for (( i=1 ; i<=$Dvcount ; i=i+1 )); do
	getShowMIB "DEVICE-ID-$i"
	getShowMIB "DEVICE-STORAGE-$i"
done
}


function getShowBranchStorage
{
getShowMIB "TOTAL-STORAGE"
getShowMIB "REMAINING-STORAGE"
}

function getShowBranchCPU
{
getShowMIB "CPU-COUNT"
cpucount=$(getMIB "CPU-COUNT")
for (( i=1 ; i<=$cpucount ; i=i+1 )); do
	getShowMIB "CPU-TEMP-$i"
done
}

function getShowBranchHDD
{
getShowMIB "HDD-COUNT"
hddcount=$(getMIB "HDD-COUNT")
for (( i=1 ; i<=$hddcount ; i=i+1 )); do
	getShowMIB "HDD-TEMP-$i"
done
}

function getShowBranchFormats
{
getShowMIB "FORMAT-COUNT"
fcount=$(getMIB "FORMAT-COUNT")
for (( i=1 ; i<=$fcount ; i=i+1 )); do
	getShowMIB "FORMAT-NAME-$i"
	getShowMIB "FORMAT-PAYLOAD-$i"
	getShowMIB "FORMAT-RATE-$i"
	getShowMIB "FORMAT-SPEC-$i"
done
}

function getShowBranchLog
{
getShowMIB "LOG-COUNT"
lcount=$(getMIB "LOG-COUNT")
for (( i=1 ; i<=$lcount ; i=i+1 )); do
	getShowMIB "LOG-ENTRY-$i"
done
}

function getShowWholeMIB
{
getShowBranchOperation
getShowBranchSchedule
getShowBranchDirectory
getShowBranchDevices
getShowBranchStorage
getShowBranchCPU
getShowBranchHDD
getShowBranchFormats
getShowBranchLog
}

function startDataSource
{
/LWA/bin/DataSource -DestinationIpAddress $dia -DestinationPort $ddp -DataSize $1 -Rate $2 -Duration `echo "$3 * 1000" | bc`&
}
function stopDataSource
{
killall DataSource
}

function delay
{
echo "Waiting for $1 seconds ..."
sleep $1
echo "... done waiting"
}



echo "==============================================================================="
echo "===                                                                         ==="
echo "===                   Initializing the MCSDR  (INI command)                 ==="
echo "===                                                                         ==="
echo "==============================================================================="
sendCommand INI "-L -D"
delay 10

echo "==============================================================================="
echo "===                                                                         ==="
echo "===                   Displaying the boot-up MIB                            ==="
echo "===                                                                         ==="
echo "==============================================================================="
getShowWholeMIB

echo "==============================================================================="
echo "===                                                                         ==="
echo "===                    Tell MCSDR to format $remDevice                      ==="
echo "===                                                                         ==="
echo "==============================================================================="
#sendCommand FMT "$remDevice"
#delay 10
#while [ `ps -e | grep -e '[m]k(e2)*fs' | wc -l` -eq 1 ];
#do
#echo "formatting ..."
#done

echo "==============================================================================="
echo "===                                                                         ==="
echo "===                    Tell MCSDR to Record TBN 100MiB/s for 30 Seconds     ==="
echo "===                    (also start datasource)                              ==="
echo "===                                                                         ==="
echo "==============================================================================="
sendCommand REC "`relativeMJD 0` `relativeMPM 3000` 30000 DEFAULT_TBN"
startDataSource 1024 104857600 45
delay 1

echo "==============================================================================="
echo "===                                                                         ==="
echo "===                    Display Schedule. Should have new record operation.  ==="
echo "===                    Also display directory info, should have new file.   ==="
echo "===                    New file should be sized by ratespec, and            ==="
echo "===                    show up as incomplete.                               ==="
echo "===                                                                         ==="
echo "==============================================================================="
getShowBranchSchedule
getShowBranchDirectory
goodfile=$(getMIB "OP-TAG")
delay 35

echo "==============================================================================="
echo "===                                                                         ==="
echo "===                    Display Schedule. Should have removed the operation. ==="
echo "===                    Also display directory info, should have new file.   ==="
echo "===                    New file should be sized ~ 3000MiB, and              ==="
echo "===                    show up as complete.                                 ==="
echo "===                                                                         ==="
echo "==============================================================================="
getShowBranchSchedule
getShowBranchDirectory
stopDataSource
delay 1

echo "==============================================================================="
echo "===                                                                         ==="
echo "===                    Tell MCSDR to Record TBN 100MiB/s for 30 Seconds     ==="
echo "===                    (also start datasource)                              ==="
echo "===                                                                         ==="
echo "==============================================================================="
sendCommand REC "`relativeMJD 0` `relativeMPM 3000` 30000 DEFAULT_TBN"
startDataSource 1024 104857600 45
delay 1


echo "==============================================================================="
echo "===                                                                         ==="
echo "===                    Tell MCSDR to Record TBN 100MiB/s for 30 Seconds     ==="
echo "===                    Should fail due to conflict with previous REC        ==="
echo "===                                                                         ==="
echo "==============================================================================="
sendCommand REC "`relativeMJD 0` `relativeMPM 3000` 30000 DEFAULT_TBN"
delay 4


echo "==============================================================================="
echo "===                                                                         ==="
echo "===                    Tell MCSDR to cancel recording                       ==="
echo "===                                                                         ==="
echo "==============================================================================="
curtag=$(getMIB "OP-TAG")
sendCommand STP "$curtag"
delay 1


echo "==============================================================================="
echo "===                                                                         ==="
echo "===                    Kill the datasource                                  ==="
echo "===                                                                         ==="
echo "==============================================================================="
stopDataSource
delay 3

echo "==============================================================================="
echo "===                                                                         ==="
echo "===                    Tell MCSDR to copy from the first recording          ==="
echo "===                    to a file called 'testOfCopy' on $remDevice          ==="
echo "===                                                                         ==="
echo "==============================================================================="
sendCommand CPY "$goodfile 0 10240 $remDevice testOfCopy"
while [ "`getMIB OP-TYPE | tr -d '\n'`" == "Idle       " ]; 
do
echo "Waiting for copy to start"
delay 3
done
while [ "`getMIB OP-TYPE | tr -d '\n'`" != "Idle       " ]; 
do
echo "Waiting for copy to complete"
delay 3
done

echo "==============================================================================="
echo "===                                                                         ==="
echo "===                    Dump the contents of testOfCopy                      ==="
echo "===                    First 16 bytes ignored, but then should be           ==="
echo "===                     000000 10 11 12 ... 1f                              ==="
echo "===                     000010 20 21 22 ... 2f                              ==="
echo "===                             ...                                         ==="
echo "===                     0027F0 f0 f1 f2 ... ff                              ==="
echo "===                                                                         ==="
echo "==============================================================================="
od -A x -t x1 /LWA_EXT$remDevice/testOfCopy 
delay 10

echo "==============================================================================="
echo "===                                                                         ==="
echo "===                    Tell MCSDR to copy from the first recording          ==="
echo "===                    to a file called 'testOfCopy' on $remDevice          ==="
echo "===                                                                         ==="
echo "==============================================================================="
sendCommand DMP "$goodfile 0 10240 19 $remDevice testOfDump"
while [ "`getMIB OP-TYPE | tr -d '\n'`" == "Idle       " ]; 
do
echo "Waiting for dump to start"
delay 3
done
while [ "`getMIB OP-TYPE | tr -d '\n'`" != "Idle       " ]; 
do
echo "Waiting for dump to complete"
delay 3
done

echo "==============================================================================="
echo "===                                                                         ==="
echo "===                    List the files created by the dump command           ==="
echo "===                    Each should be 16 bytes                              ==="
echo "===                                                                         ==="
echo "==============================================================================="
ls -l /LWA_EXT$remDevice/testOfDump* 


echo "==============================================================================="
echo "===                                                                         ==="
echo "===                    Dump the contents of first two non-garbage files     ==="
echo "===                    First file should be                                 ==="
echo "===                     000000 13 14 15 ... 22                              ==="
echo "===                     000010 23 24 25                                     ==="
echo "===                     000013                                              ==="
echo "===                    Second file should be                                ==="
echo "===                     000000 26 27 28 ... 35                              ==="
echo "===                     000010 36 37 38                                     ==="
echo "===                     000013                                              ==="
echo "==============================================================================="
DumpFiles=`ls -l /LWA_EXT$remDevice/ | nawk '/Dump/{print $8}' | head -n 3 | tail -n 2 | tr "\n" " "`
for x in $DumpFiles; do echo; echo $x; od -A x -t x1 /LWA_EXT$remDevice/$x; done



echo "==============================================================================="
echo "===                                                                         ==="
echo "===                    Test the Down and Up commands                        ==="
echo "===                                                                         ==="
echo "==============================================================================="
sendCommand DWN
delay 3
sendCommand UP
delay 3
sendCommand UP
delay 3
sendCommand DWN
delay 3
sendCommand DWN
delay 3
sendCommand UP


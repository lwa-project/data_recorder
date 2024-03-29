#!/bin/bash

## ========================= DROSv2 License preamble===========================
## Software for DROSv2 is covered jointly under GPLv3 and Boost 1.0 licenses, 
## to the extent required by included Boost sources and GPL sources, and to the 
## more restrictive case pertaining thereunto, as defined herebelow. Beyond those 
## requirements, any code not explicitly restricted by either of thowse two license 
## models shall be deemed to be licensed under the GPLv3 license and subject to 
## those restrictions.
##
## Copyright 2012, Virginia Tech, University of New Mexico, and Christopher Wolfe 
## 
## ========================= Boost License ====================================
## Boost Software License - Version 1.0 - August 17th, 2003
## 
## Permission is hereby granted, free of charge, to any person or organization
## obtaining a copy of the software and accompanying documentation covered by
## this license (the "Software") to use, reproduce, display, distribute,
## execute, and transmit the Software, and to prepare derivative works of the
## Software, and to permit third-parties to whom the Software is furnished to
## do so, all subject to the following:
## 
## The copyright notices in the Software and this entire statement, including
## the above license grant, this restriction and the following disclaimer,
## must be included in all copies of the Software, in whole or in part, and
## all derivative works of the Software, unless such copies or derivative
## works are solely in the form of machine-executable object code generated by
## a source language processor.
## 
## THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
## IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
## FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
## SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
## FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
## ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
## DEALINGS IN THE SOFTWARE.
## 
## =============================== GPL V3 ====================================
## This program is free software: you can redistribute it and/or modify
## it under the terms of the GNU General Public License as published by
## the Free Software Foundation, either version 3 of the License, or
## (at your option) any later version.
## 
## This program is distributed in the hope that it will be useful,
## but WITHOUT ANY WARRANTY; without even the implied warranty of
## MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
## GNU General Public License for more details.
## 
## You should have received a copy of the GNU General Public License
## along with this program.  If not, see <http://www.gnu.org/licenses/>.
 



if [ $EUID -ne 0 ]; then
	echo "Error: you must have root permissions to run this script."
	exit -1;
fi

# assemble any unassembled raid arrays (always)
mdadm -As



# determine the root partition (always)
ROOT_PARTITION=`df -a | nawk '/\/$/{print $1}'`

# get a list of ext2/3/4 volumes, less our root partition and any partitions listed in /LWA_STORAGE/StorageExceptionList
if [ -f /LWA_STORAGE/StorageExceptionList ]; then
	EXCEPT=`cat /LWA_STORAGE/StorageExceptionList`;
	CANDIDATES=`blkid | grep 'TYPE=\"ext' | grep -v "$EXCEPT" | grep -v "$ROOT_PARTITION" | sed 's/UUID.*$//' | sort -k 2 | sed 's/:.*$/ /' | tr -d '\n' | sed 's/\s*$//'`
else
	CANDIDATES=`blkid | grep 'TYPE=\"ext' | grep -v "$ROOT_PARTITION" | sed 's/UUID.*$//' | sort -k 2 | sed 's/:.*$/ /' | tr -d '\n' | sed 's/\s*$//'`
fi

function doDown()
{
	# unmount any device that is mounted underneath our LWA_STORAGE folder
        # use the lazy option to disconnect the moun in the background 
        # (so files amy finish writing if there are pending writes
        LAZY=-l
        if [ -d /LWA_STORAGE ]; then
                if [ -d /LWA_STORAGE/External ]; then
                        for x in /LWA_STORAGE/External/*; do
                                if mountpoint -q $x; then 
                                        umount $LAZY $x
                                fi
                                rm -rf $x;
                        done
                fi
                if [ -d /LWA_STORAGE/Internal ]; then
                        for x in /LWA_STORAGE/Internal/*; do
                                if mountpoint -q $x; then 
                                        umount $LAZY $x
                                fi
                                rm -rf $x;
                        done
                fi
        fi
	for x in $CANDIDATES; do
		for y in `mount | grep $x | nawk '{print $3}'`; do 
			umount $LAZY $y;
		 done
	done
}

prohibited="no"
function check()
{
	# test each candidate for DROSv2 required DRSU features
	features=`tune2fs -l $1 | grep features`
	prohibited="no"
	if [[ "$features" == *has_journal* ]];  then prohibited="yes"; fi
	if [[ "$features" == *dir_index* ]];    then prohibited="yes"; fi
	if [[ "$features" == *flex_bg* ]];      then prohibited="yes"; fi
	if [[ "$features" == *uninit_bg* ]];    then prohibited="yes"; fi
	if [[ "$features" != *extent* ]];       then prohibited="yes"; fi
	if [[ "$features" != *sparse_super* ]]; then prohibited="yes"; fi
	if [[ "$features" != *large_file* ]];   then prohibited="yes"; fi
	if [[ "$features" != *huge_file* ]];    then prohibited="yes"; fi
	if [[ "$features" != *dir_nlink* ]];    then prohibited="yes"; fi
}

function doUp()
{
	echo "Storage candidates are '$CANDIDATES'"
	# set up some counters
        N_INTERNAL=0
        N_EXTERNAL=0
        for x in $CANDIDATES; do
		echo "Examining '$x'"
		check $x
                if [[ $prohibited == yes ]]; then
                        #mount the drive as an external device
                        mountpoint=/LWA_STORAGE/External/$N_EXTERNAL
			echo "Non-DRSU volume: '$x' mounted at '$mountpoint'"
                        mkdir -p $mountpoint;
                        mount -t ext4 $x $mountpoint;
                        N_EXTERNAL=$((N_EXTERNAL+1));
                else
                        #mount the drive as a DRSU
                        mountpoint=/LWA_STORAGE/Internal/$N_INTERNAL
			echo "DRSU volume: '$x' mounted at '$mountpoint'"
                        mkdir -p $mountpoint;
                        mount -t ext4 -o defaults,noatime,barrier=0 $x $mountpoint
                        mkdir -p $mountpoint/DROS
                        mkdir -p $mountpoint/DROS/Rec
                        mkdir -p $mountpoint/DROS/Spec
                        N_INTERNAL=$((N_INTERNAL+1));
                fi
        done
}

function doFormat()
{
	DEVICE=$1
	if [ -z "$DEVICE" ]; then
		echo "Error: you must specify a device name. (e.g. /dev/md0)"
		exit -1;
	fi

	VOLUME_LABEL="$2"
	if [ -z "$VOLUME_LABEL" ]; then
		VOLUME_LABEL = "DRSU2-Volume.$RANDOM.$RANDOM"
	fi


	NUM_DRIVES=`mdadm --detail $DEVICE | grep "Raid Devices" |  nawk '{print $4}' | tr -d "\n"`
	CHUNK_SIZE=`mdadm --detail $DEVICE | grep "Chunk Size" |  nawk '{print $4}' | sed 's/K/ \* 1024/' | sed 's/M/ \* 1048576/' | sed 's/G/ \* 1073741824/' | bc | tr -d "\n"`

	BLOCKSIZE=4096
	STRIDE=`echo "${CHUNK_SIZE} / $BLOCKSIZE" | bc | tr -d "\n"`
	STRIPE_WIDTH=`echo "(${CHUNK_SIZE} * ${NUM_DRIVES}) / $BLOCKSIZE" | bc | tr -d "\n"`
	LAZY_ITABLE_INIT=0
	LAZY_JOURNAL_INIT=0
	NODISCARD=nodiscard
	MB_PER_INODE=64
	BYTES_PER_INODE=`echo "${MB_PER_INODE}*1024*1024" | bc | tr -d "\n"`
	CREATOR_OS="DROSv2"
	FEATURES="^dir_index,extent,filetype,^flex_bg,^has_journal,large_file,^quota,sparse_super,^uninit_bg"

	EXTENDED_OPTIONS="stride=${STRIDE}"
	EXTENDED_OPTIONS="${EXTENDED_OPTIONS},stripe_width=${STRIPE_WIDTH}"
	EXTENDED_OPTIONS="${EXTENDED_OPTIONS},lazy_itable_init=${LAZY_ITABLE_INIT}"
	EXTENDED_OPTIONS="${EXTENDED_OPTIONS},lazy_journal_init=${LAZY_JOURNAL_INIT}"
	EXTENDED_OPTIONS="${EXTENDED_OPTIONS},${NODISCARD}"

	OPTIONS="-b ${BLOCKSIZE}"
	OPTIONS="${OPTIONS} -E ${EXTENDED_OPTIONS}"
	OPTIONS="${OPTIONS} -i ${BYTES_PER_INODE}"
	OPTIONS="${OPTIONS} -O ${FEATURES}"
	#OPTIONS="${OPTIONS} -o ${CREATOR_OS}"
	OPTIONS="${OPTIONS} -t ext4"
	OPTIONS="${OPTIONS} -L '${VOLUME_LABEL}'"
	OPTIONS="${OPTIONS} -m 1"

	if mkfs.ext4 $OPTIONS $DEVICE; then
		echo "Complete!!!"
		echo
		echo "To mount the new DRSU, type \n\tmount -t ext4 -o defaults,noatime,barrier=0 $DEVICE <mountpoint> \nOr Use this script to enable use with DROS."
	else
		echo "Failed!!!"
		echo "Please see error message above"
	fi
}

case "$1" in
"up")
	doDown;
	doUp;
;;

"down")
	doDown;
;;

"format")
	doFormat "$2" "$3"
;;
"check")
	if [[ ! -b "$2"	]]; then
		echo "invalid"
	else
		check $2
		if [[ $prohibited == yes ]]; then
			echo "invalid"	
		else
			echo "valid"
		fi
	fi
;;
*)

	echo "unknown command '$1'"
	echo "Usage:"
	echo -e "\t$(basename $0) up\n\t\tBring all storage devices 'up', unmounting anything mounted under LWA_STORAGE\n"
	echo -e "\t$(basename $0) down\n\t\tBring all storage devices 'down' (does not affect volumes mounted outside \n\t\tof LWA_STORAGE\n"
	echo -e "\t$(basename $0) format device [Barcode]\n\t\tFormat the raid volume 'device' with a filesystem appropriate for DROS use. \n\t\t(Does not affect other devices up/down status, and does not 'up' \n\t\tthe newly created volume\n"
	echo -e "\t$(basename $0) check device \n\t\tVerify device is formatted suitably for use as DRSU\n"
;;
esac

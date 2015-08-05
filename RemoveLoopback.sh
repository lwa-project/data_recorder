#!/bin/bash
DevicesHome=/TempDevices
if [ $# -lt 1 -o $# -gt 4 ]; then
	echo "Illegal parameter count."
	echo "Usage:"
	echo "	RemoveLoopback.sh <Name>"
	echo
	echo "	Name 		The filename to use. Actual file should be stored in"
	echo "			'$DevicesHome/<Name>'."
	echo
	echo
	exit -1
fi
if [ "$(losetup -a | grep $1 | wc -l)" -eq 1 ]; then
	echo "Removing loopback '$1'"
	LoopDevice=$(losetup -a | grep $1 | nawk '{print $1}' | tr -d ':')
	if [ -z "$LoopDevice" ]; then
		echo "Cannot detect loopback device identifier for '$1'."
		exit -1
	else
		echo "Loopback device identifier for '$1' detected as '$LoopDevice'."
		Mountpoint=$(mount | grep $LoopDevice | nawk '{print $3}')
		if [ -n "$Mountpoint" ]; then
			echo "File '$1' (device '$LoopDevice') is mounted on '$Mountpoint'."
			echo "Attempting to unmount"
			umount $LoopDevice; result=$?
			if [ $result -ne 0 ]; then
				echo "Could not unmount '$1' (device '$LoopDevice')."
				exit -1
			else
				echo "Successfully unmounted '$1' (device '$LoopDevice')."
			fi
		fi
		losetup -d $LoopDevice; result=$?
		if [ $result -ne 0 ]; then
			echo "Could not remove '$1' (device '$LoopDevice')."
			exit -1
		else
			echo "Successfully removed '$1' (device '$LoopDevice')."
		fi
	fi
else 
	echo "Loopback '$1' doesn't exist"
	exit -1
fi
exit 0
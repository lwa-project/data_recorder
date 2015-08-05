#!/bin/bash
DevicesHome=/TempDevices
if [ $# -lt 1 -o $# -gt 4 ]; then
	echo "Illegal parameter count."
	echo "Usage:"
	echo "	MakeLoopback.sh <Name> <Size> [<FsType> [<MountPoint>]]"
	echo
	echo "	Name 		The filename to use. Actual file will be stored in"
	echo "			'$DevicesHome/<Name>'."
	echo
	echo "	Size 		Device size in MiB."
	echo
	echo "	FsType		When specified, the partition will be formatted with"
	echo "			the corresponding type, as in 'mkfs -t <FsType> device'."
	echo
	echo "	MountPoint	When specified, the the filesystem will be mounted"
	echo "			to '/media/loopbacks/<MountPoint>'."
	echo
	echo
	exit -1
fi

echo "Starting loopback device creation:"
echo " File:       $1"
echo " Size:       $2 (MiB)"
if [ -n "$3" ]; then
	echo " Filesystem: $3"
else
	echo " Filesystem: <none>"
fi
if [ "$(losetup -a | grep $1 | wc -l)" -eq 1 ]; then
	echo "Loopback '$1' already created:"
	losetup -a | grep $1
	exit -1
else 
	
	mkdir -p $DevicesHome
	CurrentHighest=$(losetup -a | nawk '{print $1}' | tr -d ':' | sed -e 's/\/dev\/loop//' | tail -n 1)
	if [ -z $CurrentHighest ]; then 
		CurrentHighest="-1"
	fi
	echo "CurrentHighest = $CurrentHighest"
	NextAvail=$(echo "1 + $CurrentHighest" | bc)
	echo "NextAvail = $NextAvail"
	touch $DevicesHome/$1
	dd if=/dev/zero of="$DevicesHome/$1" bs=1024 count=`echo "$2 * 1024" | bc` & pid=$!
	while [ "$(ps | grep $pid | wc -l)" -eq 1 ];
	do
	sleep 1	
	kill -USR1 $pid
	done
	losetup /dev/loop$NextAvail $DevicesHome/$1; result=$?
	if [ $result -ne 0 ]; then
		echo "Loopback setup of '$DevicesHome/$1' failed."
		exit -1
	else
		echo "Loopback setup of '$DevicesHome/$1' succeeded."
		if [ -n "$3" ]; then
			mkfs -t $3 /dev/loop$NextAvail; result=$?
			if [ $result -ne 0 ]; then
				echo "Failed to create $3 filesystem on '$DevicesHome/$1'."
				exit -1
			else
				echo "Successfully created $3 filesystem on '$DevicesHome/$1'."
				if [ -n "$4" ]; then
					echo "Attempting to mount '/dev/loop$NextAvail' at '/media/loopbacks/$4'."
					rm -fr /media/loopbacks/$4
					mkdir -p /media/loopbacks/$4
					mount -t $3 /dev/loop$NextAvail /media/loopbacks/$4; result=$?
					if [ $result -ne 0 ]; then
						echo "Mounting failed."
						exit -1
					else
						echo "Successfully mounted '/dev/loop$NextAvail' at '/media/loopbacks/$4'."
					fi
				fi
			fi
		fi
	fi
fi
exit 0
#!/bin/bash

countUsableDRSU=0


candidates=`cat /proc/partitions | nawk '{if (NR>1) print $4}' | tr '\n' ' '`
mdMembers=""
mdArrays=""

for x in $candidates; do
  mdadm --examine /dev/$x 2> /dev/null >/dev/null;  exit_code=$?
  if [ "$exit_code" -eq "0" ]; then
    #echo "/dev/$x is     a multidisk partition"
    #append candidate to mdMembers
    mdMembers="$mdMembers $x"
  fi
done

for x in $candidates; do
  mdadm --detail /dev/$x 2> /dev/null >/dev/null;  exit_code=$?
  if [ "$exit_code" -eq "0" ]; then
    if [[ "$x" =~ "md_" ]]; then
      mdadm --stop /dev/$x;
      exit_code=$?
      if [ "$exit_code" -eq "0" ]; then
	echo "Auto-started array /dev/$x has been successfully stopped"
      else
	echo "Warning: Auto-started array /dev/$x could not be stopped"
	#append candidate to mdMembers
	mdArrays="$mdArrays $x"
      fi
    else
      partitionTypeString=`blkid -s TYPE | grep /dev/$x | tr -d "\n"`
      if [[ -z "$partitionTypeString" ]]; then
	echo "Discovered array '/dev/$x' is potentially a DRSU (typestring = '$partitionTypeString')"
	#append candidate to mdMembers
	mdArrays="$mdArrays $x"
      else
	echo "ptype= '$partitionTypeString'"
	mdadm --stop /dev/$x 
	#was already up, won't be busy, so no errors (hopefully)
      fi
    fi
  fi
done

echo "Found the following candidate multidisk partitions:"
echo " $mdMembers"
echo
echo "Found the following active multidisk partitions:"
echo " $mdArrays"
alreadyActiveMembers=""
for x in $mdMembers; do
  memberuuid=`mdadm --examine /dev/$x | grep UUID | nawk '{print $3}' | tr -d "\n"`
  for arr in $mdArrays; do
    arruuid=`mdadm --detail /dev/$arr | grep UUID | nawk '{print $3}' | tr -d "\n"`
    if [ "$arruuid" == "$memberuuid" ]; then
      if [[ "$alreadyActiveMembers" =~ "$x" ]]; then
	echo > /dev/null
      else
      	alreadyActiveMembers="$alreadyActiveMembers $x"
      fi
    fi
  done
done
echo "Found the following already active member partitions:"
echo " $alreadyActiveMembers"
inactiveMembers=""
rm -rf UUIDS
touch UUIDS
for x in $mdMembers; do
  if [[ "$alreadyActiveMembers" =~ "$x" ]]; then
    echo > /dev/null
  else
    memberuuid=`mdadm --examine /dev/$x | grep UUID | nawk '{print $3}' | tr -d "\n"`
    inactiveMembers="$inactiveMembers $x"
    echo "$memberuuid" >> UUIDS
  fi
done
startedArrays=""
failedArrrays=""
potentialUUIDs=`cat UUIDS | sort | uniq | tr "\n" " "`
echo "found the following UUIDs"
echo "$potentialUUIDs"
bannedUUIDs=""
for uuid in $potentialUUIDs; do
  echo 
  echo "Now trying to assemble array with uuid='$uuid'"
  requiredCount=0
  discoveredcount=0
  discoveredMembers=""
  for x in $inactiveMembers; do
      memberuuid=`mdadm --examine /dev/$x | grep UUID | nawk '{print $3}' | tr -d "\n"`
      if [ "$uuid" == "$memberuuid" ]; then
	if (( requiredCount==0 )); then
	  requiredCount=`mdadm --examine /dev/$x | grep "Raid Devices" | nawk '{print $4}' | tr -d "\n"`
	else
	  thisRequiredCount=`mdadm --examine /dev/$x | grep "Raid Devices" | nawk '{print $4}' | tr -d "\n"`
	  if (( thisRequiredCount != requiredCount )); then
	    echo "Warning: multiple member partitions claim the same UUID but with different array member counts:"
	    echo "array with UUID '$uuid' will not be started"
	    bannedUUIDs="$bannedUUIDs $uuid"
	  fi
	fi
	discoveredcount=$(( $discoveredcount + 1 ))
	discoveredMembers="$discoveredMembers /dev/$x"
      fi
  done
  if (( discoveredcount == requiredCount )); then
    if [[ "$bannedUUIDs" =~ "$uuid" ]]; then
      echo "Skipping uuid $uuid. See previous warning."
    else
      id=0
      while [[ "`cat /proc/partitions | nawk '{if (NR>1) print $4}' | tr '\n' ' '`" =~ "md$id" ]]; do id=$(( id+1 ));   done
      mdadm --assemble /dev/md$id $discoveredMembers
      exit_code=$?
      if [ "$exit_code" -eq "0" ]; then
	echo "Successfully started array: /dev/md$id  ($discoveredMembers)"
	startedArrays="$startedArrays /dev/md$id"
	partitionTypeString=`blkid -s TYPE | grep /dev/md$id | tr -d "\n"`
	if [[ -z "$partitionTypeString" ]]; then
	  echo "Discovered array '/dev/md$id' is potentially a DRSU (typestring = '$partitionTypeString')"
	else
	  echo "Discovered array '/dev/md$id' is not a DRSU (typestring = '$partitionTypeString'), and will be restarted later"
	  sleep 3
	  mdadm --stop /dev/md$id
	   exit_code=$?
	   if [ "$exit_code" -eq "0" ]; then
	      echo "Non-DRSU array successfully stopped"
	      extArrays="$extArrays $discoveredMembers,"
	   else
	      echo "Warning: started array '/dev/md$id' could not be stopped and is not a DRSU"
	   fi
	fi
      else
	echo "Failed to start array: /dev/md$id  ($discoveredMembers)"
	failedArrays="$failedArrays /dev/md$id"
      fi
    fi
  else
    echo "Warning: Could not find all members of array with uuid='$uuid'"
  fi
  
done
extArrayCount=0
while [[ ! -z "$extArrays" ]]; do
  extArray=`echo "$extArrays" | cut -d "," -f 1`
  extArrays=`echo "$extArrays" | cut -d "," -f 2-`
  id=0
      while [[ "`cat /proc/partitions | nawk '{if (NR>1) print $4}' | tr '\n' ' '`" =~ "md$id" ]]; do id=$(( id+1 ));   done
      mdadm --assemble /dev/md$id $extArray
      exit_code=$?
      if [ "$exit_code" -eq "0" ]; then
	echo "External array /dev/md$id successfully restarted"
      else
	echo "External array /dev/md$id could notbe restarted"
      fi
done

partitions="`cat /proc/partitions | nawk '{if (NR>1) print $4}' | tr '\n' ' '`"
for x in $partitions; do
  echo "partition: $x"
  if [[ "$x" =~ "md" ]]; then
    echo "match"
    if [[ -z "`blkid /dev/$partitions | tr -d "\n"`" ]]; then
      echo "drsu"
      echo "okgo" > /LWA/runtime/okgo
      # we found a drsu
      exit 0;
    fi
  fi
done
#no drsu found
exit 1;

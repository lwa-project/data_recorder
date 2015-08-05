#!/bin/bash
PIDFILE=/LWA/runtime/DROSv2.pid
LOGFILE=/dev/null
ERRFILE=/dev/null
if [ ! -f /LWA/config/binary-mode ]; then
        echo -n "Spectrometer" > /LWA/config/binary-mode
fi
DROS=/LWA/bin/DROS2-`cat /LWA/config/binary-mode | tr -d "\n"`

mkdir -p /LWA/runtime
RED='\033[1;31m'
GRN='\033[1;32m'
YEL='\033[1;33m'
BLU='\033[1;34m'
MAG='\033[1;35m'
CYN='\033[1;36m'
WHT='\033[1;37m'

#spec_is_running=`ps -ef | grep '/LWA/bin/DROS2-Spectrometer' | grep -vE 'bash|grep|ps' | sed 's#.*/LWA/bin/DROS2-##' | sort | uniq | wc -l`
#lvb_is_running=`ps -ef | grep '/LWA/bin/DROS2-LiveBuffer' | grep -vE 'bash|grep|ps' | sed 's#.*/LWA/bin/DROS2-##' | sort | uniq | wc -l`





function cecho(){
	echo -en $1$2;
	tput sgr0;
	echo
}


echo "`date`: Entered startup with parameter '$1'" >> /LWA/runtime/start.info
case "$1" in
'change-binary')
	currentBinary=`cat /LWA/config/binary-mode | tr -d "\n"`
	newBinary=$2
	if [ "$newBinary" != "$currentBinary" ]; then
		case "$newBinary" in
			Spectrometer|LiveBuffer)
				echo "Changinging selected binary from '$currentBinary' to '$newBinary'"
				$0 stop;
				echo -n "$newBinary" > /LWA/config/binary-mode
				$0 start;
				$0 status;
        	        ;;
                	*)
                        	echo "Error: unknown binary version: '$1'"
	                        exit -1
        	        ;;
		esac
	else
		echo "'$currentBinary' is already the selected binary"
	fi
;;
'start')
	echo "Starting DROS";
	start-stop-daemon --start --oknodo  -p $PIDFILE -m -b -x /bin/bash -- -c "$DROS > /LWA/runtime/console.log 2>&1"
	sleep 5;
	$0 status
;;
'stop')
	echo "Stopping DROS";
	p=$(cat $PIDFILE);
	[ -n "$pid" ] && pkill -P "$pid"
	echo "Killing bash process"
	start-stop-daemon --stop --oknodo -p $PIDFILE --retry=USR1/60/TERM/20/KILL/5
	echo "Killing DROS"
	ps -ef | grep '/LWA/bin/[D]ROS2-' | grep -v bash | nawk '{print $2}' > $PIDFILE
	start-stop-daemon --stop --oknodo -p $PIDFILE --retry=USR1/60/TERM/20/KILL/5
	$0 status
;;
'restart')
	echo "Restarting DROS"
        $0 stop
	$0 start
;;
'status')
	if [ -f "$PIDFILE" ]; then
		PID=`cat $PIDFILE | tr -d "\n"`;
		if [ -z "$PID" ]; then
			echo "DROS not running -- pid file empty."
		else
			if kill -q -s 0 $PID 2>/dev/null; then
				echo "DROS is running with pid '$PID'"
			else
				echo "DROS not running -- kill says so"
			fi
		fi
	else
		echo "DROS not running -- no pid file."
	fi
;;
'upgrade')
	WD=`pwd`
	RDATE=`date -u +%y%m%d_%H%M%S_%N`
        NEXT=/LWA_UPGRADES/WORKING/DOWNLOADED.tar.gz
        CUR=/LWA_UPGRADES/INSTALLED.tar.gz
	UpgradeType=$2
	if [ -z "$UpgradeType" ]; then
		cecho $RED "Error: You must specify an upgrade type."
		echo -e "Usage: \n\t$0 upgrade release|devel"
		exit -1;
	fi
	if [ "release" == "$UpgradeType" ]; then pname=LATEST-RELEASE.tar.gz; fi
        if [ "devel"   == "$UpgradeType" ]; then pname=LATEST-DEVEL.tar.gz;   fi
	if [ -z "$pname" ]; then 
		cecho $RED "Error: unknown upgrade type : '$UpgradeType'"
                echo -e "Usage: \n\t$0 upgrade release|devel"
                exit -1;
        fi
	cecho $CYN "================== Upgrading DROS to latest $UpgradeType build ================="
        mkdir -p /LWA_UPGRADES
        mkdir -p /LWA_UPGRADES/BACKUP
        mkdir -p /LWA_UPGRADES/WORKING
	mkdir -p /LWA_UPGRADES/LOGS
        mkdir -p /LWA_UPGRADES/PACKAGES

	rm -rf /LWA_UPGRADES/WORKING/*

	cecho $YEL "Downloading latest $UpgradeType package"
	echo "DATE: $RDATE; TYPE: '$UpgradeType'; OVERRIDE: '$3'" >> /LWA_UPGRADES/wget_log.txt;

	# get the archive
	if wget http://128.173.93.173/TARBALLS/$pname -O $NEXT > /LWA_UPGRADES/LOGS/wget.$RDATE.txt 2>&1; then
                cecho $GRN "Archive retrieved: http://128.173.93.173/TARBALLS/$pname"
                cp /LWA_UPGRADES/WORKING/DOWNLOADED.tar.gz /LWA_UPGRADES/PACKAGES/$RDATE.tar.gz
	else
                cecho $RED "Error: failed to retrieve archive. See /LWA_UPGRADES/LOGS/wget.$RDATE.txt for more info. No changes made."
                rm $NEXT
                cd $WD; exit -1;
	fi

	cecho $YEL "Comparing with installed version"
	# check to see if it's more recent than the installed version
	if [ -f "$CUR" ]; then
		if diff -q $CUR $NEXT > /dev/null 2>&1; then
			cecho $YEL "Warning: The currently installed version is already the latest version.";
			if [ "anyway" == "$3" ]; then
				cecho $MAG "Reinstalling the same version...."
			else
				cecho $RED "Abandoning upgrade. No changes made."
				cd $WD; exit -1;
			fi
		fi
	fi

	# extract the archive
	cecho $YEL "Extracting archive ...."
	cd /LWA_UPGRADES/WORKING

	if tar -xvf ./DOWNLOADED.tar.gz > /LWA_UPGRADES/LOGS/extract.$RDATE.txt 2>&1; then
		cecho $GRN "Archive extracted successfully."
	else
		cecho $RED "Error: failed to extract archive. See /LWA_UPGRADES/LOGS/extract.$RDATE.txt for more info. No changes made."
		cd $WD; exit -1;
	fi


	RELEASENAME=`du --max-depth 1 | cut -f 2- | grep -v '^\.$' | sed 's/.\///'`
	cd $RELEASENAME/build;

	cecho $YEL "Installing dependencies ...."
	if make deps > /LWA_UPGRADES/LOGS/depslog.$RDATE.txt 2>&1; then
                cecho $GRN "Dependencies installed";
        else
                cecho $RED "Error: Failed to install dependencies.";
		echo -e $BLU "=============== list dependency problems =================";
		cat /LWA_UPGRADES/LOGS/depslog.$RDATE.txt
		echo -e $BLU "=============== end dependency problems ==================";
		cecho $YEL "Attempting to continue wth installation"
        fi


	cecho $YEL "Building DROS"
	if make -j 8 clean all > /LWA_UPGRADES/LOGS/buildlog.$RDATE.txt 2>&1; then
		cecho $GRN "DROS built successfully!!";
	else
		cecho $RED "Error: DROS failed to build. see /LWA_UPGRADES/LOGS/buildlog.$RDATE.txt for more info. No changes made";
		cd $WD; exit -1;
	fi

	# stop DROS
	cecho $YEL "Shutting down DROS if active"
	$0 stop

	# backup Files and logs
        cecho $YEL "Backing up current installation"
	cd /
	if tar -czv -f ./$RDATE.tar.gz ./LWA > /LWA_UPGRADES/LOGS/archivelog.$RDATE.txt 2>&1; then
		mv ./$RDATE.tar.gz /LWA_UPGRADES/BACKUP/
		cecho $GRN "Current installation backed up to /LWA_UPGRADES/BACKUP/$RDATE.tar.gz"
	else
		cecho $RED "Failed to archive current installation. see install log at /LWA_UPGRADES/LOGS/archivelog.$RDATE.txt for more inf. Restarting DROS, No other changes made."
		$0 start
		exit -1;
	fi

	# backup configuration data
	cecho $YEL "Backing up current configuration."
	cp /LWA/config/defaults_v2.cfg /LWA_UPGRADES/WORKING/defaults_v2.backup.cfg
	cp /LWA/config/binary-mode /LWA_UPGRADES/WORKING/binary-mode.backup

	# install
	cecho $YEL "Installing DROS"
	cd /LWA_UPGRADES/WORKING/$RELEASENAME/build;
	if make install > /LWA_UPGRADES/LOGS/installlog.$RDATE.txt 2>&1; then
                cecho $GRN "DROS installed successfully!!";
		cd /LWA/scripts;
                ./installStartupScript.sh StartDROS.sh > /dev/null 2>&1;
		mv /LWA_UPGRADES/WORKING/DOWNLOADED.tar.gz $CUR
        else
                cecho $RED "Error: DROS failed to install. see /LWA_UPGRADES/LOGS/installlog.$RDATE.txt for more info. System changes made";
                cd $WD; exit -1;
        fi


	# restore config data
	cecho $YEL "Restoring configuration data."
	cp /LWA_UPGRADES/WORKING/defaults_v2.backup.cfg /LWA/config/defaults_v2.cfg
        cp /LWA_UPGRADES/WORKING/binary-mode.backup /LWA/config/binary-mode

	# restart DROS
	cecho $YEL "Restarting DROS."
	$0 start

	cecho $CYN "================= DROS upgrade complete !!! =================="
;;
'monitor')
	tail --follow=name --retry /LWA/runtime/runtime.log
;;
*)
	echo -e "Unknown command '$1'\nUsage:\n\t`basename $0` start|stop|restart|status|monitor|upgrade upgrade-type [anyway]\n"
;;
esac
 

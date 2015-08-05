#!/bin/bash
DROS=/LWA/bin/DROS2
PIDFILE=/LWA/runtime/DROSv2.pid
LOGFILE=/dev/null
ERRFILE=/dev/null

mkdir -p /LWA/runtime

echo "`date`: Entered startup with parameter '$1'" >> /LWA/runtime/start.info
case "$1" in
'start')
	echo "Starting DROS";
	start-stop-daemon --start --oknodo  -p $PIDFILE -m -b -x $DROS -a $DROS
	sleep 5;
	$0 status
;;
'stop')
	echo "Stopping DROS";
	start-stop-daemon --stop --oknodo -p $PIDFILE --retry=USR1/30/TERM/20/KILL/5
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
*)
	echo -e "Unknown command '$1'\nUsage:\n\t`basename $0` start|stop|restart|status\n"
;;
esac
 

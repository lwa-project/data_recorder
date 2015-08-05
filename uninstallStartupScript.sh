#!/bin/bash 
SCRIPT=`basename $1`

if [ -z "$SCRIPT" ]; then
	echo "You must supply the name of a script."
	echo "The script must already have been installed in /etc/init.d. "
	echo "Be warned that this script makes no checks on what is being removed, aside from it being there."
else
	if [ -f "/etc/init.d/$SCRIPT" ]; then
		sudo update-rc.d -f $SCRIPT remove
		sudo rm /etc/init.d/$SCRIPT
	else
		echo "Script '$SCRIPT' was not found in /etc/init.d"
	fi
fi

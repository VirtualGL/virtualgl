#!/bin/sh
# description: Linux Client Daemon

SUSE=0
if [ -f /etc/rc.status ]; then
	SUSE=1
	. /etc/rc.status
else
	if [ -f /etc/rc.d/init.d/functions ]; then
		. /etc/rc.d/init.d/functions
	else
		echo "Unsupported platform"
		exit 1
	fi
fi

[ -x /usr/bin/vglclient ] || exit 0

RETVAL=0
LOGPATH=/var/log/

start() {
	echo -n $"Starting Client Daemon: "
	if [ $SUSE -eq 1 ]; then
		startproc /usr/bin/vglclient --service -l $LOGPATH/vglclient.log
		rc_status -v
	else
		daemon /usr/bin/vglclient --service -l $LOGPATH/vglclient.log && success || failure
	fi
	echo
	if [ ! $? -eq 0 ]; then RETVAL=$?; fi
}

stop() {
	echo -n $"Shutting down Client Daemon: "
	if [ $SUSE -eq 1 ]; then
		killproc -TERM /usr/bin/vglclient
		rc_status -v
	else
		killproc vglclient
	fi
	echo
}

reload() {
	stop
	start
}

ID=`id -u`
if [ ! $ID -eq 0 ]; then
	LOGPATH=$HOME/
fi

if [ $SUSE -eq 1 ]; then
	rc_reset
fi

case "$1" in
	start)
		start
		;;
	stop)
		stop
		;;
	restart)
		stop
		start
		;;
	*)
		echo $"Usage: $prog {start|stop|restart}"
		exit 1
esac
exit $RETVAL

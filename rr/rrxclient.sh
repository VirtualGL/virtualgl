#!/bin/sh
# description: Linux Client Daemon

REDHAT=0
if [ -f /etc/rc.d/init.d/functions ]; then
	. /etc/rc.d/init.d/functions
	REDHAT=1;
else
	if [ -f /lib/lsb/init-functions ]; then
		. /lib/lsb/init-functions
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
	if [ $REDHAT -eq 1 ]; then
		daemon /usr/bin/vglclient --service -l $LOGPATH/vglclient.log && success || failure
	else
		start_daemon /usr/bin/vglclient --service -l $LOGPATH/vglclient.log && log_success_msg || log_failure_msg
	fi
	echo
	if [ ! $? -eq 0 ]; then RETVAL=$?; fi
}

stop() {
	echo -n $"Shutting down Client Daemon: "
	if [ $REDHAT -eq 1 ]; then
		killproc vglclient
	else
		killproc vglclient && log_success_msg || log_failure_msg
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

#!/bin/sh
# kFreeBSD do not accept scripts as interpreters, using #!/bin/sh and sourcing.
if [ true != "$INIT_D_SCRIPT_SOURCED" ] ; then
    set "$0" "$@"; INIT_D_SCRIPT_SOURCED=true . /lib/init/init-d-script
fi
### BEGIN INIT INFO
# Provides:          rtl-entropy
# Required-Start:    $remote_fs $syslog
# Required-Stop:     $remote_fs $syslog
# Default-Start:     2 3 4 5
# Default-Stop:      0 1 6
# Short-Description: A service that start the rtl-entropy DAEMON

# Description:       Creates a rtl-entropy deamon with semi random settings in a range 
#                    then it starts a rgnd deamon to read the fifo in /var/run/rtl_entropy.fifo 
#                    <...>
### END INIT INFO

# Author: Matthew D Weger <matthew.d.weger@gmail.com>


DESC="rtl-entropy"

CHAN_RANGE=$(( ( RANDOM % 200000000 )  + 90000000 ))
SAMPLE_RATE_RANGE=2400000

RTL_ENTROPY_DAEMON=/usr/bin/rtl-entropy
RTL_ENTROPY_DAEMON_USER=rtl_entropy
RTL_ENTROPY_DAEMON_NAME=rtl_entropy
RTL_ENTROPY_PID_FILE=/var/run/$RTL_ENTROPY_DAEMON_NAME.pid
RTL_ENTROPY_FIFO_FILE=/var/run/$RTL_ENTROPY_DAEMON_NAME.fifo
RTL_ENTROPY_OPS="-o $RTL_ENTROPY_FIFO_FILE -s $SAMPLE_RATE_RANGE -f $CHAN_RANGE"

RNGD_DAEMON_NAME=rngd
RNGD_PID_FILE=/var/run/$RNGD_DAEMON_NAME 
RNGD_DAEMON_OPS=" -r $RTL_ENTROPY_FIFO_FILE -W95%"



do_start () {
	log_daemon_msg "Starting $RTL_ENTROPY_DAEMON_NAME  daemon"
	start-stop-daemon --start --background --pidfile $RTL_ENTROPY_PID_FILE --make-pidfile --startas $RTL_ENTROPY_DAEMON -- $RTL_ENTROPY_OPS 
	log_end_msg $?
	log_daemon_msg "Starting $RNGD_DAEMON_NAME daemon"		
	start-stop-daemon --start --background --pidfile $RNGD_PID_FILE  --make-pidfile --startas $RNGD_DAEMON_NAME -- $RNGD_DAEMON_OPS
}

do_stop () {
	log_daemon_msg "Stoping $RTL_ENTROPY_DAEMON_NAME and $RNGD_DAEMON_NAME daemon"
	start-stop-daemon --stop --pidfile $RTL_ENTROPY_PID_FILE --retry 20
	start-stop-daemon --stop --pidfile $RNGD_PID_FILE --retry 20 
	log_end_msg $?
}

case "$1" in
 
start|stop)
do_${1}
;;
 
restart|reload|force-reload)
do_stop
do_start
;;
 
status)
status_of_proc "$RTL_ENTROPY_DAEMON_NAME" "$RTL_ENTROPY_DAEMON" && exit 0 || exit $?
;;
*)
echo "Usage: /etc/init.d/$RTL_ENTROPY_DAEMON_NAME {start|stop|restart|status}"
exit 1
;;
 
esac
exit 0


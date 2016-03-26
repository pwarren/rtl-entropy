#
# Regular cron jobs for the rtl-entropy package
#
0 4	* * *	root	[ -x /usr/bin/rtl-entropy_maintenance ] && /usr/bin/rtl-entropy_maintenance

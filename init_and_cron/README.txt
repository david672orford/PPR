mouse:~ppr/src/init_and_cron/README.txt
29 June 2000

======================
 init_ppr.sh
======================

This is the System V style Init script for PPR.


======================
 cron_daily.sh
 cron_hourly.sh
======================

These scripts run things such as ppr-clean and ppr-fontindex.


======================
 ppr-clean.perl
======================

This program removes lost temporary files left behind by PPR.  It is
installed in $HOMEDIR/bin/.  Normally the crontab installed by
$HOMEDIR/fixup/fixup_cron runs this periodically.


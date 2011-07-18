#!/bin/sh


RUNDIR=~/.enigma-client4


mkdir "$RUNDIR" &&
cp ../eclient-rpc.py "$RUNDIR" &&
cp ../../enigma "$RUNDIR" &&
cp ../../dict/00trigr.cur ../../dict/00bigr.cur "$RUNDIR"


cat <<EOF > "$RUNDIR"/ecrun
#!/bin/sh

exec </dev/null
exec >/dev/null

cd $RUNDIR

nohup $RUNDIR/eclient-rpc.py http://keyserver.bytereef.org:443 2>>logfile &
EOF


cat <<EOF > "$RUNDIR"/ecboot
#!/bin/sh

PATH=/bin:/sbin:/usr/bin:/usr/sbin:/usr/local/bin:/usr/local/sbin

exec </dev/null
exec >/dev/null

cd $RUNDIR

env - PATH=\$PATH $RUNDIR/eclient-rpc.py http://keyserver.bytereef.org:443 2>>logfile
EOF


chmod +x $RUNDIR/ecrun $RUNDIR/ecboot

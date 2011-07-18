#!/bin/sh


RUNDIR=~/.enigma-client


mkdir "$RUNDIR" &&
cp ../enigma-client.py "$RUNDIR" &&
cp ../../enigma "$RUNDIR" &&
cp ../../dict/00trigr.cur ../../dict/00bigr.cur "$RUNDIR"


cat <<EOF > "$RUNDIR"/ecrun
#!/bin/sh

exec </dev/null
exec >/dev/null

cd $RUNDIR

nohup $RUNDIR/enigma-client.py keyserver.bytereef.org 65521 2>>logfile &
EOF


cat <<EOF > "$RUNDIR"/ecboot
#!/bin/sh

PATH=/bin:/sbin:/usr/bin:/usr/sbin:/usr/local/bin:/usr/local/sbin

exec </dev/null
exec >/dev/null

cd $RUNDIR

env - PATH=\$PATH $RUNDIR/enigma-client.py keyserver.bytereef.org 65521 2>>logfile
EOF


chmod +x $RUNDIR/ecrun $RUNDIR/ecboot

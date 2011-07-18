

==================================
  Create the first run directory:
==================================

./eclient-install.sh


======================================================================
  If you have more than one CPU/core, you can run multiple clients:
======================================================================


You cannot run several clients in the same run directory. Thus, you
have to create additional run directories:


# Each of the scripts just creates a _single_ run directory,
# so for four CPUs or cores execute all of them:


Default client:
===============

./eclient-install-2.sh
./eclient-install-3.sh
./eclient-install-4.sh



# /etc/inittab version:
# add additional entries as necessary:

EC:2345:respawn:/bin/su - <user> -c /home/<user>/.enigma-client/ecboot
EC2:2345:respawn:/bin/su - <user> -c /home/<user>/.enigma-client2/ecboot
EC3:2345:respawn:/bin/su - <user> -c /home/<user>/.enigma-client3/ecboot
EC4:2345:respawn:/bin/su - <user> -c /home/<user>/.enigma-client4/ecboot

telinit Q


# crontab version:
# add additional entries as necessary:

@reboot /home/<user>/.enigma-client/ecrun
@reboot /home/<user>/.enigma-client2/ecrun
@reboot /home/<user>/.enigma-client3/ecrun
@reboot /home/<user>/.enigma-client4/ecrun
 
cd ~/.enigma-client && ./ecrun
cd ~/.enigma-client2 && ./ecrun
cd ~/.enigma-client3 && ./ecrun
cd ~/.enigma-client4 && ./ecrun



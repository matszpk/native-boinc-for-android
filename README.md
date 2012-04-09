NativeBOINC for Android project.

This is fully-fledged the native BOINC client for Android and BOINC Manager (based on AndroBOINC)
especially designed for working with this version of BOINC client.

Project applications also available in this repository. We prepared applications for following projects:

* Primegrid - finding prime numbers (gcwsieve application).
* Enigma@Home - polish project maintained by TJM.
* MilkyWay@Home - modeling MilkyWay galaxy (milkyway_separation).

Binaries for this project available at http://krzyszp.info/matszpk/.

Structure of repository:

* bins/ - old binaries (obsolete)
* src/ - main source directory
  * NativeBOINC - source of BOINC Manager based on AndroBOINC (with installer and boinc client runner)
  * androboinc-6.10.17.b3-localhost - old AndroBOINC with fix for native boinc client
  * boinc-6.10.58 - old boinc client
  * boinc-6.12.38 - current version of boinc client (with update_apps and client_monitor)
  * curl-7.21.4 - sources of curl modified for Android
  * enigma-suite-0.76 - source codes for Enigma@Home applications with optimizations
  * enigma-wrapper - enigma wrapper for BOINC
  * gcwsieve-1.3.8 - one of Primegrid application with optimizations
  * milkyway_separation_0.88 - old version of MilkyWay@Home application with optimizations
  * milkyway_separation_1.00 - current version of MilkyWay@Home application with optimizations
  * openssl-1.0.0d - OpenSSL for Android
  * init.sh - script which prepares compilation environment

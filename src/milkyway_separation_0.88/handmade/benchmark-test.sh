../milkyway_separation_ifp `cat cmdline.txt`&
pid=$!
sleep 300
kill -9 $pid
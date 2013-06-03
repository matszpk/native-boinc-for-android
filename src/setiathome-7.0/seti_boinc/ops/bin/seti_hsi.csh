#! /bin/csh

# This script gets from and puts to HPSS.  It does checks on a put.
# It can also list files and directories.

set action              = $1
set filename            = $2
set target_dir          = seti/dr2_data/production
set throttle_speed      = "32M"   # 32Mbps
set md5_check_length    = 1048576
set retval              = 0
if (`hostname` == "ewen") then
	alias hsi hsi.hpss
endif	

hsi touch hpss_ping >& /dev/null
if($status == 1) then
    echo hpss is down
    exit 1
endif

if($action == "--list") then
    hsi "ls -l  $target_dir/${filename}" |& tail -1 > /tmp/$filename.ls
    if ($status) then
        echo "$filename not found"
        set retval = 1
    else
        cat /tmp/$filename.ls
    endif
    \rm /tmp/$filename.ls

else if($action == "--dir") then
    hsi "ls -l $target_dir" |& tail --lines=+31 

else if ($action == "--get") then
    touch $filename.at_hpss
    hsi "firewall -on ; get $target_dir/$filename" | throttle $throttle_speed > $filename

else if ($action == "--put") then 
    set retry = 1
    while ($retry == 1)
    echo starting the put of $filename
    throttle $throttle_speed < $filename | hsi "firewall -on ; put - : $target_dir/$filename"
    # start transfer check
    set local_length = `ls -l $filename | awk '{print $5}'`
    set hpss_length = `hsi list -l $target_dir/$filename | & tail -1 | awk '{print $5}'`
    if ($local_length != $hpss_length) then
        echo "$filename transfer was bad (file length mismatch)"
        set retval = 1
        exit 1
    else
        hsi "firewall -on ; get -O ::$md5_check_length /tmp/$filename.start.hpss : $target_dir/$filename"
        head -c $md5_check_length $filename > /tmp/$filename.start.local
        set hpss_md5 = `md5sum  /tmp/$filename.start.hpss | awk '{print $1}'`
        set local_md5 = `md5sum  /tmp/$filename.start.local | awk '{print $1}'`
        if ($hpss_md5 != $local_md5) then
            echo "$filename transfer was bad (md5 mismatch at start of file)"
            set retval = 1
        else
            set end_offset = `python -c "print $hpss_length - $md5_check_length"`
            hsi "firewall -on ; get -O ${end_offset}::$md5_check_length /tmp/$filename.end.hpss : $target_dir/$filename"
            tail --bytes=$md5_check_length $filename > /tmp/$filename.end.local
            set hpss_md5 = `md5sum  /tmp/$filename.end.hpss | awk '{print $1}'`
            set local_md5 = `md5sum  /tmp/$filename.end.local | awk '{print $1}'`
            if ($hpss_md5 != $local_md5) then
                echo "$filename transfer was bad (md5 mismatch at end of file)"
                set retval = 1
            endif
            \rm /tmp/$filename.end.hpss
            \rm /tmp/$filename.end.local
        endif
        \rm /tmp/$filename.start.hpss
        \rm /tmp/$filename.start.local
    endif
    if ($retval == 0) then
        echo "$filename transfer was good"
        set retry = 0
    else
        set retry = 1
        echo retrying in 60 seconds
        sleep 60
    endif
    # end transfer check
    end
else if ($action == "--check") then
    # if the test ping failed above, we already exited, otherwise print success
    echo "hpss is up"
    set retval = 0 

else
    echo "usage $0:t --list | --dir | --get | --put | --check"
    set retval = 1

endif

echo " "
echo " "

exit $retval

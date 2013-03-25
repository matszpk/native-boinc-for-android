#! /bin/csh


if(${#argv} == 0 || $1 == "-h") then
    echo " "
    echo spikesbytape db tape_name
    echo " "
    exit
endif

set DB     = $1
set TAPE   = $2

source ~davea/seti/db/$DB

dbaccess -e - << HERE

database sah2;

select tape.id, tape.name, tape.beam, tape.last_block_done, 
        count(workunit_grp.id) from tape, workunit_grp 
  where 
        tape.name = "$TAPE" and 
        workunit_grp.tape_info = tape.id 
  group by 
        tape.id, tape.name, tape.beam, tape.last_block_done 
  order by 
        tape.name, tape.beam; 

select count(workunit.id)/3584 from workunit, workunit_grp, tape where
        workunit.group_info = workunit_grp.id   and
        workunit_grp.tape_info = tape.id        and
        tape.name = "$TAPE";


HERE

echo "checking down at HPSS:"
/disks/thumper/raid5_d/users/seti/dr2_data/production/scripts/seti_hsi.csh --list $TAPE

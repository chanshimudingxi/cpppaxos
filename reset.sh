#!/bin/bash

killall -9 node

rm /mnt/hgfs/share/shared/cpppaxos/bin/node*.log

cd /mnt/hgfs/share/shared/cpppaxos/build
make clean
make -j4

/mnt/hgfs/share/shared/cpppaxos/bin/node /mnt/hgfs/share/shared/cpppaxos/bin/node1.log -s "12345" -t tcp -y 10000 -n 20000
sleep 1
/mnt/hgfs/share/shared/cpppaxos/bin/node /mnt/hgfs/share/shared/cpppaxos/bin/node2.log -s "67890" -t tcp -y 20000 -n 10000
sleep 1
/mnt/hgfs/share/shared/cpppaxos/bin/node /mnt/hgfs/share/shared/cpppaxos/bin/node3.log -s "abcde" -t tcp -y 30000 -n 10000
sleep 1
/mnt/hgfs/share/shared/cpppaxos/bin/node /mnt/hgfs/share/shared/cpppaxos/bin/node4.log -s "fghij" -t tcp -y 40000 -n 10000
sleep 1
/mnt/hgfs/share/shared/cpppaxos/bin/node /mnt/hgfs/share/shared/cpppaxos/bin/node5.log -s "klmno" -t tcp -y 50000 -n 10000

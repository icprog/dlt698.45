#!/bin/sh

cj dog
cj stop
sleep 1
chmod +x /dos/cjgwn/zdtest698/zdtest698
export PATH=/dos/cjgwn/zdtest698:$PATH
export LD_LIBRARY_PATH=/dos/cjgwn/zdtest698:$LD_LIBRARY_PATH
zdtest698
sleep 1
cjmain all 2> /dev/shm/null &


#!/bin/bash

if [ "$#" -eq "1" ];then
ftp -v -n $1 << EOF
user root 1
binary
prompt off
cd /nand/bin
mput cj*
cd /nor/lib
mput lib*
close quit
EOF

else
    echo "Usage: up.sh 192.168.0.xxx"
    echo "Need pass a parameter for ip to upload."
fi

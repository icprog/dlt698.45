#!/bin/sh

rm -f /nand/Utest.sh
cp ./zdtest698/Utest.sh /nand/Utest.sh
sleep 1
chmod +x /nand/Utest.sh
/nand/Utest.sh&


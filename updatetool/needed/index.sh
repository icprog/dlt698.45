chmod +x /nand/bin/*
chmod +x /dos/cjgwn/cjup
/dos/cjgwn/cjup && /dos/cjgwn/cjup &
sleep 1
cp /dos/cjgwn/app/update.sh /nand/UpFiles/update.sh
sleep 3
echo reboot >> /nand/UpFiles/reboot
sleep 1

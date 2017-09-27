chmod +x /nand/bin/*
rm /nand/Version.log.new
rm /nand/Version.log
cj stop
sleep 1
#cp /dos/cjgwn/rc.local /etc/rc.d/
cp /dos/cjgwn/app/update.sh /nand/UpFiles/update.sh
sleep 3
echo reboot >> /nand/UpFiles/reboot
sleep 1

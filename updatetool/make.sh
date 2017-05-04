#!/bin/sh
#ROOTDIRSRC=/home/Administrator/jzq_gw_20121119
ROOTDIRSRC=$(pwd)
APPDIRSRC=$ROOTDIRSRC
SHNAME=update.sh
SHNAMEPATH=$ROOTDIRSRC/$SHNAME
DIROBJ=/nand/bin
LIBOBJ=/nor/lib
TMPDIR=/nand/UpFiles
ls
rm -rf cjgwn
rm -f update.sh
rm -f $APPDIRSRC/app.tar.gz
rm ./app/*
cp ../bin_arm/cj* ./app/
cp ../bin_arm/*.so ./app/
cp ../config/* ./app/
tar -cvf app.tar ./app
gzip -9 app.tar
rm -f $SHNAMEPATH
cd $ROOTDIRSRC
echo "#!/bin/sh" > $SHNAMEPATH
echo "mkdir -p "$TMPDIR"" >> $SHNAMEPATH
echo "(read l;read l;read l;read l;read l;read l;read l;read l;read l;read l;read l;exec cat ) <  $SHNAME | gunzip -  | tar -xvf - -C $TMPDIR && ls -l /nand/bin" >>  $SHNAMEPATH
echo "cp "$TMPDIR"/app/cj* /nand/bin/" >>$SHNAMEPATH
echo "cp "$TMPDIR"/app/lib* /nor/lib/" >> $SHNAMEPATH
echo "cp "$TMPDIR"/app/*.cfg /nor/config/" >> $SHNAMEPATH
echo "===" >> $SHNAMEPATH
echo "chmod +x "$DIROBJ"/*"  >> $SHNAMEPATH
echo "echo update finished!!!" >> $SHNAMEPATH
echo "rm -rf app" >> $SHNAMEPATH
echo "exit" >> $SHNAMEPATH
cat  $APPDIRSRC/app.tar.gz >> $SHNAMEPATH
#rm -f $APPDIRSRC/app.tar.gz
chmod +x  $SHNAMEPATH

mkdir cjgwn
mkdir cjgwn/app
cp update.sh ./cjgwn/app/
cp rc.local ./cjgwn/
cd app ;md5sum * > ../QCheck/md5.ini; cd ..
KI=`md5sum clean.sh | cut -d " " -f1`
rm update.sh
cat needed/head >> update.sh
echo 'md5='$KI >> update.sh
cat needed/tail >> update.sh
cat app.tar.gz >> update.sh

echo "cp /dos/cjgwn/app/update.sh /nand/UpFiles/update.sh" >> "./cjgwn/index.sh"
echo "cp /dos/cjgwn/rc.local /nor/rc.d/rc.local" >> ./cjgwn/index.sh
echo "echo reboot >> /nand/UpFiles/reboot" >> ./cjgwn/index.sh

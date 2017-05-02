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
cp app/* QCheck/app/

echo "cp /dos/cjgwn/app/update.sh /nand/UpFiles/update.sh" >> "./cjgwn/update.sh"
echo "cp /dos/cjgwn/rc.local /nor/rc.d/rc.local" >> ./cjgwn/update.sh
echo "echo reboot >> /nand/UpFiles/reboot" >> ./cjgwn/update.sh
#!/bin/sh
#ROOTDIRSRC=/home/Administrator/jzq_gw_20121119
ROOTDIRSRC=$(pwd)
APPDIRSRC=$ROOTDIRSRC
SHNAME=update.sh
SHNAMEPATH=$ROOTDIRSRC/$SHNAME
DIROBJ=/nand/bin
LIBOBJ=/nor/lib
TMPDIR=/nand/update
ls
rm -f $APPDIRSRC/app.tar.gz
tar -cvf app.tar *
gzip -9 app.tar
rm -f $SHNAMEPATH
cd $ROOTDIRSRC
echo "#!/bin/sh" > $SHNAMEPATH
echo "mkdir -p "$TMPDIR"" >> $SHNAMEPATH
echo "(read l;read l;read l;read l;read l;read l;read l;read l;read l;read l;read l;exec cat ) <  $SHNAME | gunzip -  | tar -xvf - -C $TMPDIR && ls -l /nand/bin" >>  $SHNAMEPATH
echo "cp "$TMPDIR"/app/cj* /nand/bin/" >>$SHNAMEPATH
echo "cp "$TMPDIR"/app/lib* /nor/lib/" >> $SHNAMEPATH
echo "cp "$TMPDIR"/app/*.cfg /nor/config/" >> $SHNAMEPATH
echo "rm -rf "$TMPDIR"/*" >> $SHNAMEPATH
echo "chmod +x "$DIROBJ"/*"  >> $SHNAMEPATH
echo "echo update finished!!!" >> $SHNAMEPATH
echo "================" >> $SHNAMEPATH
echo "exit" >> $SHNAMEPATH
cat  $APPDIRSRC/app.tar.gz >> $SHNAMEPATH
rm -f $APPDIRSRC/app.tar.gz
chmod +x  $SHNAMEPATH


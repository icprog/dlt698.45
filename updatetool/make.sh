#!/bin/sh

Usage()
{
    echo "============================================="
    echo "使用方式:"
    echo "\t./make 型号 地区\n"
    echo "\t设备类型，1：I型集中器，2:II型集中器，3：III型专变"
    echo "\tZheJiang(II型),HuNan(I型)"
    echo "============================================="
    exit 1
}

ParaCheck()
{
    if [ "$#" -ne "2" ]; then
        Usage
    fi

    if [ $1 -ne "1" ] && [ $1 -ne "2" ] && [ $1 -ne "3" ]; then
        Usage
    fi

    if [ $2 != "ZheJiang" ] && [ $2 != "HuNan" ]; then
         Usage
    fi

    echo "创建必要条件..."
    if [ ! -d app ]; then
        mkdir app
    fi
}

Clean()
{
    echo "清理文件..."
    rm -rf cjgwn
    rm -rf app.tar.gz
    rm -rf update.sh
}
CopyNew()
{
    echo "复制最新的程序与库..."
    cp ../bin_arm/cj* ./app/
    cp ../bin_arm/*.so ./app/
    cp ../config/* ./app/
}

Package()
{
    echo "打包自解压文件..."
    tar -cvf app.tar ./app
    gzip -9 app.tar

    KI=`md5sum app.tar.gz | cut -d " " -f1`
    cat needed/head >> update.sh
    echo 'md5='$KI >> update.sh
    cat needed/tail >> update.sh
    cat app.tar.gz >> update.sh
    cp update.sh ./cjgwn/app/
}

CreateUSB()
{
    echo "创建U盘升级包..."
    mkdir cjgwn
    mkdir cjgwn/app
    cp needed/rc.local ./cjgwn/
    cp needed/index.sh ./cjgwn/
}

Tools()
{
    echo "更新生产检测脚本..."
    cd app ;md5sum * > ../QCheck/md5.ini; cd ..
}

Post_Clean()
{
    echo "执行清理..."
    rm app.tar.gz
    rm -rf app
}

main()
{
    ParaCheck $1 $2
    Clean
    CopyNew
    Package
    CreateUSB
    Tools
    Post_Clean
}

main $1 $2
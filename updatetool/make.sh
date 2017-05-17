#!/bin/sh

Usage()
{
    echo "============================================="
    echo "使用方式:"
    echo "\t./make 型号 地区\n"
    echo "\t设备类型，1：I型集中器，2:II型集中器，3：III型专变"
    echo "\tZheJiang(II型),ShanDong(II型),HuNan(I型)"
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

    if [ $2 != "ZheJiang" ] && [ $2 != "HuNan" ] && [ $2 != "ShanDong" ]; then
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

UpdateLocation()
{
    rm app/device.cfg
    echo "[device]" >> app/device.cfg
    echo "device="$1 >> app/device.cfg
    echo "zone="$2 >> app/device.cfg
    echo "[end]" >> app/device.cfg
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
}

CreateUSB()
{
    echo "创建U盘升级包..."
    mkdir cjgwn
    mkdir cjgwn/app
    cp needed/rc.local ./cjgwn/
    cp needed/index.sh ./cjgwn/
    cp update.sh cjgwn/app
}

Tools()
{
    echo "更新生产检测脚本..."
    cd app ;md5sum * > ../QCheck/md5.ini; cd ..
    if [ -f QCheck/back/$2.$1.check.ini ] && [ -f QCheck/back/$2.$1.config.ini ]; then
        cp QCheck/back/$2.$1.check.ini QCheck/check.ini
        cp QCheck/back/$2.$1.config.ini QCheck/config.ini
    else
        echo ">>>>>>>>>>>>>>>>>>>>>>>>>"
        echo "没有找到对应类型的出厂参数...!"
        echo ">>>>>>>>>>>>>>>>>>>>>>>>>"
    fi
}

Composer()
{
    echo "生成集合包..."
    #$path = 'history/$1.$2.$(date +%Y%m%d%H)'
    mkdir -p history/$1.$2.$(date +%Y%m%d%H)
    cp -R QCheck history/$1.$2.$(date +%Y%m%d%H)
    cp app.tar.gz history/$1.$2.$(date +%Y%m%d%H)
    cp -R cjgwn history/$1.$2.$(date +%Y%m%d%H)
    cp update.sh history/$1.$2.$(date +%Y%m%d%H)
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
    UpdateLocation $1 $2
    Package
    CreateUSB
    Tools $1 $2
    Composer $1 $2
    Post_Clean
}

main $1 $2

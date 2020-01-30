#!/bin/sh

# Install toolchain for C-SKY:
# Ubuntu/Debian
# sudo add-apt-repository ppa:nationalchip-repo/gxtools
# sudo apt-get update
# sudo apt-get install csky-linux-tools-uclibc-2.8.01
# Manually:
# wget https://launchpad.net/~nationalchip-repo/+archive/ubuntu/gxtools/+sourcefiles/csky-linux-tools-uclibc-2.8.01/1.0.0.0/csky-linux-tools-uclibc-2.8.01_1.0.0.0.tar.xz
#
# export PATH=/opt/gxtools/csky-linux-tools-uclibc-20170724/bin:$PATH

export PATH=/opt/goxceed/csky-linux-tools-i386-uclibc-20170724/bin:/opt/gxtools/csky-linux-tools-uclibc-20170724/bin:$PATH

cd ./libdvbcsa
./build.sh
cd ..

if [ ! -f config.log ]; then
	CFLAGS="-I./libdvbcsa/src" LDFLAGS="-L./libdvbcsa/src/.libs" ./configure --host=csky-linux --enable-gxapi --enable-static --enable-dvbcsa --disable-satipc
fi

make clean
make

csky-linux-strip minisatip

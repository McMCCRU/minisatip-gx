#!/bin/sh

export PATH=/opt/goxceed/csky-linux-tools-i386-uclibc-20170724/bin:$PATH

cd ./libdvbcsa
./build.sh
cd ..

if [ ! -f config.log ]; then
	CFLAGS="-I./libdvbcsa/src" LDFLAGS="-L./libdvbcsa/src/.libs" ./configure --host=csky-linux --enable-gxapi --enable-static --enable-dvbcsa --disable-satipc
fi

make clean
make

csky-linux-strip minisatip

#!/bin/sh

if [ "$1" = "1" ]; then
	export PATH=/opt/goxceed/csky-linux/bin:$PATH
else
	export PATH=/opt/goxceed/csky-linux-tools-i386-uclibc-20170724/bin:/opt/gxtools/csky-linux-tools-uclibc-20170724/bin:$PATH
fi

if [ ! -f config.log ]; then
	./configure --host=csky-linux --enable-static --disable-shared
fi

make clean
make

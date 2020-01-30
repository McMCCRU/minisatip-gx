#!/bin/sh

export PATH=/opt/goxceed/csky-linux-tools-i386-uclibc-20170724/bin:/opt/gxtools/csky-linux-tools-uclibc-20170724/bin:$PATH

if [ ! -f config.log ]; then
	./configure --host=csky-linux --enable-static --disable-shared
fi

make clean
make

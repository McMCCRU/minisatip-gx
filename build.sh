#!/bin/sh

export PATH=/opt/goxceed/csky-linux-tools-i386-uclibc-20170724/bin:$PATH

if [ ! -f config.log ]; then
	./configure --host=csky-linux --enable-gxapi --enable-static --disable-satipc
fi

make
csky-linux-strip minisatip

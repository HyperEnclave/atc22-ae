#!/bin/bash

ROOT=${PWD}

if [ ! -d lighttpd-1.4.40 ]
then
  [ ! -f lighttpd-1.4.40.tar.gz ] && wget https://download.lighttpd.net/lighttpd/releases-1.4.x/lighttpd-1.4.40.tar.gz
  tar -xvf lighttpd-1.4.40.tar.gz
fi

cp plugin-static.h lighttpd-1.4.40/src/
cd lighttpd-1.4.40

LIGHTTPD_STATIC=yes CC=occlum-gcc ./configure --disable-shared --without-zlib --without-bzip2 --without-pcre --disable-ipv6 -prefix=${ROOT}/_install
make -j && make install -j \
  || { printf " occlum build lighttpd failed\n"; exit 1; }
cd ..

cp lighttpd-1.4.40/src/lighttpd .
rm -rf lighttpd-1.4.40.tar.gz lighttpd-1.4.40

SIZES="1 10 20 30 40 50 60 70 80 90 100"
for S in $SIZES
do
    dd if=/dev/urandom of=${ROOT}/workspace/${S}K.html count=${S}K bs=1
done

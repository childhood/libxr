#!/bin/sh

export PKG_CONFIG_PATH=/usr/i486-mingw32/lib/pkgconfig

./autogen.sh --host=i486-mingw32 --prefix=/usr/i486-mingw32 --enable-shared
make -j3

rm win32-test/*
mkdir -p win32-test

cp /usr/i486-mingw32/bin/*.dll win32-test/
cp tests/*.{exe,pem} win32-test/

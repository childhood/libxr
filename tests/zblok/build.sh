#!/bin/sh

xdl-compiler -i zblok.xdl -o .

gcc -o zm-server server.c ZMCommon.c ZMServer.c ZMServer.stubs.c \
  ZMServer.xrs.c `pkg-config --cflags --libs libxr` -lssl -lcrypto

gcc -o zm-client client.c ZMCommon.c ZMServer.c ZMServer.xrc.c \
  `pkg-config --cflags --libs libxr` -lssl -lcrypto

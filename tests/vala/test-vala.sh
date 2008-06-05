#!/bin/sh

XDLC=../../xdl-compiler/xdl-compiler
VALAC=/workspace/external/vala/compiler/valac
VALAC=valac
VALAFLAGS=--thread

$XDLC -m vapi -i test.xdl -o .
$XDLC -m pub-headers -i test.xdl -o .
$XDLC -m pub-impl -i test.xdl -o .

$VALAC $VALAFLAGS --vapidir=. --pkg=T -C test.vala

gcc -o test test.c T*.c -I. -I../../include -I../../ `pkg-config --libs --cflags libxr gobject-2.0`

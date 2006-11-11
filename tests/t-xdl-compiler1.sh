#!/bin/sh

XDLC=../xdl-compiler/xdl-compiler

rm -rf t
mkdir -p t
$XDLC -i test.xdl -o .

exit $?

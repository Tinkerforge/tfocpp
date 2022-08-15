#!/bin/sh
for file in [^t]*.c; do clang -fPIC -g $file -c -o $file.o; done

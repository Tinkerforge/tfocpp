#!/bin/sh
LD_PRELOAD=$(clang -print-file-name=libclang_rt.asan-x86_64.so) LSAN_OPTIONS=suppressions=lsan_suppr.txt python -m unittest discover --buffer -k "$@"

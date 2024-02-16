#!/bin/sh
make clean
bear -- make
clang-tidy --checks="-*,bugprone-*,concurrency-*,performance-*,portability-*,-bugprone-narrowing-conversions,-bugprone-easily-swappable-parameters" src/ocpp/*.h src/ocpp/*.cpp

#!/bin/bash

code="$PWD"
opts=-g
cd build > /dev/null
g++ $opts $code/code/main.cpp -o vkgame.exe
cd $code > /dev/null

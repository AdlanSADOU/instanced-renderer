#!/bin/bash

code="$PWD"
opts=-g
cd bin > /dev/null
g++ $opts $code/src/*.cpp -o vklib
cd $code > /dev/null

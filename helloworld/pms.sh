#!/bin/bash

mpic++ --prefix /usr/local/share/OpenMPI -g -o pms pms.cpp

mpirun --prefix /usr/local/share/OpenMPI -np $1 pms
#mpirun --prefix /usr/local/share/OpenMPI -np $1 -n $1 xterm -e gdb -q -tui -x gdb.txt ./pms

rm -f hello

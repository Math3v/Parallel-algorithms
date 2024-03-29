#!/bin/bash
 
mat1=$(head -n1 mat1)
mat2=$(head -n1 mat2)
 
cpus=$((mat1*mat2))
 
mpic++ --prefix /usr/local/share/OpenMPI -g -o mm mm.cpp -std=c++0x
mpirun --prefix /usr/local/share/OpenMPI -np $cpus mm
#mpirun --prefix /usr/local/share/OpenMPI -np $cpus -n $cpus xterm -e gdb -q -tui -x gdb.txt ./mm
rm -f mm
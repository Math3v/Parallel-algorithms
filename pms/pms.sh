#!/bin/bash

dd if=/dev/random of=numbers bs=$1 count=1 >& /dev/null
F_NUMPROC=`echo "l($1)/l(2)" | bc -l`
NUMPROC=${F_NUMPROC%.*}
((NUMPROC++))

#echo "Starting test with $1 values and $NUMPROC processors"


mpic++ --prefix /usr/local/share/OpenMPI -g -o pms pms.cpp

START=$(($(date +%s%N)/1000000))
mpirun --prefix /usr/local/share/OpenMPI -np $NUMPROC pms
#mpirun --prefix /usr/local/share/OpenMPI -np $NUMPROC -n $NUMPROC xterm -e gdb -q -tui -x gdb.txt ./pms
END=$(($(date +%s%N)/1000000))

#echo "Time: `expr $END - $START`"

rm -f pms
rm -f numbers

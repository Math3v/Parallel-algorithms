#!/bin/bash
#set -x

RES=0
TOTAL=0
TESTRUN=100

if (($# != 1))
then
	echo "Error: Please specify maximum power of 2"
	exit 1
fi

MAX=$1

for (( i=1; i<=$MAX; i++))
do
	RES=`echo "2^$i" | bc`
	echo -n "$RES "

	for (( c=1; c<=$TESTRUN; c++ ))
	do
		#ARR[$c]=`./pms.sh "$RES" | tac | paste -s -d '-' | bc`
		ARR[$c]=`./pms.sh "$RES"`
		#echo "${ARR[$c]}"
	done

	TOTAL=0
	for var in "${ARR[@]}"
	do
		#var=`echo "(0$var*10000000)/1" | bc`
		#echo "From bc $var"
		TOTAL=$(($TOTAL + $var))
	done
	#echo "$TOTAL / $TESTRUN"
	average=$(($TOTAL/$TESTRUN))
	echo "$average"
done
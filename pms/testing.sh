#!/bin/bash
#set -x

RES=0
TOTAL=0
TESTRUN=100

for i in {2..14}
do
	RES=`echo "2^$i" | bc`
	echo "$RES"

	for (( c=1; c<=$TESTRUN; c++ ))
	do
		ARR[$c]=`./pms.sh "$RES" | paste -s -d '+' | bc`
		#echo "${ARR[$c]}"
	done

	for var in "${ARR[@]}"
	do
		var=`echo "(0$var*10000000)/1" | bc`
		#echo "From bc $var"
		TOTAL=$(($TOTAL + $var))
	done
	average=$(($TOTAL/$TESTRUN))
	echo "Average $average"
done
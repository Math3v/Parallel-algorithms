#!/bin/bash

#set -x

TESTRUN=10
TOTAL=0

touch mat1
touch mat2

for i in {4..20}
do
	echo -n "Matrix $i x $i: "
	MAX=$i

	echo "$MAX" >> mat1
	#Generate mat1 file
	for (( i=1; i<=$MAX; i++))
	do
		for (( j=1; j<=$MAX; j++))
		do
			number=$RANDOM
			let "number %= 20"
			echo -n "$number " >> mat1
		done
			echo "" >> mat1
	done

	echo "$MAX" >> mat2
	#Generate mat2 file
	for (( i=1; i<=$MAX; i++))
	do
		for (( j=1; j<=$MAX; j++))
		do
			number=$RANDOM
			let "number %= 20"
			echo -n "$number " >> mat2
		done
			echo "" >> mat2
	done

	for (( c=1; c<=$TESTRUN; c++ ))
	do
		ARR[$c]=`./test.sh`
	done

	TOTAL=0
	for var in "${ARR[@]}"
	do
		TOTAL=$(($TOTAL + $var))
	done
	average=$(($TOTAL/$TESTRUN))
	echo "$average"

	rm mat1
	rm mat2
done

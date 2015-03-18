#!/bin/bash

RES=0

for i in {2..14}
do
	RES=`echo "2^$i" | bc`
	echo "$RES"

	F=`./pms.sh "$RES" | paste -s -d '+' | bc`
	S=`./pms.sh "$RES" | paste -s -d '+' | bc`
	T=`./pms.sh "$RES" | paste -s -d '+' | bc`

	echo "$F+$S+$T"
	echo "scale=6;($F+$S+$T)/3" | bc
done
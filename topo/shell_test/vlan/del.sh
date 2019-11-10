#!/bin/bash

for i in a b
do
	ip link set dev br-$i down
	brctl delbr br-$i
done

for i in 1 2
do 
	ip link del tap$i-a
done


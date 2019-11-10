#!/bin/bash

for i in a b
do
	brctl addbr br-$i
	brctl stp br-$i off
	ip link set dev br-$i up
done

for i in 1 2
do
	ip link add tap$i-a type veth peer name tap$i-b
	vconfig add tap$i-a $[i+2]
	vconfig add tap$i-b $[i+2]
	ip link set dev tap$i-a up
	ip link set dev tap$i-a.$[i+2] up
	ip link set dev tap$i-b up
	ip link set dev tap$i-b.$[i+2] up
done

# for i in 1 2
# do
# 	brctl addif br-a tap$i-a.$[i+2]
# 	brctl addif br-b tap$i-b.$[i+2]
# done
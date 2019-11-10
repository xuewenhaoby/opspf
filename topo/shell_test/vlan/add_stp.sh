#!/bin/bash

for i in a b
do
	brctl addbr br-$i
	brctl stp br-$i on
	ip link set dev br-$i up
	ip netns add ns-$i
	ip link add p-$i type veth peer name b-$i
	ip link set dev p-$i up
	ip link set dev b-$i up
done

for i in 1 2
do
	ip link add tap$i-a type veth peer name tap$i-b
	brctl addif br-a tap$i-a
	brctl addif br-b tap$i-b
	ip link set dev tap$i-a up
	ip link set dev tap$i-b up
done
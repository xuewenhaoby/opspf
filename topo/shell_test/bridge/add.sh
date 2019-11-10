#!/bin/bash

ip netns add ns1
ip link add tap1 type veth peer name tap2
brctl addbr br1
ip link set tap2 netns ns1
ip link set tap1 master br1
ip netns exec ns1 ip addr add 192.168.1.11/24 dev tap2
ifconfig ens33 0.0.0.0
ip link set ens33 master br1
ip addr add 192.168.1.10/24 dev br1
ip link set br1 up
ip link set tap1 up
ip netns exec ns1 ip link set tap2 up

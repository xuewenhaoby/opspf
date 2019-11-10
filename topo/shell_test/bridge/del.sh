#!/bin/bash

ip netns del ns1
ip link set br1 down
brctl delbr br1

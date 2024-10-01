#!/bin/bash

# Create namespaces
sudo ip netns add server_ns
sudo ip netns add client_ns

# Create veth pair
sudo ip link add veth-server type veth peer name veth-client

# Move veth interfaces to the namespaces
sudo ip link set veth-server netns server_ns
sudo ip link set veth-client netns client_ns

# Assign IP addresses to the interfaces
sudo ip netns exec server_ns ip addr add 192.168.1.1/24 dev veth-server
sudo ip netns exec client_ns ip addr add 192.168.1.2/24 dev veth-client

# Bring the interfaces up
sudo ip netns exec server_ns ip link set veth-server up
sudo ip netns exec client_ns ip link set veth-client up

# Bring the loopback interfaces up in both namespaces
sudo ip netns exec server_ns ip link set lo up
sudo ip netns exec client_ns ip link set lo up

echo "Namespaces and veth interfaces set up successfully."

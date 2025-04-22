sudo ip netns exec ns-router ip link delete veth-router
# sudo ip netns exec ns-app ip link delete veth-app
sudo ip link delete veth-host
# sudo ip netns exec ns-router ip link delete veth-routerhost
sudo ip netns delete ns-router
sudo ip netns delete ns-app

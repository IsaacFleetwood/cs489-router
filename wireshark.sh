sudo ip netns exec ns-router wireshark -i veth-hostroute -k &
sudo ip netns exec ns-app wireshark -i veth-app -k &

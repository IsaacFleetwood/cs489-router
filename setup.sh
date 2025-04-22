sudo bash -c "echo 1 > /proc/sys/net/ipv4/ip_forward"

sudo ip netns add ns-router
sudo ip netns add ns-app

sudo ip link add veth-router type veth peer name veth-app
sudo ip link add veth-host type veth peer name veth-hostroute
sudo ip link set veth-router netns ns-router
sudo ip link set veth-app netns ns-app
sudo ip link set veth-hostroute netns ns-router

# sudo ip netns exec ns-app ip addr add 192.168.1.2/24 dev veth-app
sudo ip netns exec ns-router ip addr add 192.168.1.1/24 dev veth-router
sudo ip netns exec ns-router ip addr add 192.168.0.2/24 dev veth-hostroute
sudo ip addr add 192.168.0.1/24 dev veth-host

sudo ip link set veth-host up
sudo ip netns exec ns-router ip link set veth-hostroute up
sudo ip netns exec ns-router ip link set veth-router up
sudo ip netns exec ns-app ip link set veth-app up

# sudo ip netns exec ns-router ip route add default via 192.168.0.1
# sudo ip netns exec ns-app ip route add default via 192.168.1.1

sudo iptables -t nat -A POSTROUTING -s 192.168.0.0/24 -o $(cat ./interface_name.txt) -j MASQUERADE
# sudo ip netns exec ns-router iptables -A INPUT -i veth-router -j DROP
sudo ip netns exec ns-router iptables -A FORWARD -j DROP
sudo ip netns exec ns-router iptables -A INPUT -i veth-router -p udp --sport 68 --dport 67 -j ACCEPT
sudo ip netns exec ns-router iptables -A INPUT -j DROP
sudo ip netns exec ns-router iptables -A OUTPUT -p icmp --icmp-type destination-unreachable -j DROP

# Make sure frames going over veth bridgs properly use the MTU of 1500.
sudo ethtool -K veth-host gso off tso off gro off
sudo ip netns exec ns-app ethtool -K veth-app gso off tso off gro off

# echo 1 > /proc/sys/net/ipv4/ip_forward

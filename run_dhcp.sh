ip netns exec ns-router ./router/main &
routerpid=$!
sleep 1
echo "Requesting DHCP lease..."
ip netns exec ns-app dhclient veth-app &
dhcppid=$!
ip netns exec ns-app sudo -u user firefox 2>/dev/null 1>/dev/null
kill $routerpid
kill $dhcppid

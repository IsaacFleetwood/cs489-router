ip netns exec ns-router ./router/main &
routerpid=$!
ip netns exec ns-app sudo -u user firefox 2>/dev/null 1>/dev/null
kill $routerpid

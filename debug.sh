ip netns exec ns-app sudo -u user firefox 2>/dev/null 1>/dev/null &
pid=$!
ip netns exec ns-router gdb ./router/main
kill $pid

#!/bin/bash

cd ./projects/terminal-chatroom/build
echo "killing server"
kill $(pidof server)
echo "server killed"
make clean
make server
echo "starting server "
./server &
echo "server started"
exit
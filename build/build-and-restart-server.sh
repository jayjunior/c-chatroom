#!/bin/bash

echo "killing server"
kill $(pidof server)
echo "server killed"
make clean
make server
echo "starting server "
./server &
echo "server started"
exit
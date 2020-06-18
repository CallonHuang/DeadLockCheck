#!/bin/bash

gcc -Wall -g -O -fPIC -I./ -c list.c -o list.o
gcc -Wall -g -O -fPIC -I./ -c deadLockCheck.c -o deadLockCheck.o
gcc -Wall -g -shared -fPIC -o libDeadLockCheck.so deadLockCheck.o list.o -lpthread
if [ $? -ne 0 ];then
    exit -1;
fi
#sudo cp libDeadLockCheck.so /usr/lib
gcc -g -Wall main.c -L./ -o main -lDeadLockCheck -lpthread

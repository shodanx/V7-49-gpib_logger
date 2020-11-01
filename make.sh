#!/bin/sh

mkdir -p bin tmpfs
indent -kr -l210 -bli0 -di1 -i2 -bbo -nip -lp -nbad -nsai -bl -bls -nut  src/*.c
gcc ./src/main.c -O1 -lgpib -lncursesw -Wall -D_GNU_SOURCE -D_POSIX_PTHREAD_SEMANTICS -o ./bin/v7-49_logger 
umount ./tmpfs
mount -t tmpfs tmpfs tmpfs

screen -r v749
if [ $? -eq 0 ]
then
  exit 0
else
  screen -S v749  ./bin/v7-49_logger 
  exit 0
fi


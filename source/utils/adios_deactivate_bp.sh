#!/bin/bash

FNAME=$1
if [ "x$FNAME" == "x" ]; then
    echo "Missing BP data set name"
    exit 1
fi

if [ ! -d $FNAME ]; then
    echo "Not a valid BP4 output: $FNAME"
    echo "BP4 data set should be a directory"
    exit 2
fi

if [ ! -f $FNAME/md.idx ]; then
    echo "Not a valid BP4 output: $FNAME"
    echo "Missing $FNAME/md.idx file"
    exit 3
fi

printf '\x00' | dd of=$FNAME/md.idx bs=1 seek=38 conv=notrunc


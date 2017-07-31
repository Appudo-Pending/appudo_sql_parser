#!/bin/sh
if [ $2 -eq 0 ]; then
strip --strip-unneeded $1
fi

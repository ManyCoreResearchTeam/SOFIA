#!/bin/sh

if [ $# -lt 2 ]; then
  echo "USAGE: $0 <image> <dir>"
  exit 1
fi

image=$1
fs=$2
mkdir ${fs}
gzip -d < ${image} | (cd ${fs}; cpio --extract)


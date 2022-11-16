#!/bin/sh

if [ $# -lt 2 ]; then
  echo "USAGE: $0 <dir> <image>"
  exit 1
fi

fs=$1
image=$2

(cd ${fs}; find . | cpio -H newc -o) | gzip > ${image} 

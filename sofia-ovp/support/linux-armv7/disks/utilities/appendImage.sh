#!/bin/sh

if [ $# -lt 3 ]; then
  echo "USAGE: $0 <dir> <readimage> <writeimage>"
  exit 1
fi

if [ "$(uname)" = "Linux" ]; then
  CPIO=cpio
else
  CPIO="C:/Program Files/GnuWin32/bin/cpio.exe"
fi

fs=$1
readimage=$2
writeimage=$3
gzip -d -c ${readimage} > ${readimage}.cpio
(find ${fs} | "${CPIO}" -A -H newc -o -F ${readimage}.cpio)
gzip -c ${readimage}.cpio > ${writeimage}
rm -f ${readimage}.cpio

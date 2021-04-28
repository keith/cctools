#!/bin/bash

set -euo pipefail

if [[ $# -ne 1 ]]; then
  echo "usage: $0 URL" >&2
  exit 1
fi

readonly url=$1

rm -rf  ./APPLE_LICENSE ./ar ./as ./cbtlibs ./cctools.xcodeproj ./efitools ./gprof ./include ./ld ./libmacho ./libstuff ./libstuff_test ./man ./misc ./otool ./tests ./xcode

curl -L --fail "$url" -o /tmp/ld.tar.gz
tar xf /tmp/ld.tar.gz --strip-components=1 -C .

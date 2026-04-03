#!/bin/bash

set -eu

mk
./o.factotum-keyring &
pid=$!

trap 'kill "$pid"; exit 1' 1 2 3 15
go -C demo run .
kill "$pid"
exit 0

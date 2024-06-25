#!/bin/bash
echo "Syncing source code..."
rsync -r -ssh common avr librckid utils esp8266 secrets.h peta@10.0.0.38:/home/peta/devel/rckid2 --delete
echo "Buiding ${1}"
ssh peta@10.0.0.38 "cd ~/devel/rckid2/${1} && pio run --target upload"

#!/bin/bash
echo "Syncing source code..."
rsync -r -ssh include rckid test-avr utils peta@10.0.0.38:/home/peta/devel/rckid2 --delete
echo "Buiding test-avr"
ssh peta@10.0.0.38 "cd ~/devel/rckid2/test-avr && pio run --target upload"

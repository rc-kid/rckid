#!/bin/bash
echo "Syncing source code..."
rsync -r -ssh include librckid avr/rckid-test utils peta@10.0.0.38:/home/peta/devel/rckid2 --delete
echo "Buiding test-avr"
ssh peta@10.0.0.38 "cd ~/devel/rckid2/avr/rckid-test && pio run --target upload"

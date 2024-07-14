#!/bin/bash
echo "Syncing firmware $1/.pio/build/$2/firmware.bin"
#rsync -ar -ssh $1/.pio/build/$2/firmware.elf $1/.pio/build/$2/ld peta@10.0.0.38:/home/peta/devel/rckid2/$1/.pio/build/$2
#echo "Uploading"
#ssh peta@10.0.0.38 "cd ~/devel/rckid2/$1 && pio run -t nobuild -t upload --disable-auto-clean"

rsync -rv -ssh --exclude .pio/ base-station wifi-bridge cartridge common platform tools secrets.h dev-server:/home/peta/devel/rckid2 --delete
echo "Buiding ${1}"
ssh dev-server "cd ~/devel/rckid2/${1} && pio run --target upload"

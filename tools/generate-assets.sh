#!/bin/bash
#export DISPLAY=0:0
echo "Generating RGB 332 to 565 conversion table..."
build-utils/generate-rgb-332-to-565 rckid/librckid/rckid/graphics/palette_332.h
echo "Generating assets from static files..."
build-utils/generate-assets assets/rckid rckid/librckid/assets
echo "Generating fonts (Iosevka)..."
build-utils/generate-font assets/fonts/Iosevka.ttf 16 rckid/librckid/assets/fonts
build-utils/generate-font assets/fonts/Iosevka.ttf 20 rckid/librckid/assets/fonts
build-utils/generate-font assets/fonts/Iosevka.ttf 32 rckid/librckid/assets/fonts
echo "Generating fonts (OpenDyslexic)..."
build-utils/generate-font assets/fonts/OpenDyslexic.otf 16 rckid/librckid/assets/fonts
build-utils/generate-font assets/fonts/OpenDyslexic.otf 32 rckid/librckid/assets/fonts
build-utils/generate-font assets/fonts/OpenDyslexic.otf 48 rckid/librckid/assets/fonts
echo "Generating fonts (Symbols)..."
build-utils/generate-font assets/fonts/SymbolsNF.ttf 16 rckid/librckid/assets/fonts --symbols
build-utils/generate-font assets/fonts/SymbolsNF.ttf 20 rckid/librckid/assets/fonts --symbols
build-utils/generate-font assets/fonts/SymbolsNF.ttf 32 rckid/librckid/assets/fonts --symbols
echo "Generating fonts (Hasklug)..."
build-utils/generate-font assets/fonts/Hasklug.otf 16 rckid/librckid/assets/fonts
echo "Generating fonts (Hurmit)..."
build-utils/generate-font assets/fonts/Hurmit.otf 16 rckid/librckid/assets/fonts
echo "Generating fonts (VictorMono)..."
build-utils/generate-font assets/fonts/VictorMono.ttf 16 rckid/librckid/assets/fonts
build-utils/generate-font assets/fonts/VictorMonoBold.ttf 16 rckid/librckid/assets/fonts

build-utils/generate-font assets/fonts/Lilly.ttf 16 rckid/librckid/assets/fonts
build-utils/generate-font assets/fonts/Inconsolata.ttf 16 rckid/librckid/assets/fonts
build-utils/generate-font assets/fonts/PixelFJVerdana.ttf 16 rckid/librckid/assets/fonts
build-utils/generate-font assets/fonts/DPComic.ttf 16 rckid/librckid/assets/fonts

build-utils/generate-font assets/fonts/Lilly.ttf 24 rckid/librckid/assets/fonts
build-utils/generate-font assets/fonts/Inconsolata.ttf 24 rckid/librckid/assets/fonts
build-utils/generate-font assets/fonts/PixelFJVerdana.ttf 24 rckid/librckid/assets/fonts
build-utils/generate-font assets/fonts/DPComic.ttf 24 rckid/librckid/assets/fonts

echo "Generating interpolation tables..."
build-utils/generate-interp-tables 256 rckid/librckid/rckid/utils/tables.h

#echo "Generating UI tileset..."
#build-utils/generate-font-tiles assets/fonts/Iosevka.ttf rckid/librckid/assets/tiles


echo "Generating debug assets..."
mkdir -p rckid/librckid/assets-debug
build-utils/generate-assets assets/debug rckid/librckid/assets-debug
build-utils/int16-to-cpp assets/raw/faryra.raw > rckid/librckid/assets-debug/faryra.raw.inc
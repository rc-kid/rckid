#!/bin/bash
export DISPLAY=0:0
echo "Generating RGB 332 to 565 conversion table..."
build-utils/generate-rgb-332-to-565 librckid/rckid/graphics/palette_332.h
echo "Generating assets from static files..."
build-utils/generate-assets assets/rckid librckid/assets
echo "Generating fonts (Iosevka)..."
build-utils/generate-font assets/fonts/Iosevka.ttf 16 librckid/assets/fonts
build-utils/generate-font assets/fonts/Iosevka.ttf 20 librckid/assets/fonts
build-utils/generate-font assets/fonts/Iosevka.ttf 32 librckid/assets/fonts
echo "Generating fonts (OpenDyslexic)..."
build-utils/generate-font assets/fonts/OpenDyslexic.otf 32 librckid/assets/fonts
build-utils/generate-font assets/fonts/OpenDyslexic.otf 48 librckid/assets/fonts
echo "Generating fonts (Symbols)..."
build-utils/generate-font assets/fonts/SymbolsNF.ttf 16 librckid/assets/fonts --symbols
build-utils/generate-font assets/fonts/SymbolsNF.ttf 20 librckid/assets/fonts --symbols
build-utils/generate-font assets/fonts/SymbolsNF.ttf 32 librckid/assets/fonts --symbols
echo "Generating interpolation tables..."
build-utils/generate-interp-tables 256 librckid/rckid/audio/wave_tables.h


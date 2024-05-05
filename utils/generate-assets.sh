#!/bin/bash
export DISPLAY=0:0
echo "Generating assets from static files..."
build-utils/generate-assets assets/rckid librckid/assets
echo "Generating fonts (Iosevka)..."
build-utils/generate-font assets/fonts/IosevkaNF.ttf 16 8 librckid/assets/fonts
build-utils/generate-font assets/fonts/IosevkaNF.ttf 16 4 librckid/assets/fonts
build-utils/generate-font assets/fonts/IosevkaNF.ttf 16 2 librckid/assets/fonts
build-utils/generate-font assets/fonts/IosevkaNF.ttf 20 8 librckid/assets/fonts
build-utils/generate-font assets/fonts/IosevkaNF.ttf 20 4 librckid/assets/fonts
build-utils/generate-font assets/fonts/IosevkaNF.ttf 20 2 librckid/assets/fonts
build-utils/generate-font assets/fonts/IosevkaNF.ttf 32 8 librckid/assets/fonts
build-utils/generate-font assets/fonts/IosevkaNF.ttf 32 4 librckid/assets/fonts
build-utils/generate-font assets/fonts/IosevkaNF.ttf 32 2 librckid/assets/fonts
echo "Generating fonts (OpenDyslexic)..."
build-utils/generate-font assets/fonts/OpenDyslexicNF.otf 32 8 librckid/assets/fonts
build-utils/generate-font assets/fonts/OpenDyslexicNF.otf 32 4 librckid/assets/fonts
build-utils/generate-font assets/fonts/OpenDyslexicNF.otf 32 2 librckid/assets/fonts
build-utils/generate-font assets/fonts/OpenDyslexicNF.otf 48 8 librckid/assets/fonts
build-utils/generate-font assets/fonts/OpenDyslexicNF.otf 48 4 librckid/assets/fonts
build-utils/generate-font assets/fonts/OpenDyslexicNF.otf 48 2 librckid/assets/fonts
echo "Generating fonts (Symbols)..."
build-utils/generate-font assets/fonts/SymbolsNF.ttf 16 8 librckid/assets/fonts --symbols
build-utils/generate-font assets/fonts/SymbolsNF.ttf 16 4 librckid/assets/fonts --symbols
build-utils/generate-font assets/fonts/SymbolsNF.ttf 16 2 librckid/assets/fonts --symbols
build-utils/generate-font assets/fonts/SymbolsNF.ttf 20 8 librckid/assets/fonts --symbols
build-utils/generate-font assets/fonts/SymbolsNF.ttf 20 4 librckid/assets/fonts --symbols
build-utils/generate-font assets/fonts/SymbolsNF.ttf 20 2 librckid/assets/fonts --symbols
build-utils/generate-font assets/fonts/SymbolsNF.ttf 32 8 librckid/assets/fonts --symbols
build-utils/generate-font assets/fonts/SymbolsNF.ttf 32 4 librckid/assets/fonts --symbols
build-utils/generate-font assets/fonts/SymbolsNF.ttf 32 2 librckid/assets/fonts --symbols


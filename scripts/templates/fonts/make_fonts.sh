#!/bin/bash
# Rebuild the dashboard's embedded fonts from the Ubuntu font family (Ubuntu Font
# Licence 1.0, see UFL-LICENCE.txt; Debian/Ubuntu package fonts-ubuntu), subset to
# Latin so the self-contained dashboard stays small. Requires python3 fontTools.
set -euo pipefail
cd "$(dirname "$0")"
SRC=/usr/share/fonts/truetype/ubuntu
UNI='U+0020-007E,U+00A0-00FF,U+2013-2014,U+2018-201D,U+2022,U+2026,U+2190-2193,U+2212,U+2264-2265,U+00D7,U+2248'
FLAVOR=woff
python3 -c 'import brotli' 2>/dev/null && FLAVOR=woff2
rm -f UbuntuSans.* UbuntuSansMono.*
python3 -m fontTools.subset "$SRC/UbuntuSans[wdth,wght].ttf" --unicodes="$UNI" --layout-features='*' \
    --flavor="$FLAVOR" --output-file="UbuntuSans.$FLAVOR"
python3 -m fontTools.subset "$SRC/UbuntuSansMono[wght].ttf" --unicodes="$UNI" --layout-features='*' \
    --flavor="$FLAVOR" --output-file="UbuntuSansMono.$FLAVOR"
ls -la

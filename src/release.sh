#!/bin/sh

# Setup base bes directory for software release
# Run this from within the bes/src directory!

# Wipe out the config database
./emptydb.sh

# Wipe out all save states, SRAM saves, ROMs, and screenshots
cd ..
rm -rf roms
rm -rf srams
rm -rf saves

mkdir -p roms/gb
mkdir -p roms/nes
mkdir -p roms/snes

mkdir -p srams/gb
mkdir -p srams/nes
mkdir -p srams/snes

mkdir -p saves/gb
mkdir -p saves/nes
mkdir -p saves/snes

  

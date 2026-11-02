#!/bin/bash
# Installs NeoChords for the current user. Double-click (or right-click > Open the first time).
cd "$(dirname "$0")" || exit 1

VST3_DIR="$HOME/Library/Audio/Plug-Ins/VST3"
AU_DIR="$HOME/Library/Audio/Plug-Ins/Components"
mkdir -p "$VST3_DIR" "$AU_DIR"

rm -rf "$VST3_DIR/NeoChords.vst3" "$AU_DIR/NeoChords.component"
cp -R NeoChords.vst3 "$VST3_DIR/"
cp -R NeoChords.component "$AU_DIR/"

# The build is not notarised by Apple, so clear the download quarantine flag.
xattr -dr com.apple.quarantine "$VST3_DIR/NeoChords.vst3" "$AU_DIR/NeoChords.component" 2>/dev/null
killall -9 AudioComponentRegistrar 2>/dev/null

echo ""
echo "NeoChords installed:"
echo "  $VST3_DIR/NeoChords.vst3"
echo "  $AU_DIR/NeoChords.component"
echo ""
echo "Next: in FL Studio go to Options > Manage plugins > Find installed plugins."

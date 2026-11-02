#!/bin/bash
# Builds NoviChords on this Mac and installs it for FL Studio. Logs to build.log next to this script.
cd "$(dirname "$0")/.." || exit 1
ROOT="$(pwd)"
LOG="$ROOT/build.log"
exec > >(tee "$LOG") 2>&1
echo "STATUS: started $(date)"

if ! xcode-select -p >/dev/null 2>&1 || ! xcrun --find clang >/dev/null 2>&1; then
  echo "STATUS: need-clt"
  echo "Apple's Command Line Tools are needed. A dialog will appear - click Install, then run this again."
  xcode-select --install
  exit 2
fi
echo "clang: $(xcrun --find clang)"

CMAKE="$(command -v cmake)"
if [ -z "$CMAKE" ]; then
  for c in /opt/homebrew/bin/cmake /usr/local/bin/cmake /Applications/CMake.app/Contents/bin/cmake "$ROOT/.tools/cmake/CMake.app/Contents/bin/cmake"; do
    [ -x "$c" ] && CMAKE="$c" && break
  done
fi
if [ -z "$CMAKE" ]; then
  echo "STATUS: downloading-cmake"
  mkdir -p "$ROOT/.tools" && cd "$ROOT/.tools" || exit 1
  curl -fL -o cmake.tgz https://github.com/Kitware/CMake/releases/download/v3.30.5/cmake-3.30.5-macos-universal.tar.gz || { echo "STATUS: failed cmake download"; exit 3; }
  tar xzf cmake.tgz && rm -rf cmake && mv cmake-3.30.5-macos-universal cmake
  CMAKE="$ROOT/.tools/cmake/CMake.app/Contents/bin/cmake"
  cd "$ROOT" || exit 1
fi
echo "cmake: $CMAKE ($("$CMAKE" --version | head -1))"

echo "STATUS: configuring"
"$CMAKE" -B build -DCMAKE_BUILD_TYPE=Release || { echo "STATUS: failed configure"; exit 4; }

echo "STATUS: compiling"
"$CMAKE" --build build --config Release --target NoviChords_VST3 NoviChords_AU EngineTest -j "$(sysctl -n hw.ncpu)" || { echo "STATUS: failed build"; exit 5; }

./build/EngineTest | tail -1

echo "STATUS: installing"
ART=build/NoviChords_artefacts/Release
codesign --force --deep --sign - "$ART/VST3/NoviChords.vst3"
codesign --force --deep --sign - "$ART/AU/NoviChords.component"
VST3_DIR="$HOME/Library/Audio/Plug-Ins/VST3"; AU_DIR="$HOME/Library/Audio/Plug-Ins/Components"
mkdir -p "$VST3_DIR" "$AU_DIR"
rm -rf "$VST3_DIR/NoviChords.vst3" "$AU_DIR/NoviChords.component"
# remove the old NeoChords build (renamed to NoviChords)
rm -rf "$VST3_DIR/NeoChords.vst3" "$AU_DIR/NeoChords.component"
cp -R "$ART/VST3/NoviChords.vst3" "$VST3_DIR/"
cp -R "$ART/AU/NoviChords.component" "$AU_DIR/"
killall -9 AudioComponentRegistrar 2>/dev/null
ls -la "$VST3_DIR/NoviChords.vst3/Contents/MacOS" "$AU_DIR/NoviChords.component/Contents/MacOS"
lipo -info "$VST3_DIR/NoviChords.vst3/Contents/MacOS/NoviChords"
echo "STATUS: done $(date)"

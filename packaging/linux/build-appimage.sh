#!/usr/bin/env bash
# Builds a tf2_bot_detector AppImage from an already-built binary.
#
# Model: the AppImage is JUST the binary (+ its private shared libs). The runtime
# data/resource folders (cfg/ fonts/ images/ licenses/ tf2_addons/) are NOT packaged
# inside it — they ship next to the .AppImage file, exactly like the Windows portable
# zip. At runtime GetCurrentExeDir() resolves to dirname($APPIMAGE) (see
# Platform/Linux/Platform.cpp), so the binary finds them next to the file.
#
# Self-contained: downloads appimagetool if missing, uses the committed PNG icon
# (no Pillow/ImageMagick needed). Output: dist/tf2_bot_detector-x86_64.AppImage
#
# Usage: packaging/linux/build-appimage.sh [path-to-binary]
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
TOOLS="$REPO_ROOT/packaging/linux/tools"
BIN="${1:-$REPO_ROOT/build/tf2_bot_detector/tf2_bot_detector}"
ICON="$REPO_ROOT/packaging/linux/tf2_bot_detector.png"
APPDIR="$REPO_ROOT/packaging/linux/AppDir"
OUTDIR="$REPO_ROOT/dist"
APPIMAGETOOL_URL="https://github.com/AppImage/appimagetool/releases/download/continuous/appimagetool-x86_64.AppImage"

export APPIMAGE_EXTRACT_AND_RUN=1   # FUSE-less environments (CI, sandboxes)

[ -f "$BIN" ]  || { echo "binary not found: $BIN (build the project first)"; exit 1; }
[ -f "$ICON" ] || { echo "icon not found: $ICON"; exit 1; }

echo ">> fetching appimagetool (if missing)"
mkdir -p "$TOOLS"
APPIMAGETOOL="$TOOLS/appimagetool.AppImage"
[ -f "$APPIMAGETOOL" ] || curl -fsSL -o "$APPIMAGETOOL" "$APPIMAGETOOL_URL"
chmod +x "$APPIMAGETOOL"

echo ">> assembling AppDir"
rm -rf "$APPDIR"; mkdir -p "$APPDIR/usr/bin" "$APPDIR/usr/lib" "$OUTDIR"
cp "$BIN" "$APPDIR/usr/bin/tf2_bot_detector"
cp "$ICON" "$APPDIR/tf2_bot_detector.png"

# Bundle private, non-system shared libs (everything else is statically linked).
# Leave the core system/loader and host graphics libs to the host.
echo ">> bundling shared libs"
ldd "$BIN" | awk '/=> \//{print $3}' | while read -r lib; do
  case "$(basename "$lib")" in
    libc.so.*|libm.so.*|libdl.so.*|libpthread.so.*|librt.so.*|ld-linux*) ;;  # host glibc
    libGL*|libEGL*|libX11*|libxcb*|libwayland*|libdrm*) ;;                    # host graphics
    *) cp -vL "$lib" "$APPDIR/usr/lib/" ;;
  esac
done

echo ">> .desktop"
cat > "$APPDIR/tf2_bot_detector.desktop" <<'EOF'
[Desktop Entry]
Type=Application
Name=TF2 Bot Detector
Exec=tf2_bot_detector
Icon=tf2_bot_detector
Categories=Game;Utility;
Terminal=false
EOF

echo ">> AppRun"
cat > "$APPDIR/AppRun" <<'EOF'
#!/bin/sh
HERE="$(dirname "$(readlink -f "$0")")"
export LD_LIBRARY_PATH="$HERE/usr/lib:$LD_LIBRARY_PATH"
# wayland SDL backend crashes (see staging/launch_tf2bd_linux.sh); default to x11.
export SDL_VIDEODRIVER="${SDL_VIDEODRIVER:-x11}"
# Do NOT chdir: the binary uses $APPIMAGE to find its data folder next to the file.
exec "$HERE/usr/bin/tf2_bot_detector" "$@"
EOF
chmod +x "$APPDIR/AppRun"

echo ">> appimagetool"
"$APPIMAGETOOL" "$APPDIR" "$OUTDIR/tf2_bot_detector-x86_64.AppImage"
echo ">> done: $OUTDIR/tf2_bot_detector-x86_64.AppImage"

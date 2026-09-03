#!/usr/bin/env bash
set -e

# Navigate to build directory
cd build/gui/GUI_App

echo "Running windeployqt..."
windeployqt GUI_App.exe

echo "Copying MSYS2 UCRT64 runtime DLL dependencies..."
# Find all executables and DLLs and copy their UCRT64 dependencies
find . -type f \( -name "*.exe" -o -name "*.dll" \) -exec ldd {} + 2>/dev/null \
    | grep '/ucrt64/bin/' \
    | awk '{print $3}' \
    | sort -u \
    | xargs -I {} cp -u {} .

echo "Deployment finished successfully!"
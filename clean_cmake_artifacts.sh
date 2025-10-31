#!/bin/bash
# Script to remove CMake-generated files outside of build directories

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

echo "Cleaning CMake artifacts outside of build directories..."

# Check if we need sudo for permission issues
SUDO=""
if [ -w "." ]; then
    # Try to remove a test file
    if ! rm -f .test_permissions 2>/dev/null; then
        echo "Note: Some files may require elevated permissions"
        SUDO="sudo"
    fi
else
    echo "Note: Using sudo for elevated permissions"
    SUDO="sudo"
fi

# Find and remove Makefiles (excluding build directories)
echo "Removing Makefiles..."
find . -name "Makefile" -type f | grep -v "^./build/" | grep -v "^./build-docker/" | while read -r file; do
    $SUDO chmod +w "$file" 2>/dev/null || true
    $SUDO rm -f "$file" && echo "  Removed: $file"
done

# Find and remove CTestTestfile.cmake
echo "Removing CTestTestfile.cmake files..."
find . -name "CTestTestfile.cmake" -type f | grep -v "^./build/" | grep -v "^./build-docker/" | while read -r file; do
    $SUDO chmod +w "$file" 2>/dev/null || true
    $SUDO rm -f "$file" && echo "  Removed: $file"
done

# Find and remove CMakeCache.txt
echo "Removing CMakeCache.txt files..."
find . -name "CMakeCache.txt" -type f | grep -v "^./build/" | grep -v "^./build-docker/" | while read -r file; do
    $SUDO chmod +w "$file" 2>/dev/null || true
    $SUDO rm -f "$file" && echo "  Removed: $file"
done

# Find and remove cmake_install.cmake
echo "Removing cmake_install.cmake files..."
find . -name "cmake_install.cmake" -type f | grep -v "^./build/" | grep -v "^./build-docker/" | while read -r file; do
    $SUDO chmod +w "$file" 2>/dev/null || true
    $SUDO rm -f "$file" && echo "  Removed: $file"
done

# Find and remove CMakeFiles directories
echo "Removing CMakeFiles directories..."
find . -name "CMakeFiles" -type d | grep -v "^./build/" | grep -v "^./build-docker/" | while read -r dir; do
    $SUDO chmod -R +w "$dir" 2>/dev/null || true
    $SUDO rm -rf "$dir" && echo "  Removed: $dir"
done

# Find and remove DartConfiguration.tcl
echo "Removing DartConfiguration.tcl files..."
find . -name "DartConfiguration.tcl" -type f | grep -v "^./build/" | grep -v "^./build-docker/" | while read -r file; do
    $SUDO chmod +w "$file" 2>/dev/null || true
    $SUDO rm -f "$file" && echo "  Removed: $file"
done

# Find and remove Testing directories
echo "Removing Testing directories..."
find . -name "Testing" -type d | grep -v "^./build/" | grep -v "^./build-docker/" | while read -r dir; do
    $SUDO chmod -R +w "$dir" 2>/dev/null || true
    $SUDO rm -rf "$dir" && echo "  Removed: $dir"
done

echo "CMake artifact cleanup complete!"

#!/bin/bash

# Default values
BUILD_TYPE="Release"
BUILD_TARGET="install"

# Parse command line arguments
while [[ $# -gt 0 ]]; do
    case $1 in
        -d|--debug)
            BUILD_TYPE="Debug"
            shift
            ;;
        -r|--release)
            BUILD_TYPE="Release"
            shift
            ;;
        -t|--target)
            BUILD_TARGET="$2"
            shift 2
            ;;
        -h|--help)
            echo "Usage: $0 [OPTIONS]"
            echo "Options:"
            echo "  -d, --debug     Build in Debug mode (logging enabled)"
            echo "  -r, --release   Build in Release mode (logging disabled) [default]"
            echo "  -t, --target    Specify build target [default: install]"
            echo "  -h, --help      Show this help message"
            exit 0
            ;;
        *)
            echo "Unknown option: $1"
            echo "Use -h or --help for usage information"
            exit 1
            ;;
    esac
done

echo "==========Build Configuration======"
echo "Build Type: $BUILD_TYPE"
echo "Build Target: $BUILD_TARGET"
echo "==================================="

# Create build directory
BUILD_DIR="build/$BUILD_TYPE"
INSTALL_DIR="install/$BUILD_TYPE"

# Configure and build
cmake -S . -B "$BUILD_DIR" -DCMAKE_INSTALL_PREFIX="$INSTALL_DIR" -DCMAKE_BUILD_TYPE="$BUILD_TYPE"
if [ $? -ne 0 ]; then
    echo "============================"
    echo "CMAKE configuration failed!"
    echo "============================"
    exit 1
fi

cmake --build "$BUILD_DIR" --target "$BUILD_TARGET"
if [ $? -ne 0 ]; then
    echo "============================"
    echo "Build failed!"
    echo "============================"
    exit 1
fi

echo "============================"
echo "Build success!"
echo "============================"
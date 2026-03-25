#!/bin/bash

# WeighMyBru² Build Script
# This script builds firmware for both board variants with proper versioning

set -e

# Default values
VERSION=""
BUILD_NUMBER=$(date +%s)
COMMIT_HASH=$(git rev-parse --short HEAD 2>/dev/null || echo "unknown")
BUILD_DATE=$(date -u +"%b %d %Y")
BUILD_TIME=$(date -u +"%H:%M:%S")
OUTPUT_DIR="./build-output"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

print_header() {
    echo -e "${BLUE}"
    echo "========================================"
    echo "    WeighMyBru² Build Script"
    echo "========================================"
    echo -e "${NC}"
}

print_step() {
    echo -e "${YELLOW}[STEP]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

show_help() {
    echo "Usage: $0 [OPTIONS]"
    echo ""
    echo "Options:"
    echo "  -v, --version VERSION    Set version string (e.g., 2.1.0)"
    echo "  -b, --build-number NUM   Set build number (default: timestamp)"
    echo "  -o, --output DIR         Output directory (default: ./build-output)"
    echo "  -r, --release           Build release version (clean build)"
    echo "  -c, --clean             Clean build directories first"
    echo "  -h, --help              Show this help"
    echo ""
    echo "Examples:"
    echo "  $0 -v 2.1.0 -r                    # Release build v2.1.0"
    echo "  $0 -v 2.1.0-beta -b 123           # Beta build with custom build number"
    echo "  $0 -c                              # Development build (clean)"
}

parse_arguments() {
    while [[ $# -gt 0 ]]; do
        case $1 in
            -v|--version)
                VERSION="$2"
                shift 2
                ;;
            -b|--build-number)
                BUILD_NUMBER="$2"
                shift 2
                ;;
            -o|--output)
                OUTPUT_DIR="$2"
                shift 2
                ;;
            -r|--release)
                IS_RELEASE=true
                shift
                ;;
            -c|--clean)
                CLEAN_BUILD=true
                shift
                ;;
            -h|--help)
                show_help
                exit 0
                ;;
            *)
                echo "Unknown option: $1"
                show_help
                exit 1
                ;;
        esac
    done
}

setup_version() {
    if [[ -z "$VERSION" ]]; then
        # Try to extract version from git tag
        if git describe --tags --exact-match HEAD 2>/dev/null; then
            VERSION=$(git describe --tags --exact-match HEAD | sed 's/^v//')
            IS_RELEASE=true
            print_step "Detected release version from git tag: $VERSION"
        else
            # Development version
            VERSION="dev-$(date +%Y%m%d)"
            print_step "Using development version: $VERSION"
        fi
    fi
    
    FULL_VERSION="${VERSION}+build.${BUILD_NUMBER}.${COMMIT_HASH}"
    
    echo "Version: $VERSION"
    echo "Build Number: $BUILD_NUMBER"
    echo "Commit Hash: $COMMIT_HASH"
    echo "Full Version: $FULL_VERSION"
    echo "Build Date: $BUILD_DATE $BUILD_TIME"
    echo ""
}

setup_build_flags() {
    export PLATFORMIO_BUILD_FLAGS="\
        -DWEIGHMYBRU_BUILD_NUMBER=$BUILD_NUMBER \
        -DWEIGHMYBRU_COMMIT_HASH=\\\"$COMMIT_HASH\\\" \
        -DWEIGHMYBRU_BUILD_DATE=\\\"$BUILD_DATE\\\" \
        -DWEIGHMYBRU_BUILD_TIME=\\\"$BUILD_TIME\\\""
    
    print_step "Build flags: $PLATFORMIO_BUILD_FLAGS"
}

clean_build() {
    if [[ "$CLEAN_BUILD" == "true" ]] || [[ "$IS_RELEASE" == "true" ]]; then
        print_step "Cleaning build directories..."
        pio run -t clean
        rm -rf .pio/build
        print_success "Build directories cleaned"
    fi
}

build_environment() {
    local env=$1
    local board_suffix=$2
    
    print_step "Building $env ($board_suffix)..."
    
    # Build firmware
    pio run -e $env
    
    # Build filesystem
    print_step "Building filesystem for $env..."
    pio run -e $env -t buildfs
    
    print_success "Build complete for $env"
}

copy_binaries() {
    local env=$1
    local board_suffix=$2
    
    print_step "Copying binaries for $board_suffix..."
    
    mkdir -p "$OUTPUT_DIR"
    
    # Copy main firmware
    if [[ -f ".pio/build/$env/firmware.bin" ]]; then
        cp ".pio/build/$env/firmware.bin" \
           "$OUTPUT_DIR/weighmybru-${board_suffix}-v${VERSION}.bin"
        print_success "Copied firmware binary"
    fi
    
    # Copy filesystem
    if [[ -f ".pio/build/$env/littlefs.bin" ]]; then
        cp ".pio/build/$env/littlefs.bin" \
           "$OUTPUT_DIR/weighmybru-${board_suffix}-v${VERSION}-littlefs.bin"
        print_success "Copied filesystem binary"
    fi
    
    # Copy bootloader and partitions for complete flashing
    if [[ -f ".pio/build/$env/bootloader.bin" ]]; then
        cp ".pio/build/$env/bootloader.bin" \
           "$OUTPUT_DIR/weighmybru-${board_suffix}-v${VERSION}-bootloader.bin"
    fi
    
    if [[ -f ".pio/build/$env/partitions.bin" ]]; then
        cp ".pio/build/$env/partitions.bin" \
           "$OUTPUT_DIR/weighmybru-${board_suffix}-v${VERSION}-partitions.bin"
    fi
}

generate_manifest() {
    local board_suffix=$1
    local board_name=$2
    
    print_step "Generating ESP32 Web Tools manifest for $board_suffix..."
    
    cat > "$OUTPUT_DIR/manifest-${board_suffix}.json" << EOF
{
  "name": "$board_name",
  "version": "$VERSION", 
  "home_assistant_domain": "weighmybru",
  "new_install_prompt_erase": true,
  "funding_url": "https://github.com/031devstudios/weighmybru2",
  "builds": [
    {
      "chipFamily": "ESP32-S3",
      "parts": [
        {
          "path": "weighmybru-${board_suffix}-v${VERSION}-bootloader.bin",
          "offset": 0
        },
        {
          "path": "weighmybru-${board_suffix}-v${VERSION}-partitions.bin", 
          "offset": 32768
        },
        {
          "path": "weighmybru-${board_suffix}-v${VERSION}.bin",
          "offset": 65536
        },
        {
          "path": "weighmybru-${board_suffix}-v${VERSION}-littlefs.bin",
          "offset": 2686976
        }
      ]
    }
  ]
}
EOF

    print_success "Generated manifest for $board_name"
}

generate_build_info() {
    print_step "Generating build information..."
    
    cat > "$OUTPUT_DIR/build-info.json" << EOF
{
  "version": "$VERSION",
  "full_version": "$FULL_VERSION",
  "build_number": $BUILD_NUMBER,
  "commit_hash": "$COMMIT_HASH",
  "build_date": "$BUILD_DATE",
  "build_time": "$BUILD_TIME",
  "is_release": ${IS_RELEASE:-false},
  "built_at": "$(date -u --iso-8601=seconds)",
  "environments": ["esp32s3-supermini", "esp32s3-xiao"]
}
EOF

    print_success "Generated build information"
}

show_summary() {
    echo ""
    echo -e "${GREEN}========================================"
    echo "         Build Summary"
    echo -e "========================================${NC}"
    echo "Version: $VERSION"
    echo "Output Directory: $OUTPUT_DIR"
    echo ""
    echo "Generated Files:"
    if [[ -d "$OUTPUT_DIR" ]]; then
        ls -la "$OUTPUT_DIR"
    fi
    echo ""
    echo -e "${GREEN}✅ Build completed successfully!${NC}"
    echo ""
    echo "To flash firmware:"
    echo "  pio run -e esp32s3-supermini -t upload"
    echo "  pio run -e esp32s3-xiao -t upload" 
    echo ""
    echo "To upload filesystem:"
    echo "  pio run -e esp32s3-supermini -t uploadfs"
    echo "  pio run -e esp32s3-xiao -t uploadfs"
}

main() {
    print_header
    
    parse_arguments "$@"
    setup_version
    setup_build_flags
    clean_build
    
    # Build both environments
    build_environment "esp32s3-supermini" "supermini"
    copy_binaries "esp32s3-supermini" "supermini"
    generate_manifest "supermini" "WeighMyBru² - ESP32-S3 Supermini"
    
    build_environment "esp32s3-xiao" "xiao"
    copy_binaries "esp32s3-xiao" "xiao"
    generate_manifest "xiao" "WeighMyBru² - XIAO ESP32S3"
    
    generate_build_info
    show_summary
}

# Check if PlatformIO is available
if ! command -v pio &> /dev/null; then
    print_error "PlatformIO CLI not found. Please install PlatformIO first."
    exit 1
fi

# Run main function
main "$@"
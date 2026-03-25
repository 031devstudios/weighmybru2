#!/bin/bash

# WeighMyBru² Enhanced Build & Release Script (Linux)
# This script builds firmware and prepares website releases

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
BUILD_OUTPUT_DIR="build-output"
WEBSITE_RELEASES_DIR="$SCRIPT_DIR/website/releases"
LATEST_DIR="$WEBSITE_RELEASES_DIR/latest"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m'

print_header() {
    echo -e "${CYAN}"
    echo "==============================================="
    echo "    WeighMyBru² Enhanced Build & Release"
    echo "==============================================="
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
    echo "  -c, --clean             Clean build directories first"
    echo "  -r, --release           Build release version"
    echo "  -h, --help              Show this help"
    echo ""
    echo "Examples:"
    echo "  $0                      # Development build"
    echo "  $0 --release            # Release build"
    echo "  $0 --clean --release    # Clean release build"
}

# Parse arguments
CLEAN_BUILD=""
IS_RELEASE=""

while [[ $# -gt 0 ]]; do
    case $1 in
        -c|--clean)
            CLEAN_BUILD="true"
            shift
            ;;
        -r|--release)
            IS_RELEASE="true"
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

main() {
    print_header

    # Step 1: Run the existing build script
    print_step "Running main build script..."

    BUILD_ARGS=()
    if [[ "$CLEAN_BUILD" == "true" ]]; then
        BUILD_ARGS+=("--clean")
    fi
    if [[ "$IS_RELEASE" == "true" ]]; then
        BUILD_ARGS+=("--release")
    fi

    "$SCRIPT_DIR/build.sh" "${BUILD_ARGS[@]}"
    if [[ $? -ne 0 ]]; then
        print_error "Build failed!"
        exit 1
    fi

    # Step 2: Extract version from Version.h
    print_step "Extracting version information..."

    local version_file="$SCRIPT_DIR/include/Version.h"

    local version_major version_minor version_patch
    version_major=$(grep "WEIGHMYBRU_VERSION_MAJOR" "$version_file" | awk '{print $3}')
    version_minor=$(grep "WEIGHMYBRU_VERSION_MINOR" "$version_file" | awk '{print $3}')
    version_patch=$(grep "WEIGHMYBRU_VERSION_PATCH" "$version_file" | awk '{print $3}')

    local current_version="$version_major.$version_minor.$version_patch"
    print_success "Current version: $current_version"

    # Step 3: Create website release directories
    print_step "Setting up website release directories..."

    mkdir -p "$WEBSITE_RELEASES_DIR"
    mkdir -p "$LATEST_DIR"

    local version_dir="$WEBSITE_RELEASES_DIR/v$current_version"
    mkdir -p "$version_dir"

    # Step 4: Copy manifest and firmware files
    print_step "Copying release files to website..."

    local build_latest_dir="$BUILD_OUTPUT_DIR"

    # Copy manifest files to latest
    if [[ -f "$build_latest_dir/manifest-supermini.json" ]]; then
        cp "$build_latest_dir/manifest-supermini.json" "$LATEST_DIR/"
        print_success "Copied manifest-supermini.json"
    fi

    if [[ -f "$build_latest_dir/manifest-xiao.json" ]]; then
        cp "$build_latest_dir/manifest-xiao.json" "$LATEST_DIR/"
        print_success "Copied manifest-xiao.json"
    fi

    # Copy firmware binaries to latest
    if [[ -d "$build_latest_dir" ]]; then
        cp "$build_latest_dir"/*.bin "$LATEST_DIR/" 2>/dev/null || true
        print_success "Copied firmware binaries"
    fi

    # Copy all files to version-specific directory
    cp -r "$LATEST_DIR"/* "$version_dir/" 2>/dev/null || true

    print_success "Files copied to:"
    echo "  - $LATEST_DIR"
    echo "  - $version_dir"

    # Step 5: Update releases index
    print_step "Updating releases index..."

    local release_date
    release_date=$(date -u +"%Y-%m-%d")

    cat > "$WEBSITE_RELEASES_DIR/index.json" << EOF
{
    "latest": "$current_version",
    "releases": [
        {
            "version": "$current_version",
            "date": "$release_date",
            "supermini_manifest": "./v$current_version/manifest-supermini.json",
            "xiao_manifest": "./v$current_version/manifest-xiao.json"
        }
    ]
}
EOF

    print_success "Generated release index"

    # Step 6: Show summary
    echo ""
    echo "========================================"
    echo "     Release Preparation Complete!"
    echo "========================================"
    echo "Version: $current_version"
    echo "Release Date: $release_date"
    echo ""
    echo "Website files updated:"
    echo "  - $LATEST_DIR/manifest-*.json"
    echo "  - $LATEST_DIR/*.bin"
    echo "  - $version_dir/*"
    echo "  - $WEBSITE_RELEASES_DIR/index.json"
    echo ""
    echo "Next Steps:"
    echo "  1. Review the changes:"
    echo "     git status"
    echo ""
    echo "  2. Commit and push to deploy:"
    echo "     git add website/releases/"
    echo "     git commit -m \"Release v$current_version - Updated firmware manifests\""
    echo "     git push"
    echo ""
    echo "  3. Your Cloudflare site will auto-update with the new version!"
    echo ""
}

main "$@"

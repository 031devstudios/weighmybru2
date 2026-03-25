#!/bin/bash

# WeighMyBru² - GitHub Release to Website Sync Script
# This script syncs the latest GitHub release files to your website
# Run this after each release or set up as a webhook/cron job

set -e  # Exit on any error

# Configuration
GITHUB_REPO="031devstudios/weighmybru2"
WEBSITE_RELEASES_DIR="./releases"
LATEST_DIR="$WEBSITE_RELEASES_DIR/latest"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Logging functions
log_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

log_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

log_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Function to check if required tools are installed
check_dependencies() {
    local missing_deps=()
    
    if ! command -v curl &> /dev/null; then
        missing_deps+=("curl")
    fi
    
    if ! command -v jq &> /dev/null; then
        missing_deps+=("jq")
    fi
    
    if ! command -v unzip &> /dev/null; then
        missing_deps+=("unzip")
    fi
    
    if [ ${#missing_deps[@]} -gt 0 ]; then
        log_error "Missing required dependencies: ${missing_deps[*]}"
        log_info "Please install missing dependencies and try again"
        exit 1
    fi
}

# Function to get latest release info from GitHub
get_latest_release() {
    log_info "Fetching latest release information from GitHub..."
    
    local release_info
    release_info=$(curl -s "https://api.github.com/repos/$GITHUB_REPO/releases/latest")
    
    if [ $? -ne 0 ] || [ -z "$release_info" ]; then
        log_error "Failed to fetch release information from GitHub"
        exit 1
    fi
    
    local tag_name
    tag_name=$(echo "$release_info" | jq -r '.tag_name')
    
    if [ "$tag_name" = "null" ] || [ -z "$tag_name" ]; then
        log_error "No releases found in repository"
        exit 1
    fi
    
    echo "$release_info"
}

# Function to download release assets
download_release_assets() {
    local release_info="$1"
    local tag_name
    tag_name=$(echo "$release_info" | jq -r '.tag_name')
    
    log_info "Downloading release assets for version $tag_name..."
    
    # Create release directory
    local release_dir="$WEBSITE_RELEASES_DIR/$tag_name"
    mkdir -p "$release_dir"
    
    # Download each asset
    local download_urls
    download_urls=$(echo "$release_info" | jq -r '.assets[].browser_download_url')
    
    if [ -z "$download_urls" ]; then
        log_error "No assets found in release"
        exit 1
    fi
    
    while IFS= read -r url; do
        local filename
        filename=$(basename "$url")
        local filepath="$release_dir/$filename"
        
        log_info "Downloading $filename..."
        if curl -L -o "$filepath" "$url"; then
            log_success "Downloaded $filename"
        else
            log_error "Failed to download $filename"
            exit 1
        fi
    done <<< "$download_urls"
    
    echo "$tag_name"
}

# Function to extract and organize release files
organize_release_files() {
    local tag_name="$1"
    local release_dir="$WEBSITE_RELEASES_DIR/$tag_name"
    
    log_info "Organizing release files for $tag_name..."
    
    # Extract firmware files if they exist
    for zip_file in "$release_dir"/*.zip; do
        if [ -f "$zip_file" ]; then
            log_info "Extracting $(basename "$zip_file")..."
            unzip -o "$zip_file" -d "$release_dir/"
            rm "$zip_file"  # Remove zip after extraction
        fi
    done
    
    # Verify required files exist
    local required_files=(
        "manifest-supermini.json"
        "manifest-xiao.json"
        "firmware-supermini.bin"
        "firmware-xiao.bin"
        "bootloader.bin"
        "partitions.bin"
        "spiffs.bin"
    )
    
    for file in "${required_files[@]}"; do
        if [ ! -f "$release_dir/$file" ]; then
            log_warning "Required file $file not found in release"
        fi
    done
}

# Function to update latest symlink
update_latest_symlink() {
    local tag_name="$1"
    local release_dir="$WEBSITE_RELEASES_DIR/$tag_name"
    
    log_info "Updating latest release symlink..."
    
    # Remove existing latest directory/symlink
    if [ -L "$LATEST_DIR" ] || [ -d "$LATEST_DIR" ]; then
        rm -rf "$LATEST_DIR"
    fi
    
    # Create new symlink to latest release
    if ln -s "$tag_name" "$LATEST_DIR"; then
        log_success "Updated latest symlink to $tag_name"
    else
        # Fallback: copy files instead of symlink (for systems that don't support symlinks)
        log_warning "Symlink failed, copying files instead..."
        mkdir -p "$LATEST_DIR"
        cp -r "$release_dir"/* "$LATEST_DIR/"
        log_success "Copied latest release files to $LATEST_DIR"
    fi
}

# Function to generate release index
generate_release_index() {
    log_info "Generating release index..."
    
    local index_file="$WEBSITE_RELEASES_DIR/index.json"
    local releases=()
    
    # Collect all release directories
    for dir in "$WEBSITE_RELEASES_DIR"/*/; do
        if [ -d "$dir" ] && [ "$(basename "$dir")" != "latest" ]; then
            local version
            version=$(basename "$dir")
            releases+=("\"$version\"")
        fi
    done
    
    # Generate JSON index
    cat > "$index_file" << EOF
{
    "releases": [$(IFS=,; echo "${releases[*]}")],
    "latest": "$(readlink "$LATEST_DIR" 2>/dev/null || echo "unknown")",
    "last_updated": "$(date -u +"%Y-%m-%dT%H:%M:%SZ")"
}
EOF
    
    log_success "Generated release index at $index_file"
}

# Function to validate manifests
validate_manifests() {
    local release_dir="$1"
    
    log_info "Validating ESP32 Web Tools manifests..."
    
    for manifest in "$release_dir"/manifest-*.json; do
        if [ -f "$manifest" ]; then
            if jq . "$manifest" > /dev/null 2>&1; then
                log_success "Valid JSON: $(basename "$manifest")"
                
                # Check for required fields
                local name version
                name=$(jq -r '.name' "$manifest" 2>/dev/null)
                version=$(jq -r '.version' "$manifest" 2>/dev/null)
                
                if [ "$name" != "null" ] && [ "$version" != "null" ]; then
                    log_success "Manifest valid: $name $version"
                else
                    log_warning "Manifest missing name or version: $(basename "$manifest")"
                fi
            else
                log_error "Invalid JSON in $(basename "$manifest")"
            fi
        fi
    done
}

# Function to cleanup old releases (keep last 5)
cleanup_old_releases() {
    log_info "Cleaning up old releases..."
    
    local release_dirs
    release_dirs=$(find "$WEBSITE_RELEASES_DIR" -maxdepth 1 -type d -name "v*" | sort -V | head -n -5)
    
    if [ -n "$release_dirs" ]; then
        while IFS= read -r dir; do
            log_info "Removing old release: $(basename "$dir")"
            rm -rf "$dir"
        done <<< "$release_dirs"
        log_success "Cleaned up old releases"
    else
        log_info "No old releases to clean up"
    fi
}

# Main execution
main() {
    echo "==============================================="
    echo "   WeighMyBru² Release Sync Script"
    echo "==============================================="
    echo
    
    # Check dependencies
    check_dependencies
    
    # Create releases directory if it doesn't exist
    mkdir -p "$WEBSITE_RELEASES_DIR"
    
    # Get latest release information
    local release_info
    release_info=$(get_latest_release)
    
    # Download and organize release files
    local tag_name
    tag_name=$(download_release_assets "$release_info")
    organize_release_files "$tag_name"
    
    # Validate release files
    validate_manifests "$WEBSITE_RELEASES_DIR/$tag_name"
    
    # Update latest symlink
    update_latest_symlink "$tag_name"
    
    # Generate release index
    generate_release_index
    
    # Cleanup old releases
    cleanup_old_releases
    
    echo
    log_success "✅ Successfully synced release $tag_name to website"
    log_info "📁 Release files are ready in: $WEBSITE_RELEASES_DIR"
    log_info "🌐 Latest files accessible at: $LATEST_DIR"
    echo
    echo "Next Steps:"
    echo "1. Upload the releases/ directory to your website"
    echo "2. Update your flash.html page if needed"
    echo "3. Test the ESP32 Web Tools installation"
    echo
}

# Show help if requested
if [[ "$1" == "--help" || "$1" == "-h" ]]; then
    echo "WeighMyBru² Release Sync Script"
    echo
    echo "This script downloads the latest GitHub release and prepares"
    echo "files for hosting ESP32 Web Tools on your website."
    echo
    echo "Usage: $0 [options]"
    echo
    echo "Options:"
    echo "  -h, --help    Show this help message"
    echo
    echo "Requirements:"
    echo "  - curl (for downloading files)"
    echo "  - jq (for JSON parsing)"
    echo "  - unzip (for extracting archives)"
    echo
    echo "Output:"
    echo "  ./releases/          - All release versions"
    echo "  ./releases/latest/   - Symlink to latest release"
    echo "  ./releases/index.json - Release index file"
    echo
    exit 0
fi

# Run main function
main "$@"
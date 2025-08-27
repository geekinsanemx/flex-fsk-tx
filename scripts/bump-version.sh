#!/bin/bash

# Version Bump Script for flex-fsk-tx
# Automatically increments version and updates all relevant files
# Usage: ./scripts/bump-version.sh [major|minor|patch] "Description of changes"

set -e  # Exit on any error

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Script configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
VERSION_FILE="$PROJECT_DIR/VERSION"
CHANGELOG_FILE="$PROJECT_DIR/CHANGELOG.md"

# Function to print colored output
print_status() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# Function to show usage
show_usage() {
    echo "Usage: $0 [major|minor|patch] \"Description of changes\""
    echo ""
    echo "Version increment types:"
    echo "  major  - Breaking changes, API incompatibility (X+1.0.0)"
    echo "  minor  - New features, backward compatible (x.Y+1.0)"  
    echo "  patch  - Bug fixes, documentation updates (x.y.Z+1)"
    echo ""
    echo "Examples:"
    echo "  $0 patch \"Fix antenna detection bug\""
    echo "  $0 minor \"Add message scheduling feature\""
    echo "  $0 major \"Redesign AT command protocol\""
    exit 1
}

# Function to validate version format
validate_version() {
    local version=$1
    if [[ ! $version =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]]; then
        print_error "Invalid version format: $version"
        print_error "Expected format: MAJOR.MINOR.PATCH (e.g., 3.2.0)"
        exit 1
    fi
}

# Function to parse version components
parse_version() {
    local version=$1
    local major=$(echo $version | cut -d. -f1)
    local minor=$(echo $version | cut -d. -f2)  
    local patch=$(echo $version | cut -d. -f3)
    echo "$major $minor $patch"
}

# Function to calculate new version
calculate_new_version() {
    local increment_type=$1
    local current_version=$2
    
    read -r major minor patch <<< $(parse_version $current_version)
    
    case $increment_type in
        "major")
            major=$((major + 1))
            minor=0
            patch=0
            ;;
        "minor")
            minor=$((minor + 1))
            patch=0
            ;;
        "patch")
            patch=$((patch + 1))
            ;;
        *)
            print_error "Invalid increment type: $increment_type"
            show_usage
            ;;
    esac
    
    echo "$major.$minor.$patch"
}

# Function to create timestamped backup
create_backup() {
    local file=$1
    local timestamp=$(date +"%Y%m%d%H%M%S")
    local backup_file="${file}.bkp-${timestamp}"
    
    if [[ -f "$file" ]]; then
        cp "$file" "$backup_file"
        print_status "Created backup: $backup_file"
    fi
}

# Function to update version in file
update_version_in_file() {
    local file=$1
    local old_version=$2
    local new_version=$3
    local pattern=$4
    
    if [[ -f "$file" ]]; then
        create_backup "$file"
        
        if [[ -n "$pattern" ]]; then
            # Use custom pattern if provided
            sed -i "s/${pattern}/${new_version}/g" "$file"
        else
            # Default: replace version numbers
            sed -i "s/${old_version}/${new_version}/g" "$file"
        fi
        
        print_status "Updated version in: $(basename "$file")"
    else
        print_warning "File not found: $file"
    fi
}

# Function to update changelog
update_changelog() {
    local new_version=$1
    local description=$2
    local date=$(date +"%Y-%m-%d")
    
    if [[ -f "$CHANGELOG_FILE" ]]; then
        create_backup "$CHANGELOG_FILE"
        
        # Create temporary file with new entry
        local temp_file=$(mktemp)
        
        # Add new version entry after the header
        {
            # Copy header (first 6 lines)
            head -6 "$CHANGELOG_FILE"
            
            # Add new version entry
            echo ""
            echo "## [$new_version] - $date"
            echo ""
            echo "### Added"
            echo "- $description"
            echo ""
            echo "### Changed"
            echo "- (Add changes here)"
            echo ""
            echo "### Fixed"
            echo "- (Add fixes here)"
            echo ""
            echo "### Technical Details"
            echo "- (Add technical details here)"
            
            # Add rest of file (skip first 6 lines)
            tail -n +7 "$CHANGELOG_FILE"
        } > "$temp_file"
        
        mv "$temp_file" "$CHANGELOG_FILE"
        print_status "Updated CHANGELOG.md with new version entry"
    else
        print_warning "CHANGELOG.md not found"
    fi
}

# Function to validate git status
validate_git_status() {
    if ! git status --porcelain | grep -q "^"; then
        print_status "Git working directory is clean"
    else
        print_warning "Git working directory has uncommitted changes:"
        git status --porcelain
        echo ""
        read -p "Continue anyway? [y/N] " -n 1 -r
        echo ""
        if [[ ! $REPLY =~ ^[Yy]$ ]]; then
            print_error "Aborted by user"
            exit 1
        fi
    fi
}

# Function to commit and tag
commit_and_tag() {
    local new_version=$1
    local description=$2
    local old_version=$3
    
    # Add all changed files
    git add VERSION CHANGELOG.md README.md Makefile flex-fsk-tx.cpp
    git add "Devices/TTGO LoRa32-OLED/"*.ino 2>/dev/null || true
    git add "Devices/Heltec LoRa32 V3/"*.ino 2>/dev/null || true
    
    # Create commit
    local commit_message="Bump version to v${new_version}

${description}

Version change: v${old_version} -> v${new_version}"
    
    git commit -m "$commit_message"
    print_success "Created git commit for version $new_version"
    
    # Create tag
    local tag_message="Release v${new_version}

${description}

This release includes improvements and changes as detailed in CHANGELOG.md"
    
    git tag -a "v${new_version}" -m "$tag_message"
    print_success "Created git tag: v${new_version}"
}

# Main execution
main() {
    print_status "Starting version bump process..."
    
    # Change to project directory
    cd "$PROJECT_DIR"
    
    # Validate arguments
    if [[ $# -ne 2 ]]; then
        print_error "Invalid number of arguments"
        show_usage
    fi
    
    local increment_type=$1
    local description=$2
    
    # Validate increment type
    if [[ ! "$increment_type" =~ ^(major|minor|patch)$ ]]; then
        print_error "Invalid increment type: $increment_type"
        show_usage
    fi
    
    # Check if VERSION file exists
    if [[ ! -f "$VERSION_FILE" ]]; then
        print_error "VERSION file not found: $VERSION_FILE"
        exit 1
    fi
    
    # Read current version
    local current_version=$(cat "$VERSION_FILE")
    print_status "Current version: $current_version"
    
    # Validate current version format
    validate_version "$current_version"
    
    # Calculate new version
    local new_version=$(calculate_new_version "$increment_type" "$current_version")
    print_status "New version: $new_version"
    
    # Validate git status
    validate_git_status
    
    # Update VERSION file
    echo "$new_version" > "$VERSION_FILE"
    print_status "Updated VERSION file"
    
    # Update all files with version references
    update_version_in_file "$PROJECT_DIR/README.md" "$current_version" "$new_version"
    update_version_in_file "$PROJECT_DIR/Makefile" "$current_version" "$new_version"
    update_version_in_file "$PROJECT_DIR/flex-fsk-tx.cpp" "$current_version" "$new_version"
    
    # Update firmware files
    update_version_in_file "$PROJECT_DIR/Devices/TTGO LoRa32-OLED/ttgo_fsk_tx_AT_v3.ino" "$current_version" "$new_version"
    update_version_in_file "$PROJECT_DIR/Devices/Heltec LoRa32 V3/heltec_fsk_tx_AT_v3.ino" "$current_version" "$new_version"
    
    # Update changelog
    update_changelog "$new_version" "$description"
    
    # Commit and tag
    commit_and_tag "$new_version" "$description" "$current_version"
    
    # Summary
    echo ""
    print_success "Version bump completed successfully!"
    echo ""
    print_status "Summary:"
    echo "  Old version: $current_version"
    echo "  New version: $new_version"
    echo "  Increment type: $increment_type"
    echo "  Description: $description"
    echo ""
    print_status "Files updated:"
    echo "  - VERSION"
    echo "  - README.md"
    echo "  - CHANGELOG.md"
    echo "  - Makefile"
    echo "  - flex-fsk-tx.cpp"
    echo "  - Firmware files (*.ino)"
    echo ""
    print_status "Git:"
    echo "  - Commit: $(git rev-parse --short HEAD)"
    echo "  - Tag: v${new_version}"
    echo ""
    print_warning "Next steps:"
    echo "  1. Review and edit CHANGELOG.md entry"
    echo "  2. Test the new version"
    echo "  3. Push changes: git push && git push --tags"
}

# Run main function
main "$@"
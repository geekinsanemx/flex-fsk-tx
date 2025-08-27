#!/bin/bash

# Version Information Script for flex-fsk-tx
# Provides comprehensive version information for Claude Code interactions
# Usage: ./scripts/version-info.sh

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

# Script configuration
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"
VERSION_FILE="$PROJECT_DIR/VERSION"

# Function to print colored output
print_header() {
    echo -e "${CYAN}=== $1 ===${NC}"
}

print_info() {
    echo -e "${BLUE}$1${NC}"
}

print_value() {
    echo -e "${GREEN}$1${NC}"
}

print_warning() {
    echo -e "${YELLOW}$1${NC}"
}

# Function to get current version
get_current_version() {
    if [[ -f "$VERSION_FILE" ]]; then
        cat "$VERSION_FILE"
    else
        echo "UNKNOWN"
    fi
}

# Function to parse version components
parse_version() {
    local version=$1
    if [[ $version =~ ^([0-9]+)\.([0-9]+)\.([0-9]+)$ ]]; then
        local major=${BASH_REMATCH[1]}
        local minor=${BASH_REMATCH[2]}
        local patch=${BASH_REMATCH[3]}
        echo "$major $minor $patch"
    else
        echo "0 0 0"
    fi
}

# Function to calculate next versions
calculate_next_versions() {
    local current_version=$1
    read -r major minor patch <<< $(parse_version $current_version)
    
    local next_patch="$major.$minor.$((patch + 1))"
    local next_minor="$major.$((minor + 1)).0"
    local next_major="$((major + 1)).0.0"
    
    echo "$next_patch $next_minor $next_major"
}

# Function to check git status
get_git_info() {
    local current_branch=$(git rev-parse --abbrev-ref HEAD 2>/dev/null || echo "N/A")
    local current_commit=$(git rev-parse --short HEAD 2>/dev/null || echo "N/A")
    local git_status="clean"
    
    if git status --porcelain 2>/dev/null | grep -q "^"; then
        git_status="modified"
    fi
    
    echo "$current_branch $current_commit $git_status"
}

# Function to get latest git tag
get_latest_tag() {
    git describe --tags --abbrev=0 2>/dev/null || echo "N/A"
}

# Function to check for uncommitted version-related changes
check_version_consistency() {
    local current_version=$1
    local issues=()
    
    # Check if README.md contains current version
    if ! grep -q "v$current_version" "$PROJECT_DIR/README.md" 2>/dev/null; then
        issues+=("README.md may not contain current version")
    fi
    
    # Check firmware files
    if ! grep -q "\"$current_version\"" "$PROJECT_DIR/Devices/TTGO LoRa32-OLED/"*.ino 2>/dev/null; then
        issues+=("TTGO firmware may not contain current version")
    fi
    
    # Check Makefile
    if ! grep -q "$current_version" "$PROJECT_DIR/Makefile" 2>/dev/null; then
        issues+=("Makefile may not contain current version")
    fi
    
    if [[ ${#issues[@]} -gt 0 ]]; then
        echo "INCONSISTENT"
        for issue in "${issues[@]}"; do
            echo "  - $issue"
        done
    else
        echo "CONSISTENT"
    fi
}

# Function to suggest version increment based on git history
analyze_recent_changes() {
    local since_tag=$(get_latest_tag)
    
    if [[ "$since_tag" != "N/A" ]]; then
        local commits_since=$(git log --oneline "${since_tag}..HEAD" 2>/dev/null | wc -l)
        echo "$commits_since"
        
        if [[ $commits_since -gt 0 ]]; then
            print_info "Recent commits since $since_tag:"
            git log --oneline --max-count=5 "${since_tag}..HEAD" 2>/dev/null | sed 's/^/  /'
        fi
    else
        echo "0"
    fi
}

# Function to provide version increment recommendations
get_increment_recommendations() {
    echo "Version Increment Guidelines:"
    echo ""
    echo "PATCH (x.y.Z+1) - Choose for:"
    echo "  • Bug fixes and stability improvements"
    echo "  • Documentation updates"
    echo "  • UI/UX improvements without new functionality"
    echo "  • Performance optimizations"
    echo "  • Error message improvements"
    echo ""
    echo "MINOR (x.Y+1.0) - Choose for:"
    echo "  • New features that are backward compatible"
    echo "  • New hardware platform support"
    echo "  • New AT commands or API endpoints"
    echo "  • New interfaces or protocols"
    echo "  • Enhanced capabilities"
    echo ""
    echo "MAJOR (X+1.0.0) - Choose for:"
    echo "  • Breaking AT command protocol changes"
    echo "  • Incompatible REST API modifications"  
    echo "  • Removal of supported features"
    echo "  • Major architecture changes"
    echo "  • Changes requiring user intervention"
}

# Main execution
main() {
    cd "$PROJECT_DIR"
    
    local current_version=$(get_current_version)
    read -r next_patch next_minor next_major <<< $(calculate_next_versions $current_version)
    read -r git_branch git_commit git_status <<< $(get_git_info)
    local latest_tag=$(get_latest_tag)
    local commits_since=$(analyze_recent_changes)
    local consistency=$(check_version_consistency $current_version)
    
    print_header "FLEX-FSK-TX VERSION INFORMATION"
    echo ""
    
    print_info "Current Version:"
    print_value "  $current_version"
    echo ""
    
    print_info "Next Possible Versions:"
    print_value "  PATCH: $next_patch"
    print_value "  MINOR: $next_minor" 
    print_value "  MAJOR: $next_major"
    echo ""
    
    print_info "Git Information:"
    print_value "  Branch: $git_branch"
    print_value "  Commit: $git_commit"
    print_value "  Status: $git_status"
    print_value "  Latest Tag: $latest_tag"
    if [[ "$commits_since" != "0" ]]; then
        print_value "  Commits Since Tag: $commits_since"
    fi
    echo ""
    
    print_info "Version Consistency:"
    if [[ "$consistency" == "CONSISTENT" ]]; then
        print_value "  $consistency"
    else
        print_warning "  $consistency"
    fi
    echo ""
    
    print_header "CLAUDE CODE INSTRUCTIONS"
    echo ""
    
    get_increment_recommendations
    echo ""
    
    print_info "To bump version:"
    print_value "  ./scripts/bump-version.sh patch \"Description\""
    print_value "  ./scripts/bump-version.sh minor \"Description\""
    print_value "  ./scripts/bump-version.sh major \"Description\""
    echo ""
    
    print_info "Quick Commands:"
    print_value "  # Check current version"
    print_value "  cat VERSION"
    echo ""
    print_value "  # See version management guide"
    print_value "  cat VERSION_MANAGEMENT.md"
    echo ""
    
    if [[ "$git_status" == "modified" ]]; then
        print_warning "Note: Working directory has uncommitted changes"
        print_info "Modified files:"
        git status --porcelain | sed 's/^/  /'
    fi
}

# Run main function
main "$@"
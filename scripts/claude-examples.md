# Claude Code Version-Aware Development Examples

This document provides specific examples of how Claude Code should handle different types of changes with the automated versioning system.

## 🤖 Claude Code Workflow Examples

### Example 1: Bug Fix (PATCH Version)

**Scenario**: User reports "Device doesn't respond to AT commands after WiFi disconnect"

**Claude Code Approach**:
```bash
# Step 1: Check current state
./scripts/version-info.sh
# Current version: 3.2.0

# Step 2: Analyze the issue (identify it as a bug fix)
# Step 3: Implement the fix in firmware code
# Step 4: Apply version bump
./scripts/bump-version.sh patch "Fix AT command responsiveness after WiFi disconnect"
```

**Result**: Version becomes 3.2.1

### Example 2: New Feature (MINOR Version)

**Scenario**: User requests "Add message scheduling feature to queue messages for later transmission"

**Claude Code Approach**:
```bash
# Step 1: Check current state
./scripts/version-info.sh
# Current version: 3.2.0

# Step 2: Identify as new feature (backward compatible)
# Step 3: Implement scheduling system
# Step 4: Apply version bump
./scripts/bump-version.sh minor "Add message scheduling system with time-based queue"
```

**Result**: Version becomes 3.3.0

### Example 3: Breaking Change (MAJOR Version)

**Scenario**: User wants "Redesign REST API to use different authentication method"

**Claude Code Approach**:
```bash
# Step 1: Check current state
./scripts/version-info.sh
# Current version: 3.2.0

# Step 2: Identify as breaking change (incompatible API)
# Step 3: Implement new authentication system
# Step 4: Apply version bump
./scripts/bump-version.sh major "Redesign REST API with OAuth2 authentication"
```

**Result**: Version becomes 4.0.0

## 📋 Change Classification Guide

### PATCH Examples (x.y.Z+1)

**Bug Fixes**:
```bash
./scripts/bump-version.sh patch "Fix memory leak in WiFi reconnection logic"
./scripts/bump-version.sh patch "Correct frequency display rounding error"
./scripts/bump-version.sh patch "Fix OLED display corruption on power cycle"
```

**Documentation**:
```bash
./scripts/bump-version.sh patch "Update QUICKSTART.md with latest setup procedures"
./scripts/bump-version.sh patch "Fix typos and formatting in REST_API.md"
./scripts/bump-version.sh patch "Add troubleshooting section for Heltec devices"
```

**UI/UX Improvements**:
```bash
./scripts/bump-version.sh patch "Improve web interface button spacing"
./scripts/bump-version.sh patch "Enhance error messages for invalid capcodes"
./scripts/bump-version.sh patch "Optimize OLED display contrast settings"
```

### MINOR Examples (x.Y+1.0)

**New Features**:
```bash
./scripts/bump-version.sh minor "Add message templates to web interface"
./scripts/bump-version.sh minor "Implement message history logging"
./scripts/bump-version.sh minor "Add MQTT integration for IoT connectivity"
```

**Hardware Support**:
```bash
./scripts/bump-version.sh minor "Add support for ESP32-C3 LoRa devices"
./scripts/bump-version.sh minor "Implement SX1268 radio chipset support"
./scripts/bump-version.sh minor "Add battery monitoring for LilyGO T-Beam"
```

**New AT Commands**:
```bash
./scripts/bump-version.sh minor "Add AT+SCHEDULE command for timed messages"
./scripts/bump-version.sh minor "Implement AT+TEMPLATE commands for message templates"
./scripts/bump-version.sh minor "Add AT+LOG commands for message history"
```

### MAJOR Examples (X+1.0.0)

**Breaking API Changes**:
```bash
./scripts/bump-version.sh major "Redesign REST API with v2 endpoints"
./scripts/bump-version.sh major "Replace AT command protocol with JSON-based commands"
./scripts/bump-version.sh major "Remove support for v1 and v2 firmware compatibility"
```

**Architecture Changes**:
```bash
./scripts/bump-version.sh major "Rewrite firmware with FreeRTOS architecture"
./scripts/bump-version.sh major "Migrate from Arduino framework to ESP-IDF"
./scripts/bump-version.sh major "Replace web interface with native mobile app"
```

## 🔍 Before-Change Checklist

Every Claude Code interaction should start with:

```bash
# 1. Check current version and project state
./scripts/version-info.sh

# 2. Understand what needs to be changed
# 3. Classify the change type (PATCH/MINOR/MAJOR)
# 4. Consider backward compatibility impact
# 5. Plan the implementation approach
```

## 🎯 Post-Change Validation

After using the version bump script, always:

```bash
# 1. Verify version was updated correctly
cat VERSION

# 2. Check that all files were updated
git status

# 3. Review the CHANGELOG entry
head -20 CHANGELOG.md

# 4. Verify git tag was created
git tag -l | tail -1

# 5. Test basic functionality if possible
```

## 🚨 Common Mistakes to Avoid

### DON'T: Manual Version Updates
```bash
# WRONG - Never do this
echo "3.2.1" > VERSION
sed -i 's/3.2.0/3.2.1/g' README.md
```

### DO: Use Automated Script
```bash
# CORRECT - Always use the script
./scripts/bump-version.sh patch "Fix description here"
```

### DON'T: Skip Version Classification
```bash
# WRONG - Unclear what type of change this is
./scripts/bump-version.sh minor "Various fixes and improvements"
```

### DO: Be Specific About Changes
```bash
# CORRECT - Clear description and proper classification
./scripts/bump-version.sh patch "Fix AT command timeout handling"
./scripts/bump-version.sh minor "Add message encryption feature"
```

### DON'T: Mix Change Types
```bash
# WRONG - Don't combine breaking changes with new features
# in a single version bump
```

### DO: Separate Different Types of Changes
```bash
# CORRECT - Make incremental changes
./scripts/bump-version.sh minor "Add new feature X"
# Later, after testing and user feedback:
./scripts/bump-version.sh patch "Fix bug in feature X"
```

## 📝 Example Interaction Workflow

**User Request**: "I need to add support for message encryption and fix a bug in the queue system"

**Claude Code Response**:
```bash
# Step 1: Check current state
./scripts/version-info.sh

# Step 2: Address the bug fix first (PATCH)
# Implement queue system fix
./scripts/bump-version.sh patch "Fix queue overflow handling bug"

# Step 3: Implement encryption feature (MINOR)  
# Implement message encryption
./scripts/bump-version.sh minor "Add AES-256 message encryption feature"
```

This approach ensures:
- Bug fixes are released quickly
- Features are properly versioned
- Users can adopt changes incrementally
- Version history remains clear and meaningful

## 🔄 Integration with Existing CLAUDE.md

These examples complement the existing CLAUDE.md instructions and should be used together with the established development guidelines for the flex-fsk-tx project.
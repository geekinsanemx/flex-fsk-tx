# flex-fsk-tx Scripts Directory

This directory contains automated tools for version management and development workflow.

## 📋 Available Scripts

### `bump-version.sh`
**Primary version management tool** - Automates the entire version increment process.

```bash
./scripts/bump-version.sh [major|minor|patch] "Description of changes"
```

**Features**:
- Validates current version and git status
- Calculates new version based on SemVer rules
- Creates timestamped backups of all modified files
- Updates VERSION, README.md, CHANGELOG.md, Makefile, firmware files
- Creates git commit with standardized message format
- Creates git tag with detailed release notes
- Provides summary and next steps

**Examples**:
```bash
./scripts/bump-version.sh patch "Fix antenna detection bug"
./scripts/bump-version.sh minor "Add message scheduling feature"
./scripts/bump-version.sh major "Redesign AT command protocol"
```

### `version-info.sh`
**Version analysis and guidance tool** - Provides comprehensive project state information.

```bash
./scripts/version-info.sh
```

**Features**:
- Shows current version and next possible versions
- Displays git information (branch, commit, status)
- Checks version consistency across all files
- Provides increment type recommendations
- Shows Claude Code workflow instructions
- Lists uncommitted changes

### Supporting Documentation

- **`claude-examples.md`**: Specific examples for Claude Code interactions
- **`README.md`**: This file - overview of all scripts

## 🤖 Claude Code Integration

These scripts are specifically designed for Claude Code interactions to ensure:

1. **Consistent Versioning**: All version changes follow Semantic Versioning
2. **Automated Updates**: No manual version number editing required
3. **Complete Documentation**: Every change is properly documented
4. **Git Integration**: Automatic commits and tags with standardized messages
5. **Error Prevention**: Validation prevents common versioning mistakes

## 🔄 Typical Usage Workflow

1. **Start any development task**:
   ```bash
   ./scripts/version-info.sh
   ```

2. **After implementing changes**:
   ```bash
   ./scripts/bump-version.sh [type] "Description"
   ```

3. **Validate results**:
   ```bash
   git log --oneline -3
   git tag -l | tail -3
   ```

## 📁 File Locations

Scripts automatically update these files:
- `VERSION` - Main version file
- `README.md` - Version section
- `CHANGELOG.md` - Version history
- `Makefile` - Build system version flags
- `flex-fsk-tx.cpp` - Host application version display
- `Devices/TTGO LoRa32-OLED/ttgo_fsk_tx_AT_v3.ino` - Firmware version
- `Devices/Heltec LoRa32 V3/heltec_fsk_tx_AT_v3.ino` - Firmware version

## 🛡️ Safety Features

- **Backup Creation**: Automatic timestamped backups before any changes
- **Validation**: Version format and git status validation
- **Rollback Support**: All changes are committed, allowing easy rollback
- **Consistency Checks**: Ensures version numbers match across all files

## 📖 See Also

- **[VERSION_MANAGEMENT.md](../VERSION_MANAGEMENT.md)**: Comprehensive versioning guide
- **[CLAUDE.md](../CLAUDE.md)**: Full development guidelines
- **[CHANGELOG.md](../CHANGELOG.md)**: Complete version history

These scripts are the foundation of the flex-fsk-tx version management system, ensuring reliable and consistent version control for all future development.
# Makefile for flex-fsk-tx v3.2.0
# FSK transmitter application for ESP32 LoRa32 devices

# Version information
VERSION = 3.2.0
BUILD_DATE = $(shell date +"%Y-%m-%d")

# Compiler and flags
CC = gcc
CXX = g++
CFLAGS = -Wall -Wextra -O2 -std=c99 -DVERSION=\"$(VERSION)\" -DBUILD_DATE=\"$(BUILD_DATE)\"
CXXFLAGS = -Wall -Wextra -O2 -std=c++11 -Wno-maybe-uninitialized -DVERSION=\"$(VERSION)\" -DBUILD_DATE=\"$(BUILD_DATE)\"
LDFLAGS =

# Directories
SRC_DIR = .
OBJ_DIR = obj
BIN_DIR = bin
INSTALL_DIR = /usr/local/bin

# tinyflex library paths
TINYFLEX_DIR = include/tinyflex
TINYFLEX_HEADER = $(TINYFLEX_DIR)/tinyflex.h

# Source and object files
SOURCES = flex-fsk-tx.cpp
OBJECTS = $(OBJ_DIR)/flex-fsk-tx.o
TARGET = $(BIN_DIR)/flex-fsk-tx

# Include paths
INCLUDES = -I$(SRC_DIR) -I$(TINYFLEX_DIR)

# Default target
all: directories $(TARGET)

# Create necessary directories
directories:
	@mkdir -p $(OBJ_DIR) $(BIN_DIR)

# Build the main executable
$(TARGET): $(OBJECTS)
	@echo "Linking $(TARGET)..."
	$(CXX) $(OBJECTS) -o $(TARGET) $(LDFLAGS)
	@echo "Build complete: $(TARGET)"

# Compile main application (depends on tinyflex header)
$(OBJ_DIR)/flex-fsk-tx.o: flex-fsk-tx.cpp $(TINYFLEX_HEADER)
	@echo "Compiling flex-fsk-tx.cpp..."
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# Install target
install: $(TARGET)
	@echo "Installing $(TARGET) to $(INSTALL_DIR)..."
	@if [ ! -d "$(INSTALL_DIR)" ]; then \
		echo "Creating directory $(INSTALL_DIR)..."; \
		mkdir -p $(INSTALL_DIR); \
	fi
	cp $(TARGET) $(INSTALL_DIR)/
	chmod 755 $(INSTALL_DIR)/flex-fsk-tx
	@echo "Installation complete. You can now run 'flex-fsk-tx' from anywhere."

# Uninstall target
uninstall:
	@echo "Removing $(INSTALL_DIR)/flex-fsk-tx..."
	@if [ -f "$(INSTALL_DIR)/flex-fsk-tx" ]; then \
		rm -f $(INSTALL_DIR)/flex-fsk-tx; \
		echo "Uninstall complete."; \
	else \
		echo "flex-fsk-tx is not installed in $(INSTALL_DIR)."; \
	fi

# Clean build artifacts
clean:
	@echo "Cleaning build artifacts..."
	@rm -rf $(OBJ_DIR) $(BIN_DIR)
	@echo "Clean complete."

# Debug build
debug: CFLAGS += -g -DDEBUG
debug: CXXFLAGS += -g -DDEBUG
debug: directories $(TARGET)

# Check if tinyflex submodule is properly initialized
check-deps:
	@echo "Checking dependencies..."
	@if [ ! -f "$(TINYFLEX_HEADER)" ]; then \
		echo "Error: tinyflex library not found!"; \
		echo "Please run: git submodule update --init --recursive"; \
		echo "Or clone with: git clone --recursive <repository-url>"; \
		exit 1; \
	fi
	@echo "Dependencies OK."

# Help target
help:
	@echo "Available targets:"
	@echo "  all        - Build the application (default)"
	@echo "  install    - Install to $(INSTALL_DIR)"
	@echo "  uninstall  - Remove from $(INSTALL_DIR)"
	@echo "  clean      - Remove build artifacts"
	@echo "  debug      - Build with debug symbols"
	@echo "  check-deps - Verify tinyflex dependency"
	@echo "  help       - Show this help message"
	@echo ""
	@echo "Example usage:"
	@echo "  make"
	@echo "  sudo make install"
	@echo "  make clean"

# Phony targets
.PHONY: all clean install uninstall debug check-deps help directories

# Dependencies check before building
$(OBJECTS): | check-deps

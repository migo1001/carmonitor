# Define the board's Fully Qualified Board Name (FQBN)
FQBN := esp32:esp32:esp32wroverkit

# Define the serial port where the board is connected
PORT := /dev/ttyUSB0

# Use the current directory for the sketch
SKETCH_DIR := .

# Build directory
BUILD_DIR := build

# Include libraries directory (if you have additional libraries)
LIB_DIR := $(SKETCH_DIR)/libraries

# Compiler optimization flags
BUILD_FLAGS := \
--build-property compiler.cpp.extra_flags="-Os -w" \
--build-property upload.maximum_size=3500000 \
--build-property build.partitions=huge_app

# Default target
all: compile upload monitor

# Compile the sketch
compile:
	@echo "Compiling the sketch with optimization..."
	arduino-cli compile $(BUILD_FLAGS) --fqbn $(FQBN) \
	--libraries $(LIB_DIR) \
	--build-path $(BUILD_DIR) $(SKETCH_DIR)

# Upload the compiled sketch to the board
upload:
	@echo "Uploading the sketch..."
	arduino-cli upload -p $(PORT) --fqbn $(FQBN) --input-dir $(BUILD_DIR)

# Start the serial monitor
monitor:
	@echo "Starting serial monitor..."
	arduino-cli monitor -p $(PORT) --config baudrate=115200

# Clean up build files
clean:
	@echo "Cleaning up..."
	rm -rf $(BUILD_DIR)

.PHONY: all compile upload monitor clean

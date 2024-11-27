# Define the board's Fully Qualified Board Name (FQBN)
FQBN := esp32:esp32:esp32wroverkit
# FQBN in file path format
FQBN_PATH := esp32.esp32.esp32wroverkit

# Define the serial port where the board is connected
PORT := /dev/ttyUSB0

# Use current directory as the sketch directory
SKETCH_DIR := .
BUILD_DIR := $(SKETCH_DIR)/build/$(FQBN_PATH)

# Dynamically determine the main sketch filename (assume one .ino file per directory)
SKETCH_FILE := $(notdir $(wildcard $(SKETCH_DIR)/*.ino))

LIB_DIR := $(SKETCH_DIR)/libraries/
INCLUDED_LIBRARIES := 

# Define the output file
OUTPUT_FILE := $(BUILD_DIR)/$(SKETCH_FILE).bin

SKETCH_MODIFIED := $(shell find $(SKETCH_DIR) -name '*.ino' -newer $(OUTPUT_FILE) -print -quit)

# Default target
all: compile upload

# Compile the sketch only if necessary
$(OUTPUT_FILE): $(wildcard $(SKETCH_DIR)/*.ino)
	@echo "Compiling the sketch..."
	arduino-cli compile --fqbn $(FQBN) --libraries $(LIB_DIR) --build-path $(BUILD_DIR) $(SKETCH_DIR)

compile: $(OUTPUT_FILE)

# Upload the compiled sketch to the board
upload: compile
	@echo "Uploading the sketch..."
	arduino-cli upload --verify -p $(PORT) --fqbn $(FQBN) --input-dir $(BUILD_DIR) $(SKETCH_DIR)
	@echo "Starting serial monitor..."
	arduino-cli monitor -p $(PORT) --config baudrate=115200

# Start the serial monitor
monitor:
	@echo "Starting serial monitor..."
	arduino-cli monitor -p $(PORT) --config baudrate=115200

# Clean up build files
clean:
	@echo "Cleaning up..."
	rm -rf $(BUILD_DIR)

.PHONY: all compile upload monitor clean

# Convenience commands for the CMake-based desktop game (Ubuntu/Linux).
# Requires make, cmake, a C compiler and raylib's system dependencies.
.DEFAULT_GOAL := all

.PHONY: all build run test clean

BUILD_DIR := build

all: build

build:
	cmake -S . -B $(BUILD_DIR) -DCMAKE_BUILD_TYPE=Release
	cmake --build $(BUILD_DIR) -j2

run: build
	./$(BUILD_DIR)/snake_desktop

test: build
	ctest --test-dir $(BUILD_DIR) --output-on-failure

clean:
	cmake -E rm -rf $(BUILD_DIR)

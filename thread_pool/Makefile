# Compiler and flags
CXX := g++
CXXFLAGS := -std=c++20 -Wall -Wextra -O2 -Isrc -pthread

# Directories
SRC_DIR := src
TEST_DIR := tests
BIN_DIR := bin

# Source files
SRC_CPP := $(wildcard $(SRC_DIR)/*.cpp)
SRC_HPP := $(wildcard $(SRC_DIR)/*.hpp)

# Test files
TESTS := $(wildcard $(TEST_DIR)/*.cpp)

# Output binaries (one per test)
TEST_BINARIES := $(patsubst $(TEST_DIR)/%.cpp, $(BIN_DIR)/%, $(TESTS))

# Default target: build all tests
all: $(TEST_BINARIES)

# Rule to compile each test, linking with src files
$(TEST_BIN_DIR)/%: $(TEST_DIR)/%.cpp $(SRC_CPP) | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) $^ -o $@

# Create bin directory if it doesn't exist
$(TEST_BIN_DIR):
	mkdir -p $(TEST_BIN_DIR)

# Run all tests
.PHONY: test
test: all
	@for test in $(TEST_BINARIES); do \
		echo "=> Running $$test..."; \
		 ./$$test | sed 's/^/    /' || exit 1; \
	done
	@echo "All tests passed."

# Clean compiled binaries
.PHONY: clean
clean:
	rm -rf $(TEST_BIN_DIR)


# Makefile for Satellite Telemetry Simulator
# Fallback build system when CMake is not available

CXX = clang++
CXXFLAGS = -std=c++20 -Wall -Wextra -Wpedantic -O2 -Iinclude
LDFLAGS = -pthread

# Source files
SRC_DIR = src
SOURCES = $(SRC_DIR)/crc.cpp \
          $(SRC_DIR)/packet.cpp \
          $(SRC_DIR)/link.cpp \
          $(SRC_DIR)/satellite.cpp \
          $(SRC_DIR)/ground_station.cpp \
          $(SRC_DIR)/main.cpp

# Test files
TEST_SOURCES = tests/basic_tests.cpp \
               $(SRC_DIR)/crc.cpp \
               $(SRC_DIR)/packet.cpp \
               $(SRC_DIR)/link.cpp

# Object files
BUILD_DIR = build
OBJECTS = $(SOURCES:$(SRC_DIR)/%.cpp=$(BUILD_DIR)/%.o)
TEST_OBJECTS = $(TEST_SOURCES:%.cpp=$(BUILD_DIR)/test_%.o)

# Executables
TARGET = $(BUILD_DIR)/satcom
TEST_TARGET = $(BUILD_DIR)/satcom_tests

.PHONY: all clean test run

all: $(TARGET) $(TEST_TARGET)

$(TARGET): $(OBJECTS) | $(BUILD_DIR)
	$(CXX) $(LDFLAGS) -o $@ $^
	@echo "✓ Built satcom executable"

$(TEST_TARGET): $(BUILD_DIR)/test_tests/basic_tests.o $(BUILD_DIR)/test_src/crc.o $(BUILD_DIR)/test_src/packet.o $(BUILD_DIR)/test_src/link.o | $(BUILD_DIR)
	$(CXX) $(LDFLAGS) -o $@ $^
	@echo "✓ Built satcom_tests executable"

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(BUILD_DIR)/test_%.o: %.cpp | $(BUILD_DIR)/test_tests $(BUILD_DIR)/test_src
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/test_tests:
	mkdir -p $(BUILD_DIR)/test_tests

$(BUILD_DIR)/test_src:
	mkdir -p $(BUILD_DIR)/test_src

clean:
	rm -rf $(BUILD_DIR)

test: $(TEST_TARGET)
	@echo "Running tests..."
	@$(TEST_TARGET)

run: $(TARGET)
	@$(TARGET) --duration-sec 10 --verbose

help:
	@echo "Satellite Telemetry Simulator - Build System"
	@echo ""
	@echo "Targets:"
	@echo "  all     - Build both satcom and satcom_tests (default)"
	@echo "  test    - Build and run tests"
	@echo "  run     - Build and run simulation (10 seconds)"
	@echo "  clean   - Remove build artifacts"
	@echo "  help    - Show this help message"

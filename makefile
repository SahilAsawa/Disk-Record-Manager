CXX = g++
CXXFLAGS = -std=c++20 -Wall -Iinclude

# Output directories
BUILD_DIR = build
LIB_DIR = lib

# Source files
DISK_SRC = src/DiskManager/DiskManager.cpp
INDEX_SRC = src/Indexes/BPlusTreeIndex.cpp

# Header include paths (already covered by -Iinclude)
DISK_HEADERS = include/DiskManager/DiskManager.hpp
INDEX_HEADERS = include/Indexes/BPlusTreeIndex.hpp include/Indexes/HashIndex.hpp

# Object files
DISK_OBJ = $(BUILD_DIR)/DiskManager.o
INDEX_OBJ = $(BUILD_DIR)/BPlusTreeIndex.o

# Static libraries
DISK_LIB = $(LIB_DIR)/libdiskmanager.a
INDEX_LIB = $(LIB_DIR)/libindexes.a

# Test
TEST = test
TEST_SRC = test.cpp
TEST_OBJ = $(BUILD_DIR)/test.o

# Default target
all: $(TEST)

# Compile test.cpp and link with both static libs
$(TEST): $(TEST_OBJ) $(DISK_LIB) $(INDEX_LIB)
	$(CXX) $(CXXFLAGS) -o $@ $(TEST_OBJ) -L$(LIB_DIR) -ldiskmanager -lindexes

# Build object files
$(BUILD_DIR)/%.o: src/DiskManager/%.cpp
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: src/Indexes/%.cpp
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/test.o: $(TEST_SRC)
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Create static libraries
$(DISK_LIB): $(DISK_OBJ)
	@mkdir -p $(LIB_DIR)
	ar rcs $@ $^

$(INDEX_LIB): $(INDEX_OBJ)
	@mkdir -p $(LIB_DIR)
	ar rcs $@ $^

# Clean build artifacts
clean:
	rm -rf $(BUILD_DIR) $(LIB_DIR) $(TEST)

.PHONY: all clean

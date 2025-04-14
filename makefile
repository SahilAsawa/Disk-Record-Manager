CXX = g++
CXXFLAGS = -std=c++20 -Wall -Iinclude

# Output directories
BUILD_DIR = build
LIB_DIR = lib

# Source files
BUFFER_SRC = src/Storage/BufferManager.cpp
DISK_SRC = src/Storage/Disk.cpp
BPT_SRC = src/Indexes/BPlusTreeIndex.cpp

# Header include paths (already covered by -Iinclude)
STORAGE_HEADERS = include/Storage/Disk.hpp include/Storage/BufferManager.hpp
INDEX_HEADERS = include/Indexes/BPlusTreeIndex.hpp include/Indexes/HashIndex.hpp

# Object files
BUFFER_OBJ = $(BUILD_DIR)/BufferManager.o
DISK_OBJ = $(BUILD_DIR)/Disk.o
BPT_OBJ = $(BUILD_DIR)/BPlusTreeIndex.o

# Static libraries
STORAGE_LIB = $(LIB_DIR)/libstorage.a
INDEX_LIB = $(LIB_DIR)/libindexes.a

# Test
TEST = test
TEST_SRC = test.cpp

# External Merge Sort
EMS = ems
EMS_SRC = externalMergeSort.cpp

# Table
TABLE = table
TABLE_SRC = table.cpp

# Default target
all: $(TEST) $(EMS) $(TABLE)

# Compile test.cpp and link with both static libs
$(TEST): $(TEST_SRC) $(STORAGE_LIB) $(INDEX_LIB)
	$(CXX) $(CXXFLAGS) -o $@ $(TEST_SRC) -L$(LIB_DIR) -lstorage -lindexes

# Compile External Merge Sort
$(EMS): $(EMS_SRC) $(STORAGE_LIB) $(INDEX_LIB)
	$(CXX) $(CXXFLAGS) -o $@ $(EMS_SRC) -L$(LIB_DIR) -lstorage -lindexes

# Compile Table
$(TABLE): $(TABLE_SRC) $(STORAGE_LIB) $(INDEX_LIB)
	$(CXX) $(CXXFLAGS) -o $@ $(TABLE_SRC) -L$(LIB_DIR) -lstorage -lindexes

# Build object files
$(BUILD_DIR)/%.o: src/Storage/%.cpp $(STORAGE_HEADERS)
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: src/Indexes/%.cpp $(INDEX_HEADERS)
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Create static libraries
$(STORAGE_LIB): $(DISK_OBJ) $(BUFFER_OBJ)
	@mkdir -p $(LIB_DIR)
	ar rcs $@ $^

$(INDEX_LIB): $(BPT_OBJ)
	@mkdir -p $(LIB_DIR)
	ar rcs $@ $^

# Clean build artifacts
clean:
	rm -rf $(BUILD_DIR) $(LIB_DIR) $(TEST) $(EMS) $(TABLE) files/*.bin

.PHONY: all clean

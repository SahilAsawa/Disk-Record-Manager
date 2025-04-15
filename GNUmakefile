UNAME := $(shell uname)

CXX = g++
CXXFLAGS = -std=c++20 -Wall -Iinclude -fPIC

# Output directories
BUILD_DIR = build
LIB_DIR = lib
TESTING_DIR = testing
BIN_DIR = bin
RES_DIR = Results
STATS_DIR = Stats

# Source files
BUFFER_SRC = src/Storage/BufferManager.cpp
DISK_SRC = src/Storage/Disk.cpp
BPT_SRC = src/Indexes/BPlusTreeIndex.cpp
HASH_SRC = src/Indexes/HashIndex.cpp
UTIL_SRC = src/Utilities/Utils.cpp

# Header include paths (already covered by -Iinclude)
STORAGE_HEADERS = include/Storage/Disk.hpp include/Storage/BufferManager.hpp
INDEX_HEADERS = include/Indexes/BPlusTreeIndex.hpp include/Indexes/HashIndex.hpp
UTILS_HEADERS = include/Utilities/Utils.hpp

# Object files
BUFFER_OBJ = $(BUILD_DIR)/BufferManager.o
DISK_OBJ = $(BUILD_DIR)/Disk.o
BPT_OBJ = $(BUILD_DIR)/BPlusTreeIndex.o
HASH_OBJ = $(BUILD_DIR)/HashIndex.o
UTILS_OBJ = $(BUILD_DIR)/Utils.o

# Shared libraries
ifeq ($(UNAME), Linux)
	STORAGE_LIB = $(LIB_DIR)/libstorage.so
	INDEX_LIB = $(LIB_DIR)/libindexes.so
	UTILS_LIB = $(LIB_DIR)/libutils.so
else ifeq ($(UNAME), Darwin)
	STORAGE_LIB = $(LIB_DIR)/libstorage.dylib
	INDEX_LIB = $(LIB_DIR)/libindexes.dylib
	UTILS_LIB = $(LIB_DIR)/libutils.dylib
else
	STORAGE_LIB = $(LIB_DIR)/libstorage.dll
	INDEX_LIB = $(LIB_DIR)/libindexes.dll
	UTILS_LIB = $(LIB_DIR)/libutils.dll
endif

# Test targets
TEST = test
TEST_SRC = $(TESTING_DIR)/test.cpp

EMS = ems
EMS_SRC = $(TESTING_DIR)/externalMergeSort.cpp

TABLE = table
TABLE_SRC = $(TESTING_DIR)/table.cpp

ISORT = isort
ISORT_SRC = $(TESTING_DIR)/indexSort.cpp

NEST = nest
NEST_SRC = $(TESTING_DIR)/nestedLoopJoin.cpp

# Default target
all: $(TEST) $(EMS) $(TABLE) $(ISORT) $(NEST)

# Compile test.cpp and link with shared libs
$(TEST): $(TEST_SRC) $(STORAGE_LIB) $(INDEX_LIB) $(UTILS_LIB)
	$(CXX) $(CXXFLAGS) -o $@ $(TEST_SRC) -L$(LIB_DIR) -Wl,-rpath,$(LIB_DIR) -lstorage -lindexes -lutils

$(EMS): $(EMS_SRC) $(STORAGE_LIB) $(UTILS_LIB)
	$(CXX) $(CXXFLAGS) -o $@ $(EMS_SRC) -L$(LIB_DIR) -Wl,-rpath,$(LIB_DIR) -lstorage -lutils

$(TABLE): $(TABLE_SRC) $(UTILS_LIB)
	$(CXX) $(CXXFLAGS) -o $@ $(TABLE_SRC) -L$(LIB_DIR) -Wl,-rpath,$(LIB_DIR) -lutils

$(ISORT): $(ISORT_SRC) $(STORAGE_LIB) $(INDEX_LIB) $(UTILS_LIB)
	$(CXX) $(CXXFLAGS) -o $@ $(ISORT_SRC) -L$(LIB_DIR) -Wl,-rpath,$(LIB_DIR) -lstorage -lindexes -lutils

$(NEST): $(NEST_SRC) $(STORAGE_LIB) $(UTILS_LIB)
	$(CXX) $(CXXFLAGS) -o $@ $(NEST_SRC) -L$(LIB_DIR) -Wl,-rpath,$(LIB_DIR) -lstorage -lutils

# Build object files
$(BUILD_DIR)/%.o: src/Storage/%.cpp $(STORAGE_HEADERS)
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: src/Indexes/%.cpp $(INDEX_HEADERS)
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o: src/Utilities/%.cpp $(UTILS_HEADERS)
	@mkdir -p $(BIN_DIR) $(RES_DIR) $(STATS_DIR) $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Create shared libraries
$(STORAGE_LIB): $(DISK_OBJ) $(BUFFER_OBJ)
	@mkdir -p $(LIB_DIR)
	$(CXX) -shared -o $@ $^

$(UTILS_LIB): $(UTILS_OBJ)
	@mkdir -p $(LIB_DIR)
	$(CXX) -shared -o $@ $^

$(INDEX_LIB): $(BPT_OBJ) $(HASH_OBJ)
	@mkdir -p $(LIB_DIR)
	$(CXX) -shared -o $@ $^

# Clean build artifacts
clean:
	rm -rf $(BUILD_DIR) $(LIB_DIR) $(TEST) $(EMS) $(TABLE) $(ISORT) $(NEST) $(BIN_DIR) $(RES_DIR) $(STATS_DIR)

.PHONY: all clean

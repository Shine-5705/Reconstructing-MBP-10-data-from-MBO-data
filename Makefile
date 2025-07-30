# Makefile for Orderbook Reconstruction
CXX = g++
CXXFLAGS = -std=c++17 -O3 -Wall -Wextra -march=native -flto
TARGET = reconstruction_gupta.exe
SOURCE = orderbook_reconstruction.cpp
TEST_TARGET = unit_tests.exe
TEST_SOURCE = unit_tests.cpp

# Default target
all: $(TARGET)

# Build the executable
$(TARGET): $(SOURCE)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SOURCE)

# Build and run tests
test: $(TEST_TARGET)
	./$(TEST_TARGET)

$(TEST_TARGET): $(TEST_SOURCE)
	$(CXX) -std=c++17 -O2 -o $(TEST_TARGET) $(TEST_SOURCE)

# Clean build artifacts
clean:
	rm -f $(TARGET) $(TEST_TARGET) output_mbp.csv test_input.csv

# Run the program with the input file
run: $(TARGET)
	./$(TARGET) mbo.csv

# Debug build
debug: CXXFLAGS = -std=c++17 -g -Wall -Wextra -DDEBUG
debug: $(TARGET)

# Performance optimized build
release: CXXFLAGS = -std=c++17 -O3 -DNDEBUG -march=native -flto -funroll-loops
release: $(TARGET)

.PHONY: all clean run debug release test

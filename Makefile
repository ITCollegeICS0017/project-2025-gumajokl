# Compiler and flags
CXX=g++
CXXFLAGS=-Wall -Wextra -Werror -std=c++17 -Iinclude

# Source and object files
SRC=$(wildcard src/*.cpp)
OBJ=$(SRC:.cpp=.o)

# Executable name
BIN=main

# Detect OS to add .exe for Windows
ifeq ($(OS),Windows_NT)
    BIN_EXE=$(BIN).exe
else
    BIN_EXE=$(BIN)
endif

# Phony targets
.PHONY: all run clean

# Default target
all: $(BIN_EXE)

# Compile object files and link
$(BIN_EXE): $(OBJ)
	$(CXX) $(CXXFLAGS) -o $@ $^

# Run the executable
run: $(BIN_EXE)
	./$(BIN_EXE)

# Clean build
clean:
	rm -f $(BIN_EXE) src/*.o

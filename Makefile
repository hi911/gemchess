# ============================================================================
#  Gemchess Makefile
#  Derived from the logic of Stockfish's Makefile
# ============================================================================

# --- 1. General Configuration ---

# Define the executable name
EXE = gemchess

# Determine the operating system
KERNEL := $(shell uname -s)
ifeq ($(KERNEL),Linux)
	OS := Linux
endif

# Windows detection (MinGW/Git Bash)
ifeq ($(OS),Windows_NT)
	EXE = gemchess.exe
endif

# --- 2. Compiler Setup ---

# Default compiler is g++, but you can override it (e.g., make COMP=clang)
CXX = g++
ifeq ($(COMP),clang)
	CXX = clang++
endif

# Standard C++17 is required (Stockfish uses C++17)
CXXFLAGS = -std=c++17 -Wall -Wextra -Wshadow

# --- 3. Automatic Optimization & Architecture Detection ---

# -O3: Enables aggressive optimizations (loop unrolling, inlining, etc.)
# -march=native: Automatically detects your CPU (Intel/AMD/ARM) and enables
#                the best instruction sets (AVX2, AVX-512, NEON, etc.).
CXXFLAGS += -O3 -march=native

# Linker flags (needed for threading)
LDFLAGS = 

# Detect Windows and force static linking
ifeq ($(OS),Windows_NT)
	LDFLAGS += -static
else
	# Linux/macOS usually need pthread linked explicitly
	LDFLAGS += -lpthread
endif

# --- 4. Source Files ---

# Currently only main.cpp. As you add files (bitboard.cpp, movegen.cpp),
# add them to this list.
SRCS = main.cpp position.cpp bitboard.cpp movegen.cpp evaluate.cpp search.cpp zobrist.cpp tt.cpp

# Create object file names automatically
OBJS = $(SRCS:.cpp=.o)

# --- 5. Targets ---

# Default target: build the engine
all: $(EXE)

# Compile source files into object files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Link object files into the executable
$(EXE): $(OBJS)
	$(CXX) $(OBJS) $(LDFLAGS) -o $(EXE)

# Clean up build files
clean:
	rm -f $(EXE) $(OBJS)

# Help message
help:
	@echo "Gemchess Makefile"
	@echo ""
	@echo "Commands:"
	@echo "  make        - Build the engine (auto-detects architecture)"
	@echo "  make clean  - Remove built files"
	@echo "  make COMP=clang - Build using Clang instead of GCC"
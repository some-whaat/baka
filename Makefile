# Compiler and flags
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -Wpedantic
LDFLAGS = -lglfw -lvulkan -ldl -lpthread -lX11 -lXxf86vm -lXrandr -lXi

# Source files
CPP_FILES = $(wildcard *.cpp)
TARGET = baka

# Build configurations
DEBUG_FLAGS = -g -O0 -DDEBUG -fno-omit-frame-pointer
RELEASE_FLAGS = -O2 -DNDEBUG -s

# Default build (release)
all: release

# Debug build with full debug info
debug: CXXFLAGS += $(DEBUG_FLAGS)
debug: $(TARGET)

# Release build (optimized, no debug)
release: CXXFLAGS += $(RELEASE_FLAGS)
release: $(TARGET)

# Build target
$(TARGET): $(CPP_FILES)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(CPP_FILES) $(LDFLAGS)
	@echo "Build complete: $(TARGET)"
	@if echo "$(CXXFLAGS)" | grep -q "\-g"; then \
		echo "Debug symbols: ENABLED"; \
	else \
		echo "Debug symbols: DISABLED"; \
	fi

# Run targets
run: $(TARGET)
	./$(TARGET)

# Debug run (using gdb directly)
debug-run: debug
	gdb -q -ex "set confirm off" -ex "run" -ex "bt" --args ./$(TARGET)

# Clean
clean:
	rm -f $(TARGET)
	rm -f core.*
	@echo "Cleaned up"

# Phony targets
.PHONY: all debug release run debug-run clean t c
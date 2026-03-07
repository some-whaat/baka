# Compiler and flags
CXX = g++
CXXFLAGS = -std=c++17 -Wall -Wextra -Wpedantic -I/home/somewhat/projects/grathics_stuff/baka/libs
LDFLAGS = -lglfw -lvulkan -ldl -lpthread -lX11 -lXxf86vm -lXrandr -lXi

# Source files
CPP_FILES = $(wildcard *.cpp)
TARGET = baka

# Build configurations
DEBUG_FLAGS = -g -O0 -DDEBUG -fno-omit-frame-pointer
RELEASE_FLAGS = -O2 -DNDEBUG -s

# Shader compiler
SHADER_C = glslc
SHADER_FLAGS = -O

# Find all shader source files
VERT_SOURCES = $(shell find ./shaders -type f -name "*.vert")
FRAG_SOURCES = $(shell find ./shaders -type f -name "*.frag")
COMP_SOURCES = $(shell find ./shaders -type f -name "*.comp")
SHADER_SOURCES = $(VERT_SOURCES) $(FRAG_SOURCES) $(COMP_SOURCES)

# Corresponding SPIR-V output files
VERT_SPV = $(VERT_SOURCES:%.vert=%.vert.spv)
FRAG_SPV = $(FRAG_SOURCES:%.frag=%.frag.spv)
COMP_SPV = $(COMP_SOURCES:%.comp=%.comp.spv)
SHADER_SPV = $(VERT_SPV) $(FRAG_SPV) $(COMP_SPV)

# Default build (release)
all: release

# Debug build with full debug info
debug: CXXFLAGS += $(DEBUG_FLAGS)
debug: $(TARGET)

# Release build (optimized, no debug)
release: CXXFLAGS += $(RELEASE_FLAGS)
release: $(TARGET)

$(TARGET): $(SHADER_SPV) $(CPP_FILES)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(CPP_FILES) $(LDFLAGS)
	@echo "Build complete: $(TARGET)"
	@if echo "$(CXXFLAGS)" | grep -q "\-g"; then \
		echo "Debug symbols: ENABLED"; \
	else \
		echo "Debug symbols: DISABLED"; \
	fi

# Rule to compile vertex shaders
%.vert.spv: %.vert
	@echo "Compiling vertex shader: $< -> $@"
	@mkdir -p $(dir $@)
	$(SHADER_C) $(SHADER_FLAGS) $< -o $@

# Rule to compile fragment shaders
%.frag.spv: %.frag
	@echo "Compiling fragment shader: $< -> $@"
	@mkdir -p $(dir $@)
	$(SHADER_C) $(SHADER_FLAGS) $< -o $@

# Rule to compile compute shaders (optional, if you use them)
%.comp.spv: %.comp
	@echo "Compiling compute shader: $< -> $@"
	@mkdir -p $(dir $@)
	$(SHADER_C) $(SHADER_FLAGS) $< -o $@

# Run targets
run: $(TARGET)
	./$(TARGET)

# Debug run (using gdb directly)
debug-run: debug
	gdb -q -ex "set confirm off" -ex "run" -ex "bt" --args ./$(TARGET)

# Clean everything including shaders
clean:
	rm -f $(TARGET)
	rm -f core.*
	rm -f $(SHADER_SPV)
	@echo "Cleaned up binary and compiled shaders"

# Clean only compiled shaders
clean-shaders:
	rm -f $(SHADER_SPV)
	@echo "Cleaned up compiled shaders"

# List all shaders
list-shaders:
	@echo "Vertex shaders:"
	@echo "$(VERT_SOURCES)"
	@echo ""
	@echo "Fragment shaders:"
	@echo "$(FRAG_SOURCES)"
	@echo ""
	@echo "Compute shaders:"
	@echo "$(COMP_SOURCES)"

# Compile only shaders
shaders: $(SHADER_SPV)
	@echo "All shaders compiled"

# Recompile everything
rebuild: clean all

# Phony targets
.PHONY: all debug release run debug-run clean clean-shaders list-shaders shaders rebuild
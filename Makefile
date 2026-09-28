# Makefile for PassPix
# Optimized for MinGW make on Windows and native builds on Linux

# Compiler and tools
CXX ?= g++

# Detect platform
ifeq ($(OS),Windows_NT)
    # Windows with MinGW
    PLATFORM = Windows
    EXE_EXT = .exe
    RM = cmd /c del /Q
    RMDIR = cmd /c rd /s /q
    MKDIR = cmd /c if not exist
    PATH_SEP = \\
else
    # Linux/Unix
    PLATFORM = Linux
    EXE_EXT =
    RM = rm -f
    RMDIR = rm -rf
    MKDIR = mkdir -p
    PATH_SEP = /
endif

# Project settings
TARGET = passpix$(EXE_EXT)
BUILD_DIR = build
OBJ_DIR = $(BUILD_DIR)/obj

# Source files
SOURCES = src/main.cpp \
          src/crypto_utils.cpp \
          src/image_gen.cpp \
          src/stego.cpp \
          src/image_utils.cpp \
          src/terminal.cpp \
          src/clipboard.cpp \
          Include/lodepng.cpp

# Object files
OBJECTS = $(SOURCES:%.cpp=$(OBJ_DIR)/%.o)

# Include directories
INCLUDES = -I. -IInclude -Isrc

# Optimization and warning flags
CXXFLAGS = -std=c++17 -Wall -Wextra -pedantic -D_FORTIFY_SOURCE=2 -fstack-protector-strong
DEPFLAGS = -MMD -MP
OPTFLAGS = -O3 -march=native -mtune=native -flto
LDFLAGS = -flto

# Extra flags appended to every compile and link (used by CI)
EXTRA_CXXFLAGS ?=

# Treat warnings as errors
ifdef WERROR
    EXTRA_CXXFLAGS += -Werror
endif

# Address and undefined-behavior sanitizers (implies a debug build)
ifdef SANITIZE
    DEBUG = 1
    EXTRA_CXXFLAGS += -fsanitize=address,undefined -fno-omit-frame-pointer
endif

# Portable build (overrides native arch for reproducible release binaries)
ifdef PORTABLE
    OPTFLAGS = -O3 -march=x86-64-v2 -mtune=generic -flto
endif

# Platform-specific settings
ifeq ($(PLATFORM),Windows)
    # Windows/MinGW specific settings
    LIBS = -lsodium -ladvapi32
    # Use static linking on Windows to avoid DLL dependencies
    LDFLAGS += -static
else
    # Linux specific settings
    LIBS = -lsodium -lpthread
    LDFLAGS += -Wl,-z,relro -Wl,-z,now
endif

# Debug build support
ifdef DEBUG
    CXXFLAGS := $(filter-out -D_FORTIFY_SOURCE=2,$(CXXFLAGS)) -g -O0 -DDEBUG
    OPTFLAGS =
    LDFLAGS =
else
    CXXFLAGS += $(OPTFLAGS) -DNDEBUG
endif

CXXFLAGS += $(EXTRA_CXXFLAGS)

export CXX EXTRA_CXXFLAGS

# Build rules
.PHONY: all clean rebuild install help

all: $(TARGET)

# Link executable
$(TARGET): $(OBJECTS)
	@echo Linking $@
	$(CXX) $(CXXFLAGS) $(OBJECTS) -o $@ $(LDFLAGS) $(LIBS)
	@echo Build complete: $(TARGET)

# Compile source files
$(OBJ_DIR)/%.o: %.cpp
	@echo Compiling $<
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) $(DEPFLAGS) $(INCLUDES) -c $< -o $@

# Clean build artifacts
clean:
ifeq ($(PLATFORM),Windows)
	@if exist "$(subst /,\,$(BUILD_DIR))" $(RMDIR) "$(subst /,\,$(BUILD_DIR))"
	@if exist "$(TARGET)" $(RM) "$(TARGET)"
else
	@$(RMDIR) $(BUILD_DIR) 2>/dev/null || true
	@$(RM) $(TARGET) 2>/dev/null || true
endif
	-@$(MAKE) -s -C test clean
	@echo "Clean complete"

# Rebuild from scratch
rebuild: clean all

# Install (copy to /usr/local/bin on Linux, not applicable on Windows)
install: $(TARGET)
ifeq ($(PLATFORM),Linux)
	install -m 755 $(TARGET) /usr/local/bin/
	@echo "Installed $(TARGET) to /usr/local/bin/"
else
	@echo "Install target not supported on Windows. Copy $(TARGET) manually to desired location."
endif

# Help target
help:
	@echo "PassPix Makefile"
	@echo ""
	@echo "Usage:"
	@echo "  make              - Build the project (optimized)"
	@echo "  make DEBUG=1      - Build with debug symbols"
	@echo "  make SANITIZE=1   - Debug build with address/UB sanitizers"
	@echo "  make WERROR=1     - Treat compiler warnings as errors"
	@echo "  make PORTABLE=1   - Build for generic x86-64-v2 (release binaries)"
	@echo "  make clean        - Remove build artifacts"
	@echo "  make rebuild      - Clean and build"
	@echo "  make test         - Run unit, integration and smoke tests"
	@echo "  make install      - Install binary (Linux only)"
	@echo "  make help         - Show this help message"
	@echo ""
	@echo "Platform: $(PLATFORM)"
	@echo "Compiler: $(CXX)"

# Test target
.PHONY: test
test: $(TARGET)
	@echo "Running unit and integration tests..."
	@$(MAKE) -C test test
	@echo "Running smoke tests..."
	@bash test/smoke_test.sh ./$(TARGET)

# Dependencies
-include $(OBJECTS:.o=.d)

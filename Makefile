TITLEID     := RAYMARCH
TARGET      := raymarch
SOURCES     := src

INCLUDES    := include

# Vita-specific libraries
VITA_LIBS = -lvitaGL -lSceLibKernel_stub -lSceAppMgr_stub -lSceAppUtil_stub -lmathneon \
    -lc -lSceCommonDialog_stub -lm -lSceGxm_stub -lSceDisplay_stub -lSceSysmodule_stub \
    -lvitashark -lSceShaccCg_stub -lSceKernelDmacMgr_stub -lstdc++ -lSceCtrl_stub \
    -ltoloader -lSceShaccCgExt -ltaihen_stub -lpng -lz -lm

# Linux-specific libraries
LINUX_LIBS = -lGL -lGLU -lglfw -lGLEW -lm -lpng -lz -lstdc++

BUILD_DIR := build
LINUX_BUILD_DIR := build_linux

CFILES    := $(foreach dir,$(SOURCES), $(wildcard $(dir)/*.c))
CPPFILES  := $(foreach dir,$(SOURCES), $(wildcard $(dir)/*.cpp))
BINFILES  := $(foreach dir,$(DATA), $(wildcard $(dir)/*.bin))

# Generate object file names with proper paths
OBJS      := $(addprefix $(BUILD_DIR)/,$(addsuffix .o,$(BINFILES)) \
              $(CFILES:.c=.o) $(CPPFILES:.cpp=.o))

LINUX_OBJS := $(addprefix $(LINUX_BUILD_DIR)/,$(addsuffix .o,$(BINFILES)) \
              $(CFILES:.c=.o) $(CPPFILES:.cpp=.o))

PREFIX    = arm-vita-eabi
CC        = $(PREFIX)-gcc
CXX       = $(PREFIX)-g++
CFLAGS    = -g -Wl,-q -O2 -ftree-vectorize
CXXFLAGS  = $(CFLAGS) -fno-exceptions -std=c++11 -fpermissive
ASFLAGS   = $(CFLAGS)

# Linux compiler flags
LINUX_CC = gcc
LINUX_CXX = g++
LINUX_CFLAGS = -g -O2 -Wall
LINUX_CXXFLAGS = $(LINUX_CFLAGS) -std=c++11

# Default target
all: vita

# Vita build
vita: $(BUILD_DIR)/$(TARGET).vpk

# Linux build
linux: $(LINUX_BUILD_DIR)/$(TARGET)

$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

$(LINUX_BUILD_DIR):
	@mkdir -p $(LINUX_BUILD_DIR)

$(BUILD_DIR)/$(TARGET).vpk: $(BUILD_DIR)/eboot.bin
	vita-mksfoex -s TITLE_ID=$(TITLEID) "$(TARGET)" $(BUILD_DIR)/param.sfo
	vita-pack-vpk -s $(BUILD_DIR)/param.sfo -b $(BUILD_DIR)/eboot.bin \
		-a assets/shaders/raymarch.vert=raymarch.vert \
		-a assets/shaders/raymarch.frag=raymarch.frag $@

$(BUILD_DIR)/eboot.bin: $(BUILD_DIR)/$(TARGET).velf
	vita-make-fself -s $< $@

$(BUILD_DIR)/%.velf: $(BUILD_DIR)/%.elf
	vita-elf-create $< $@

$(BUILD_DIR)/$(TARGET).elf: $(OBJS) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $^ $(VITA_LIBS) -o $@

# Linux executable
$(LINUX_BUILD_DIR)/$(TARGET): $(LINUX_OBJS) | $(LINUX_BUILD_DIR)
	$(LINUX_CXX) $(LINUX_CXXFLAGS) $^ $(LINUX_LIBS) -o $@

# Build rules for C files (Vita)
$(BUILD_DIR)/%.o: %.c | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -I$(INCLUDES) -c $< -o $@

# Build rules for C++ files (Vita)
$(BUILD_DIR)/%.o: %.cpp | $(BUILD_DIR)
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -I$(INCLUDES) -c $< -o $@

# Build rules for C files (Linux)
$(LINUX_BUILD_DIR)/%.o: %.c | $(LINUX_BUILD_DIR)
	@mkdir -p $(dir $@)
	$(LINUX_CC) $(LINUX_CFLAGS) -I$(INCLUDES) -DLINUX_BUILD -c $< -o $@

# Build rules for C++ files (Linux)
$(LINUX_BUILD_DIR)/%.o: %.cpp | $(LINUX_BUILD_DIR)
	@mkdir -p $(dir $@)
	$(LINUX_CXX) $(LINUX_CXXFLAGS) -I$(INCLUDES) -DLINUX_BUILD -c $< -o $@

clean:
	@rm -rf $(BUILD_DIR) $(LINUX_BUILD_DIR)

# Install Linux dependencies (Ubuntu/Debian)
install-deps:
	sudo apt-get update
	sudo apt-get install -y libglfw3-dev libglew-dev libpng-dev libgl1-mesa-dev

# Run Linux build
run: linux
	$(LINUX_BUILD_DIR)/$(TARGET)
# raylib build (macOS + Linux).
#
#   macOS : brew install raylib
#   Linux : libraylib-dev (Debian/Ubuntu), raylib (Arch/Fedora), or from source
#
# If raylib is built from source rather than installed via a package manager:
#   make RAYLIB_PATH=/path/to/raylib

TARGET := jellyfish
SRC    := main.c
WARN   := -Wall -Wextra -std=c11

# ---- platform detection -----------------------------------------------------
ifeq ($(shell uname -s),Darwin)
    PLATFORM := macos
else
    PLATFORM := linux          # also covers *BSD, close enough to link
endif

# ---- toolchain --------------------------------------------------------------
# Only override make's built-in CC default, never an explicit CC= from the user.
ifeq ($(origin CC),default)
    ifeq ($(PLATFORM),macos)
        CC := clang
    else
        CC := gcc
    endif
endif

# ---- locating raylib --------------------------------------------------------
RAYLIB_CFLAGS := $(shell pkg-config --cflags raylib 2>/dev/null)
RAYLIB_LDPATH := $(shell pkg-config --libs-only-L raylib 2>/dev/null)

# An explicit RAYLIB_PATH always wins over whatever pkg-config reported.
ifdef RAYLIB_PATH
    RAYLIB_CFLAGS := -I$(RAYLIB_PATH)/src
    RAYLIB_LDPATH := -L$(RAYLIB_PATH)/src
endif

# ---- platform link libraries ------------------------------------------------
ifeq ($(PLATFORM),macos)
    PLATFORM_LIBS := -framework CoreVideo -framework IOKit -framework Cocoa -framework OpenGL
else
    PLATFORM_LIBS := -lm -ldl -lpthread -lGL -lrt -lX11
endif

CFLAGS  := $(WARN) $(RAYLIB_CFLAGS)
LDFLAGS := $(RAYLIB_LDPATH)
LDLIBS  := -lraylib $(PLATFORM_LIBS)

# ---- rules ------------------------------------------------------------------
all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) $(SRC) -o $@ $(LDFLAGS) $(LDLIBS)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -f $(TARGET)

.PHONY: all run clean

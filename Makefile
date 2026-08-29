
# ─────────────────────────────────────────────
# Compiler
# ─────────────────────────────────────────────

CXX := g++


# ─────────────────────────────────────────────
# Project files
# ─────────────────────────────────────────────

TARGET := executables/snake

SOURCES := main.cpp \
           lib/Vector2.cpp


# ─────────────────────────────────────────────
# Compiler flags
# ─────────────────────────────────────────────

CXXFLAGS := -Wall -Wextra -std=c++17

SDL_CFLAGS := $(shell sdl2-config --cflags)makefile
SDL_LIBS   := $(shell sdl2-config --libs)

LIBS := $(SDL_LIBS) \
        -lSDL2_image \
        -lSDL2_ttf \
        -lSDL2_mixer


# ─────────────────────────────────────────────
# Build
# ─────────────────────────────────────────────

all: $(TARGET)


$(TARGET): $(SOURCES)
	$(CXX) $(CXXFLAGS) $(SDL_CFLAGS) $(SOURCES) -o $(TARGET) $(LIBS)


# ─────────────────────────────────────────────
# Clean
# ─────────────────────────────────────────────

clean:
	rm -f $(TARGET)


.PHONY: all clean

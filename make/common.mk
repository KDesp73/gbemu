# Compiler and flags
CC = gcc
CFLAGS = -Wall -Isrc -fPIC
LDFLAGS =

# Headless mode: skip SDL entirely
ifneq ($(HEADLESS),1)
    CFLAGS  += $(shell PKG_CONFIG_PATH=/opt/homebrew/lib/pkgconfig pkg-config --cflags sdl3)
    LDFLAGS += $(shell PKG_CONFIG_PATH=/opt/homebrew/lib/pkgconfig pkg-config --libs sdl3)
endif

# Directories
SRC_DIR = src
FRONTEND_DIR = frontend
APPS_DIR = apps
BUILD_DIR = build
DIST_DIR = dist
PREFIX = /usr/local/bin

LIBRARY_NAME = gbemu
SO_NAME = lib$(LIBRARY_NAME).so
A_NAME = lib$(LIBRARY_NAME).a

# Target
TARGET = $(LIBRARY_NAME)-cli

# Determine the build type
ifeq ($(type), RELEASE)
	CFLAGS += -O3
else
	SANITIZERS = -fsanitize=address,undefined
	CFLAGS  += -DDEBUG -ggdb
	CFLAGS  += $(SANITIZERS)
	LDFLAGS += $(SANITIZERS)
endif

# Source and object files (core library only, excluding frontend and apps)
SRC_FILES := $(shell find $(SRC_DIR) -maxdepth 1 -name '*.c')
OBJ_FILES = $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SRC_FILES))

ifeq ($(HEADLESS),1)
    CFLAGS  += -DEMU_HEADLESS
    FRONTEND_SRC = $(FRONTEND_DIR)/headless.c
else
    FRONTEND_SRC = $(FRONTEND_DIR)/sdl.c
endif

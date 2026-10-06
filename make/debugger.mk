# Graphical debugger (SDL frontend only)
#
# Builds Dear ImGui + cimgui and the debugger UI into gbemu-cli.
# Enabled by default for SDL builds when the vendored submodules exist:
#
#   make all              # debugger on when third_party/imgui is present
#   make all DEBUGGER=0   # force off
#   make all DEBUGGER=1   # force on (hard error if submodules are missing)
#
# TERM=1 and HEADLESS=1 builds never enable it. wasm/esp32 targets only use
# the core half of the debugger (src/debug.c), never ImGui.

DEBUGGER ?= auto

IMGUI_DIR := third_party/imgui
CIMGUI_DIR := third_party/cimgui
DEPS_BUILD_DIR := $(BUILD_DIR)/deps

# Defaults: no debugger, plain C link, no dependency objects.
GBEMU_DEBUGGER :=
LINK_CC := $(CC)
DEPS_OBJ :=

ifeq ($(TERM),1)
    # Terminal frontend: no debugger.
else ifeq ($(HEADLESS),1)
    # Headless frontend: no debugger.
else
    ifeq ($(DEBUGGER),0)
        # Explicitly disabled.
    else ifeq ($(DEBUGGER),1)
        ifeq ($(wildcard $(IMGUI_DIR)/imgui.h),)
$(error DEBUGGER=1 but $(IMGUI_DIR)/imgui.h is missing - run: git submodule update --init --recursive)
        endif
        GBEMU_DEBUGGER := 1
    else # auto
        ifneq ($(wildcard $(IMGUI_DIR)/imgui.h),)
            GBEMU_DEBUGGER := 1
        else
$(warning third_party/imgui is missing - building without the debugger. Run: git submodule update --init --recursive)
        endif
    endif
endif

ifeq ($(GBEMU_DEBUGGER),1)
    CFLAGS += -DGBEMU_DEBUGGER -I$(CIMGUI_DIR)
    LINK_CC := $(CXX)

    # The ImGui panels are frontend code and build like the rest of the frontend.
    FRONTEND_SRC += $(FRONTEND_DIR)/debugger.c

    DEPS_SRC := \
        $(IMGUI_DIR)/imgui.cpp \
        $(IMGUI_DIR)/imgui_demo.cpp \
        $(IMGUI_DIR)/imgui_draw.cpp \
        $(IMGUI_DIR)/imgui_tables.cpp \
        $(IMGUI_DIR)/imgui_widgets.cpp \
        $(IMGUI_DIR)/backends/imgui_impl_sdl3.cpp \
        $(IMGUI_DIR)/backends/imgui_impl_sdlrenderer3.cpp \
        $(CIMGUI_DIR)/cimgui.cpp \
        $(FRONTEND_DIR)/imgui_backends.cpp
    DEPS_OBJ := $(patsubst %.cpp,$(DEPS_BUILD_DIR)/%.o,$(DEPS_SRC))

    # -Ithird_party lets cimgui.cpp resolve its "#include ./imgui/..." against
    # our pinned imgui checkout instead of cloning a second copy.
    $(DEPS_BUILD_DIR)/%.o: %.cpp
	@mkdir -p $(dir $@)
	@echo "[DEPS] Compiling $<"
	@$(CXX) $(CFLAGS) -I$(IMGUI_DIR) -I$(IMGUI_DIR)/backends -I$(CIMGUI_DIR) -Ithird_party -c -o $@ $<
endif

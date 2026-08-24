# ESP32 target: cross-compile the emulator core as a static library
#
#   make esp32-lib                    # build/esp32/libgbemu-<target>.a
#   make ESP32_TARGET=esp32s3 esp32-lib
#
# Only the Espressif toolchain needs to be in PATH (e.g. after sourcing
# esp-idf/export.sh). The core is freestanding C: no ESP-IDF headers are
# required, so it links into any external ESP-IDF project.
#
# frontend/esp32.c (display/input/audio) needs the full IDF component
# environment; copy or reference it from your own ESP-IDF project.

ESP32_TARGET ?= esp32

ifeq ($(ESP32_TARGET),esp32)
    ESP32_TOOLCHAIN := xtensa-esp32-elf-
else ifeq ($(ESP32_TARGET),esp32s2)
    ESP32_TOOLCHAIN := xtensa-esp32s2-elf-
else ifeq ($(ESP32_TARGET),esp32s3)
    ESP32_TOOLCHAIN := xtensa-esp32s3-elf-
else ifeq ($(ESP32_TARGET),esp32c3)
    ESP32_TOOLCHAIN := riscv32-esp-elf-
else
    $(error Unsupported ESP32_TARGET '$(ESP32_TARGET)'. Use esp32|esp32s2|esp32s3|esp32c3)
endif

ESP32_CC := $(ESP32_TOOLCHAIN)gcc
ESP32_AR := $(ESP32_TOOLCHAIN)ar

ESP32_BUILD_DIR := build/esp32
ESP32_A_NAME := libgbemu-$(ESP32_TARGET).a

# No sanitizers/PIE here; the target is bare-metal newlib. ESP_PLATFORM is the
# ecosystem-wide marker that switches the portable paths in loop.c/bus.c.
ESP32_CFLAGS = -Wall -Isrc -O2 -DESP_PLATFORM -ffunction-sections -fdata-sections

.PHONY: esp32-lib
esp32-lib: $(ESP32_BUILD_DIR) $(addprefix $(ESP32_BUILD_DIR)/,$(notdir $(OBJ_FILES)))  ## Cross-compile the gbemu core for $(ESP32_TARGET)
	@command -v $(ESP32_CC) >/dev/null 2>&1 || { \
		echo "[ERR] $(ESP32_CC) not found in PATH."; \
		echo "      Install ESP-IDF and source its environment, e.g.:"; \
		echo "        . \$${HOME}/esp/esp-idf/export.sh"; \
		exit 1; }
	@$(ESP32_AR) rcs $(ESP32_BUILD_DIR)/$(ESP32_A_NAME) $(addprefix $(ESP32_BUILD_DIR)/,$(notdir $(OBJ_FILES)))
	@echo "[INFO] Built $(ESP32_BUILD_DIR)/$(ESP32_A_NAME)"

$(ESP32_BUILD_DIR):
	mkdir -p $(ESP32_BUILD_DIR)

$(ESP32_BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@echo "[ESP32] Compiling $<"
	@$(ESP32_CC) $(ESP32_CFLAGS) -c -o $@ $<

.PHONY: esp32-clean
esp32-clean:  ## Remove ESP32 build artifacts
	@rm -rf $(ESP32_BUILD_DIR)
	@echo "[INFO] Cleaned ESP32 artifacts."

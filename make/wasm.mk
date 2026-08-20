EMCC = emcc
WASM_DIR = web
WASM_BUILD_DIR = build/wasm
WASM_OUT = $(WASM_DIR)/emu-wasm
WASM_CFLAGS = -Wall -Isrc -Iinclude
WASM_LDFLAGS = \
	-s ALLOW_MEMORY_GROWTH \
	-s INITIAL_MEMORY=16777216 \
	-s EXPORTED_RUNTIME_METHODS='["FS"]' \
	-s EXPORTED_FUNCTIONS='["_main","_wasm_set_key","_wasm_load_rom","_wasm_is_running"]' \
	--shell-file $(WASM_DIR)/shell.html

ifeq ($(type), RELEASE)
	WASM_CFLAGS += -O3
else
	WASM_CFLAGS += -O2 -g
	WASM_LDFLAGS += -s ASSERTIONS=2
endif

WASM_SRC_FILES := $(shell find $(SRC_DIR) -name '*.c' ! -name 'main.c' ! -name 'main_wasm.c' ! -path '*/frontend/*' ! -name 'loop.c')
WASM_OBJ_FILES = $(patsubst $(SRC_DIR)/%.c,$(WASM_BUILD_DIR)/%.o,$(WASM_SRC_FILES))
WASM_FRONTEND_SRC = $(SRC_DIR)/frontend/wasm.c
WASM_MAIN_SRC = $(SRC_DIR)/main_wasm.c

ifdef WASM_ROM
	WASM_LDFLAGS += --preload-file "$(WASM_ROM)"@/rom.gb
endif

.PHONY: wasm
wasm: $(WASM_BUILD_DIR) $(WASM_DIR) $(WASM_OBJ_FILES) ## Build WASM target for web browsers
	@echo "[INFO] Compiling WASM build..."
	@$(EMCC) $(WASM_CFLAGS) $(WASM_LDFLAGS) \
		$(WASM_OBJ_FILES) $(WASM_FRONTEND_SRC) $(WASM_MAIN_SRC) \
		-o $(WASM_OUT).html
	@echo "[INFO] Build complete: $(WASM_OUT).html"

$(WASM_BUILD_DIR):
	@mkdir -p $(WASM_BUILD_DIR)

$(WASM_DIR):
	@mkdir -p $(WASM_DIR)

$(WASM_BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@echo "[WASM] Compiling $<"
	@$(EMCC) $(WASM_CFLAGS) -c -o $@ $<

.PHONY: wasm.serve
wasm.serve: wasm ## Start a local HTTP server for testing the WASM build
	@echo "Serving at http://localhost:8080"
	@python3 -m http.server 8080 -d $(WASM_DIR)

.PHONY: wasm.clean
wasm.clean: ## Remove WASM build artifacts
	@echo "[INFO] Cleaning WASM build."
	@rm -rf $(WASM_BUILD_DIR) $(WASM_OUT).html $(WASM_OUT).js $(WASM_OUT).wasm $(WASM_OUT).data

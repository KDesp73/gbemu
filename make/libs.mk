# Total source file count
TOTAL_FILES := $(words $(SRC_FILES))

# Counter to track progress
counter = 0

# Application and frontend are compiled to objects so the final link can use
# $(LINK_CC) (g++ when the graphical debugger is enabled, plain $(CC) otherwise).
APP_OBJ = $(BUILD_DIR)/cli.o
FRONTEND_OBJ = $(patsubst $(FRONTEND_DIR)/%.c,$(BUILD_DIR)/frontend_%.o,$(FRONTEND_SRC))

.PHONY: all
all: check_tools $(BUILD_DIR) static shared $(TARGET)  ## Build the project
	@echo "Build complete."

$(BUILD_DIR):  ## Create the build directory if it doesn't exist
	@echo "[INFO] Creating build directory"
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c  ## Compile source files with progress
	$(eval counter=$(shell echo $$(($(counter)+1))))
	@echo "[$(counter)/$(TOTAL_FILES)] Compiling $< -> $@"
	@$(CC) $(CFLAGS) -c -o $@ $<

$(APP_OBJ): $(APPS_DIR)/cli.c | $(BUILD_DIR)  ## Compile the CLI entry point
	@echo "[APP] Compiling $<"
	@$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD_DIR)/frontend_%.o: $(FRONTEND_DIR)/%.c | $(BUILD_DIR)  ## Compile the active frontend
	@echo "[FRONTEND] Compiling $<"
	@$(CC) $(CFLAGS) -c -o $@ $<

$(TARGET): $(BUILD_DIR) static $(APP_OBJ) $(FRONTEND_OBJ) $(DEPS_OBJ)  ## Build executable using static library
	@echo "[INFO] Building executable: $(TARGET)"
	@$(LINK_CC) $(APP_OBJ) $(FRONTEND_OBJ) $(DEPS_OBJ) -o $(TARGET) -L. $(A_NAME) $(CFLAGS) $(LDFLAGS)

.PHONY: shared
shared: $(BUILD_DIR) $(OBJ_FILES)  ## Build shared library
	@echo "[INFO] Building shared library: $(SO_NAME)"
	@$(CC) -shared $(CFLAGS) -o $(SO_NAME) $(OBJ_FILES)

.PHONY: static
static: $(BUILD_DIR) $(OBJ_FILES)  ## Build static library
	@echo "[INFO] Building static library: $(A_NAME)"
	@$(AR) rcs $(A_NAME) $(OBJ_FILES)

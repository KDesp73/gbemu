.PHONY: check_tools
check_tools: ## Check if necessary tools are available
	@command -v gcc >/dev/null 2>&1 || { echo >&2 "[ERRO] gcc is not installed."; exit 1; }
	@command -v bear >/dev/null 2>&1 || { echo >&2 "[WARN] bear is not installed. Skipping compile_commands.json target."; }

.PHONY: clean
clean: ## Remove all build files and the executable
	@echo "[INFO] Cleaning up build directory and executable."
	rm -rf $(BUILD_DIR) $(TARGET) $(SO_NAME) $(A_NAME)

.PHONY: distclean
distclean: clean ## Perform a full clean, including backup and temporary files
	@echo "[INFO] Performing full clean, removing build directory, dist files, and editor backups."
	rm -f *~ core $(SRC_DIR)/*~ $(DIST_DIR)/*.tar.gz

.PHONY: dist
dist: $(SRC_FILES) ## Create a tarball of the project
	@echo "[INFO] Creating a tarball for version $(VERSION)"
	mkdir -p $(DIST_DIR)
	tar -czvf $(DIST_DIR)/$(TARGET).tar.gz $(SRC_DIR) $(FRONTEND_DIR) $(APPS_DIR) Makefile README.md

.PHONY: compile_commands.json
compile_commands.json: $(SRC_FILES) ## Generate compile_commands.json
	@echo "[INFO] Generating compile_commands.json"
	bear -- make all

.PHONY: docs
docs: ## Generate docs using tinydocs
	tinydocs-cli \
		--files src/emu.h,src/frontend.h \
		--markers docs/tiny.markers.json \
		--ignore .gitignore \
		-o docs \
		--comment-style "//" \
		--name $(LIBRARY_NAME)

.PHONY: install
install: ## Install emulator system-wide (run with sudo)
	cp $(TARGET) $(PREFIX)/$(TARGET)

.PHONY: uninstall
uninstall: ## Uninstalls the executable (run with sudo)
	rm $(PREFIX)/$(TARGET)


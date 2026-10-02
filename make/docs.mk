MARKERS_FILE := docs/dev/tiny.markers.json
HEADER_FILES := src/gbemu.h,src/frontend.h

.PHONY: docs
docs: ## Generate docs using tinydocs
	tinydocs-cli \
		--files $(HEADER_FILES) \
		--markers $(MARKERS_FILE) \
		--ignore .gitignore \
		-o docs \
		--comment-style "//" \
		--name $(LIBRARY_NAME)

MANUAL_DIR := docs/manual
MANUAL_TEX := manual/manual.tex

.PHONY: manual
manual: ## Compile the reference manual into docs/manual/manual.pdf
	@echo "[INFO] Building the manual."
	@mkdir -p $(MANUAL_DIR)
	cd docs && \
		pdflatex -interaction=nonstopmode -halt-on-error -output-directory=manual $(MANUAL_TEX) && \
		makeindex -o manual/manual.ind manual/manual.idx && \
		pdflatex -interaction=nonstopmode -halt-on-error -output-directory=manual $(MANUAL_TEX) && \
		pdflatex -interaction=nonstopmode -halt-on-error -output-directory=manual $(MANUAL_TEX)
	@echo "[INFO] Built $(MANUAL_DIR)/manual.pdf"

.PHONY: manual.clean
manual.clean: ## Remove manual build artifacts
	@echo "[INFO] Cleaning manual build artifacts."
	@rm -rf $(MANUAL_DIR)

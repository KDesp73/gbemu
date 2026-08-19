.DEFAULT_GOAL := help

include ./make/common.mk
include ./make/libs.mk
include ./make/utils.mk
include ./make/wasm.mk

.PHONY: help
help: ## Show this help message
	@echo "Available commands:"
	@grep -h -E '^[a-zA-Z_-]+:.*?## .*$$' $(MAKEFILE_LIST) | awk 'BEGIN {FS = ":.*?## "}; {printf "\033[36m%-20s\033[0m %s\n", $$1, $$2}'

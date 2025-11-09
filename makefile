# ==================================================
# Connected Components Project, developed for:
#
# Parralel and Distributed Systems,
# Department of Electrical and Computer Engineering,
# Aristotle University of Thessaloniki.
# ==================================================

# Project Name
PROJECT := connected_components

# Compiler
CC := gcc

# Compiler flags
CFLAGS := -Wall -Wextra -Wpedantic -std=c11 -O3 -march=native
CFLAGS += -fopenmp
CFLAGS += -Isrc/core -Isrc/algorithms -Isrc/utils

# Linker flags
LDFLAGS := -fopenmp
LDLIBS  := -lmatio -lm

# Directories
SRC_DIR   := src
BUILD_DIR := build
BIN_DIR   := bin
OBJ_DIR   := $(BUILD_DIR)/obj
DEP_DIR   := $(BUILD_DIR)/deps

# Source files
CORE_SRCS := $(wildcard $(SRC_DIR)/core/*.c)
ALGO_SRCS := $(wildcard $(SRC_DIR)/algorithms/*.c)
UTILS_SRCS := $(wildcard $(SRC_DIR)/utils/*.c)
MAIN_SRC := $(SRC_DIR)/main.c
SRCS := $(CORE_SRCS) $(ALGO_SRCS) $(UTILS_SRCS) $(MAIN_SRC)

# Object files
OBJS := $(CORE_SRCS:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o) \
        $(ALGO_SRCS:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o) \
        $(UTILS_SRCS:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o) \
        $(MAIN_SRC:$(SRC_DIR)/%.c=$(OBJ_DIR)/%.o)

# Dependency files
DEPS := $(OBJS:.o=.d)

# Target executable
TARGET := $(BIN_DIR)/$(PROJECT)

# Pretty Output
ECHO := /bin/echo -e
COLOR_RESET := \033[0m
COLOR_GREEN := \033[1;32m
COLOR_YELLOW := \033[1;33m
COLOR_BLUE := \033[1;34m
COLOR_MAGENTA := \033[1;35m
COLOR_CYAN := \033[1;36m

# ============================================
# Directory creation
# ============================================

$(BIN_DIR):
	@mkdir -p $@

$(OBJ_DIR) $(OBJ_DIR)/core $(OBJ_DIR)/algorithms $(OBJ_DIR)/utils:
	@mkdir -p $@

$(DEP_DIR) $(DEP_DIR)/core $(DEP_DIR)/algorithms $(DEP_DIR)/utils:
	@mkdir -p $@

# ============================================
# Main targets
# ============================================

.PHONY: all
all: $(TARGET)

# Link executable
$(TARGET): $(OBJS) | $(BIN_DIR)
	@$(ECHO) "$(COLOR_GREEN)Linking executable:$(COLOR_RESET) $@"
	@$(CC) $(LDFLAGS) $(OBJS) $(LDLIBS) -o $@
	@$(ECHO) "$(COLOR_GREEN)✓ Build successful!$(COLOR_RESET)"

# Compile source files for each directory
$(OBJ_DIR)/core/%.o: $(SRC_DIR)/core/%.c | $(OBJ_DIR)/core $(DEP_DIR)/core
	@$(ECHO) "$(COLOR_BLUE)Compiling [core]:$(COLOR_RESET) $<"
	@$(CC) $(CFLAGS) -MMD -MP -MF $(DEP_DIR)/core/$*.d -c $< -o $@

$(OBJ_DIR)/algorithms/%.o: $(SRC_DIR)/algorithms/%.c | $(OBJ_DIR)/algorithms $(DEP_DIR)/algorithms
	@$(ECHO) "$(COLOR_BLUE)Compiling [algo]:$(COLOR_RESET) $<"
	@$(CC) $(CFLAGS) -MMD -MP -MF $(DEP_DIR)/algorithms/$*.d -c $< -o $@

$(OBJ_DIR)/utils/%.o: $(SRC_DIR)/utils/%.c | $(OBJ_DIR)/utils $(DEP_DIR)/utils
	@$(ECHO) "$(COLOR_BLUE)Compiling [utils]:$(COLOR_RESET) $<"
	@$(CC) $(CFLAGS) -MMD -MP -MF $(DEP_DIR)/utils/$*.d -c $< -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c | $(OBJ_DIR) $(DEP_DIR)
	@$(ECHO) "$(COLOR_BLUE)Compiling [main]:$(COLOR_RESET) $<"
	@$(CC) $(CFLAGS) -MMD -MP -MF $(DEP_DIR)/$*.d -c $< -o $@

# Include dependency files (for changes tracking)
-include $(DEPS)

# ============================================
# Cleaning
# ============================================

.PHONY: clean
clean:
	@$(ECHO) "$(COLOR_YELLOW)Cleaning build artifacts...$(COLOR_RESET)"
	@rm -rf $(BUILD_DIR) $(BIN_DIR)
	@$(ECHO) "$(COLOR_GREEN)✓ Clean complete$(COLOR_RESET)"

.PHONY: rebuild
   rebuild: clean all

# ============================================
# Project structure
# ============================================

.PHONY: tree
tree:
	@$(ECHO) "$(COLOR_BLUE)Project structure:$(COLOR_RESET)"
	@tree -I 'build|bin' --dirsfirst || \
		($(ECHO) "$(COLOR_YELLOW)tree command not found, using find:$(COLOR_RESET)" && \
		 find . -not -path '*/build/*' -not -path '*/bin/*' -not -path '*/.git/*' | sort)

.PHONY: list-sources
list-sources:
	@$(ECHO) "$(COLOR_BLUE)Source files:$(COLOR_RESET)"
	@$(ECHO) "$(COLOR_MAGENTA)Core:$(COLOR_RESET)"; \
	for f in $(CORE_SRCS); do echo "  $$f"; done
	@$(ECHO) "$(COLOR_MAGENTA)Algorithms:$(COLOR_RESET)"; \
	for f in $(ALGO_SRCS); do echo "  $$f"; done
	@$(ECHO) "$(COLOR_MAGENTA)Utils:$(COLOR_RESET)"; \
	for f in $(UTILS_SRCS); do echo "  $$f"; done
	@$(ECHO) "$(COLOR_MAGENTA)Main:$(COLOR_RESET)"; \
	echo "  $(MAIN_SRC)"

# ============================================
# Information and help
# ============================================

.PHONY: info
info:
	@$(ECHO) "$(COLOR_BLUE)════════════════════════════════════════$(COLOR_RESET)"
	@$(ECHO) "$(COLOR_GREEN)Build Configuration$(COLOR_RESET)"
	@$(ECHO) "$(COLOR_BLUE)════════════════════════════════════════$(COLOR_RESET)"
	@echo "  Project:    $(PROJECT)"
	@echo "  Compiler:   $(CC)"
	@echo "  CFLAGS:     $(CFLAGS)"
	@echo "  LDFLAGS:    $(LDFLAGS)"
	@echo "  LDLIBS:     $(LDLIBS)"
	@echo ""
	@$(ECHO) "$(COLOR_BLUE)Source Organization:$(COLOR_RESET)"
	@echo "  Core:       $(words $(CORE_SRCS)) files"
	@echo "  Algorithms: $(words $(ALGO_SRCS)) files"
	@echo "  Utils:      $(words $(UTILS_SRCS)) files"
	@echo "  Total:      $(words $(SRCS)) files"
	@echo ""
	@$(ECHO) "$(COLOR_BLUE)Build Artifacts:$(COLOR_RESET)"
	@echo "  Objects:    $(words $(OBJS)) files"
	@echo "  Target:     $(TARGET)"

.PHONY: check-deps
check-deps:
	@$(ECHO) "$(COLOR_BLUE)Checking dependencies...$(COLOR_RESET)"
	@which $(CC) > /dev/null || ($(ECHO) "$(COLOR_YELLOW)✗ gcc not found$(COLOR_RESET)" && exit 1)
	@$(ECHO) "  $(COLOR_GREEN)✓$(COLOR_RESET) gcc found: $(shell $(CC) --version | head -n1)"
	@gcc -pthread -E - </dev/null >/dev/null 2>&1 || ($(ECHO) "$(COLOR_YELLOW)✗ pthreads not found$(COLOR_RESET)" && exit 1)
	@$(ECHO) "  $(COLOR_GREEN)✓$(COLOR_RESET) pthreads found"
	@gcc -fopenmp -dM -E - < /dev/null | grep -i openmp > /dev/null || ($(ECHO) "$(COLOR_YELLOW)✗ openmp not found$(COLOR_RESET)" && exit 1)
	@$(ECHO) "  $(COLOR_GREEN)✓$(COLOR_RESET) openmp found"
	@echo -e '#include <cilk/cilk.h> \n int main() { cilk_spawn; return 0; }' | clang -fopencilk -xc - -o /dev/null 2> /dev/null || \
		($(ECHO) "$(COLOR_YELLOW)✗ opencilk not found$(COLOR_RESET)" && exit 1)
	@$(ECHO) "  $(COLOR_GREEN)✓$(COLOR_RESET) opencilk found"
	@pkg-config --exists matio && $(ECHO) "  $(COLOR_GREEN)✓$(COLOR_RESET) matio library found" || \
		($(ECHO) "  $(COLOR_YELLOW)✗$(COLOR_RESET) matio library not found" && exit 1)
	@which tree > /dev/null && $(ECHO) "  $(COLOR_GREEN)✓$(COLOR_RESET) tree found (optional)" || \
		$(ECHO) "  $(COLOR_YELLOW)○$(COLOR_RESET) tree not found (optional)"
	@$(ECHO) "$(COLOR_GREEN)All required dependencies found!$(COLOR_RESET)"

.PHONY: help
help:
	@$(ECHO) "$(COLOR_GREEN)════════════════════════════════════════$(COLOR_RESET)"
	@$(ECHO) "$(COLOR_GREEN)Connected Components - Available Targets$(COLOR_RESET)"
	@$(ECHO) "$(COLOR_GREEN)════════════════════════════════════════$(COLOR_RESET)"
	@echo ""
	@$(ECHO) "$(COLOR_BLUE)Building:$(COLOR_RESET)"
	@$(ECHO) "  $(COLOR_MAGENTA)all$(COLOR_RESET)           - Build the project (default)"
	@$(ECHO) "  $(COLOR_MAGENTA)clean$(COLOR_RESET)         - Remove build artifacts"
	@$(ECHO) "  $(COLOR_MAGENTA)rebuild$(COLOR_RESET)       - Build from scratch"
	@echo ""
	@$(ECHO) "$(COLOR_BLUE)Information:$(COLOR_RESET)"
	@$(ECHO) "  $(COLOR_MAGENTA)info$(COLOR_RESET)          - Show build configuration"
	@$(ECHO) "  $(COLOR_MAGENTA)tree$(COLOR_RESET)          - Show project structure"
	@$(ECHO) "  $(COLOR_MAGENTA)list-sources$(COLOR_RESET)  - List all source files by category"
	@$(ECHO) "  $(COLOR_MAGENTA)check-deps$(COLOR_RESET)    - Verify dependencies are installed"
	@$(ECHO) "  $(COLOR_MAGENTA)help$(COLOR_RESET)          - Show this message"
	@echo ""

# Default target when no arguments given
.DEFAULT_GOAL := all

# ============================================
# Phony targets (prevent conflicts with files)
# ============================================

.PHONY: all clean rebuild tree list-sources info check-deps help
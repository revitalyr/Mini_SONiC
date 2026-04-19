CC = gcc
CFLAGS = -O2 -Wall -Wextra -pthread -Iinclude -g
LDFLAGS = -pthread

SRCDIR = src
INCDIR = include
OBJDIR = obj
BINDIR = bin

SOUSRCS = $(wildcard src/*.c)
# Exclude STM32 specific files and dashboard files from Linux build
EXCLUDE_SRCS = src/main_stm32.c src/switch_hal.c src/switch_core.c src/switch_tasks.c src/main_dashboard.c src/main_simple.c src/simple_dashboard.c src/dashboard.c
SRCS := $(filter-out $(EXCLUDE_SRCS), $(SOUSRCS))
OBJECTS = $(SRCS:src/%.c=$(OBJDIR)/%.o)
TARGET = $(BINDIR)/mini_switch
DASHBOARD_TARGET = $(BINDIR)/mini_switch_dashboard
SIMPLE_TARGET = $(BINDIR)/mini_switch_simple

.PHONY: all clean debug demo install test

all: $(TARGET)

dashboard: $(DASHBOARD_TARGET)

simple: $(SIMPLE_TARGET)

$(TARGET): $(OBJECTS) | $(BINDIR)
	$(CC) $(OBJECTS) -o $@ $(LDFLAGS)
	@echo "Built: $@"

# Simple dashboard version with ncurses
$(SIMPLE_TARGET): src/main_simple.c src/simple_dashboard.c $(filter-out src/main.c,$(OBJECTS)) | $(BINDIR)
	$(CC) $(CFLAGS) -lncurses src/main_simple.c src/simple_dashboard.c $(filter-out src/main.c,$(OBJECTS)) -o $@ $(LDFLAGS)
	@echo "Built: $@"

# Dashboard version with ncurses
$(DASHBOARD_TARGET): src/main_dashboard.c src/dashboard.c src/visual.c $(filter-out src/main.c,$(OBJECTS)) | $(BINDIR)
	$(CC) $(CFLAGS) -lncurses src/main_dashboard.c src/dashboard.c src/visual.c $(filter-out src/main.c,$(OBJECTS:.o=)) -o $@ $(LDFLAGS)
	@echo "Built: $@"

$(OBJDIR)/%.o: $(SRCDIR)/%.c | $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR):
	mkdir -p $(OBJDIR)

$(BINDIR):
	mkdir -p $(BINDIR)

debug: CFLAGS += -DDEBUG -O0
debug: $(TARGET)

clean:
	rm -rf $(OBJDIR) $(BINDIR)
	@echo "Clean complete"

demo:
	@echo "Setting up demo topology..."
	sudo bash scripts/demo.sh
	@echo "Demo complete"

demo-clean:
	@echo "Setting up clean demo (no CLI)..."
	sudo bash scripts/demo_no_cli.sh
	@echo "Clean demo complete"

demo-simple:
	@echo "Setting up demo with simple dashboard..."
	sudo bash scripts/demo_simple.sh
	@echo "Simple dashboard demo complete"

demo-debug:
	@echo "Setting up debug demo with diagnostics..."
	sudo bash scripts/demo_debug.sh
	@echo "Debug demo complete"

test-demo:
	@echo "Running quick demo test..."
	bash scripts/test_demo.sh
	@echo "Test complete"

install: $(TARGET)
	sudo cp $(TARGET) /usr/local/bin/
	@echo "Installed to /usr/local/bin/mini_switch"

test:
	@echo "Running basic tests..."
	@if [ ! -f $(TARGET) ]; then $(MAKE) all; fi
	@echo "Tests complete"

help:
	@echo "Available targets:"
	@echo "  all            - Build the mini_switch"
	@echo "  dashboard      - Build mini_switch with full dashboard"
	@echo "  simple         - Build mini_switch with simple dashboard"
	@echo "  debug          - Build with debug symbols"
	@echo "  clean          - Remove build artifacts"
	@echo "  demo           - Setup network namespace demo (with CLI)"
	@echo "  demo-clean     - Setup clean demo (no CLI, cleaner output)"
	@echo "  demo-simple    - Setup demo with simple real-time dashboard"
	@echo "  demo-debug     - Setup debug demo with diagnostics"
	@echo "  test-demo      - Quick demo test (no sudo required for setup)"
	@echo "  install        - Install to /usr/local/bin"
	@echo "  test           - Run basic tests"
	@echo "  help           - Show this help"

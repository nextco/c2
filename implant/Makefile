CARGO ?= cargo
PROFILE ?= release

LINUX := x86_64-unknown-linux-musl
WINDOWS := x86_64-pc-windows-gnu

LINUX_BIN := target/$(LINUX)/$(PROFILE)/pika
WINDOWS_BIN := target/$(WINDOWS)/$(PROFILE)/pika.exe

ifneq ($(shell command -v cross 2>/dev/null),)
USE_CROSS := 1
CROSS := cross
else
USE_CROSS := 0
CROSS := $(CARGO)
endif

ifeq ($(USE_CROSS),1)
BUILD_LINUX = RUSTFLAGS="-C target-feature=+crt-static" $(CROSS) build --target $(LINUX) --$(PROFILE)
BUILD_WINDOWS = RUSTFLAGS="-C target-feature=+crt-static" $(CROSS) build --target $(WINDOWS) --$(PROFILE)
else
BUILD_LINUX = RUSTFLAGS="-C target-feature=+crt-static" $(CARGO) build --target $(LINUX) --$(PROFILE)
BUILD_WINDOWS = RUSTFLAGS="-C target-feature=+crt-static" $(CARGO) build --target $(WINDOWS) --$(PROFILE)
endif

.PHONY: all linux windows clean help install-targets

all: linux windows

linux: install-targets
	$(BUILD_LINUX)
	@cp $(LINUX_BIN) ./pika
	@file ./pika
	@echo "$(LINUX_BIN) -> ./pika"

windows: install-targets
	$(BUILD_WINDOWS)
	@cp $(WINDOWS_BIN) ./pika.exe
	@file ./pika.exe
	@echo "$(WINDOWS_BIN) -> ./pika.exe"

install-targets:
	rustup target add $(LINUX) $(WINDOWS)

clean:
	$(CARGO) clean
	$(RM) ./pika ./pika.exe

help:
	@echo "make linux | make windows | make all | make clean"
	@echo "cross detected: $(USE_CROSS)"

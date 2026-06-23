CARGO ?= cargo
PROFILE ?= release

LINUX := x86_64-unknown-linux-gnu
WINDOWS := x86_64-pc-windows-gnu

LINUX_BIN := target/$(LINUX)/$(PROFILE)/pika
WINDOWS_BIN := target/$(WINDOWS)/$(PROFILE)/pika.exe

ifneq ($(shell command -v cross 2>/dev/null),)
USE_CROSS := 1
CROSS := cross
else
USE_CROSS := 0
CROSS := cargo
endif

ifeq ($(USE_CROSS),1)
BUILD = $(CROSS) build --target $(1) --$(PROFILE)
else
BUILD = $(CARGO) build --target $(1) --$(PROFILE)
endif

.PHONY: all linux windows clean help install-targets

all: linux windows

linux: install-targets
	$(call BUILD,$(LINUX))
	@cp $(LINUX_BIN) ./pika
	@echo $(LINUX_BIN) -> ./pika

windows: install-targets
	$(call BUILD,$(WINDOWS))
	@cp $(WINDOWS_BIN) ./pika.exe
	@file ./pika.exe
	@echo $(WINDOWS_BIN) -> ./pika.exe

install-targets:
	rustup target add $(LINUX) $(WINDOWS)

clean:
	$(CARGO) clean
	$(RM) ./pika ./pika.exe

help:
	@echo make linux | make windows | make all | make clean
	@echo cross detected: $(USE_CROSS)

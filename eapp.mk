# eapp.mk - build rules for an iPod nano 3G eApp (click-wheel game).
#
# A game's Makefile sets a few variables and includes this file:
#
#   EAPP_SDK # path to this repository
#   NAME     # Games menu title
#   FOLDER   # folder under iPod_Control/games_RO/, unique per game
#   SRCS     # C sources, relative to the game's directory
#   include $(EAPP_SDK)/eapp.mk
#
# Optional: IMPORTS (OS modules, default "OpenGLES InputEvents"; see tools/modules.py),
# GUID (default FOLDER), VERSION, HEAP_KB, EXE, EXTRA_CFLAGS, B (build dir).
#
# Targets:
#   make                               build
#   make install IPOD=<ipod root dir>  copy the game folder to the iPod (disk mode)
#   make clean
#
# The iPod must run an OS with the manifest signature check patched out (see the main
# README); stock firmware refuses unsigned games.
#
# mostly written by an LLM because I hate writing Makefiles. just look at that syntax!

EAPP_SDK ?= $(patsubst %/,%,$(dir $(lastword $(MAKEFILE_LIST))))

# ---- game settings ------------------------------------------------------------
ifeq ($(strip $(NAME)),)
$(error set NAME (the Games menu title) before including eapp.mk)
endif
ifeq ($(strip $(FOLDER)),)
$(error set FOLDER (the game's folder name under games_RO) before including eapp.mk)
endif
ifeq ($(strip $(SRCS)),)
$(error set SRCS (your C sources) before including eapp.mk)
endif
FOLDER   := $(strip $(FOLDER))
GUID     ?= $(FOLDER)
GUID     := $(strip $(GUID))
VERSION  ?= 1.0
HEAP_KB  ?= 1024
EXE      ?= Game.bin
IMPORTS  ?= OpenGLES InputEvents

# ---- toolchain ------------------------------------------------------------------
# GNU ARM toolchain: arm-none-eabi-gcc/-ld/-objcopy (binutils-arm-none-eabi +
# gcc-arm-none-eabi on Linux, `brew install arm-none-eabi-gcc` on macOS). Override
# CC/LD/OBJCOPY/LIBGCC directly if yours live outside PATH or are named differently.
ifneq ($(filter default undefined,$(origin CC)),)
CC       := arm-none-eabi-gcc
endif
ifneq ($(filter default undefined,$(origin LD)),)
LD       := arm-none-eabi-ld
endif
OBJCOPY  ?= arm-none-eabi-objcopy
PYTHON   ?= python3
ifeq ($(strip $(shell command -v $(CC) 2>/dev/null)),)
$(error compiler '$(CC)' not found: install gcc-arm-none-eabi, or pass CC=...)
endif
ifeq ($(strip $(shell command -v $(LD) 2>/dev/null)),)
$(error linker '$(LD)' not found: install binutils-arm-none-eabi, or pass LD=...)
endif
# libgcc supplies integer division, 64-bit and soft-float helpers (__aeabi_*).
# Override with LIBGCC=/path/to/libgcc.a.
LIBGCC   ?= $(shell $(CC) -marm -march=armv5te -mfloat-abi=soft \
                          -print-libgcc-file-name 2>/dev/null)

TARGET   := -marm -march=armv5te -mfloat-abi=soft
CFLAGS   := $(TARGET) -Os -std=c11 -ffreestanding -fno-builtin -fno-common \
            -fno-exceptions -fno-unwind-tables -fno-asynchronous-unwind-tables \
            -ffunction-sections -fdata-sections -Wall -Wextra \
            -I$(EAPP_SDK)/sdk/include $(EXTRA_CFLAGS)
ASFLAGS  := $(TARGET)
LDFLAGS  := -T $(EAPP_SDK)/sdk/eapp.ld --gc-sections -nostdlib -static

# ---- outputs --------------------------------------------------------------------
B        ?= build/$(FOLDER)
PKG      := $(B)/$(FOLDER)
SDKOBJS  := $(B)/sdk/crt0.o $(B)/sdk/imports.o $(B)/sdk/libc.o
OBJS     := $(SDKOBJS) $(patsubst %.c,$(B)/obj/%.o,$(SRCS))
TOOLS    := $(EAPP_SDK)/tools

.PHONY: all install clean
all: $(PKG)/Manifest.plist

$(B)/sdk/imports.S: $(TOOLS)/mkimports.py $(TOOLS)/modules.py $(MAKEFILE_LIST)
	@mkdir -p $(dir $@)
	$(PYTHON) $(TOOLS)/mkimports.py $@ $(strip $(IMPORTS))

$(B)/obj/%.o: %.c $(wildcard $(EAPP_SDK)/sdk/include/*.h) $(MAKEFILE_LIST)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(B)/sdk/libc.o: $(EAPP_SDK)/sdk/libc.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(B)/sdk/crt0.o: $(EAPP_SDK)/sdk/crt0.S
	@mkdir -p $(dir $@)
	$(CC) $(ASFLAGS) -c $< -o $@

$(B)/sdk/imports.o: $(B)/sdk/imports.S
	$(CC) $(ASFLAGS) -c $< -o $@

$(B)/game.elf: $(OBJS) $(EAPP_SDK)/sdk/eapp.ld
	$(LD) $(LDFLAGS) -Map $(B)/game.map $(OBJS) $(LIBGCC) -o $@

$(PKG)/$(EXE): $(B)/game.elf
	@mkdir -p $(PKG)
	$(OBJCOPY) -O binary $< $@

$(PKG)/Manifest.plist: $(PKG)/$(EXE) $(TOOLS)/mkmanifest.py $(MAKEFILE_LIST)
	$(PYTHON) $(TOOLS)/mkmanifest.py --out $@ --exe $< --name "$(NAME)" --guid "$(GUID)" \
	    --version "$(VERSION)" --heap-kb $(HEAP_KB)

install: all
	@test -n "$(IPOD)" || (echo "usage: make install IPOD=/Volumes/<iPod>"; exit 1)
	@test -d "$(IPOD)/iPod_Control" || (echo "$(IPOD) is not an iPod in disk mode"; exit 1)
	@d="$(IPOD)/iPod_Control/games_RO"; \
	 for c in "$(IPOD)"/iPod_Control/[Gg]ames_RO; do [ -d "$$c" ] && d="$$c"; done; \
	 mkdir -p "$$d/$(FOLDER)" && \
	 cp $(PKG)/Manifest.plist $(PKG)/$(EXE) "$$d/$(FOLDER)/" && sync && \
	 echo "installed to $$d/$(FOLDER)"

clean:
	rm -rf $(B)

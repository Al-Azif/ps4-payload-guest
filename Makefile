# Package metadata
TITLE         := Payload Guest
VERSION       := 0.97
TITLE_ID      := AZIF00003
CONTENT_ID    := IV0000-AZIF00003_00-PAYLOADGUEST0000
AUTHINFO      := 000000000000000000000000001C004000FF000000000080000000000000000000000000000000000000008000400040000000000000008000000000000000080040FFFF000000F000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000000

# Libraries linked into the ELF
LIBS          := -lc -lkernel -lc++ -lSceVideoOut -lSceGnmDriver -lSceSysmodule -lSceFreeType -lSceHttp -lScePad -lSceUserService -lSceSystemService -lSceNet -lSceHttp -lSceSsl -lSceBgft -lSceAppInstUtil -lSceImeDialog
LIBS					+= -Llibs -lLog -ljbc

# Directorys to include
INCLUDES      := -Iinclude -Iexternal

# Additional compile flags
#   ERRORFLAGS and OTHERFLAGS will be included in C and C++ flags
#   OTHERCXXFLAGS will only be included in C++ flags
ERRORFLAGS    := -Wall -Wextra -Wpedantic # -Werror
OTHERFLAGS    := -std=c99 -O3 -D_DEFAULT_SOURCE
OTHERCXXFLAGS := -std=c++14

# PKG Asset directories
ASSETS        := $(shell find assets -type f -name '*.*')
SCEMODULES    := $(shell find sce_module -type f -name '*.*')
SCESYS        := $(shell find sce_sys -type f -name '*.*')

# -----------------------------------------------------------------------------
# Do not edit below this line unless you know what you are doing
# -----------------------------------------------------------------------------

TOOLCHAIN     := $(OO_PS4_TOOLCHAIN)
ODIR          := build
SDIR          := src
EXTRAFLAGS    := $(INCLUDES) $(ERRORFLAGS) $(OTHERFLAGS)
CFLAGS        := --target=x86_64-pc-freebsd12-elf -fPIC -funwind-tables -c -isysroot $(TOOLCHAIN) -isystem $(TOOLCHAIN)/include $(EXTRAFLAGS)
CXXFLAGS      := $(CFLAGS) -isystem $(TOOLCHAIN)/include/c++/v1 $(OTHERCXXFLAGS)
LDFLAGS       := -m elf_x86_64 -pie --script $(TOOLCHAIN)/link.x --eh-frame-hdr -L$(TOOLCHAIN)/lib $(LIBS) $(TOOLCHAIN)/lib/crt1.o

CFILES        := $(wildcard $(SDIR)/*.c)
CFILES        := $(filter-out $(SDIR)/ChainLauncher.c,$(CFILES))
CPPFILES      := $(wildcard $(SDIR)/*.cpp)
OBJS          := $(patsubst $(SDIR)/%.c, $(ODIR)/%.o, $(CFILES)) $(patsubst $(SDIR)/%.cpp, $(ODIR)/%.o, $(CPPFILES))
OBJS          := $(filter-out $(ODIR)/eboot.o,$(OBJS))

UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Linux)
		CC        := clang
		CXX       := clang++
		LD        := ld.lld
		CDIR      := linux
endif
ifeq ($(UNAME_S),Darwin)
		CC        := /usr/local/opt/llvm/bin/clang
		CXX       := /usr/local/opt/llvm/bin/clang++
		LD        := /usr/local/opt/llvm/bin/ld.lld
		CDIR      := macos
endif

.PHONY: all pkg.gp4 sce_sys/param.sfo eboot.bin guest.elf clean
.DEFAULT := all

all: $(CONTENT_ID).pkg

$(CONTENT_ID).pkg: pkg.gp4
	$(TOOLCHAIN)/bin/$(CDIR)/PkgTool.Core pkg_build $< .

pkg.gp4: eboot.bin guest.elf sce_sys/param.sfo $(SCESYS) $(SCEMODULES) $(ASSETS)
	$(TOOLCHAIN)/bin/$(CDIR)/create-gp4 -out $@ --content-id=$(CONTENT_ID) --files "$^"

sce_sys/param.sfo: Makefile
	$(TOOLCHAIN)/bin/$(CDIR)/PkgTool.Core sfo_new $@
	$(TOOLCHAIN)/bin/$(CDIR)/PkgTool.Core sfo_setentry $@ APP_TYPE --type Integer --maxsize 4 --value 1
	$(TOOLCHAIN)/bin/$(CDIR)/PkgTool.Core sfo_setentry $@ APP_VER --type Utf8 --maxsize 8 --value '$(VERSION)'
	$(TOOLCHAIN)/bin/$(CDIR)/PkgTool.Core sfo_setentry $@ ATTRIBUTE --type Integer --maxsize 4 --value 0
	$(TOOLCHAIN)/bin/$(CDIR)/PkgTool.Core sfo_setentry $@ CATEGORY --type Utf8 --maxsize 4 --value 'gde'
	$(TOOLCHAIN)/bin/$(CDIR)/PkgTool.Core sfo_setentry $@ CONTENT_ID --type Utf8 --maxsize 48 --value '$(CONTENT_ID)'
	$(TOOLCHAIN)/bin/$(CDIR)/PkgTool.Core sfo_setentry $@ DOWNLOAD_DATA_SIZE --type Integer --maxsize 4 --value 0
	$(TOOLCHAIN)/bin/$(CDIR)/PkgTool.Core sfo_setentry $@ SYSTEM_VER --type Integer --maxsize 4 --value 0
	$(TOOLCHAIN)/bin/$(CDIR)/PkgTool.Core sfo_setentry $@ TITLE --type Utf8 --maxsize 128 --value '$(TITLE)'
	$(TOOLCHAIN)/bin/$(CDIR)/PkgTool.Core sfo_setentry $@ TITLE_ID --type Utf8 --maxsize 12 --value '$(TITLE_ID)'
	$(TOOLCHAIN)/bin/$(CDIR)/PkgTool.Core sfo_setentry $@ VERSION --type Utf8 --maxsize 8 --value '$(VERSION)'

eboot.bin: guest.elf
	$(CC) -cc1 -triple x86_64-pc-freebsd-elf -munwind-tables -fuse-init-array -emit-obj -isysroot $(TOOLCHAIN) -isystem $(TOOLCHAIN)/include -std=c99 -O3 -Wall -Wextra -Wpedantic -Werror -o $(ODIR)/eboot.o $(SDIR)/ChainLauncher.c
	$(LD) $(ODIR)/eboot.o -o $(ODIR)/eboot.elf -m elf_x86_64 -pie --script $(TOOLCHAIN)/link.x --eh-frame-hdr -L$(TOOLCHAIN)/lib -lc -lkernel -lSceSystemService $(TOOLCHAIN)/lib/crt1.o
	$(TOOLCHAIN)/bin/$(CDIR)/create-fself -in=$(ODIR)/eboot.elf -out=$(ODIR)/eboot.oelf --eboot "eboot.bin" --paid 0x3800000000000011

guest.elf: $(ODIR) $(OBJS)
	$(LD) $(OBJS) -o $(ODIR)/guest.elf $(LDFLAGS)
	$(TOOLCHAIN)/bin/$(CDIR)/create-fself -in=$(ODIR)/guest.elf -out=$(ODIR)/guest.oelf --eboot "guest.elf" --paid 0x3800000000000011

$(ODIR)/%.o: $(SDIR)/%.c
	$(CC) $(CFLAGS) -o $@ $<

$(ODIR)/%.o: $(SDIR)/%.cpp
	$(CXX) $(CXXFLAGS) -o $@ $<

$(ODIR):
	@mkdir -p $@

clean:
	rm -rf *.pkg pkg.gp4 sce_sys/param.sfo eboot.bin guest.elf $(ODIR)

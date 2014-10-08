TARGETS = d68k bdb blsgen

INSTALL_DIR ?= $(HOME)/bin
INSTALL_TARGETS = $(TARGETS) d68ktest

all: $(TARGETS)
clean:
	rm -f *.o $(TARGETS)

# default globals
CFLAGS ?= -std=gnu99 -O0 -g3 -pedantic -Wall -Wextra
CC = gcc
LD = gcc

BLAST_AS ?= m68k-elf-as
BLAST_CC ?= m68k-elf-gcc
BLAST_CXX ?= m68k-elf-g++
BLAST_CFLAGS ?= -Os -pipe
BLAST_LD ?= m68k-elf-ld
BLAST_LDFLAGS ?=
BLAST_OBJCOPY ?= m68k-elf-objcopy
BLAST_NM ?= m68k-elf-nm
BLAST_ASMX ?= asmx
BLSPREFIX ?= $(shell dirname $$(dirname $$(which $(BLAST_CC) ) ) )

# Per-source rules
d68k: d68k.o d68k_mod.o blsbinparse.o blsfile.o blsparse.o blsaddress.o
bdb: bdb.o bdp.o d68k_mod.o blsbinparse.o blsparse.o blsfile.o blsaddress.o
blsgen: blsgen.o blsgen_gcc.o blsgen_png.o blsgen_img.o blsgen_asmx.o blsconf.o mdconf.o blsaddress.o blspack.o blswrite.o blsparse.o blsfile.o
d68ktest:


# Per-source flags
CFLAGS += -DBLSPREFIX="\"$(BLSPREFIX)\""
CFLAGS_blsgen_png = $(shell libpng-config --cflags)
LDFLAGS_blsgen = $(shell libpng-config --ldflags)
LDFLAGS_bdb = -lreadline

# Special targets
bdb.o: bdb.c blsload_sim.bin.c *.h

blsload_sim.bin.c: blsload_sim.bin
	bin2c blsload_sim.bin

blsload_sim.bin: ../../src/blsload_sim.asm
	asmx -b 0xA04 -C 68000 -o "$@" "$^" -I ../../include -i bls.inc -d BUS:=2 -d CHIP:=9 -d TARGET:=2
	#asmx -b 0xA04 -C 68000 -w -e -l /dev/stdout -o "$@" "$^" -I ../../include -i bls.inc -d BUS:=2 -d CHIP:=9 -d TARGET:=2


install: $(TARGETS)
	for f in $(INSTALL_TARGETS); do cp "$$f" "$(INSTALL_DIR)"; done

uninstall:
	for f in $(INSTALL_TARGETS); do rm -f "$(INSTALL_DIR)/$$f"; done

%.o: %.c *.h
	$(CC) $(CFLAGS) $(CFLAGS_$*) -o $@ -c $<

%: %.o
	$(LD) $(LDFLAGS) $(LDFLAGS_$*) -o $@ $^ $(LDFLAGS_$*) $(LDFLAGS)


# vim: ts=8 sw=8 sts=8 noet

.PHONY: clean test all

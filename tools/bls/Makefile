TARGETS = d68k bdb scdupload blsgen

INSTALL_DIR ?= $(HOME)/bin
INSTALL_TARGETS = $(TARGETS) d68ktest

all: $(TARGETS)
clean:
	rm -f *.o $(TARGETS)

# default globals
CFLAGS ?= -std=gnu99 -O0 -g3 -pedantic -Wall -Wextra
CC = gcc
LD = gcc

INCDIR ?= $(BLSPREFIX)/share/blast/inc
ASMDIR ?= $(BLSPREFIX)/share/blast/asm
INCLUDEDIR ?= $(BLSPREFIX)/share/blast/include
SRCDIR ?= $(BLSPREFIX)/share/blast/src
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
blsgen: blsgen.o blsgen_gcc.o blsgen_png.o blsgen_img.o blsconf.o mdconf.o blsaddress.o blspack.o
blsbuild: blsbuild.o address.o compile.o config.o output.o source.o utils.o img.o
bdb: bdb.o bdp.o d68k_mod.o symtable.o utils.o binparse.o
d68k: d68k.o d68k_mod.o symtable.o utils.o
scdupload: scdupload.o bdp.o utils.o
d68ktest:


# Per-source flags
CFLAGS += -DBLSPREFIX="\"$(BLSPREFIX)\""
CFLAGS_img = $(shell libpng-config --cflags)
CFLAGS_blsbuild = -DINCDIR="\"$(INCDIR)\"" -DINCLUDEDIR="\"$(INCLUDEDIR)\"" -DSRCDIR="\"$(SCDIR)\"" -DBLAST_AS="\"$(BLAST_AS)\"" -DBLAST_CC="\"$(BLAST_CC)\"" -DBLAST_CXX="\"$(BLAST_CXX)\"" -DBLAST_CFLAGS="\"$(BLAST_CFLAGS)\"" -DBLAST_LD="\"$(BLAST_LD)"\" -DBLAST_LDFLAGS="\"$(BLAST_LDFLAGS)\"" -DBLAST_NM="\"$(BLAST_NM)\"" -DBLAST_OBJCOPY="\"$(BLAST_OBJCOPY)\"" -DBLAST_ASMX="\"$(BLAST_ASMX)\""
CFLAGS_config = $(CFLAGS_blsbuild)
CFLAGS_compile = $(CFLAGS_blsbuild)
LDFLAGS_blsgen = -lpng
LDFLAGS_blsbuild = $(shell libpng-config --ldflags)
LDFLAGS_bdb = -lreadline


install: $(TARGETS)
	for f in $(INSTALL_TARGETS); do cp "$$f" "$(INSTALL_DIR)"; done

uninstall:
	for f in $(INSTALL_TARGETS); do rm -f "$(INSTALL_DIR)/$$f"; done

%.o: %.c *.h
	$(CC) $(CFLAGS) $(CFLAGS_$*) -o $@ -c $<

%: %.o
	$(LD) $(LDFLAGS) $(LDFLAGS_$*) -o $@ $^ $(LDFLAGS_$*) $(LDFLAGS)


.PHONY: clean test all

# Todo : compile library / install to BLSPREFIX

CC ?= m68k-elf-gcc
LD ?= m68k-elf-ld
AS ?= m68k-elf-as
AR ?= m68k-elf-ar
CFLAGS += -I ../include -fomit-frame-pointer -m68000 -Os -fno-builtin-memset -fno-builtin-memcpy

ifneq ($(wildcard ../sgdk),)
CFLAGS += -I ../sgdk/include
LIBS += ../sgdk/lib
endif

all: libbls.a

install: libbls.a
	install -D -m 0644 libbls.a "$BLSPREFIX/share/blastsdk/lib/libbls.a"

clean:
	rm *.o libbls.a

uninstall:
	rm "$BLSPREFIX/share/blastsdk/lib/libbls.a"


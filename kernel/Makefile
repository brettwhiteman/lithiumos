PROJDIRS := .
SRCFILES := $(shell find $(PROJDIRS) -type f -name '*.c')
HDRFILES := $(shell find $(PROJDIRS) -type f -name '*.h')
OBJFILES := $(patsubst %.c,%.o,$(SRCFILES))
DEPFILES := $(patsubst %.c,%.d,$(SRCFILES))
WARNINGS := -Wall -Wextra -pedantic -Wshadow -Wpointer-arith -Wcast-align \
            -Wwrite-strings -Wmissing-prototypes -Wmissing-declarations \
            -Wredundant-decls -Wnested-externs -Winline -Wno-long-long \
            -Wuninitialized -Wconversion -Wstrict-prototypes -Werror-implicit-function-declaration
CFLAGS := -std=c99 $(WARNINGS) -Iinclude -ffreestanding -nostartfiles -nostdlib
CC := i586-elf-gcc
ASMFILES := $(shell find $(PROJDIRS) -type f -name '*.asm' | grep -v loader.asm)
ASMOBJ := $(patsubst %.asm,%.asmo,$(ASMFILES))
ASM := nasm

all: KERNEL.BIN

-include $(DEPFILES)

KERNEL.BIN: $(OBJFILES) $(ASMOBJ) loader.asmo linker.ld
	@$(CC) $(CFLAGS) -T linker.ld -o KERNEL.BIN $(OBJFILES) $(ASMOBJ)

%.asmo: %.asm Makefile
	$(ASM) -f elf -o $@ $<

%.o: %.c Makefile
	@$(CC) $(CFLAGS) -MD -MP -c $< -o $@

clean:
	-@rm -f $(wildcard $(OBJFILES) $(ASMOBJ) $(DEPFILES) loader.asmo)
	@echo Cleaned

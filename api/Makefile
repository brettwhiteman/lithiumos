PROJDIRS := .
SRCFILES := $(shell find $(PROJDIRS) -type f -name '*.c')
OBJFILES := $(patsubst %.c,%.o,$(SRCFILES))
DEPFILES := $(patsubst %.c,%.d,$(SRCFILES))
WARNINGS := -Wall -Wextra -pedantic -Wshadow -Wpointer-arith -Wcast-align \
            -Wwrite-strings -Wmissing-prototypes -Wmissing-declarations \
            -Wredundant-decls -Wnested-externs -Winline -Wno-long-long \
            -Wuninitialized -Wconversion -Wstrict-prototypes -Werror-implicit-function-declaration
CFLAGS := -std=c99 $(WARNINGS) -Iinclude -ffreestanding -nostartfiles -nostdlib
CC := i586-elf-gcc
LD := i586-elf-ld

all: lilibc

-include $(DEPFILES)

lilibc: $(OBJFILES)
	@$(LD) -r $(OBJFILES) -o lilibc

%.o: %.c Makefile
	@$(CC) $(CFLAGS) -MD -MP -c $< -o $@

clean:
	-@rm -f $(wildcard $(OBJFILES) $(DEPFILES) lilibc)
	@echo Cleaned


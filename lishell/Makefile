PROJDIRS := .
SRCFILES := $(shell find $(PROJDIRS) -type f -name '*.c')
OBJFILES := $(patsubst %.c,%.o,$(SRCFILES))
DEPFILES := $(patsubst %.c,%.d,$(SRCFILES))
WARNINGS := -Wall -Wextra -pedantic -Wshadow -Wpointer-arith -Wcast-align \
            -Wwrite-strings -Wmissing-prototypes -Wmissing-declarations \
            -Wredundant-decls -Wnested-externs -Winline -Wno-long-long \
            -Wuninitialized -Wconversion -Wstrict-prototypes -Werror-implicit-function-declaration
CFLAGS := -std=c99 $(WARNINGS) -I../api/include -ffreestanding -nostartfiles -nostdlib
CC := i586-elf-gcc

all: lishell

-include $(DEPFILES)

lishell: $(OBJFILES) ../api/lilibc
	@$(CC) $(CFLAGS) -T linker.ld -o lishell $(OBJFILES) ../api/lilibc

%.o: %.c Makefile
	@$(CC) $(CFLAGS) -MD -MP -c $< -o $@

clean:
	-@rm -f $(wildcard $(OBJFILES) $(DEPFILES) lishell)
	@echo Cleaned


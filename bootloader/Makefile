all: bootloader.bin STAGE2.BIN

bootloader.bin: bootloader.asm
	@nasm -f bin -o bootloader.bin bootloader.asm

STAGE2.BIN: stage2.asm
	@nasm -f bin -o STAGE2.BIN stage2.asm

clean:
	-@rm -f bootloader.bin STAGE2.BIN
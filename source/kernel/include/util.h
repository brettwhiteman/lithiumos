#ifndef UTIL_H
#define UTIL_H

#include <types.h>

void* memset(byte* dest, byte val, uint32_t count);
void* memsetd(uint32_t* dest, uint32_t val, uint32_t count);
byte inportb (uint16_t port);
void outportb(uint16_t port, byte data);
void itoa(uint32_t number, char* buf, uint32_t base);
uint16_t inportw(uint16_t port);
void disable_interrupts(void);
void enable_interrupts(void);
void halt_cpu(void);
uint32_t strcmp(char* string1, char* string2);
void memcpy(void *dest, const void *source, size_t num);

#endif

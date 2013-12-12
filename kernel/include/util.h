#ifndef UTIL_H
#define UTIL_H

#include <types.h>

void* memset(uint8_t* dest, uint8_t val, size_t count);
void* memsetd(uint32_t* dest, uint32_t val, size_t count);
uint8_t inportb (uint16_t port);
void outportb(uint16_t port, uint8_t data);
void itoa(uint32_t number, char* buf, uint32_t base);
uint16_t inportw(uint16_t port);
void disable_interrupts(void);
void enable_interrupts(void);
void halt_cpu(void);
uint32_t strcmp(char* string1, char* string2);
void memcpy(void *dest, const void *source, size_t num);

#endif

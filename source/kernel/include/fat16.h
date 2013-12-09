#ifndef FAT16_H
#define FAT16_H

#include <stdinc.h>

bool fat16_read_file(char* filepath, void* buffer);
bool fat16_read_sectors();

#endif

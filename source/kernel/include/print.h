#ifndef PRINT_H
#define PRINT_H

#include "stdinc.h"

#define SCREEN_COLS 80
#define SCREEN_ROWS 25

void print_char(char c);
void print_string(char *s);
void print_string_at(char *s, uint32_t x, uint32_t y);
void clear_screen(void);
void set_colour(byte c);
void update_cursor_pos(void);
void set_vid_mem(void *vmem);
byte get_colour(void);
void hide_cursor(void);

#endif

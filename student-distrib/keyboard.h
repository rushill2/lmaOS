#ifndef _KEYBOARD_H_
#define _KEYBOARD_H_

#include "lib.h"
#include "types.h"
#include "i8259.h"
#ifndef ASM
#define KEYBOARD_IRQ 1
#define MAP_LEN     0x84
#define KB_BUF_SIZE 128
#define KB_PORT     0x60
#define KB_CMD      0x64
#define NUM_COLS    80
#define NUM_ROWS    25
#define VIDEO       0xB8000
#define ATTRIB      0x7

/* Init the KB */
void keyboard_init ();
/* KB interrupt */
void kb_intr ();

/* CP2 key mapping */
void get_kb_buffer(char* buf);

void set_kb_buffer(char* buf);

void check_enter_pressed(uint8_t* flag);

void set_stop_x(uint8_t current_screen_x);

void reset_keyboard();

uint8_t get_kb_index();

void set_kb_index(uint8_t index);


#endif
#endif


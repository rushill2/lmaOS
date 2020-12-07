#ifndef TERMINAL_H
#define TERMINAL_H

#include "types.h"
#include "keyboard.h"
#include "paging.h"
#include "syscall.h"

#define KB4     0x1000
#define MB64    0x4000000
#define NUM_TERM 3
#ifndef	ASM
typedef struct terminal_t {
	int8_t processes[4];	// max 4 proc on one terminal
	pcb_t* term_proc;	// current process running in this terminal
	int num_proc;		// number of processes currently active in this terminal
	int32_t screen_x;					//x pos of cursor
	int32_t screen_y;					//y pos of cursor
	uint8_t first_run;					//is this the first time this terminal is being opened? 1 if so, 0 if not
	//uint8_t screen_buffer[KB4];		//stores the terminal's display for when we switch back to it
	char kb_buffer[KB_BUF_SIZE];				//stores the keyboard buffer for this terminal
	uint8_t kb_buffer_index;			//the current index of this terminal's keyboard buffer
	//uint32_t esp;						// terminal stack pointer
	//uint32_t cr3;						// terminal cr3
	//uint32_t kesp;						// kernel esp
} terminal_t;

terminal_t terminals[3];
void terminal_init();
void switch_terminal(uint8_t tid);
uint8_t get_current_terminal();
int32_t terminal_read(int32_t fd, void* buf, int32_t nbytes);
int32_t terminal_write(int32_t fd, const void* buf, int32_t nbytes);
int32_t terminal_open(const uint8_t* filename);
int32_t terminal_close(int32_t fd);

#endif
#endif



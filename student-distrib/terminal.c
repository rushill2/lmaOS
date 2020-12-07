#include "terminal.h"
#include "keyboard.h"
#include "lib.h"
#include "paging.h"
#include "syscall.h"
#include "x86_desc.h"
#include "scheduling.h"

int32_t cur_term = 0;       // id of displayed terminal

/* 
 * void terminal_init()
 * Description: Initializes paging and struct values for three terminal structs
 * Inputs: None
 * Outputs: None
 * Return:
 * Side Effects: Initialize terminal(s) and put cursor on the top left.
 */
void terminal_init() 
{
	//initialize terminal struct
	uint8_t i, j;
	for(i = 0; i < 3; i++) {
		terminals[i].screen_x = 0;
		terminals[i].screen_y = 0;
		terminals[i].first_run = 1;
		terminals[i].kb_buffer_index = 0;
		terminals[i].num_proc = 0;
		terminals[i].term_proc = (pcb_t*)NULL;
		for (j = 0; j < 4; j++) 
			terminals[i].processes[j] = -1;
	}
	cur_term = 0;
	running_term = 1;
}


/*
 * void switch_terminal(uint8_t tid)
 * Description: Switches to a specified terminal, sets terminal's struct parameters and 
 *				displays the specified terminal on the screen
 * Input: tid, the id of the terminal to switch to (0 for A, 1 for B, 2 for C)
 * Output: none
 * Side Effects: switches which terminal is displayed and which process is running
 */
void switch_terminal(uint8_t tid) 
{
	/* ================== SAVE CURRENT TERMINAL ========================= */
	
	// save esp and ebp for current terminal
	pcb_t* cur_process = get_pcb_address();
	
	asm volatile(
		"movl %%esp, %0;" 
		"movl %%ebp, %1;"
		:"=r"(cur_process->esp), "=r"(cur_process->ebp)
	);
	
	// save x and y screen values for current terminal
	terminals[cur_term].screen_x = get_screen_x();
	terminals[cur_term].screen_y = get_screen_y();

	//save typed input to old terminal and display new terminal's typed input
	get_kb_buffer((char*)&terminals[cur_term].kb_buffer);
	terminals[cur_term].kb_buffer_index = get_kb_index();
	reset_keyboard();
	set_kb_buffer(terminals[tid].kb_buffer);
	set_kb_index(terminals[tid].kb_buffer_index);
	
	//save old terminal's display to its screen buffer
	memcpy((uint32_t*)((VIDMEM + cur_term + 1) << 12), (uint32_t*)(VIDMEM << 12), ENTRY_SIZE);
	
	/* ================== RESTORE NEXT TERMINAL ========================= */
	
	//put new terminal's screen buffer on display
	cur_term = tid;

	// switch process to next terminal's current active process
	cur_process = get_pcb_address();
	memcpy((uint32_t*)(VIDMEM << 12), (uint32_t*)((VIDMEM + cur_term + 1) << 12), ENTRY_SIZE);
	
	//set display's screen x and screen y from values in new terminal's struct
	set_screen_x((uint8_t)terminals[cur_term].screen_x);
	set_screen_y((uint8_t)terminals[cur_term].screen_y);
	uint8_t x = get_screen_x();
	uint8_t y = get_screen_y();
	update_cursor(x, y);
	
	/* ================== SWITCH EXECUTION ========================= */
	
	// switch page back to next terminal's process
	set_process_page(_8MB + (cur_process->pid * _4MB));

	// stack switching
	// point tss to next terminal's process	
	tss.ss0 = KERNEL_DS;
	tss.esp0 = _8MB - (cur_process->pid * _8KB) - 4;
	
	// load ebp and esp from next process into their registers
    asm volatile(
        "movl %0, %%ebp;"
        "movl %1, %%esp;"
        :
        : "r" (cur_process->ebp), "r" (cur_process->esp)
        : "ebp", "esp"
    );
	
	//run shell program if this terminal hasn't been run before
	if(terminals[cur_term].first_run) {
		execute((uint8_t*)"shell");
	} 

}

/*
 * uint8_t get_current_terminal()
 * Description: returns cur_term for use in other files
 * Inputs: none
 * Outputs: cur_term
 * Side Effects: none
 */
uint8_t get_current_terminal() {
	return cur_term;
}


/* 
 * int32_t terminal_read(int32_t fd, void* buf, int32_t nbytes) 
 * Description:	reads a buffer from the keyboard
 * Inputs: int32_t fd - file descriptor,
 * 		   const void* buf - input buf to write,
 * 		   int32_t nbytes - number of bytes to write
 * Outputs: None
 * Return Value: # bytes to read (0 if invalid)
 * Side Effects: 
 */
int32_t terminal_read(int32_t fd, void* buf, int32_t nbytes) 
{
	/* check for invalid params */
	if(nbytes == 0 || buf == NULL)
		return 0;

	/* init local variables */
	char* term_buf = buf;
	char kb_buf[KB_BUF_SIZE];
	uint8_t enter;
	int i, upper_limit;

	/* check #bytes to read */
	if(nbytes > KB_BUF_SIZE) {	// cannot exceed 128
		upper_limit = KB_BUF_SIZE;
	} else {
		upper_limit = nbytes;
	}

	/* init terminal buf to null chars */
	for(i = 0; i < upper_limit; i++) {
		term_buf[i] = '\0';
	}

	/* check enter flag */
	check_enter_pressed(&enter);

	/* loop until enter is pressed */
	while(enter != 1) {
		check_enter_pressed(&enter);
	}
	
	/* get the buffer from kb input */
	get_kb_buffer(kb_buf);

	/* write terminal buffer from keyboard buffer */
	for(i = 0; i < upper_limit; i++) {
		term_buf[i] = kb_buf[i];
	}

	/* reset kb */
	reset_keyboard();
	term_buf[upper_limit - 1] = '\n';
	/* null at end of terminal buffer */
	term_buf[upper_limit] = '\0';
	/* return number of bytes read */
	return upper_limit;
	
}

/*
 * int32_t terminal_write(int32_t fd, const void* buf, int32_t nbytes) 
 * Description: writes a given buffer to the terminal
 * Inputs: int32_t fd - file descriptor,
 * 		   const void* buf - input buf to write,
 * 		   int32_t nbytes - number of bytes to write
 * Outputs: 
 * Return Value: number of bytes written, -1 if failure
 * Side Effects: Print terminal_read content to screen
 */
int32_t terminal_write(int32_t fd, const void* buf, int32_t nbytes) 
{
	cli();
	const char* term_buf = buf;

	/* check invalid input */
	if(nbytes == 0 || buf == NULL) {
		return 0;
	}

	/* print terminal buffer to screen */
	int i;	// loop index for terminal buffer
	for(i = 0; i < nbytes; i++) {
		putc(term_buf[i]);
	}

	/* set screen_x at printed location so terminal output cannot be deleted */
	uint8_t current_screen_x = (uint8_t)get_screen_x();
	set_stop_x(current_screen_x);
	sti();
	return nbytes;
}

/* 
 * int32_t terminal_open(const uint8_t* filename) 
 * Description: This function opens a file if it's terminal_type (stdin and stdout)
 * Inputs: const uint8_t* filename - name of file
 * Outputs: None
 * Return Value: -1 if failure, 0 if success
 * Side Effects:
 */
int32_t terminal_open(const uint8_t* filename) 
{
	// check invalid param
	if(filename == NULL) 
		return -1;
	return 0;
}

/* 
 * int32_t terminal_close(int32_t fd) 
 * Description: This function closes a file if it's terminal_type (stdin and stdout)
 * Inputs: int32_t fd - file descriptor of file to read
 * Outputs: None
 * Return Value: 0 (success)
 * Side Effects: Terminal is disabled.
 */
int32_t terminal_close(int32_t fd) 
{
	return 0;
}


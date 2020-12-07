#include "intr_handler.h"
#include "lib.h"
#include "idt.h"
#include "x86_desc.h"
#include "i8259.h"
#include "keyboard.h"
#include "types.h"
#include "syscall.h"
#include "terminal.h"

static char* video_mem = (char *)VIDEO;
char kb_buffer[KB_BUF_SIZE];
uint8_t kb_buffer_index = 0;
uint8_t cursor_x;	
uint8_t enter_flag = 0;		// 0: released, 1: pressed
uint8_t cl_flag = 0;		// 0: released, 1: pressed
uint8_t lshift_flag = 0;    // 0: released, 1: pressed
uint8_t rshift_flag = 0;	// 0: released, 1: pressed
uint8_t ctrl_flag = 0;		// press: 0x1D, release: 0x9D  (left ctrl)
uint8_t alt_flag = 0;		// press: 0x38, release: 0xB8  (left alt)

/* key maps to determine char for keyboard entry */
/* key mapping: shift and caps lock are pressed at the same time (shift = 1, cl = 1) */
static char scancode_shift_caps_lock[MAP_LEN] = { 
	'\0','\0','!','@','#','$','%','^','&','*','(',')','_','+','\0','\0',
	'q','w','e','r','t','y','u','i','o','p','{','}','\0','\0','a','s',
	'd','f','g','h','j','k','l',':','"','~','\0','|','z','x','c','v',
	'b','n','m','<','>','?','\0','*','\0',' ','\0',
	'\0','\0','\0','\0','\0','\0','\0','\0','\0','\0',
	'\0','\0','7','8','9','-','4','5','6','+','1','2','3','0',
	'.','\0','\0','\0','\0','\0'	
} ;
/* key mapping: shift is released and caps lock is pressed (cl = 1, shift = 0) */
static char scancode_caps_lock[MAP_LEN] = { 
	'\0','\0','1','2','3','4','5','6','7','8','9','0','-','=','\0','\0',
	'Q','W','E','R','T','Y','U','I','O','P','[',']','\0','\0','A','S',
	'D','F','G','H','J','K','L',';','\'','`','\0','\\','Z','X','C','V',
	'B','N','M',',','.','/','\0','*','\0',' ','\0',
	'\0','\0','\0','\0','\0','\0','\0','\0','\0','\0',
	'\0','\0','7','8','9','-','4','5','6','+','1','2','3','0',
	'.','\0','\0','\0','\0','\0'
};
/* key mapping: shift is pressed and caps lock is released (shift = 1, cl = 0) */ 
static char scancode_shift[MAP_LEN] = { 
	'\0','\0','!','@','#','$','%','^','&','*','(',')','_','+','\0','\0',
	'Q','W','E','R','T','Y','U','I','O','P','{','}','\0','\0','A','S',
	'D','F','G','H','J','K','L',':','"','~','\0','|','Z','X','C','V',
	'B','N','M','<','>','?','\0','*','\0',' ','\0',
	'\0','\0','\0','\0','\0','\0','\0','\0','\0','\0',
	'\0','\0','7','8','9','-','4','5','6','+','1','2','3','0',
	'.','\0','\0','\0','\0','\0'	
};
/* key mapping: no shift/CL pressed (shift = 0, cl = 0) */
static char scancode_normal[MAP_LEN] = { 
	'\0','\0','1','2','3','4','5','6','7','8','9','0','-','=','\0','\0',
	'q','w','e','r','t','y','u','i','o','p','[',']','\0','\0','a','s',
	'd','f','g','h','j','k','l',';','\'','`','\0','\\','z','x','c','v',
	'b','n','m',',','.','/','\0','*','\0',' ','\0',
	'\0','\0','\0','\0','\0','\0','\0','\0','\0','\0',
	'\0','\0','7','8','9','-','4','5','6','+','1','2','3','0',
	'.','\0','\0','\0','\0','\0'	
	
};


/*
 * void keyboard_init ()
 * Description: Clear the screen and initialize keyboard IRQ.
 * Inputs: None
 * Outputs: None
 * Return Value: None
 * Side Effect: Clear the screen and enable keyboard entries.
 */
void keyboard_init ()
{

	/* enable KB IRQ */
    enable_irq (KEYBOARD_IRQ);  
}


/*
 * void kb_intr ()
 * Description: Read key from keyboard port 0x60 to issue an interrupt
 *              and echo the entry to screen.
 * Inputs: None
 * Outputs: None
 * Return Value: None
 * Side Effect: Write port 0x60 to register C and handles kb interrupts.
 */
void kb_intr ()
{
    /* 
	========== CP1 work ==========
	Acknowledge input 
    uint8_t status = inb(KB_CMD);
    char key;
    if (status & 0x01) {
        // find the key from map
        key = inb(KB_PORT);
        // echo input key 
        char output = key_map[key];
        putc(output);
    }
    */
    char key;
	uint8_t idx;
	uint8_t current_screen_x;
	uint8_t current_screen_y;
	int x;	// screen_x
	int y;	// screen_y
	//while(1){
		idx = inb(KB_PORT);  // read key from port 0x60
		//if (idx >= 0)break;
	//}
	key = scancode_normal[idx];
	/* write char to video_memory_buffer */

	if (idx == 0x2A) {		        // left shift pressed
		lshift_flag = 1;
	}
	else if (idx == 0x36) {	        // right shift pressed
		rshift_flag = 1; 
	}
	else if (idx == 0xAA) {	        // left shift released
		lshift_flag = 0; 
	}
	else if (idx == 0xB6) {	        // right shift released
		rshift_flag = 0; 
	}
	
	else if (idx == 0x3A) {         // caps lock pressed
		cl_flag = cl_flag ^ 0x1; 
	
	} else if(idx == 0x38) {	    // alt pressed
		alt_flag = 1;
	} else if(idx == 0xB8) {	    // alt released
		alt_flag = 0;
	} else if(idx == 0x1C) {		// enter pressed
		current_screen_x = (uint8_t)get_screen_x();
		current_screen_y = (uint8_t)get_screen_y();

		enter_flag = 1;
		kb_buffer_index = 0;	
		
		/* check if need to scroll down */
		if (current_screen_y == (NUM_ROWS - 1))
			scroll_down();
		else {
			set_screen_y(current_screen_y + 1);
			set_screen_x(0);
			x = get_screen_x();
			y = get_screen_y();
			update_cursor(x, y);
		}
	} else if(idx == 0x1D) {		// ctrl pressed
		ctrl_flag = 1;
	} else if(idx == 0x9D) {		// ctrl released
		ctrl_flag = 0;
	} else if(idx == 0x0E) {		// backspace pressed
		current_screen_x = (uint8_t)get_screen_x();
		current_screen_y = (uint8_t)get_screen_y();
		/* check if hit the top left of screen */
		//if(!((current_screen_x == 0 && current_screen_y == 0) || current_screen_x == cursor_x)) {

		if ( current_screen_x != 0 && current_screen_x != cursor_x) {
			/* decrement screen_x */
			current_screen_x--;
			// replace last char with null \0
			*(uint8_t *)(video_mem + ((NUM_COLS * current_screen_y + current_screen_x) * 2)) = '\0';

			// remove last char from kb buffer
        	kb_buffer[kb_buffer_index] = '\0';
        	kb_buffer_index--;
			kb_buffer[kb_buffer_index] = '\0';
			set_screen_x(current_screen_x);	
			x = get_screen_x();
			y = get_screen_y();
      		update_cursor(x, y);
      	} 
		else {
			if (current_screen_y != 0 && current_screen_x != cursor_x) {
				/* decrement screen_y and set screen_x to the last char on prev line */
				current_screen_x = NUM_COLS - 1;
				current_screen_y--;
				// replace last char with null \0
				*(uint8_t *)(video_mem + ((NUM_COLS * current_screen_y + current_screen_x) * 2)) = '\0';

				// remove last char from kb buffer
        		kb_buffer[kb_buffer_index] = '\0';
        		kb_buffer_index--;
				set_screen_x(current_screen_x);
				set_screen_y(current_screen_y);
				x = get_screen_x();
				y = get_screen_y();
				update_cursor(x,y);
			}
		}
	} else if(alt_flag && idx == 0x3B) {		// alt + F1 pressed
		uint8_t cur_term = get_current_terminal();
		if(cur_term != 0) {
			send_eoi(KEYBOARD_IRQ);
			switch_terminal(0);
			return;
		}
	} else if(alt_flag && idx == 0x3C) {		// alt + F2 pressed
		uint8_t cur_term = get_current_terminal();
		if(cur_term != 1) {
			send_eoi(KEYBOARD_IRQ);
			switch_terminal(1);
			return;
		}
	} else if(alt_flag && idx == 0x3D) {		// alt + F3 pressed
		uint8_t cur_term = get_current_terminal();
		if(cur_term != 2) {
			send_eoi(KEYBOARD_IRQ);
			switch_terminal(2);
			return;
		}
	}
	else if (idx < 88){
		if(ctrl_flag && (idx == 0x26 || idx == 0x2E)) {	//ctrl + l/ctrl + L= clear sc and reset buffer
			clear();
			set_screen_x(0);
			set_screen_y(0);
			x = get_screen_x();
			y = get_screen_y();
			update_cursor(x, y);
			if (shell_flag == 1) {
				puts((int8_t*)"391OS> ");
				set_screen_x(7);
				set_screen_y(0);
				x = get_screen_x();
				y = get_screen_y();
				update_cursor(x, y);
			}
			kb_buffer_index = 0;
		} else if(kb_buffer_index >= KB_BUF_SIZE) {
			send_eoi(KEYBOARD_IRQ);
			return;
		}
		else if ((cl_flag == 0) && (lshift_flag == 0) && (rshift_flag == 0)) {	    //use lowercase letters
			putc(scancode_normal[idx]);
			kb_buffer[kb_buffer_index] = scancode_normal[idx];
			kb_buffer_index++;
		}
		else if ((cl_flag == 1) && (lshift_flag == 0) && (rshift_flag == 0)) {	    //use uppercase letters
			putc(scancode_caps_lock[idx]);
			kb_buffer[kb_buffer_index] = scancode_caps_lock[idx];
			kb_buffer_index++;
		}
		else if ((cl_flag == 0) && ((lshift_flag == 1) || (rshift_flag == 1))) {	//use shift scancodes
			putc(scancode_shift[idx]);
			kb_buffer[kb_buffer_index] = scancode_shift[idx];
			kb_buffer_index++;
		}
		else if ((cl_flag == 1) && ((lshift_flag == 1) || (rshift_flag == 1))) {	//use shift + caps lock scancodes
			putc(scancode_shift_caps_lock[idx]);
			kb_buffer[kb_buffer_index] = scancode_shift_caps_lock[idx];
			kb_buffer_index++;
		}
	}

    /* update cursor position*/
	x = get_screen_x();
	y = get_screen_y();
	update_cursor(x, y); 

    /* End of interrupt */
    send_eoi(KEYBOARD_IRQ);
}


/* 
 * void get_kb_buffer(char* buf) 
 * Description: returns the keyboard buffer for use by other functions (specifically terminal)
 * Inputs: None
 * Outputs: None
 * Return Value: None
 * Side Effects:
 */
void get_kb_buffer(char* buf) 
{
	int i;
	for(i = 0; i < KB_BUF_SIZE; i++) {
		buf[i] = kb_buffer[i];
	}
}


/* 
 * void set_kb_buffer(char* buf) 
 * Description: This function sets the keyboard buffer to the input buf.
 * Inputs: None
 * Outputs: None
 * Return Value: None
 * Side Effects: 
 */
void set_kb_buffer(char* buf) 
{
	int i;
	for(i = 0; i < KB_BUF_SIZE; i++) {
		kb_buffer[i] = buf[i];
	}
}


/* 
 * void check_enter_pressed(uint8_t* flag) 
 * Description: This function checks if enter key is pressed.
 * Inputs: None
 * Outputs: None
 * Return Value: None
 * Side Effects: None
 */
void check_enter_pressed(uint8_t* flag) 
{
	*flag = enter_flag;
}


/* 
 * void set_stop_x(uint8_t current_screen_x) 
 * Description:	This function sets the cursor_x to screen_x and the cursor stops deleting.
 * Inputs: None
 * Outputs: None
 * Return Value: None
 * Side Effects:
 */
void set_stop_x(uint8_t current_screen_x) 
{
	cursor_x = current_screen_x;
}


/* 
 * void reset_keyboard()
 * Description: This functions resets kb buffer, kb buffer index and enter flag.
 * Inputs: None
 * Outputs: None
 * Return Value: None
 * Side Effects: 
 */
void reset_keyboard() 
{
	int i;
	for (i = 0; i < KB_BUF_SIZE; i++)
		kb_buffer[i] = '\0';

	enter_flag = 0;
	kb_buffer_index = 0;
}


/* 
 * uint8_t get_kb_index()
 * Description: This function returns the global kb buffer index
 * Inputs: None
 * Outputs: None
 * Return Value: None
 * Side Effects: 
 */
uint8_t get_kb_index() 
{
	return kb_buffer_index;
}


/* 
 * void set_kb_index(uint8_t index)
 * Description: This function sets the global kb buf index
 * Inputs: uint8_t index - the kb buf index for a terminal being switched to
 * Outputs: None
 * Return Value: None
 * Side Effects: kb buf index available for caller
 */
void set_kb_index(uint8_t index)
{
	kb_buffer_index = index;
}


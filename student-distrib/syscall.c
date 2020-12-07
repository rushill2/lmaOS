#include "intr_handler.h"
#include "lib.h"
#include "idt.h"
#include "x86_desc.h"
#include "i8259.h"
#include "rtc.h"
#include "syscall.h"
#include "terminal.h"
#include "keyboard.h"
#include "paging.h"
#include "filesys.h"
#include "scheduling.h"

/* initialize global variables */
file_op_jumptable_t file_op = {open_file, close_file, read_file, write_file};
file_op_jumptable_t rtc_op = {rtc_open, rtc_close, rtc_read, rtc_write};
file_op_jumptable_t dir_op = {open_dir, close_dir, read_dir, write_dir};
file_op_jumptable_t stdin_op = {bad_call, bad_call, terminal_read, bad_call};
file_op_jumptable_t stdout_op = {bad_call, bad_call, bad_call, terminal_write};
file_op_jumptable_t do_nothing = {bad_call, bad_call, bad_call, bad_call};

uint8_t process_state[6] = {0, 0, 0, 0, 0, 0};  // task switching by state of processes
int8_t last_shell[3] = {-1, -1, -1};    // current pid of last shell on this terminal


/* 
 * int32_t halt (uint8_t status)
 * Description: The execute system call attempts to halt the program.
 * Inputs: uint8_t status - program status
 * Outputs: None
 * Return Value: -1 (failure), (uint32_t)status (success)
 * Side Effects: halt program
 */
int32_t halt (uint8_t status) 
{
    cli ();

	pcb_t* cur_process = get_pcb_address();
	uint32_t actual_status = (uint32_t)status;
	
	if(exception_status == 256){
		actual_status = exception_status;
		exception_status = 0;
	}
    uint8_t cur_term = get_current_terminal();
    // first, decrement number of proc in current terminal 
    terminals[cur_term].num_proc = terminals[cur_term].num_proc - 1;
	// check if we are trying to halt an inactive process
	if(process_state[cur_process->pid] == 0) {
		printf("halt error: inactive process\n");
		return -1;
	}

	// check if halting only remaining active process
	uint8_t is_process0 = 0;
    if (terminals[cur_term].num_proc == 0) {
        is_process0 = 1;   
    }
    
	// mark process as no longer active
    int i;  // loop index
	process_state[cur_process->pid] = 0;
    for(i = 0; i < 4; i++) {
        if(terminals[cur_term].processes[i] == cur_process->pid) {
			terminals[cur_term].processes[i] = -1;
			break;
		}
	}

    terminals[cur_term].term_proc = get_pcb(cur_process->parent_pid);
    /* check if exiting a shell */
    if (last_shell[cur_term] == cur_process->pid) 
        last_shell[cur_term] = cur_process->parent_pid;
	// close all open files for this process

	for(i = 0; i < 8; i++) {
    	if(i > 1 && i < 8 && cur_process->file[i].flags == 1) {
      		close(i);
    	}
        cur_process->file[i].file_op = &do_nothing;
  	}

	// run a new shell if last one halted
	if(is_process0) {
		execute((uint8_t*)"shell");
	}

	// switch page back to parent process
	set_process_page(_8MB + (cur_process->parent_pid * _4MB));

	// point tss to parent process
	tss.esp0 = _8MB - (cur_process->parent_pid * _8KB) - 4;
	
	
    //printf("%d\n", actual_status);
    asm volatile(
		"movl %2, %%eax;"
        "movl %0, %%ebp;"
        "movl %1, %%esp;"
        "sti;"
        "leave;"
        "ret;"
        :
        : "r" (cur_process->parent_ebp), "r" (cur_process->parent_esp), "r" (actual_status)
        : "eax", "ebp", "esp"
    );

	return actual_status;

}


/* 
 * int32_t execute (const uint8_t* command)
 * Description: The execute system call attempts to load and execute a new program, 
 *				handing off the processor to the new program until it terminates. 
 * Inputs: const uint8_t* command - command to execute
 * Outputs: None
 * Return Value: -1 if the program does not exist or not an executable 
 * 				256 if the program dies by an exception
 * 				0 to 255 if the program executes a halt system call
 * Side Effects: run program in shell
 */
int32_t execute (const uint8_t* command)
{
    shell_flag = 1; // flag to reprint shell prompt after clearing the screen
    uint8_t cur_term = get_current_terminal();
    if (command == NULL || command[0] == '\0') {
        printf("execute error: command error\n");
        return -1;  // failure
    }
	if (process_state[0] == 1 && process_state[1] == 1 && process_state[2] == 1
        && process_state[3] == 1 && process_state[4] == 1 && process_state[5] == 1) {
		printf("execute: no available process(es)\n");
		return -1;  // failure
	}

    if (terminals[cur_term].num_proc > 3) {
        printf("execute: no available process(es)\n");
        return -1;  // failure
    }
    /* init variables */
    int8_t exe_identifier[4] = {0x7f, 0x45, 0x4c, 0x46};
    uint8_t fname[33];
    uint8_t active_pid;
    int i; // loop index
    //int fname_len = 0;     // length of file name
    uint32_t length = len_inodes;
    uint8_t file_data[(int)length];
    /* initialize process */
    /* activate process by order */
    for (i = 0; i < 6; i++) {
        if (process_state[i] == 0) {
            process_state[i] = 1;
            active_pid = i;
            break;
        }
    }

    /* parse: arg and fname */
    int8_t arg[CMD_LEN+1];
    int exe_idx = 0;
    int arg_idx = 0;
    
    /* 
     * 0 - not reached exe yet
     * 1 - reached exe
     * 2 - finished parsing exe
     * 3 - reached arg
     */
    int command_flag = 0;

    for(i = 0; i < strlen((int8_t*)command); i++){

        if(command[i] != ' ' && (command_flag == 0 || command_flag == 1)){
            if(exe_idx >= 32){
                printf("execute error: filename too long\n");
                return -1;
            }
            fname[exe_idx] = command[i];
            exe_idx++;
            command_flag = 1;
        }
        else if(command[i] == ' ' && command_flag == 1){
            command_flag = 2;
        }
        else if(command[i] != ' ' && (command_flag == 2 || command_flag==3)){
            arg[arg_idx] = command[i];
            arg_idx++;
            command_flag = 3;
        }
        else if(command[i] == ' ' && command_flag == 3){
            arg[arg_idx] = command[i];
            arg_idx++;
            command_flag = 3;
        }
    }

    fname[exe_idx] = '\0'; // set final character to null

    arg[arg_idx] = '\0'; // set final character to null
    /* Initialize dentry and buffer variables */
    dentry_t dentry;


    /* Retrieve file dentry */
    if(read_dentry_by_name(fname, &dentry) == -1) {
        printf("execute error: file not found\n");
        process_state[active_pid] = 0;
        return -1;  // failure
    }

    /* Read first 4 bytes of data */

    uint16_t nbytes = read_data(dentry.inode_num, 0, file_data, length);
    /* exe check */
    /* user-level program loader*/
    /* Check to see if strings are the same */
    if(strncmp((int8_t*)file_data, exe_identifier, 4) != 0){
        printf("execute error: file not an executable\n");
        process_state[active_pid] = 0;
        return -1;  // not an exe file
    }

    /* create PCB */
    /*
    uint32_t temp_esp, temp_ebp;

    // check if first run for current running term
    if(terminals[running_term].term_proc != NULL && terminals[running_term].term_proc->sche_enable)
    {
        temp_esp = terminals[running_term].term_proc->esp;
        temp_ebp = terminals[running_term].term_proc->ebp;
        terminals[running_term].term_proc->sche_enable = 0;
    } else{
        temp_esp = 0;
        temp_ebp = 0;
    }

    // overwrite with esp and ebp from scheduling
    //pcb_t pcb;
    */
    pcb_t* cur_process = (pcb_t*)(_8MB - (_8KB * (active_pid + 1)));
    uint8_t cur_pid = active_pid;
    cur_process->pid = cur_pid;

    /* set scheduling flag */
    //cur_process->sche_enable = 0;
    //cur_process = get_pcb(cur_pid);
    
    //cur_process->esp = temp_esp;
    //cur_process->ebp = temp_ebp;
    /* determine child process for current terminal */
    terminals[cur_term].term_proc = cur_process; // assign current process as base shell 

    if (terminals[cur_term].first_run) {
        if (cur_term == 0) last_shell[0] = 0;   // first run, shell = process 0
        terminals[cur_term].processes[0] = cur_pid; // assign current process as base shell 
        terminals[cur_term].first_run = 0;          // clear first run flag
    } else {
        for (i = 0; i < 4; i++) {
            if (terminals[cur_term].processes[i] == -1) {   // check all processes under current terminal
                terminals[cur_term].processes[i] = cur_pid; // assign current process as the last process
                break;
            }
        }
    }

    // determine parent process for current terminal 
    if (terminals[cur_term].num_proc == 0) {
        cur_process->parent_pid = cur_pid; // set base shell parent pid to base shell
        cur_process->parent_pid = terminals[cur_term].processes[0]; // set base shell parent pid to base shell
    }
    else 
        cur_process->parent_pid = last_shell[cur_term];             // set pid to last shell 

    
    /* setup stdin */
    cur_process->file[0].flags = 1;
    cur_process->file[0].file_op = &stdin_op;
    cur_process->file[0].pos = 0;
    /* setup stdout */
    cur_process->file[1].flags = 1;
    cur_process->file[1].file_op = &stdout_op;
    cur_process->file[1].pos = 0;


    strncpy((int8_t*)cur_process->arg, arg, strlen(arg)+1); // load to pcb



    // increment number of processes on currently displayed terminal
    terminals[cur_term].num_proc = terminals[cur_term].num_proc + 1;
    // check if shell
    if(fname[0] == 's' && fname[1] == 'h' && fname[2] == 'e' && fname[3] == 'l' && fname[4] == 'l' && fname[5] == '\0') {
		last_shell[cur_term] = cur_pid;
	}
    /* paging:
    When processing the execute system call, your kernel
    must create a virtual address space for the new process. 
    This will involve setting up a new Page Directory with entries. */
    /* starting at physical 8MB, setup a 4MB page dir entry */
    set_process_page(_8MB + (cur_pid * _4MB)); 
    /* The program image itself is linked to execute at virtual address 0x08048000 */

    uint8_t* program_image_addr = (uint8_t*)PROGRAM_IMG_ADDR;   // copy to program img addr at virtual
    memcpy(program_image_addr, file_data, nbytes);


    /* save parent esp */
    uint32_t parent_esp;
    asm volatile (
        "movl   %%esp, %0" 
		: "=g"(parent_esp)
    );
    cur_process->parent_esp = parent_esp;
    /* save parent ebp */
    uint32_t parent_ebp;
    asm volatile (
        "movl   %%ebp, %0"
        : "=g"(parent_ebp)
    );
    cur_process->parent_ebp = parent_ebp;
    // set process to current running terminal
    terminals[running_term].term_proc = cur_process;
    // entry point
    uint8_t entry_ptr[4];
    /* context switch */
    //  The EIP you need to jump to is the entry point from bytes 24-27 of the executable that you have just loaded
    for (i = 0; i < 4; i++) {
        entry_ptr[i] = file_data[i + 24]; 
    }
    
    /* the important fields are SS0 and ESP0. 
    These fields contain the stack segment and stack pointer that 
    the x86 will put into SS and ESP when performing a privilege switch 
    from privilege level 3 to privilege level 0 
    (for example, when a user-level program makes a system call, 
    or when a hardware interrupt ours while a user-level program is executing). 
    These fields must be set to point to the kernel's stack segment 
    and the process's kernel-mode stack, respectively */
    tss.ss0 = KERNEL_DS;
    tss.esp0 = _8MB - (cur_pid * _8KB) - 4;

    /* Push IRET context to kernel stack */
	asm volatile (
		"cli;"
		"pushl  $0x2B;"         // user ds
		"pushl  $0x083FFFFC;"   // esp
		"pushfl;"               // eflag
		"popl   %%eax;"
		"orl    $0x200, %%eax;" // re-enable interrupts
		"pushl  %%eax;"
		"pushl  $0x23;"         //user_cs
		"movl   %0, %%eax;"
		"pushl  (%%eax);"       //eip
		"iret;"
		:
		: "r" (entry_ptr)
		: "%eax" 
    );

    return 0;   // success
}

/* 
 * int32_t read (int32_t fd, void* buf, int32_t nbytes)
 * Description: System call reads data from the keyboard, a file, device (RTC), 
 * 				or directory. 
 * Inputs:  int32_t fd - file descriptor
 * 			void* buf - buffer to read
 * 			int32_t nbytes - number of bytes to read
 * Outputs: None
 * Return Value: -1 (failure), 0 from other funcs
 * Side Effects: read cmd from shell
 */
int32_t read (int32_t fd, void* buf, int32_t nbytes)
{
    if (buf == NULL) 
        return -1;
    
    if ((fd < 0) || (fd > 7) || (nbytes < 0)) // check if fd 0-7
        return -1;   

    pcb_t* pcb = get_pcb_address();

    if (pcb->file[fd].flags == 0)
        return -1;

    //int32_t length = get_inode_length(dentry.i_node);
    /*
    if (pcb->file[fd].pos >= get_inode_length(pcb->file[fd].inode)) 
        return 0;
    */
    // update pos in file
    uint32_t nbytes_read = pcb->file[fd].file_op->read(fd, (char*)buf, nbytes);
    //pcb->file[fd].pos += nbytes_read;
    // return nbytes read

    return nbytes_read;
}

/* 
 * int32_t write (int32_t fd, const void* buf, int32_t nbytes)
 * Description: System call writes data to the terminal or to a device (RTC).
 * Inputs:  int32_t fd - file descriptor
 * 			const void* buf - buffer to write
 * 			int32_t nbytes - number of bytes to write
 * Outputs: None
 * Return Value: -1 (failure), number of bytes to write
 * Side Effects: write response
 */
int32_t write (int32_t fd, const void* buf, int32_t nbytes)
{
    if (buf == NULL) 
        return -1;
    if ((fd < 0) || (fd > 7) || (nbytes < 0)) // check if fd 0-7
        return -1;

    pcb_t* pcb = get_pcb_address();
    if (pcb->file[fd].flags == 0)
        return -1;

    return pcb->file[fd].file_op->write(fd, (char*)buf, nbytes);
}

/* 
 * int32_t open (const uint8_t* filename)
 * Description: System call open a file according to its type
 * Inputs: const uint8_t* filename - filename
 * Outputs: None
 * Return Value: -1 (failure), number of bytes to open
 * Side Effects: open a file
 */
int32_t open (const uint8_t* filename)
{
    int32_t fd;                           // minimum FD value
    pcb_t* pcb = get_pcb_address();
    dentry_t dentry;
    // filename too long or no filename entered
    if (strlen((int8_t*)filename) > 32 || strlen((int8_t*)filename) == 0){
        return -1;
    }
    // read dentry failed
	if (read_dentry_by_name(filename, &dentry)==-1) 
		return -1;

    for (fd = 2; fd < 8; fd++) {
        if (pcb->file[fd].flags != 1) {
            //pcb->file[fd].flags = 1;
            break;
        }
    }
    if (pcb->file[fd].flags != 1) {
        if(dentry.file_type == 0 || !strncmp((int8_t*)filename, "rtc", 3)){                // if we are dealing with the rtc
            if(rtc_open(filename) != -1){
                pcb->file[fd].file_op = &rtc_op;
                pcb->file[fd].pos = 0;
                pcb->file[fd].inode = 0;
                pcb->file[fd].flags = 1;
                return fd;
            } else {
                return -1;
            }
        }
        else if (dentry.file_type == 1 || !strncmp((int8_t*)filename, ".", 1)){                // if we are dealing with a directory
            if(open_dir(filename) != -1){
                pcb->file[fd].file_op = &dir_op;
                pcb->file[fd].pos = 0;
                pcb->file[fd].inode = 0;
                pcb->file[fd].flags = 1;
                return fd;
            } else {
                return -1;
            }
        }
        else if (dentry.file_type == 2){                // if we are dealing with a file
            if(open_file(filename) != -1){
                pcb->file[fd].file_op = &file_op;
                pcb->file[fd].pos = 0;
                pcb->file[fd].inode = dentry.inode_num;
                pcb->file[fd].flags = 1;
                return fd;
            } else {
                return -1;
            }
        }
        else {
            return -1;
        }
    }
    else {
        return -1;
    }
    pcb->file[fd].file_op->open(filename);
    return fd;
}

/* 
 * int32_t close (ine32_t fd)
 * Description: System call close a file and reset file data
 * Inputs: int32_t fd - file descriptor
 * Outputs: None
 * Return Value: -1 (failure), 0 (success)
 * Side Effects: close a file
 */
int32_t close (int32_t fd)
{
    // check if fd is valid
    if ((fd < 2) || (fd > 7)) 
        return -1;
    pcb_t* pcb = get_pcb_address();
    // check if file is opened
    if (pcb->file[fd].flags != 1) {
        return -1;
    }
    // reset file related variables
    pcb->file[fd].file_op->close(fd);
    pcb->file[fd].flags = 0;
    pcb->file[fd].inode = 0;
    pcb->file[fd].pos = 0;
    //pcb->file[fd].file_op->read = 0;
    //pcb->file[fd].file_op->write = 0;
    //pcb->file[fd].file_op->open = 0;
    //pcb->file[fd].file_op->close = 0;
    //pcb->file[fd].file_op = stdin_op;
    return 0;
}


/*
 * int32_t getargs (uint8_t* buf, int32_t nbytes) 
 * Description: This function reads the program's command line arguments into a user-level buffer.
 * Inputs:  uint8_t* buf - user-level buffer to store program's cmd line arguments
 *          int32_t nbytes - number of bytes read from cmd line arguments
 * Outputs: None
 * Return Value: -1 (failure), 0 (success)
 * Side Effects: None
 */
int32_t getargs (uint8_t* buf, int32_t nbytes) 
{   
    // check null pointer
    if (buf == NULL) return -1;
    pcb_t* cur_process = get_pcb_address();
    // check if first char of cmd line arguments is null
    if (cur_process->arg[0] == '\0') return -1;
    strncpy((int8_t*)buf, (int8_t*)cur_process->arg, nbytes);
    return 0;
}

/*
 * int32_t vidmap (uint8_t** screen_start)
 * Description: This function maps the text-mode video memory into user space at a pre-set virtual address.
 * Inputs:  uint8_t** screen_start - pointer to the screen start addr of destination
 * Outputs: None
 * Return Value: -1 (failure), virtual addr (132MB)
 * Side Effects: None
 */
int32_t vidmap (uint8_t** screen_start) 
{
    // null ptr
    if (screen_start == NULL)
        return -1;
    // check if addr in range
    if (*screen_start < (uint8_t)VIRTUAL_MEM_ADDR || *screen_start > (uint8_t)(VIRTUAL_MEM_ADDR + KERNEL_ADDR))
        return -1;
    // virtual addr
    uint32_t virtual_addr = VIRTUAL_MEM_ADDR + KERNEL_ADDR;
    // map text-mode vid mem to user space
    map2user((VIDMEM << 12), 0, 1);
    // store virtual addr
    *screen_start = (uint8_t*)virtual_addr;
	return virtual_addr;
}

/*
 * int32_t set_handler (int32_t signum, void* handler_address) 
 * Description: This function changes the default action taken when a signal is received
 * Inputs:  int32_t signum - specifies which signal's hanlder to change
 *          void* handler_address - points to a user-level function to be run when that signal is received
 * Outputs: None
 * Return Value: -1 (failure), OS does not support signals
 * Side Effects: None
 */
int32_t set_handler (int32_t signum, void* handler_address) 
{
    return -1;
}

/*
 * int32_t sigreturn (void)
 * Description: This function copys the hardware context that was on the user-level stack back onto the processor.
 * Inputs: void
 * Outputs: None
 * Return Value: -1 (failure), OS does not support signals
 * Side Effects: None
 */
int32_t sigreturn (void)
{
    return -1;
}

/* 
 * pcb_t* get_pcb_address()
 * Description: get pcb addr
 * Inputs: None
 * Outputs: None
 * Return Value: pointer to process running on active terminal
 * Side Effects: none
 */
pcb_t* get_pcb_address() 
{
    int i;  // loop index
	uint32_t cur_pid;
    uint8_t cur_term = get_current_terminal();
    
    // find the last process running on current terminal
    for (i = 0; i < 4; i++) {
        if(terminals[cur_term].processes[i] == -1) {
            break;
        }
    } 
    //if (i > 0)
    cur_pid = terminals[cur_term].processes[i-1];
	//else 
	//	return NULL;
    //pcb_t* cur_process = (pcb_t*)(_8MB - (_8KB * (cur_pid + 1)));
    pcb_t* cur_process = (pcb_t*)(_8MB - (_8KB * (cur_pid + 1)));
	return cur_process;

	//return terminals[cur_term].term_proc;
	
    /*
    uint32_t cur_esp;

	asm ("movl %%esp, %0"
        : "=r" (cur_esp)
        );

	return (pcb_t*)(cur_esp & PCB_BITMASK);
    */
}

/*
 * get_pcb(uint32_t pid)
 * Description: retrieve pointer process given pid
 * Inputs: pid, processor id number of a process
 * Outputs: pointer to process struct
 * Side Effects: none
 */

pcb_t* get_pcb(uint32_t pid) 
{
    pcb_t* cur_process = (pcb_t*)(_8MB - (_8KB * (pid + 1)));
    return cur_process;
}

/* 
 * int32_t bad_call()
 * Description: handle bad calls e.g. terminal open/close
 * Inputs: None
 * Outputs: None
 * Return Value: -1
 * Side Effects: None
 */
int32_t bad_call() 
{
	return -1;
}


#ifndef SYSCALL_H
#define SYSCALL_H

#define PCB_BITMASK 0xFFFFE000 // kernel stack has 8kB alignment
#define _8KB 		0x2000
#define _4MB 		0x400000
#define _8MB		0x800000
#define MAX_DATA_LEN		0xFFFF
#define PROGRAM_IMG_ADDR	0x08048000
#define VIRTUAL_MEM_ADDR	0x08000000
#define FD_CAP		7
#define FD_FLOOR	2
#define CMD_LEN		128
#ifndef ASM

/* declare global variable */
uint32_t exception_status;
int shell_flag; // flag to indicate if ctrl + l/L leaves the shell prompt on screen
/* system call prototypes */

int32_t halt (uint8_t status);
int32_t execute (const uint8_t* command);
int32_t read (int32_t fd, void* buf, int32_t nbytes);
int32_t write (int32_t fd, const void* buf, int32_t nbytes);
int32_t open (const uint8_t* filename);
int32_t close (int32_t fd);
int32_t getargs (uint8_t* buf, int32_t nbytes);
int32_t vidmap(uint8_t** screen_start);
int32_t set_handler (int32_t signum, void* handler_address);
int32_t sigreturn (void);
/* bad call, for stdin/stdout */
int32_t bad_call();


typedef struct file_op_jumptable_t {
	int32_t (*open) (const uint8_t * filename);
	int32_t (*close) (int32_t fd);
	int32_t (*read) (int32_t fd, void * buf, int32_t nbytes);
	int32_t (*write) (int32_t fd, const void * buf, int32_t nbytes); 

} file_op_jumptable_t;

typedef struct fd_t {
	file_op_jumptable_t* file_op; 
	uint32_t inode; 
	uint32_t pos;	
	uint32_t flags; 
} fd_t;

typedef struct pcb_t {
    fd_t file[8];			// files processed in current process, up to 8
	char arg[CMD_LEN + 1];	// arguments
	int32_t pid; 			// each process has a pid to identify it, starting from 0
	int32_t parent_pid; 	// pid of the process that executed this one
	uint32_t parent_ebp;			// base pointer for parent process, used to return to the parent process when this one is halted
	uint32_t parent_esp;			// stack pointer for parent process, used to return to the parent process when this one is halted
	uint32_t ebp;			// base pointer for current process
	uint32_t esp;			// stack pointer for current process
	uint32_t cr3;			// cr3 pointer for current process
	uint32_t sche_enable;	// flag to enable scheduling and override esp and ebp for pcb from scheduling function
} pcb_t;

pcb_t* get_pcb_address();

pcb_t* get_pcb(uint32_t pid);
#endif /* ASM */

#endif /* _SYSCALL_H */


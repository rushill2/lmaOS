#include "scheduling.h"
#include "syscall.h"
#include "terminal.h"


/*
 * void init_pit()
 * Description: Initializes PIT for use in scheduling
 * Inputs: none
 * Outputs: none
 * Side Effects: loads values into PIT registers and enables the PIT
 */ 
void init_pit()
{
    // get value to use for PIT frequency
	int32_t freq = FALLING_EDGE/RELOAD_VALUE;
    
	cli();
	
	// load values into PIT control registers
    outb(MODE_3, MC_REG);
    outb(freq & 0xFF, CHANNEL_0);
	outb(freq >> 8, CHANNEL_0);

	// enable PIT in PIC
	enable_irq(PIT_IRQ);
	
	sti();
    
	return;
}

/*
 * void pit_intr()
 * Description: Is called by pit_handler asm function and calls scheduling algorithm
 * Inputs: none
 * Outputs: none
 * Side effects: Scheduling function is called
 */
void pit_intr()
{
    scheduling();
	return;
}


/*
 * void scheduling()
 * Description: This function creates processes for terminals in a round robin fashion after pit interrupt is handled.
 * Inputs: None
 * Outputs: None
 * Return Value: None
 * Side Effect: flush tlb
 */
void scheduling() 
{
    /* ================== SAVE CURRENT PROCESS ================== */
	printf("success!\n");
	/* get pointer to current process */
	pcb_t* cur_process = get_pcb_args(running_term);
	if (cur_process == NULL) {
		pcb_t proc;
		cur_process->sche_enable = 1;
		/* run shell program if this terminal hasn't been run before */
		/* save esp and ebp */
		asm volatile(
			"movl %%esp, %0;" 
			"movl %%ebp, %1;"
			:"=r"(proc.esp), "=r"(proc.ebp)
			:
       		: "memory"
		);
		
		// assign process to terminal
		terminals[running_term].term_proc = &proc;
		// switch to running terminal
		switch_terminal(running_term);
		// send end of intr signal
		send_eoi(PIT_IRQ);
		// first run? create base shell
		if(terminals[running_term].first_run) {
			execute((uint8_t*)"shell");
		}
		
	}
	/* save esp and ebp */
	
	asm volatile(
		"movl %%esp, %0;" 
		"movl %%ebp, %1;"
		:"=r"(cur_process->esp), "=r"(cur_process->ebp)
		:
        : "memory"

	);
	
	/* save cr3 
	asm volatile("movl %%CR3, %0;"
	:"=r"(cur_process->cr3)
	);
	*/
	/* ================== SWITCH TO NEXT PROCESS ================== */
	
	/* update process number */
	running_term = (running_term + 1) % NUM_TERM;
	//switch_terminal(process_num);

	if(terminals[running_term].term_proc == NULL) {
		send_eoi(PIT_IRQ);
		return;
	}
	// remap and save vidmem
	//TODO:
	uint8_t cur_term = get_current_terminal();
	if (cur_term != running_term)
		map2user((uint32_t)(VIDEO + (running_term + 1) * ENTRY_SIZE), 0, 1);
	else 
		map2user((uint32_t)VIDEO, 0 , 1);
	//save_vidmem(running_term);

	/* get pointer to next process */
	pcb_t* next_process = get_pcb_args(running_term);
	/* switch page to next terminal's process */
	set_process_page(_8MB + (next_process->pid * _4MB));
	
	/* point tss to next process */
	tss.ss0 = KERNEL_DS;
	tss.esp0 = _8MB - (next_process->pid * _8KB);
	/* send end of intr signal, pit has the highest prio */
	send_eoi(PIT_IRQ);
	/* load registers */
	asm volatile(
        "movl %0, %%ebp;"
        "movl %1, %%esp;"
        :
        : "r" (next_process->ebp), "r" (next_process->esp)
        : "ebp", "esp"
    );

    return;
}

/*
 * pcb_t* get_pcb_args(uint8_t term)
 * Description: gets the process of the terminal specified by the argument
 * Inputs: term, the index of the terminal whose process is being retrieved
 * Outputs: a pointer to the pcb_t object of the process
 * Side Effects: none
 */
pcb_t* get_pcb_args(uint8_t term)
{
	// ensure input is correct
	if(term < 3){
		return terminals[term].term_proc;
	}
	return NULL;
}

/*
 * uint8_t get_running()
 * Description: returns the currently running terminal
 * Inputs: none
 * Outpus: a uint8_t of the terminal that is currently running
 * Side Effects: none
 */
uint8_t get_running() 
{
	return (uint8_t)running_term;
}

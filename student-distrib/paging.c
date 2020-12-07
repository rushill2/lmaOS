/* paging.c - functions to interact with page directory and tables */

#include "paging.h"
#include "types.h"
#include "lib.h"
#include "scheduling.h"

/* Set up page directory for 4 GB */
uint32_t page_directory[PAGES_NUM] __attribute__((aligned(ENTRY_SIZE)));

/* Set up page table for first 4 MB */
uint32_t page_table[PAGES_NUM] __attribute__((aligned(ENTRY_SIZE)));

/* Set up video memory page table */
uint32_t vidmem_pagetable[PAGES_NUM] __attribute__((aligned(ENTRY_SIZE)));


/* Set up vidmem for pagetable for first 4 KB */
//uint32_t page_table_vidmem[PAGES_NUM] __attribute__((aligned(ENTRY_SIZE)));

/* Function: initPaging
 * Description: Creates and initializes page directory and page table, then enables paging.
 * Inputs: None
 * Outputs: None
 */
void initPaging(){
	unsigned int i = 0;
	
	/* initialize page directory to not present for now */
	for(i = 0; i < PAGES_NUM; i++){
		page_directory[i] = (RW); /* set r/w bit to 1, present bit to 0 */
	}
	
	/* map kernel memory to its place in physical memory
	 * set following entry bits:
	 * 	G = 1
	 * 	PS = 1
	 *	PCD = 1
	 *	R/W = 1
	 *	P = 1
	 */
	page_directory[1] = ((unsigned int)KERNEL_ADDR) | (ENTRY_4MB | P | RW);
	
	/* initialize page table to not present for now */
	for(i = 0; i < PAGES_NUM; i++){
		if (i >= VIDMEM && i <= VIDMEM + 3)
			page_table[i] = (i * ENTRY_SIZE) | (P | RW | US); /* set U/S to 0 (supervisor), R/W and present to 1 */
		else 
			page_table[i] = (i * ENTRY_SIZE) | (RW);	/* set present and U/S to 0 (supervisor), RW to 1 */
	}
	
	/* map video memory in virtual memory to its place in physical memory
	 * set following entry bits:
	 *	U/S = 1
	 *	R/W = 1
	 * 	P = 1
	
	page_table[VIDMEM / ENTRY_SIZE] = VIDMEM | 7;
	*/
	
	/* put page table in page directory */
	page_directory[0] = ((unsigned int)page_table) | (P | RW | US);
	pagedir_cr3(page_directory);
	enablePaging();

	/* call helper function 
	enablePaging(page_directory); */
}


/*
 * void set_process_page(int32_t addr)
 * Description: sets process page (4MB each) in page directory then flushes tlb.
 * Inputs: int32_t addr - physical addr of program
 * Outputs: None
 * Return Value: None
 * Side Effects: None
 */
void set_process_page(int32_t addr)
{
  	page_directory[32]= addr | 0x87;	// ENTRY_4MB(0x80) | P (1) | RW(2) | US(4)

  	flush_tlb();
}


/* 
 * void flush_tlb()
 * Inputs: None
 * Outputs: None
 * Return Value: None
 * Side Effects: Clobbers eax
 * Reference wiki.osdev.org/TLB
 */
void flush_tlb() 
{
	asm volatile (
		"mov %%cr3, %%eax;"
		"mov %%eax, %%cr3;"
		: 
		: 
		: "%eax" 
		);
}


/* 
 * void map2user(uint32_t virt_addr, uint32_t phys_addr, uint32_t dest_page, uint8_t type) 
 * Description: This function maps text-mode vid mem to user space. Either to user program pagetable or vidmem pagetable.
 * Inputs:  uint32_t virt_addr - virtual addr, used to obtain the page directory entry for user program/vidmem pagetable
 *  		uint32_t phys_addr - physical addr to map vid mem 
 * 			uint32_t dest_page - destination page, in user program pagetable
 * 			uint8_t type - determine if mape to user program pagetable or vidmem pagetable
 * Outputs: None
 * Return Value: None
 * Side Effects: flush tlb
 */
void map2user(uint32_t phys_addr, uint32_t dest_page, uint8_t type) 
{

    uint32_t pagedir_entry = 33;
	if (type == 1) {

    	vidmem_pagetable[dest_page] = phys_addr | (US | RW | P); 
		
    	page_directory[pagedir_entry] = ((unsigned int)vidmem_pagetable) | (US | RW | P);
    }
    flush_tlb();
}
/*
void save_vidmem (int32_t tid) 
{
	if (cur_term == running_term)
		page_table[VIDMEM] = ((uint32_t)VIDEO) | US | RW | P;
	else 
		page_table[VIDMEM] = ((uint32_t)(VIDEO + (tid + 1)*ENTRY_SIZE)) | US | RW | P;
	flush_tlb();
}

void restore_vidmem() 
{
	page_table[VIDMEM] = (uint32_t)VIDEO | US | RW | P;
	flush_tlb();
}
*/

/* paging.h - Defines functions used in interacting with page directory and tables */

#ifndef PAGING_H
#define PAGING_H
#include "types.h"

/* flags (https://wiki.osdev.org/Paging) */
#define P   0x01    // Present flag
#define RW  0x02    // Read/Write flag
#define US  0x04    // User/Supervisor flag
#define PAGES_NUM 0x400                      /* Number of pages per directory/table 	*/
#define ENTRY_SIZE 0x1000                   /* Size of page entries 				*/
#define ENTRY_4MB 0x80                            /* 4 MB */
#define KERNEL_ADDR 0x400000				/* Address of kernel					*/
#define VIDMEM 0xB8						/* Address of video memory				*/
#ifndef ASM

extern uint32_t page_directory[PAGES_NUM] __attribute__((aligned(ENTRY_SIZE)));
extern uint32_t page_table[PAGES_NUM] __attribute__((aligned(ENTRY_SIZE)));


//Initializes and enables paging
void initPaging();
void set_process_page(int32_t addr);
void flush_tlb();
void map2user(uint32_t phys_addr, uint32_t dest_page, uint8_t type);
//void save_vidmem (int32_t tid);
//void restore_vidmem();
//Helper function for initPaging used to enable paging
extern void pagedir_cr3(unsigned int*);
extern void enablePaging();

#endif /* PAGING_H */
#endif

/* Header file for filesystems handler functions */
#ifndef _FILESYS_H
#define _FILESYS_H


#include "multiboot.h"
#include "x86_desc.h"
#include "lib.h"
#include "i8259.h"
#include "debug.h"
#include "tests.h"
#include "idt.h"
#include "terminal.h"
#include "paging.h"
#include "filesys.h"
#include "syscall.h"

/* Constants Defined HERE */


#define FILENAME_LEN 32
#define ABS_BLOCK_SIZE 4096
#define NUM_FILES 63
#define DATABLOCK_SIZE 1023
#define RESERVE1 24
#define RESERVE2 52
#ifndef ASM		// ASM

/* Directory Entry data structure */
typedef struct dentry {
		int8_t file_name[FILENAME_LEN];	/* Name of dentry */
		int32_t file_type;			/* (0 = user level access for RTC, 1 = directory, 2 = Regular file) */
		int32_t inode_num;			/* ignored for RTC and dir types, only useful for regular files. */
		int8_t reserved[RESERVE1];		/* Reserved 24B */
} dentry_t;

/* Index node data structure */
typedef struct inode { 
		int32_t length;									// 
		int32_t data_block[DATABLOCK_SIZE];			// 
} inode_t;

/* Boot Block Data Structure */
typedef struct boot_block {
		int32_t dir_count;
		int32_t inode_count;
		int32_t data_count;
		int8_t reserved[RESERVE2];
		dentry_t dir_entries[NUM_FILES];
} boot_block_t;


/*	Data Private to this file				*/

boot_block_t* boot_block;			// will be used to store the filesys boot img addr
uint32_t data_block_length;
uint32_t boot_block_end;
uint32_t boot_block_start;
uint32_t len_inodes;                // len to use in syscall
boot_block_t* boot_block_ptr;
uint32_t data_block_start;
/* FILESYSTEM OPERATIONS helper functions */
void fs_init(module_t *file_sys_boot);

int32_t read_dentry_by_name(const uint8_t* fname, dentry_t* dentry);
int32_t read_dentry_by_index(uint32_t index, dentry_t* dentry);
int32_t read_data(uint32_t inode, uint32_t offset, uint8_t* buf, uint32_t length);

int32_t read_file(int32_t fd, void* buf, int32_t nbytes);
int32_t write_file(int32_t fd, const void* buf, int32_t nbytes);
int32_t close_file(int32_t fd);
int32_t open_file(const uint8_t* filename);

int32_t read_dir(int32_t fd, void* buf, int32_t nbytes);
int32_t write_dir(int32_t fd, const void* buf, int32_t nbytes);
int32_t close_dir(int32_t fd);
int32_t open_dir(const uint8_t* filename);


#endif // ASM

#endif

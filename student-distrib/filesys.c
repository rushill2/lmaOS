#include "filesys.h"

/*
 * void fs_init(module_t* file_sys_boot)
 * Inputs: module_t* file_sys_boot - mod->start init by entry (kernel.c)
 * Outputs: None
 * Return Value: 0 (success), -1 (failure)
 * Side Effects: Reads in the 
 */
void fs_init(module_t* file_sys_boot){
	boot_block = (boot_block_t*)file_sys_boot->mod_start;		// initalizes boot_block_t ptr to be pointing to start of filesys bootimg
	//boot_block_start = (uint32_t)file_sys_boot;
	//boot_block_ptr = (boot_block_t*)((uint32_t)(file_sys_boot->mod_start));
	/* Length until start of inode section */
	boot_block_end = (uint32_t)((file_sys_boot->mod_start) + ABS_BLOCK_SIZE);
	/* Length from start of Inodes to start of Datablocks */
	len_inodes = (uint32_t)(boot_block->inode_count * ABS_BLOCK_SIZE);
	/* Length until start of data blocks */
	//data_block_length = (boot_block_end +  len_inodes);
	data_block_start = (unsigned int)boot_block + (boot_block->inode_count+1)*ABS_BLOCK_SIZE;
}

/* Function: read_dentry_by_name
 * Description: searches filesystem for a file by its file name and copies its dentry
 * Inputs:
 * 	fname - name of file to search for
 *	dentry - dentry_t struct to write to
 * Outputs:	returns 0 if dentry was copied, otherwise -1 is returned.
 * Side Effects: dentry for file is copied to dentry input
 */
int32_t read_dentry_by_name(const uint8_t* fname, dentry_t* dentry)
{
	/* Initialize Variables used */
	int32_t num_dir_entries;
	uint32_t dir_entry_idx;
	dentry_t* current;
	int32_t match;
	/* Set current to point to the first fname in the current directory entry */
	current = &boot_block->dir_entries[0];
	/* Set the number of directory entries count for looping */
	num_dir_entries = boot_block->dir_count;
	/* Scan through all the dir entries in the bootlock */
	for(dir_entry_idx = 0; dir_entry_idx < num_dir_entries; dir_entry_idx++){
		/* Go through all dir entries and find the fname that matches the given input's */
		match = 0;
		match = strncmp((int8_t*)current->file_name,(int8_t*)fname, FILENAME_LEN);
		if(match == 0){
			read_dentry_by_index(dir_entry_idx, dentry);
			return 0;
		}
		current++;	// increment current to point to next fname
	}
	// Return -1 if the fnames do not match
	return -1; 
}

/* Function: read_dentry_by_index
 * Description: searches filesystem for file by its index and copies its dentry
 * Inputs:
 *	index - index of file to search for
 *	dentry - dentry_t struct to write to
 * Outputs: returns 0 if dentry was copied, otherwise -1 is returned.
 * Side Effects: dentry for file is copied to dentry input
 */
int32_t read_dentry_by_index(uint32_t index, dentry_t* dentry)
{
	/* Else call memcpy to populate the dentry parameters */

	//strlen((int8_t*)&boot_block->dir_entries[index])
	if(index < 0 || index >= boot_block->dir_count){
		return -1;
	}
	// get the current dentry we want to read
	dentry_t* current_dentry = &boot_block->dir_entries[index];
	// fill in the paramter values for dentry struct
	strncpy(dentry->file_name, current_dentry->file_name,FILENAME_LEN);
	dentry->file_type = current_dentry->file_type;
	dentry->inode_num = current_dentry->inode_num;

	return 0;
}

/*
 * int32_t read_data(uint32_t inode, uint32_t offset, uint8_t* buf, uint32_t length)
 * Inputs: an inode num, offset = where to start reading from, buf = buffer to write to,length= position to read until
 * Outputs: none 
 * Return Value: bytes read into buffer
 * Side Effects: Reads in the data from the filesystem memory
 */
int32_t read_data(uint32_t inode, uint32_t offset, uint8_t* buf, uint32_t length){

	// Initialize all FileSystem address variables 
	uint32_t data_bytes,datablock,byte_offset;
	uint8_t* curr_datablock_loc;
	inode_t* curr_inode;
	int i;
	// Check if given inode number is invalid 
	if( inode < 0 || inode >= boot_block->inode_count)
		return 0;
	// Current Inode pointer
	curr_inode = (inode_t*)(boot_block_end + (inode * ABS_BLOCK_SIZE));
	// index to get datablock number from the current inode 
	datablock  = offset / ABS_BLOCK_SIZE;			
	// how many bytes to start from in each datablock 
	byte_offset = offset % ABS_BLOCK_SIZE;	
	//	compute Current Datablock's location start 
	curr_datablock_loc = (uint8_t*) (data_block_start + (curr_inode->data_block[datablock] * ABS_BLOCK_SIZE) + byte_offset);
	// Initialize the number of bytes to copy to buffer to be ABS_BLOCK_SIZE, unless changed 
	data_bytes = ABS_BLOCK_SIZE;
	for(i = 0; i < length; i ++){
		// Check if we have reached end of current inode
		if(i + offset >= curr_inode->length ) {
			return i;
		}	
		// Check if we have reached end of a datablock
		if(byte_offset >= data_bytes) {
			// go to next datablock
			datablock++;
			// reset byte offset at beginning of each datablock
			byte_offset = 0;
			//compute the address of the datablock's data and also factor in an offset to determine how many bytes of the datablock we should copy
			curr_datablock_loc = (uint8_t *)(data_block_start + (curr_inode->data_block[datablock] * data_bytes));
		}
		// Copy over specified amount of bytes to the buffer 
		buf[i] = *curr_datablock_loc;
		// move ptr to get address of next datablock 
		curr_datablock_loc++;
		//increment byte offset
		byte_offset++;
	}

	return i;

}


/*
 * int32_t read_file(int32_t fd, void* buf, int32_t nbytes)
 * Inputs: 	int32_t fd - file descriptor
 * 			void* buf - buffer to read
 * 			int32_t nbytes - number of bytes to read
 * Outputs: None
 * Return Value: bytes read into buffer
 * Side Effects: Reads in a file and writes to the buffer
 */
int32_t read_file(int32_t fd, void* buf, int32_t nbytes)
{
	//pcb_t* current_process = get_pcb_address();

	//return read_data(current_process->file[fd].inode, current_process->file[fd].pos, buf, nbytes);
	pcb_t* current_process = get_pcb_address();

	int32_t nbytes_read;

	nbytes_read = read_data(current_process->file[fd].inode, current_process->file[fd].pos, buf, nbytes);

	current_process->file[fd].pos += nbytes_read;

	return nbytes_read;

}

/*
 * int32_t write_file(int32_t fd, const void* buf, int32_t nbytes)
 * Inputs:  int32_t fd - file descriptor
 * 			void* buf - buffer to write
 * 			int32_t nbytes - number of bytes to write
 * Outputs: None
 * Return Value: bytes read into buffer
 * Side Effects: Reads a buf and write the output
 */
int32_t write_file(int32_t fd, const void* buf, int32_t nbytes)
{
	return -1;
}

/*
 * int32_t close_file(int32_t fd)
 * Inputs: int32_t fd - file descriptor
 * Outputs: None
 * Return Value: 0 
 * Side Effects: None
 */
int32_t close_file(int32_t fd)
{
	return 0;
}

/*
 * int32_t open_file(const uint8_t* filename)
 * Inputs: const uint8_t* filename - filename
 * Outputs: None
 * Return Value: -1 (failure), fd
 * Side Effects: None
 */
int32_t open_file(const uint8_t* filename)
{
	dentry_t dentry;

	if (read_dentry_by_name(filename, &dentry) == -1) {
		return -1;
	}

	pcb_t* cur_process = get_pcb_address();

	int i;	// loop index
	for(i = 0; i < 8; i++) {
		if(cur_process->file[i].flags == 0) {
			return i;
		}
	}

	return -1;
	
}
int32_t read_dir_idx = 0;
/*
 * int32_t read_dir(int32_t fd, void* buf, int32_t nbytes)
 * Inputs: int32_t fd, void* buf, int32_t nbytes
 * Outputs: None
 * Return Value: -1 (read-only)
 * Side Effects: None
 */
int32_t read_dir(int32_t fd, void* buf, int32_t nbytes)
{
	dentry_t dentry;
	//int i = 0;
	/*
	pcb_t* cur_process = get_pcb_address();

	if(read_dentry_by_index(cur_process->file[fd].pos, &dentry) == -1) {
		return 0;
	}

	cur_process->file[fd].pos++;

	memcpy(buf, dentry.file_name, nbytes);

	return nbytes;
	*/
    int i = 0;
	pcb_t* cur_process = get_pcb_address();

    if(read_dentry_by_index(cur_process->file[fd].pos, &dentry) == 0){
		
        for(i = 0; i < FILENAME_LEN +1; i++){
            ((int8_t*)(buf))[i] = '\0';
        }
		
        strncpy((int8_t*)buf, (int8_t*)dentry.file_name, nbytes);
        cur_process->file[fd].pos++;
		int32_t len = strlen((int8_t*)dentry.file_name);
		// max file name len is 32
		if (len > 32) {
			len = 32;
		}
        return len;
    }
    else{
        return 0;
    }
}

/* 
 * int32_t write_dir(int32_t fd, const void* buf, int32_t nbytes)
 * Inputs:  int32_t fd - file descriptor
 * 			const void* buf - buffer to write
 * 			int32_t nbytes - number of bytes to write
 * output:none 
 * return: -1 (read-only)
 * Side Effects: None
 */
int32_t write_dir(int32_t fd, const void* buf, int32_t nbytes)
{
	return -1;
}

/*
 * int32_t close_dir(int32_t fd)
 * Inputs: int32_t fd - file descriptor
 * Outputs: None
 * Return Value: 0
 * Side Effects: None
 */
int32_t close_dir(int32_t fd)
{
	return 0;
}

/*
 * int32_t open_dir(const uint8_t* filename)
 * Inputs: const uint8_t* filename - filename to open
 * Outputs: None
 * Return Value: 0
 * Side Effects: None
 */
int32_t open_dir(const uint8_t* filename)
{
	
	dentry_t dentry;

	if (read_dentry_by_name(filename, &dentry) == -1) 
		return -1;

	pcb_t* cur_process = get_pcb_address();

	int i;
	for(i = 0; i < 8; i++) {
		if(cur_process->file[i].flags == 0) {
			return i;
		}
	}
	return -1;

}

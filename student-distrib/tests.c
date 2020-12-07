#include "tests.h"
#include "x86_desc.h"
#include "lib.h"
#include "filesys.h"
#include "terminal.h"
#include "rtc.h"
#include "paging.h"
#include "syscall.h"

#define PASS 1
#define FAIL 0
#define PARTIAL 500

/* format these macros as you see fit */
#define TEST_HEADER 	\
	printf("[TEST %s] Running %s at %s:%d\n", __FUNCTION__, __FUNCTION__, __FILE__, __LINE__)
#define TEST_OUTPUT(name, result)	\
	printf("[TEST %s] Result = %s\n", name, (result) ? "PASS" : "FAIL");

static inline void assertion_failure(){
	/* Use exception #15 for assertions, otherwise
	   reserved by Intel */
	asm volatile("int $15");
}


/* Checkpoint 1 tests */

/* IDT Test - Example
 * 
 * Asserts that first 10 IDT entries are not NULL
 * Inputs: None
 * Outputs: PASS/FAIL
 * Side Effects: None
 * Coverage: Load IDT, IDT definition
 * Files: x86_desc.h/S
 */
int idt_test(){
	TEST_HEADER;

	int i;
	int result = PASS;
	for (i = 0; i < 10; ++i){
		if ((idt[i].offset_15_00 == NULL) && 
			(idt[i].offset_31_16 == NULL)){
			assertion_failure();
			result = FAIL;
		}
	}

	return result;
}

// add more tests here
/* =================================================paging tests============================= */

/* 
 * void null_test()
 * Description: This function deref NULL.
 * Inputs: none
 * Outputs: Page Fault.
 * Side Effects: Kernel freeze.
 */
void null_test(){
	TEST_HEADER;
	int* ptr = NULL;
	int PF;
	PF = *(ptr);
}


/* 
 * void PF_test()
 * Description: This function deref an uninitialized page.
 * Inputs: none
 * Outputs: PF
 * Side Effects: Kernel freeze.
 */
void PF_test(){
	TEST_HEADER;
	int* ptr = (int*)(0x800000 + 1);
	int PF;
	PF = *(ptr);
}


/* 
 * int VM_paging_test()
 * Description: This function deref an addr in vidmem.
 * Inputs: none
 * Outputs: none
 * Return Value: PASS 1/ FAIL 0
 * Side Effects: none
 */
int VM_paging_test(){
	TEST_HEADER;
	int * ptr = (int*)(0xB8000 + 1);
	int VM;
	VM = *ptr;
	return PASS;
}

/* 
 * int KM_paging_test()
 * Description: This function deref an addr in kernel.
 * Inputs: none
 * Outputs: none
 * Return Value: PASS 1/ FAIL 0
 * Side Effects: none
 */
int KM_paging_test(){
	TEST_HEADER;
	int * ptr = (int*)(0x400000 + 1);
	int KM;
	KM = *ptr;
	return PASS;
}

/* =============================== rtc test ====================================== */

/*
 * int rtc_test()
 * Description: This function tests rtc interrupt handler.
 * Return Value: PASS
 * Side Effects: rtc interrupts enabled
 */
int rtc_test() {
	TEST_HEADER;
	rtc_intr();

	return PASS;
}


/* Checkpoint 2 tests */
// terminal tests
// test for an input buffer with length <= MAX_BUF_SIZE
int terminal_test1(){
	TEST_HEADER;
	//const int8_t* buf2 = "\nterminal read/write test: ";
	//uint8_t buf2len = (uint8_t)strlen(buf2);
	char buf[50];

//	while(1) {

		//terminal_write(0, buf2, buf2len);

		terminal_read(0, buf, 49);
		terminal_write(0, buf, 49);

//	}
	return PASS;
}
// test for an input buffer with length > MAX_BUF_SIZE
int terminal_test2(){
	TEST_HEADER;
	//const int8_t* buf2 = "\nterminal read/write test: ";
	//uint8_t buf2len = (uint8_t)strlen(buf2);
	char buf[300];

//	while(1) {

		//terminal_write(0, buf2, buf2len);

		terminal_read(0, buf, 299);
		terminal_write(0, buf, 299);

//	}
	return PASS;
}

// filesys tests
void read_dir_test(){
	
	int32_t fd = 0;
	int32_t nbytes = 32;
	int32_t read_result = 1;
	int8_t buf[33];
	
	while(read_result != 0){
		read_result = read_dir(fd, buf, nbytes);
		puts(buf);
		putc('\n');
	}
}

//void read_file_test(){
	//int temp;
	//int32_t fd;
	//int i=0;
	//int8_t file_str[PARTIAL];					// does a partial read of 500 chars for the file
	//while(i < PARTIAL){
	//	file_str[i] = 0;
    //i++;
	//}
	//temp = read_file(fd, file_str, 1, 6000);
	//puts(file_str);
//}
/*
void read_file_test(int32_t index, uint32_t size){
    dentry_t dentry;
    read_dentry_by_index(index, &dentry);
    int8_t buf[size];
	int i;
	for(i = 0; i < size; i++) buf[i] = '\0';
    read_data(dentry.i_node, 0, buf, size);
    for(i = 0; i < size; i++) putc(buf[i]);
}

void read_dentry_name_test(char* name){
	
	int8_t buf[33];
	int8_t fname[33];
	strncpy((int8_t*)fname, name, 33);
	printf("Looking for file: ");
	puts((int8_t*)fname);
	putc('\n');
	dentry_t dentry;
	read_dentry_by_name(fname, &dentry);
	strncpy(buf, dentry.file_name, LEN_NAME);
	buf[32] = '\0';
	printf("Found file: ");
	puts(buf);
	putc('\n');
	
}

*/

/* ====================== rtc tests for cp2 ============================== */

/*
 * int rtc_open_test()
 * open the rtc and set frequency to 2 Hz
 * Return Value: PASS
 */
int rtc_open_test(){
	TEST_HEADER;
	rtc_open((uint8_t*)(0));
	return PASS;
}

/*
 * int rtc_write_test()
 * Description: This functions sets the frequency to a certain value (e.g. 512 Hz)
 * Return Value: PASS
 * Side Effects: rtc_intr() prints indicator msg for test
 */
int rtc_write_test(){
	TEST_HEADER;
	rtc_open((uint8_t*)(0));
	int32_t buf[10];				// allocate enough space for buff
	buf[0] = 32;					//set the frequency to 512
	rtc_write(0, buf, 4);			//4 is the length to be written
	return PASS;
}

/*
 * void rtc_combined_test()
 * Description: This function tests rtc at various frequencies.
 * Side Effects: Print instructions for test case.
 */
void rtc_combined_test() 
{

	TEST_HEADER;
	int key_pressed;

	printf("\nrtc tests:\nEnter 1 to test 8Hz\nEnter 2 to test 32Hz\nEnter 3 to test 1024 Hz\nEnter 4 to test open/close/read/write\n");

	while(key_pressed != 0x02 && key_pressed != 0x03 && key_pressed != 0x04 && key_pressed != 0x05) {
		key_pressed = inb(0x60);
	}

	clear();

	if(key_pressed == 0x02 || key_pressed == 0x03 || key_pressed == 0x04) {
		printf("\n");
		int32_t rval;
		if(key_pressed == 0x02) {
			rval = set_frequency(8);
			if (rval == 0) 
				printf("PASS\n\n");
			else 
				printf("FAIL\n\n");
		} else if(key_pressed == 0x03) {
			rval = set_frequency(32);
			if (rval == 0) 
				printf("PASS\n\n");
			else 
				printf("FAIL\n\n");
		} else if(key_pressed == 0x04) {
			rval = set_frequency(1024);
			if (rval == 0) 
				printf("PASS\n\n");
			else 
				printf("FAIL\n\n");
		} 
	} else if(key_pressed == 0x05) {
		printf("rtc_read test\n    Result: ");
		int32_t freq = 16;
		rtc_write(5, &freq, 4);
	
		int32_t rval = rtc_read(5, &freq, 4);
		if (rval == 0)
			printf("PASS\n\n");
		else
			printf("FAIL\n\n");
	
		printf("rtc_write test (valid params 16Hz)\n    Result: ");
		freq = 16;
		rval = rtc_write(4, &freq, 4);
		if (rval == 0)
			printf("PASS\n");
		else
			printf("FAIL\n");
	
		printf("rtc_write test (invalid params 7Hz)\n    Result: ");
		freq = 7;
		rval = rtc_write(0, &freq, 4);
		if (rval == -1)		
			printf("PASS\n");
		else
			printf("FAIL\n");
	
		
		printf("rtc_open test (32Hz)\n    Result: ");
		freq = 32;
		rtc_write(2, &freq, 4);
		uint8_t temp = (uint8_t)freq;
		rval = rtc_open(&temp);
		if (rval != 0)
			printf("FAIL\n");
		else
			printf("PASS\n");
	
		printf("rtc_close test\n    Result: ");
		rval = rtc_close(2);
		if (rval != 0)
			printf("FAIL\n");
		else
			printf("PASS\n");
	}
}


/* Checkpoint 3 tests */
int execute_test(){
	TEST_HEADER;
	execute((uint8_t*)"shell");
	return PASS;
}
/* Checkpoint 4 tests */
/* Checkpoint 5 tests */


/* Test suite entry point */
void launch_tests(){
	/* clear screen and reset cursor*/
	/*
	clear();
	set_screen_x(0);
	set_screen_y(0);
	int x; 
	x= get_screen_x();
	int y; 
	y = get_screen_y();
	update_cursor(x,y);
	*/
	//TEST_OUTPUT("idt_test", idt_test());
	// launch your tests here

	/* cp1 tests */
	//null_test();
	//PF_test();
	//TEST_OUTPUT("vidmem PF test", VM_paging_test());
	//TEST_OUTPUT("kernel PF test", KM_paging_test());

	/* cp2 tests */	
	//terminal_test1();
	//terminal_test2();
	//read_dir_test();
	//read_file_test(10, 187);
	//TEST_OUTPUT("read_data_test", read_data_test());
	//read_dentry_name_test();
	//TEST_OUTPUT("rtc_open_test", rtc_open_test());
	//TEST_OUTPUT("rtc_write_test", rtc_write_test());
	//rtc_combined_test();
	//rtc_test();
	/* cp3 tests */
	//TEST_OUTPUT("execute_test", execute_test());
}


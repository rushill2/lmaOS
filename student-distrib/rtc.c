#include "rtc.h"
#include "intr_handler.h"
#include "lib.h"
#include "idt.h"

volatile int32_t int_flag = 0; // init flag for interrupts


/*
 * rtc_init ()
 * Description: Initialize RTC and interrupt handlers.
 * Inputs: None
 * Outputs: None
 * Return Value: None
 * Side Effects: Enable RTC input and write register C to port 0x70
 */
void 
rtc_init ()
{
	cli();
	/* set freq and write register A to port 0x70 */
	outb(RTC_A, RTC_PORT);
	char prev = inb(RTC_DATA);
	outb(RTC_A,RTC_PORT);
	outb(((prev & W_MASK) | FREQ), RTC_DATA);

	/* enable IR8 and set period */
	outb(RTC_B, RTC_PORT);
	prev = inb(RTC_DATA);
	outb(RTC_B, RTC_PORT);
	outb((prev | ENABLE), RTC_DATA);
	enable_irq(IRQ8);
	sti();
}


/*
 * void rtc_intr ()
 * Description: Receive and handle RTC interrupts.
 * Inputs: None
 * Outputs: None
 * Return Value: None
 * Side Effects: Handle RTC interrupt and write register C to port 0x70
 */
void 
rtc_intr ()
{
	cli ();

    /* CP1 rtc test */
    //test_interrupts(); 
	/* CP2 rtc test */
	//printf("391 ");
    /* end of interrupt signal */
	send_eoi(IRQ8);
	int_flag = 1;
	
	outb(RTC_C, RTC_PORT);
	inb(RTC_DATA);

	enable_irq(IRQ8);
	
    sti ();
}


/* 
 * int32_t rtc_read (int32_t fd, void* buf, int32_t nbytes)
 * Description:	This function reads a rtc_type file.
 * Inputs: int32_t fd - file descriptor of file to read
 * 		   void* buf - buffer to read
 *		   int32_t nbytes - number of bytes to read
 * Outputs: None
 * Return Value: None
 * Side Effects: write function knows what to write
 */
int32_t rtc_read (int32_t fd, void* buf, int32_t nbytes) 
{
	int_flag = 0;	// make sure goes into while loop
	while(!int_flag) {
		
	}
	// interrupt occurred
	int_flag = 0;	// reset interrupt flag

	return 0;
}


/*
 * int32_t rtc_write (int32_t fd, const void* buf, int32_t nbytes)
 * Description: This function writes a file if it's rtc_type.
 * Inputs: int32_t fd - file descriptor of file to read
 * 		   void* buf - buffer to read
 *		   int32_t nbytes - number of bytes to read
 * Outputs: None
 * Return Value: -1 if failure, number of bytes to write if success 
 * Side Effects: write content in buffer to rtc
 */
int32_t rtc_write (int32_t fd, const void* buf, int32_t nbytes) 
{
	cli();
	// check if input length is 4 and buf isn't null
	if(fd == 0 || fd == 1|| nbytes != 4 || buf == NULL) {
		return -1;
	}
	// set freq from input buffer
	int32_t freq = *((int32_t*) buf);
	// if frequency not power of 2, return failure
	if (set_frequency(freq) != 0) {
		return -1;
	}
	sti();
	return nbytes;
}


/*
 * int32_t rtc_open (const uint8_t* filename) 
 * Description: This function opens a file if it's rtc_type and sets default frequency of 2 Hz
 * Inputs: const uint8_t* filename - name of file to be opened
 * Outputs: None
 * Return Value: -1 if failure, 0 if success
 * Side Effects: File can be read.
 */
int32_t rtc_open (const uint8_t* filename) 
{
	if (filename == NULL) 
		return -1;
	return set_frequency(2);
}


/*
 * int32_t rtc_close(int32_t fd)
 * Description: This function close a file if it is a rtc_type and disables the rtc irq
 * Inputs: int32_t fd - file descriptor of file to close
 * Outputs: None
 * Return Value: -1 if failure, 0 if success
 * Side Effects: File is closed.
 */
int32_t rtc_close (int32_t fd) 
{
	if (fd < 2) {
		return -1;
	}
	return 0;
}


/* 
 * int32_t set_frequency (int32_t target_frequency) 
 * Description:	This function sets RTC frequency. (must be power(s) of two and <= 1024)
 * Inputs: int32_t target_frequency - frequency to set
 * Outputs: None
 * Return Value: -1 if failure, 0 if success
 * Side Effects: set rtc frequency
 */
int32_t set_frequency (int32_t target_frequency) 
{
	char freq_hexcode;

	switch(target_frequency) {
		case 2:
			freq_hexcode = 0x0F;
			break;
    	case 4:
    	    freq_hexcode = 0x0E;
    	    break;
    	case 8:
    	    freq_hexcode = 0x0D;
    	    break;
    	case 16:
    	    freq_hexcode = 0x0C;
    	    break;
    	case 32:
    	    freq_hexcode = 0x0B;
    	    break;
    	case 64:
    	    freq_hexcode = 0x0A;
    	    break;
    	case 128:
    	    freq_hexcode = 0x09;
    	    break;
    	case 256:
    	    freq_hexcode = 0x08;
    	    break;
    	case 512:
    	    freq_hexcode = 0x07;
    	    break;
    	case 1024:
    	    freq_hexcode = 0x06;
    	    break;
    	default:
    	    return -1;
    }
	cli();
    outb(RTC_A, RTC_PORT);
	char prev = inb(RTC_DATA);
	outb(RTC_A, RTC_PORT);
	outb(((prev & W_MASK) | freq_hexcode), RTC_DATA);
	sti();
	//printf("%x\n", freq_hexcode);
    return 0;
}


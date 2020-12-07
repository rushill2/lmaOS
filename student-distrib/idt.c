#include "x86_desc.h"
#include "idt.h"
#include "intr_handler.h"
#include "keyboard.h"
#include "rtc.h"
#include "i8259.h"
#include "syscall.h"

void idt_0();
void idt_1();
void idt_2();
void idt_3();
void idt_4();
void idt_5();
void idt_6();
void idt_7();
void idt_8();
void idt_9();
void idt_10();
void idt_11();
void idt_12();
void idt_13();
void idt_14();
void idt_15();
void idt_16();
void idt_17();
void idt_18();
void idt_19();
void reserved ();
void non_reserved ();
//void sys_call();

/*
 * idt_init ()
 * Description: Initialize IDT with entries for exceptions, a few interrupts, and system calls.
 *
 *
 */
void 
idt_init ()
{
    int i; // loop index
    for (i = 0; i < NUM_VEC; i++) {
        idt[i].seg_selector = KERNEL_CS;
        idt[i].reserved4 = 0;
        if (i >= 0 && i < 32 && i != 3) // fault/trap
            idt[i].reserved3 = 1;
        else                            // interrupt
            idt[i].reserved3 = 0;
        idt[i].reserved2 = 1;
        idt[i].reserved1 = 1;
        idt[i].size = 1;
        idt[i].reserved0 = 0;
        if (i == 128) {
            idt[i].dpl = 3;
            idt[i].reserved3 = 1;
        }
        else 
            idt[i].dpl = 0;
        idt[i].present = 1;
    }
    SET_IDT_ENTRY (idt[0], idt_0);
    SET_IDT_ENTRY (idt[1], idt_1);
    SET_IDT_ENTRY (idt[2], idt_2);
    SET_IDT_ENTRY (idt[3], idt_3);
    SET_IDT_ENTRY (idt[4], idt_4);
    SET_IDT_ENTRY (idt[5], idt_5);
    SET_IDT_ENTRY (idt[6], idt_6);
    SET_IDT_ENTRY (idt[7], idt_7);
    SET_IDT_ENTRY (idt[8], idt_8);
    SET_IDT_ENTRY (idt[9], idt_9);
    SET_IDT_ENTRY (idt[10], idt_10);
    SET_IDT_ENTRY (idt[11], idt_11);
    SET_IDT_ENTRY (idt[12], idt_12);
    SET_IDT_ENTRY (idt[13], idt_13);
    SET_IDT_ENTRY (idt[14], idt_14);
    SET_IDT_ENTRY (idt[15], idt_15);
    SET_IDT_ENTRY (idt[16], idt_16);
    SET_IDT_ENTRY (idt[17], idt_17);
    SET_IDT_ENTRY (idt[18], idt_18);
    SET_IDT_ENTRY (idt[19], idt_19);
    for (i = 20; i < 32; i++) SET_IDT_ENTRY (idt[i], reserved);
    for (i = 32; i < 256; i++) SET_IDT_ENTRY (idt[i], non_reserved);
    SET_IDT_ENTRY (idt[32], pit_handler);
    SET_IDT_ENTRY (idt[33], kb_handler);
    SET_IDT_ENTRY (idt[40], rtc_handler);
    SET_IDT_ENTRY (idt[128], syscall_handler);
}




/* 
 * exception_lookup (int idx) 
 * Description: Lookup table of exceptions and interrupts, get the index of idt and return a msg.
 * 
 * 
void 
exception_lookup (int idx)  
{
    if (idx == 0) 
    if (idx == 1) printf ("RESERVED \n");
    if (idx == 2) printf ("NMI interrupt \n");
    if (idx == 3) 
    if (idx == 4) 
    if (idx == 5) 
    if (idx == 6) 
    if (idx == 7) 
    if (idx == 8) 
    if (idx == 9) 
    if (idx == 10) 
    if (idx == 11) 
    if (idx == 12) 
    if (idx == 13) 
    if (idx == 14) 
    if (idx == 15) 
    if (idx == 16) 
    if (idx == 17) 
    if (idx == 18) 
    if (idx == 19) 
    if (idx >= 20 && idx < 32) {
        
    }
    if (idx >= 32 && idx < 256) {
    }
    // system call at entry 0x80 
    //if (idx == 128) printf ("System calls \n");
    //cli ();
    //while (1);
//} */ 

void idt_0() 
{
    printf ("Divide Error \n");
	exception_status = EXCEPTION_FLAG;
    halt((uint8_t)EXCEPTION_FLAG);
}


void idt_1()
{
    printf ("RESERVED \n");
	exception_status = EXCEPTION_FLAG;
    halt((uint8_t)EXCEPTION_FLAG);
}

void idt_2()
{
    printf ("NMI interrupt \n");
	exception_status = EXCEPTION_FLAG;
    halt((uint8_t)EXCEPTION_FLAG);
}
void idt_3()
{
    printf ("Breakpoint \n");
	exception_status = EXCEPTION_FLAG;
    halt((uint8_t)EXCEPTION_FLAG);
}
void idt_4() 
{
    printf ("Overflow \n");
	exception_status = EXCEPTION_FLAG;
    halt((uint8_t)EXCEPTION_FLAG);
}
void idt_5() {
    printf ("BOUND Range Exceeded \n");
	exception_status = EXCEPTION_FLAG;
    halt((uint8_t)EXCEPTION_FLAG);

}
void idt_6() {
    printf ("Invalid Opcode (Undefined Opcode) \n");
	exception_status = EXCEPTION_FLAG;
    halt((uint8_t)EXCEPTION_FLAG);

}
void idt_7() {
    printf ("Device Not Available (No Math Coprocessor) \n");
	exception_status = EXCEPTION_FLAG;
    halt((uint8_t)EXCEPTION_FLAG);
}
void idt_8() {
    printf ("Double fault \n");
	exception_status = EXCEPTION_FLAG;
    halt((uint8_t)EXCEPTION_FLAG);
}
void idt_9() {
    printf ("Coprocessor Segment Overrun (reserved) \n");
	exception_status = EXCEPTION_FLAG;
    halt((uint8_t)EXCEPTION_FLAG);
}
void idt_10() {
    printf ("Invalid TSS \n");
	exception_status = EXCEPTION_FLAG;
    halt((uint8_t)EXCEPTION_FLAG);
}
void idt_11() {
    printf ("Segment Not Present \n");
	exception_status = EXCEPTION_FLAG;
    halt((uint8_t)EXCEPTION_FLAG);
}
void idt_12() {
    printf ("Stack-Segment Fault \n");
	exception_status = EXCEPTION_FLAG;
    halt((uint8_t)EXCEPTION_FLAG);
}
void idt_13() {
    printf ("General Protection \n");
	exception_status = EXCEPTION_FLAG;
    halt((uint8_t)EXCEPTION_FLAG);
}
void idt_14() {
    printf ("Page Fault \n");
    exception_status = EXCEPTION_FLAG;
    halt((uint8_t)EXCEPTION_FLAG);
}
void idt_15() {
    printf ("(Intel reserved. Do not use.) \n");
	exception_status = EXCEPTION_FLAG;
    halt((uint8_t)EXCEPTION_FLAG);
}
void idt_16() {
    printf ("x87 FPU Floating-Point Error (Math Fault) \n");
	exception_status = EXCEPTION_FLAG;
    halt((uint8_t)EXCEPTION_FLAG);
}
void idt_17() {
    printf ("Alignment Check \n");
	exception_status = EXCEPTION_FLAG;
    halt((uint8_t)EXCEPTION_FLAG);
}
void idt_18() {
    printf ("Machine Check \n");
	exception_status = EXCEPTION_FLAG;
    halt((uint8_t)EXCEPTION_FLAG);
}
void idt_19() {
    printf ("SIMD Floating-Point Exception \n");
	exception_status = EXCEPTION_FLAG;
    halt((uint8_t)EXCEPTION_FLAG);
}
void reserved () {
    printf ("Intel reserved. Do not use. \n");
	exception_status = EXCEPTION_FLAG;
    halt((uint8_t)EXCEPTION_FLAG);
}
void non_reserved () {
    printf ("User Defined (Non-reserved) Interrupts \n");
	exception_status = EXCEPTION_FLAG;
    halt((uint8_t)EXCEPTION_FLAG);
}

/* Replaced in cp3
void sys_call() {
    printf ("System calls \n");
    while (1);
}
*/



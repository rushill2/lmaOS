#ifndef SCHEDULING_H
#define SCHEDULING_H

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

#define CHANNEL_0   0x40    // channel 0
#define MODE_3     0x36     // mode 3 square wave
#define MC_REG      0x43    // mode/cmd reg
#define PIT_IRQ     0x00    // pit has the highest prio
#define FALLING_EDGE    1193182  
#define RELOAD_VALUE    100 // actual freq
#ifndef ASM

int32_t running_term;   // id of running terminal


void init_pit();
void pit_intr();
void scheduling();
pcb_t* get_pcb_args(uint8_t term);
uint8_t get_running();

#endif
#endif

#ifndef _INTR_HANDLER_H_
#define _INTR_HANDLER_H_
#ifndef ASM

#include "x86_desc.h"
#include "keyboard.h"
#include "rtc.h"

extern void kb_handler();
extern void rtc_handler();
extern void syscall_handler();
extern void pit_handler();

#endif
#endif

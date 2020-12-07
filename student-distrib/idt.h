#ifndef _IDT_H_
#define _IDT_H_

#include "lib.h"
#include "types.h"
#include "x86_desc.h"
#include "intr_handler.h"

#ifndef ASM
#define EXCEPTION_FLAG 256
void idt_init();


#endif 
#endif 



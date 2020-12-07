#define ASM 1
#include "intr_handler.h"
#include "lib.h"
#include "idt.h"
#include "x86_desc.h"
#include "i8259.h"
#include "keyboard.h"
#include "rtc.h"
#include "syscall.h"
#include "scheduling.h"
.text

.globl kb_handler
.globl rtc_handler
.globl pit_handler
.globl syscall_handler

.align 4

# void kb_handler(void);
# Handles interrupts from keyboard and calls kb_intr C function
# Inputs	: none
# Outputs	: none
# Registers	: Standard C calling conventions

kb_handler:
	pushl	%ebp
	pushl	%eax
	pushl	%ebx
	pushl	%ecx
	pushl	%edx
	pushfl
	call	kb_intr
	popfl
	popl	%edx
	popl	%ecx
	popl	%ebx
	popl	%eax
	popl	%ebp
	iret

# void rtc_handler(void);
# Handles interrupts from the rtc and calls rtc_intr C function
# Inputs	: none
# Outputs	: none
# Registers	: Standard C calling conventions

rtc_handler:
	pushl	%ebp
	pushl	%eax
	pushl	%ebx
	pushl	%ecx
	pushl	%edx
	pushfl
	call	rtc_intr
	popfl
	popl	%edx
	popl	%ecx
	popl	%ebx
	popl	%eax
	popl	%ebp
	iret

# void pit_handler(void);
# Handles interrupts from pit and calls pit_intr C function
# Inputs	: none
# Outputs	: none
# Registers	: Standard C calling conventions

pit_handler:
	pushl	%ebp
	pushl	%eax
	pushl	%ebx
	pushl	%ecx
	pushl	%edx
	pushfl
	call	pit_intr
	popfl
	popl	%edx
	popl	%ecx
	popl	%ebx
	popl	%eax
	popl	%ebp
	iret

# Jumptable used by syscall_handler
syscall_jumptable:
	.long 0x0	# skip
	.long halt
	.long execute
	.long read
	.long write
	.long open
	.long close
	# functions for next checkpoint
	.long getargs
	.long vidmap
	.long set_handler
	.long sigreturn

# void syscall_handler(void);
# Handles interrupts from system calls and calls the applicable C function using the jumptable
# Inputs	: none
# Outputs	: none
# Registers	: Standard C calling conventions

syscall_handler:
	pushl	%edi
	pushl	%esi
	pushl	%edx
	pushl	%ecx
	pushl	%ebx
	pushfl
	# check if the call exists
	cmpl	$8, %eax
	ja		syscall_error
	cmpl	$1, %eax
	jb		syscall_error

	# store edx, ecx, ebx in such order (3rd, 2nd, 1st args)
	pushl	%edx
	pushl	%ecx
	pushl	%ebx
	# call a function using a jumptable
	call	*syscall_jumptable(, %eax, 4) 
	# return from jumptable call
	addl	$12, %esp # tear down the stack
	cmpl	$-1, %eax # check return error
	je		syscall_error
	popfl
	popl	%ebx
	popl	%ecx
	popl	%edx
	popl	%esi
	popl	%edi
	iret

# int syscall_error(void);
# Handles errors from syscall_handler
# Inputs	: none
# Outputs	: Returns an integer -1 to indicate failure
# Registers	: Standard C calling conventions
syscall_error:
	movl	$-1, %eax # return -1 (failure)
	popfl
	popl	%ebx
	popl	%ecx
	popl	%edx
	popl	%esi
	popl	%edi
	iret

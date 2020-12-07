# paging_asm.S - holds assembly function for loading and enabling paging
# reference: https://wiki.osdev.org/Paging

.text
.globl pagedir_cr3
.globl enablePaging

pagedir_cr3:
	push %ebp
	mov %esp, %ebp
	mov 8(%esp), %eax
	mov %eax, %cr3						
	mov %ebp, %esp
	pop %ebp
	ret

enablePaging:
	push %ebp
	mov %esp, %ebp
	mov %cr4, %eax
	or  $0x00000010, %eax
	mov %eax, %cr4
	mov %cr0, %eax
	or $0x80000000, %eax
	mov %eax, %cr0
	mov %ebp, %esp
	pop %ebp
	ret



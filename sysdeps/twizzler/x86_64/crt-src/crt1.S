.section .text
.global _start
_start:
	andq $0xfffffffffffffff0, %rsp
	mov $main, %rsi
	call __mlibc_entry
.section .note.GNU-stack,"",%progbits

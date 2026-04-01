.section .text
.global _start
_start:
	andq $0xfffffffffffffff0, %rsp
	lea main(%rip), %rsi
	call __mlibc_entry
.section .note.GNU-stack,"",%progbits

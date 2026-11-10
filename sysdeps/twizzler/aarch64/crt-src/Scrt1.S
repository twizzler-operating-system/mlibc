.section .text
.global _start
_start:
	// adrp: `adr` reaches only +-1 MiB, which a large executable's `main` is outside of.
	adrp x1, main
	add x1, x1, :lo12:main

	bl __mlibc_entry
	brk #0

.section .note.GNU-stack,"",%progbits

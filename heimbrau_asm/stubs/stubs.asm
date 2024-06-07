[BITS 64]

section .text

global asm_stopPIC;
; Disable the PIC
asm_stopPIC:
	mov	al,		0xFF
	out	0xA1,	al
	out	0x21,	al
	ret

; Set the GDT to the passed pointer (passed in rcx)
global asm_lgdt;
asm_lgdt:
	lgdt [rcx]
	ret

; Set the current page table (passed in rcx)
global asm_loadPT;
asm_loadPT:
	mov cr3, rcx
	ret

; Enable FPU - unimplemented in x86-64 as we use SSE
global asm_enableFPU;
asm_enableFPU:

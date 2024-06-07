extern PML4;
extern PDPT;
extern PD;
extern PT;

[BITS 32]

global mb2_entry;

section .text
align 4

GDT64:                           ; Global Descriptor Table (64-bit).
    .Null: equ $ - GDT64         ; The null descriptor.
    dw 0                         ; Limit (low).
    dw 0                         ; Base (low).
    db 0                         ; Base (middle)
    db 0                         ; Access.
    db 0                         ; Granularity.
    db 0                         ; Base (high).
    .Code: equ $ - GDT64         ; The code descriptor.
    dw 0                         ; Limit (low).
    dw 0                         ; Base (low).
    db 0                         ; Base (middle)
    db 10011000b                 ; Access.
    db 00100000b                 ; Granularity.
    db 0                         ; Base (high).
    .Data: equ $ - GDT64         ; The data descriptor.
    dw 0                         ; Limit (low).
    dw 0                         ; Base (low).
    db 0                         ; Base (middle)
    db 10010000b                 ; Access.
    db 00000000b                 ; Granularity.
    db 0                         ; Base (high).
    .Pointer:                    ; The GDT-pointer.
    dw $ - GDT64 - 1             ; Limit.
    dq GDT64                     ; Base.

STACKSIZE	equ 0x1000

mb2_entry:
	mov esp, stack + STACKSIZE	 ; Create stack.
	push eax					 ; Push the first Multiboot 2 parameter onto the stack.
	push ebx					 ; Push the second Multiboot 2 parameter onto the stack.

	; set up page table
	mov edi, PML4				 ; Move temporary PML4 address into EDI register.
	mov cr3, edi				 ; Move EDI register into CR3 register
	xor eax, eax				 ; Zero EAX register
	mov ecx, 4096				 ; Move 0x1000 into ECX register
	rep stosd					 ; Store EAX (0) into data pointed to by EDI (PML4) ECX (0x1000) times
								 ; In effect, we are zeroing out the PML4 structure
	mov edi, cr3				 ; Move EDI (PML4 address) into EDI register.
	
	mov DWORD [edi], PDPT + 3	 ; Push PDPT address into PML4
	mov edi, PDPT
	mov DWORD [edi], PD + 3		 ; Push PD address into PDPT
	mov edi, PD
	mov DWORD [edi], PT + 3		 ; Push PT address into PD
	mov edi, PT
	
	mov ebx, 0x00000003			 ; Set the first two bits of each page
	mov ecx, 512				 ; We need to update 512 pages (one page table)
	
.SetEntry:
	mov DWORD [edi], ebx		 ; Loop through the page table and update each page to be set to present and r/w (bits 0 and 1)
	add ebx, 0x1000
	add edi, 8
	loop .SetEntry
	
	mov eax, cr4				 ; Set the 6th bit of CR4 (PAE Extensions)
	or eax, 1 << 5
	mov cr4, eax
	
	mov ecx, 0xC0000080			 ; Set 9th bit in MSR (Long Mode bit)
	rdmsr
	or eax, 1 << 8
	wrmsr
	
	mov eax, cr0				 ; Set 32nd bit in CR0 (enable paging)
	or eax, 1 << 31
	mov cr0, eax

	pop ebx						 ; Pop second MB2 argument into EBX
	pop eax						 ; Pop first MB2 argument into EAX
	
	lgdt [GDT64.Pointer]		 ; Initialize the GDT
	jmp GDT64.Code:Realm64		 ; Long Jump into 64-bit code, clearing all segments
hang:
	hlt							 ; If we somehow fail to jump, halt forever.
	jmp hang
	
section .text

[BITS 64]

extern mb2_entry64;

Realm64:
	cli							 ; Make sure interrupts are disabled.
	mov rdx, rax				 ; Save MB2 arguments
	mov rcx, rbx				 ; Save MB2 arguments
	mov ax, GDT64.Data           ; Set the A-register to the data descriptor.
	mov ds, ax                   ; Set the data segment to the A-register.
	mov es, ax                   ; Set the extra segment to the A-register.
	mov fs, ax                   ; Set the F-segment to the A-register.
	mov gs, ax                   ; Set the G-segment to the A-register.
	
	call mb2_entry64			 ; Call our C++ entry point.

section .bss
align 4
stack: resb STACKSIZE
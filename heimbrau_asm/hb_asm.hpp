#pragma once

#include "common.hpp"

#include <intrin.h>
#include <cstdint>

extern "C"
{
	// Disable the 8259 Programmable Interrupt Controller (PIC)
	void asm_stopPIC ();
	// Set the LGDT to point to the given address
	void asm_lgdt (const void *pGDT);
	// Set the pointed-to Page Table to be the active page table
	// It must be the highest-tier page structure for the given architecture
	// PML4 on x86-64
	void asm_loadPT (const void *pPT);
	// Enable the x87 FPU
	// Not implemented on x86-64 as we use SSE and do not support x87.
	void asm_enableFPU ();
}

namespace native
{
	template <typename L, typename R>
	// 16-byte aligned memcpy
	// TODO MMK : replace all memcpys with template overloaded versions
	inline void memcpy_16 (L dst, const R src, u64 sz) {
		for (u64 i = 0; i < sz >> 4; ++i)
		{
			((__m128i *)dst)[i] = ((__m128i *)src)[i];
		}
	}
	template <typename L, typename R>
	// 8-byte aligned memcpy
	// TODO MMK : replace all memcpys with template overloaded versions
	inline void memcpy_8 (L dst, const R src, u64 sz) {
		for (u64 i = 0; i < sz >> 3; ++i)
		{
			((u64 *)dst)[i] = ((u64 *)src)[i];
		}
	}
	template <typename L, typename R>
	// 4-byte aligned memcpy
	// TODO MMK : replace all memcpys with template overloaded versions
	inline void memcpy_4 (L dst, const R src, u64 sz) {
		for (u64 i = 0; i < sz >> 2; ++i)
		{
			((u32 *)dst)[i] = ((u32 *)src)[i];
		}
	}
	template <typename L, typename R>
	// Unaligned memcpy
	// TODO MMK : replace all memcpys with template overloaded versions
	inline void memcpy (L dst, const R src, u64 sz) {
		for (u64 i = 0; i < sz; ++i)
		{
			((u8 *)dst)[i] = ((u8 *)src)[i];
		}
	}
	template <typename L>
	// 16-byte aligned memset
	// TODO MMK : replace all memsets with template overloaded versions
	inline void memset_16 (L dst, __m128i val, u64 sz) {
		for (u64 i = 0; i < (sz >> 4ULL); ++i)
		{
			((__m128i *)dst)[i] = val;
		}
	}
	template <typename L>
	// 16-byte aligned memset
	// TODO MMK : replace all memsets with template overloaded versions
	inline void memset_16 (L dst, u64 val, u64 sz) {
		__m128i _val;
		_val.m128i_u64[0] = val;
		_val.m128i_u64[1] = 0;
		for (u64 i = 0; i < (sz >> 4ULL); ++i)
		{
			((__m128i *)dst)[i] = _val;
		}
	}
	template <typename L>
	// 8-byte aligned memset
	// TODO MMK : replace all memsets with template overloaded versions
	inline void memset_8 (L dst, u64 val, u64 sz) {
		for (u64 i = 0; i < (sz >> 3ULL); ++i)
		{
			((u64 *)dst)[i] = val;
		}
	}
	template <typename L>
	// 4-byte aligned memset
	// TODO MMK : replace all memsets with template overloaded versions
	inline void memset_4 (L dst, u32 val, u64 sz) {
		for (u64 i = 0; i < (sz >> 2ULL); ++i)
		{
			((u32 *)dst)[i] = val;
		}
	}
	template <typename L>
	// Unaligned memset
	// TODO MMK : replace all memsets with template overloaded versions
	inline void memset (L dst, u8 val, u64 sz) {
		for (u64 i = 0; i < sz; ++i)
		{
			((u8 *)dst)[i] = val;
		}
	}
	// Permanently halts the system until an interrupt.
	__declspec(noreturn) inline void fullhalt () { for (;;) __halt(); }
	// Executes x86 hlt instruction.
	// The hlt instruction will stop CPU execution until an interrupt is received.
	inline void halt () { __halt(); }
	// Disable the 8259 Programmable Interrupt Controller (PIC)
	inline void stopPIC () { asm_stopPIC(); }
	// Clears Interrupts - Disables interrupts for the current CPU/Core
	inline void cli () { _disable(); }
	// Start Interrupts - Starts interrupts for the current CPU/Core
	inline void sti () { _enable(); }
	// Permanently halts the system and disables interrupts.
	__declspec(noreturn) inline void stop () { cli(); for (;;) __halt(); }
	// Set the Interrupt Descriptor Table (IDT) at the given pointer.
	inline void lidt (const void *ptr) { __lidt((void *)ptr); }
	// Set the Global Descriptor Table (GDT) at the given pointer.
	inline void lgdt (const void *ptr) { asm_lgdt(ptr); }
	// Invalidates a page in the Translation Lookaside Buffer (TLB)
	inline void invlpg (void * const ptr) { __invlpg(ptr); }
	// Set the current top-level page structure to the given pointer.
	// On x86-64, this is the PML4 (Page Map Level 4)
	inline void loadPT (const void *ptr) { asm_loadPT(ptr); }
	// No Operation opcode
	inline void nop () { __nop(); }
	// Returns the value of the given Control Register (CR)
	// TODO MMK : Implement the rest of the CRs (will have to use asm stubs)
	inline u64 readCR (int reg)
	{
		switch (reg)
		{
			// Microsoft only implements the following as intrinsics.
		case 0:	return __readcr0();
		case 2: return __readcr2();
		case 3: return __readcr3();
		case 4: return __readcr4();
		case 8: return __readcr8();
		default:;
			// TODO for the others
		}
		// TODO implement some kind of assertion here.
	}
	// Return the value of the given Debug Register (DR)
	inline u64 readDR (int reg) { return __readdr(reg); }
	// Return the value of the given Model-Specific Register (MSR)
	inline u64 readMSR (int reg) { return __readmsr(reg); }
	// Write the value of the given Control Register (CR)
	// TODO MMK : Implement the rest of the CRs (will have to use asm stubs)
	inline void writeCR (int reg, u64 v)
	{
		switch (reg)
		{
			// Microsoft only implements the following as intrinsics.
		case 0:	__writecr0(v); break;
		case 3: __writecr3(v); break;
		case 4: __writecr4(v); break;
		case 8: __writecr8(v); break;
		default:;
			// TODO for the others
		}
		// TODO implement some kind of assertion here.
	}
	// Write the value of the given Debug Register (DR)
	inline void writeDR (int reg, u64 v) { return __writedr(reg, v); }
	// Write the value of the given Model-Specific Register (MSR)
	inline void writeMSR (int reg, u64 v) { return __writemsr(reg, v); }
	// Store (reads) the value of the IDT to the given pointer.
	inline void sidt (void *ptr) { __sidt(ptr); }
	// Enables the x87 FPU
	// Not implemented on x86-64, as we use SSE
	inline void enableFPU () { asm_enableFPU(); }
	// Enables Streaming SIMD Extensions (SSE)
	inline void enableSSE () {
		u64 cr0 = readCR(0);
		cr0 &= ~(1 << 2);	// EM : Enable the FPU (yes, odd), also known as "Disable Emulation"
		cr0 |= (1 << 1);	// MP : Allows SSE/x87 state to be saved after task switching
		writeCR(0, cr0);

		u64 cr4 = readCR(4);
		cr4 |= (1 << 9);	// OSFXSR : Enable support for fast FPU register save/restore
		cr4 |= (1 << 10);	// OSXMMEXCPT : Enable unmasked SSE exceptions
		writeCR(4, cr4);
	}
}
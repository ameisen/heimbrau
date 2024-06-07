#include "common.hpp"

#include "Multiboot2.hpp"

#include "../Loader/Loader.hpp"

using namespace multiboot2;

extern "C" 
{
	void mb2_entry ();

	_align(0x1000) u64 PML4[512];
	_align(0x1000) u64 PDPT[512];
	_align(0x1000) u64 PD[512];
	_align(0x1000) u64 PT[512];
}

static const u32 mb2_flags = u32(e_ModuleAlign) | u32(e_MemInfo) | u32(e_ValidOffsets)/* | u64(e_MMapInfo)*/;

#pragma code_seg(push, ".a$0")

__declspec(allocate(".a$0"))

volatile multiboot2::header mb2_header = {
	s_GrubHeader,
	mb2_flags,
	u32(-s32(s_GrubHeader + mb2_flags)),

	0,
	0x100000,
	0,
	0,
	u32(&mb2_entry),

	0,
	0,
	0
};

#pragma comment(linker, "/merge:.text=.a")

#pragma code_seg(pop)

// Linker puts constructors between these sections, and we use them to locate constructor pointers.
#pragma section(".CRT$XIA",long,read)
#pragma section(".CRT$XIZ",long,read)
#pragma section(".CRT$XCA",long,read)
#pragma section(".CRT$XCZ",long,read)

// Put .CRT data into .rdata section
#pragma comment(linker, "/merge:.CRT=.rdata")
 
typedef void (__cdecl *_PVFV)(void);
typedef int  (__cdecl *_PIFV)(void);

// Pointers surrounding constructors
__declspec(allocate(".CRT$XIA")) _PIFV __xi_a[] = { 0 };
__declspec(allocate(".CRT$XIZ")) _PIFV __xi_z[] = { 0 };
__declspec(allocate(".CRT$XCA")) _PVFV __xc_a[] = { 0 };
__declspec(allocate(".CRT$XCZ")) _PVFV __xc_z[] = { 0 };
 
extern __declspec(allocate(".CRT$XIA")) _PIFV __xi_a[];
extern __declspec(allocate(".CRT$XIZ")) _PIFV __xi_z[];    // C initializers
extern __declspec(allocate(".CRT$XCA")) _PVFV __xc_a[];
extern __declspec(allocate(".CRT$XCZ")) _PVFV __xc_z[];    // C++ initializers

extern "C" void mb2_entry ();

extern "C" __declspec(noreturn) void mb2_entry64 (u64 mbinfo, u64 magic)
{
	{
		_PIFV * pfbegin = __xi_a;
		_PIFV * pfend = __xi_z;
        while ( pfbegin < pfend)
        {
            // if current table entry is non-NULL, call thru it.
            if ( *pfbegin != 0 )
                (**pfbegin)();
            ++pfbegin;
        }
	}
	{
		_PVFV * pfbegin = __xc_a;
		_PVFV * pfend = __xc_z;
        while ( pfbegin < pfend )
        {
            // if current table entry is non-NULL, call thru it.
            if ( *pfbegin != 0 )
                (**pfbegin)();
            ++pfbegin;
        }
	}
	loader::entry(*(const multiboot2::info *)mbinfo, magic);
}

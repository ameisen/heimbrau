#include "common.hpp"

#include "..\..\heimbrau_asm\hb_asm.hpp"

#pragma pack(push, 1)

// Entry for the PML4
struct PML4E
{
	union
	{
		struct
		{
			u64	present		: 1; // Present Bit
			u64 R_W			: 1; // Read-Write Bit : 0 == read only
			u64 U_S			: 1; // User-Supervisor : 0 = supervisor only
			u64 PWT			: 1; // page-level write-through
			u64 PCD			: 1; // page-level cache disable
			u64 A			: 1; // accessed
			u64				: 1;
			u64 PS			: 1; // Reserved (must be zero)
			u64				: 4;
			u64	PDPT_PP		: 40; // Physical pointer to PDPT (Page Directory Pointer Table)
			u64				: 11;
			u64 NX			: 1;
		};
		u64 _i;
	};

	PML4E () : _i(0) {}
};

// The _entire_ PML4 for the system, residing in the kernel binary.
// As of current implementations, the PML4 is only 512 entries.
// When future CPUs are released that do not have 48-bit memory addressing,
// this will likely need to be updated. This should be a trivial task.
_align(0x1000) struct PML4
{
	PML4E	entries[512];

	PML4E & operator [] (size_t i) { return entries[i]; }
	PML4E operator [] (size_t i) const { return entries[i]; }
};

// Page Directory Pointer Table entry
struct PDPTE
{
	union
	{
		struct
		{
			u64	present		: 1; // Present Bit
			u64 R_W			: 1; // Read-Write Bit : 0 == read only
			u64 U_S			: 1; // User-Supervisor : 0 = supervisor only
			u64 PWT			: 1; // page-level write-through
			u64 PCD			: 1; // page-level cache disable
			u64 A			: 1; // accessed
			u64	D			: 1; // dirty
			u64 PS			: 1; // Reserved (must be zero unless it maps a 1GB page)
			u64	G			: 1; // global
			u64				: 3;
			u64	PD_PP		: 40; // Physical pointer to Page Table
			u64				: 11;
			u64 NX			: 1;
		};
		u64 _i;
	};

	PDPTE () : _i(0) {}
};

// TODO :
// Everything below this is going to have to be restructured for dynamic physical memory allocation.
// We should only _statically_ have room for the page structures requires for the kernel itself. All others should be
// dynamically allocated.

// Array of 512 PDPT entries. Not flexible, meant for kernel use.
_align(0x1000) struct PDPT
{

	PDPTE	entries[512];

	PDPTE & operator [] (size_t i) { return entries[i]; }
	PDPTE operator [] (size_t i) const { return entries[i]; }
};

// Page Directory Entry
struct PDE
{
	union
	{
		struct
		{
			u64	present		: 1; // Present Bit
			u64 R_W			: 1; // Read-Write Bit : 0 == read only
			u64 U_S			: 1; // User-Supervisor : 0 = supervisor only
			u64 PWT			: 1; // page-level write-through
			u64 PCD			: 1; // page-level cache disable
			u64 A			: 1; // accessed
			u64	D			: 1; // dirty
			u64 PS			: 1; // Reserved (must be zero unless it maps a 2MB page)
			u64	G			: 1; // global
			u64				: 3;
			u64	PT_PP		: 40; // Physical pointer to Page Table
			u64				: 11;
			u64 NX			: 1;
		};
		u64 _i;
	};

	PDE () : _i(0) {}
};

// Array of 512 Page Directory Entries
_align(0x1000) struct PD
{
	PDE	entries[512];

	PDE & operator [] (size_t i) { return entries[i]; }
	PDE operator [] (size_t i) const { return entries[i]; }
};

// Page Table Entry
struct PTE
{
	union
	{
		struct
		{
			u64	present		: 1; // Present Bit
			u64 R_W			: 1; // Read-Write Bit : 0 == read only
			u64 U_S			: 1; // User-Supervisor : 0 = supervisor only
			u64 PWT			: 1; // page-level write-through
			u64 PCD			: 1; // page-level cache disable
			u64 A			: 1; // accessed
			u64	D			: 1; // dirty
			u64 PAT			: 1; // Indirectly determines the memory type used to access the 4-KByte page referenced by this entry (see Section 4.9.2)
			u64	G			: 1; // global
			u64				: 3;
			u64	PD_PP		: 40; // Physical pointer to Page Entry
			u64				: 11;
			u64 NX			: 1;
		};
		u64 _i;
	};

	PTE () : _i(0) {}
};

// Array of 512 Page Table Entries
_align(0x1000) struct PT
{
	PTE	entries[512];

	PTE & operator [] (size_t i) { return entries[i]; }
	PTE operator [] (size_t i) const { return entries[i]; }
};

#pragma pack(pop)

#if defined(KERNEL)
extern "C" 
{
	// Only the PML4 is stored statically. Everything else should be dynamic in some fashion.
	// We must make sure to know the _physical_ address of this, as well. REMEMBER: ALL PAGING POINTERS
	// ARE PHYSICAL ADDRESSES. The tables aren't stored in the logical space, except for your editing
	// convenience.
	_export _align(0x1000) u64 PML4[512];
}

namespace Paging
{
	// System PML4
	_align(0x1000) u64 *PML4	= ::PML4;
}

// Put here to make sure that the kernel is generated as a valid image early on.
void fake_entry () {}

// The Loader requires a single chain of page structures to set up long mode. They are local to the kernel
// and this file is shared by the loader and kernel.
#elif defined(LOADER)
namespace kerneldata
{
#	include "..\..\bin64\kernel_kdf.hpp"
}

_align(0x1000) u64 LDR_PML4[512];
_align(0x1000) u64 LDR_PDPT[512];
_align(0x1000) u64 LDR_PD[512];
_align(0x1000) u64 LDR_PT[512];

void initializeProtoPaging ()
{
	// TODO
}

void initializePagingSystem ()
{
	// TODO

	//u64 *phy_PML4 = 
}

#pragma pack(push, 1)
struct SimpleMemoryEntry
{
	u64 offset;
	u64 extent;
	u32 type;
};
#pragma pack(pop)

#include "../../heimbrau_loader/LoaderIO/lio.hpp"

// Gets basic memory information.
// We pass in the memory map, which is sorted and cleaned.
// nentries it the number of entries in the input memory map.
// This function is sort of complex. It will determine the number of 4k pages requires to store the kernel image.
// It takes an array (allocPages) of pointers that it will populate with physical page addresses that are to be 
// used for the kernel, none of them being less than the value of startAt.
void getKernelPPages (const SimpleMemoryEntry *entries, u32 nentries, void **allocPages, u64 startAt)
{
	static const u32 numPages = ((kerneldata::kernelSize % 4096) == 0) ? 
		kerneldata::kernelSize / 4096:
		(kerneldata::kernelSize + (4096 - (kerneldata::kernelSize % 4096))) / 4096;

	u32 iterator = 1;
	const SimpleMemoryEntry *lastEntry = &entries[0];
	u64	lastOffset = lastEntry->offset;	
	if ((lastOffset % 4096) != 0)
	{
		lastOffset += 4096 - (lastOffset % 4096);
	}

	auto getPage = [entries, nentries, &lastEntry, &lastOffset, &iterator, startAt] () -> u64
	{
		restart:
		if ((lastOffset + 4096 + 4096) > (lastEntry->extent + lastEntry->offset))
		{
			// Get next entry.
			for (u32 i = iterator; i < nentries; ++i)
			{
				if (entries[i].type != 1)
					continue;

				lastEntry = &entries[i];
				iterator = i + 1;

				//check for alignment
				lastOffset = entries[i].offset;
				if ((lastOffset % 4096) != 0)
				{
					lastOffset += 4096 - (lastOffset % 4096);
				}
				// Perform the check again.
				goto restart;
			}
		}

		u64 offset = lastOffset;
		lastOffset += 4096;

		if (offset <= startAt)
		{
			goto restart;
		}

		return offset;
	};

	for (u32 i = 0; i < numPages; ++i)
	{
		allocPages[i] = (void *)getPage();
	}
}

#endif // defined(KERNEL)
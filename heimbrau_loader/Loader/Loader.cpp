#include "Loader.hpp"

// MMK : For hashing
#include "common/hash.hpp"

#include "MemoryMap.hpp"

#include "../LoaderIO/lio.hpp"

namespace kerneldata
{
#	include "..\..\bin64\kernel_kdf.hpp"
}

// Gets basic memory information.
// We pass in the memory map, which is sorted and cleaned.
// nentries it the number of entries in the input memory map.
// This function is sort of complex. It will determine the number of 4k pages requires to store the kernel image.
// It takes an array (allocPages) of pointers that it will populate with physical page addresses that are to be 
// used for the kernel, none of them being less than the value of startAt.
extern void getKernelPPages (const SimpleMemoryEntry *, u32, void **, u64);

// These are used to store physical pointers for the lPPA
// These are the actual physical pages that will be used to store the kernel image.
static void *s_AllocationPages[
	((kerneldata::kernelSize % 4096) == 0) ? 
		kerneldata::kernelSize / 4096:
		(kerneldata::kernelSize + (4096 - (kerneldata::kernelSize % 4096))) / 4096
];

// Multiboot 2 Header
extern volatile multiboot2::header mb2_header;

__declspec(noreturn)
void loader::entry (const multiboot2::info &mbinfo, u64 magic)
{
	// Enable SSE support, used by VC++ for lots of things including variadics.
	native::enableSSE();

	// Initialize Loader I/O (really just O)
	lio::init();

	lio::printf("Testing LIO: 0x%016LX 0x%016LX \n", &mbinfo, magic);
	lio::printf("Test\n");
	lio::printf("flags: 0x%08lX\n", u64(mbinfo.m_Flags));
	lio::printf("mmap addr: 0x%016LX\n", u64(mbinfo.m_MemoryMapAddress));
	lio::printf("mmap len: 0x%08lX\n", mbinfo.m_MemoryMapLength);

	u32 entries = 0;
	
	SimpleMemoryEntry *smmap = (SimpleMemoryEntry *)mbinfo.m_MemoryMapAddress;
	MemoryMap::Process(smmap, entries, mbinfo);

	// Get physical pages for the kernel.
	getKernelPPages(
		smmap,
		entries,
		s_AllocationPages,
		mb2_header.m_LoadEndAddress > mb2_header.m_BSSEndAddress ? 
			mb2_header.m_LoadEndAddress : mb2_header.m_BSSEndAddress
	);
	lio::printf("Physical Pages Prepared:\n");
	for (u32 i = 0; i < sizeof(s_AllocationPages) / sizeof(void *); ++i)
	{
		lio::printf("\tPP %u: 0x%016LX\n", i, s_AllocationPages[i]);
	}


	native::stop();
}

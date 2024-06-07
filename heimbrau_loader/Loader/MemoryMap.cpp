#include "MemoryMap.hpp"
#include "heimbrau_loader\LoaderIO\lio.hpp"

// MMK : So we can get MB2 header information.
extern volatile multiboot2::header mb2_header;

void MemoryMap::Process (SimpleMemoryEntry *_mmap, u32 &entries, const multiboot2::info &mbinfo)
{
	SimpleMemoryEntry *smmap = _mmap;

	// Get and output the memory map.
	const multiboot2::mmap_entry *mmap = (const multiboot2::mmap_entry *)mbinfo.m_MemoryMapAddress;
	while (u64(mmap) < u64(mbinfo.m_MemoryMapAddress) + mbinfo.m_MemoryMapLength)
	{
		++entries;
		lio::printf("Entry: 0x%016LX 0x%08lX\n", mmap, mmap->getSize());
		lio::printf("Entry: 0x%016LX | 0x%016LX | 0x%08lX\n", mmap->m_BaseAddress, mmap->m_Length, mmap->m_Type);

		const multiboot2::mmap_entry *next = mmap->getNext();
		u64 base = mmap->m_BaseAddress;
		u64 extent = mmap->m_Length;
		u32 type = mmap->m_Type;

		// Write new values.

		(*smmap).offset = base;
		(*smmap).extent = extent;
		(*smmap).type = type;
		++smmap;

		mmap = next;
	}

	u32 oldEntries = entries;
	smmap = (SimpleMemoryEntry *)mbinfo.m_MemoryMapAddress;
	SimpleMemoryEntry::cleanup(smmap, entries);

	u64 usable = 0;
	u64 usablepage = 0;

	// Output cleaned up memory map
	lio::printf("Original/New Entries: %u / %u\n", oldEntries, entries);
	for (u32 i = 0; i < entries; ++i)
	{
		const SimpleMemoryEntry &entry = smmap[i];
		lio::printf("Entry: 0x%016LX | 0x%016LX | 0x%08lX\n", entry.offset, entry.extent, entry.type);
		if (entry.type == 1)
		{
			usable += entry.extent;
			u64 offset = entry.offset;
			u64 diff = offset % 4096;
			if (diff != 0)
			{
				offset += 4096 - diff;
			}
			while ((offset += 4096) < entry.offset + entry.extent)
			{
				usablepage += 4096;
			}
		}
	}

	lio::printf("Usable Memory: %Lu KiB\n", usable / 1024);
	lio::printf("Usable Memory Page-wise: %Lu KiB\n", usablepage / 1024);

	lio::printf("Image End: 0x%016LX\n", mb2_header.m_LoadEndAddress > mb2_header.m_BSSEndAddress ? 
			mb2_header.m_LoadEndAddress : mb2_header.m_BSSEndAddress);
}

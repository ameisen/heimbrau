#pragma once

#include "common.hpp"

#include "heimbrau_loader\Multiboot2\Multiboot2.hpp"

#pragma pack(push, 1)
struct SimpleMemoryEntry
{
	u64 offset;
	u64 extent;
	u32 type;

	// Cleans up and sorts the input Memory Map
	static void cleanup (SimpleMemoryEntry *entrylist, u32 &entries)
	{
		// sort first. Let's do a trivial bubble sort.
		for (u32 i = 0; i < entries; ++i)
		{
			uintptr_t offset = entrylist[i].offset;

			u32 nindex = -1U;

			for (u32 ii = i + 1; ii < entries; ++ii)
			{
				uintptr_t offset2 = entrylist[ii].offset;
			
				if (offset2 < offset)
				{
					if (nindex == -1ULL || entrylist[nindex].offset > offset2)
					{
						nindex = ii;
					}
				}
			}

			if (nindex != -1U)
			{
				// Swap!
				SimpleMemoryEntry temp = entrylist[i];
				entrylist[i] = entrylist[nindex];
				entrylist[nindex] = temp;
			}
		}

		// Merge
		// TODO edge cases where the next entry is entirely inside the current entry, and actually ends before the current one.
		for (u32 i = 0; i < entries; ++i)
		{
			SimpleMemoryEntry &entry = entrylist[i];

			if (i < entries - 1)
			{
				// This means that one still exists after it.
				SimpleMemoryEntry &nextentry = entrylist[i + 1];
				if (nextentry.offset <= entry.offset + entry.extent)
				{
					// If it's a different type, we need to do different stuff.
					if (nextentry.type != entry.type)
					{
						if (nextentry.type > entry.type)
						{
							// They are a higher priority, they get the overlap.
							uintptr_t diff = nextentry.offset - (entry.offset + entry.extent);
							entry.extent -= diff;
							if (entry.extent == 0)
							{
								uint64_t count = (entries - 2) - i;

								if (count)
									native::memcpy(&entry, (&entry) + 1, sizeof(SimpleMemoryEntry) * count);

								--entries;
							}
						}
						else
						{
							// They are of a lower priority, we get the overlap.
							uintptr_t diff = (entry.offset + entry.extent) - nextentry.offset;
							nextentry.offset += diff;
							nextentry.extent -= diff;

							if (nextentry.extent == 0)
							{
								u32 count = (entries - 2) - i;

								if (count)
									native::memcpy(&nextentry, (&nextentry) + 1, sizeof(SimpleMemoryEntry) * count);

								--entries;
							}
						}
					}
					else
					{
						// Need to eliminate it!

						uintptr_t end = nextentry.offset + nextentry.extent;
						end = end - (entry.offset + entry.extent);

						entry.extent += end;

						u32 count = (entries - 2) - i;
						if (count)
							native::memcpy(&nextentry, (&nextentry) + 1, sizeof(SimpleMemoryEntry) * count);

						--entries;

						// Decrement i so we can check again.
						--i;
					}
				}
			}
		}
	}
};
#pragma pack(pop)

namespace MemoryMap
{
	extern void Process (SimpleMemoryEntry *_mmap, u32 &entries, const multiboot2::info &mbinfo);
}

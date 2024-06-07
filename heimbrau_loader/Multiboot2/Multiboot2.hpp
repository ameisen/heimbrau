#pragma once

#include "common.hpp"

namespace multiboot2
{
#	pragma pack (push, 1)
	struct header
	{
		u32		m_Magic;
		u32		m_Flags;
		u32		m_Checksum;

		u32		m_HeaderAddress;
		u32		m_LoadAddress;
		u32		m_LoadEndAddress;
		u32		m_BSSEndAddress;
		u32		m_EntryAddress;

		u32		m_Width;
		u32		m_Height;
		u32		m_Depth;
	};

	struct info
	{
		u32		m_Flags;
		u64		m_Memory;
		u32		m_BootDevice;
		u32		m_CmdLine;
		u32		m_ModsCount;
		u32		m_ModsAddr;
		u8		m_Syms[16];
		u32		m_MemoryMapLength;
		u32		m_MemoryMapAddress;
		u32		m_DrivesLength;
		u32		m_DrivesAddress;
		u32		m_ConfigTable;
		u32		m_BootLoaderName;
		u32		m_APMTable;
		u32		m_VBEControlInfo;
		u32		m_VBEModeInfo;
		u32		m_VBEMode;
		u32		m_VBEInterfaceSeg;
		u32		m_VBEInterfaceOffset;
		u32		m_VBEInterfaceLength;
	};

	struct mmap_entry
	{
		u32		m_Size;
		u64		m_BaseAddress;
		u64		m_Length;
		u32		m_Type;

		u32		getSize () const
		{
			return m_Size + sizeof(u32);
		}

		mmap_entry * getNext () const
		{
			return (mmap_entry *)(((const u8 *)this) + getSize());
		}
	};
#	pragma pack (pop)

	enum
	{
		e_ModuleAlign	=	(1 << 0),
		e_MemInfo		=	(1 << 1),
		e_DisplayMode	=	(1 << 2),
		e_MMapInfo		=	(1 << 6),
		e_ValidOffsets	=	(1 << 16),
	};

	static const u32 s_GrubHeader = 0x1BADB002U;
}

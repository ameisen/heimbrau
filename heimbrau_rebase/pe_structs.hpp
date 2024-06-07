#pragma once

#include <cstdint>

#pragma pack(push, 1)

template <typename T>
T toLittle (T in)
{
	char *ob = (char *)&in;
	for (size_t i = 0; i < sizeof(T) / 2; ++i)
	{
		char temp = ob[i];
		ob[i] = ob[sizeof(T) - (i + 1)];
		ob[sizeof(T) - (i + 1)] = temp;
	}
	return in;
}

class Stream
{
	void			*buffer;
	mutable size_t	offset;
	size_t			size;
protected:
	Stream () {}

public:
	Stream (void *buf, size_t sz) : buffer(buf), offset(0), size(sz)
	{}

	virtual ~Stream ()
	{}

	virtual bool exists () const { return true; }

	virtual size_t getSize () const
	{
		return size;
	}

	virtual void read (size_t sz, void *out = nullptr) const
	{
		if (out)
			memcpy(out, (char *)buffer + offset, sz);
		offset += sz;
	}

	template<typename T>
	void read (T &out) const
	{
		memcpy(&out, (char *)buffer + offset, sizeof(T));
		offset += sizeof(T);
	}

	virtual void forward (size_t sz) const
	{
		offset += sz;
	}

	virtual void rewind () const
	{
		offset = 0;
	}

	virtual void seek (size_t off) const
	{
		offset = off;
	}

	template <typename T>
	T & at (size_t off, bool useOffset = true) const
	{
		const char *_buffer = (const char *)buffer;
		_buffer += off + (useOffset ? offset : 0);
		T & temp = *(T *)_buffer;
		return temp;
	}
};

class File : public Stream
{
	FILE *fp;

public:
	File (const char *file, const char *mode)
	{
		fp = fopen(file, mode);
	}

	virtual ~File ()
	{
		if (fp)
			fclose(fp);
	}

	virtual bool exists () const { return fp != nullptr; }

	virtual size_t getSize () const
	{
		fseek(fp, 0, SEEK_END);
		size_t sz = ftell(fp);
		::rewind(fp);
		return sz;
	}

	virtual void read (size_t sz, void *out = nullptr) const
	{
		if (out)
			fread(out, 1, sz, fp);
		else
			fseek(fp, long(sz), SEEK_CUR);
	}

	virtual void rewind () const
	{
		::rewind(fp);
	}

	virtual void seek (size_t off) const
	{
		fseek(fp, long(off), SEEK_SET);
	}
};

struct dos_stub
{
	static bool exists (const Stream &stream)
	{
		const uint16_t magic = stream.at<uint16_t>(0);
		const bool isDos = magic == uint16_t(0x5A4DU);
		return isDos;
	}
};


struct pe_header
{
	uint16_t	machine;
	uint16_t	numSections;
	uint32_t	timestamp;
	uint32_t	ptrSymbolTable;
	uint32_t	numSymbols;
	uint16_t	sizeOptional;
	uint16_t	characteristics;

	static bool exists (const Stream &stream)
	{
		const uint32_t magic = stream.at<uint32_t>(0);
		const bool isNT = magic == uint32_t(0x00004550U);
		return isNT;
	}
};

struct optional_header_pe32p
{
	struct data_directory
	{
		uint32_t	relativeAddress;
		uint32_t	size;
	};

	uint16_t	magic;
	uint8_t		majorLinker;
	uint8_t		minorLinker;
	uint32_t	codeSize;
	uint32_t	initSize;
	uint32_t	uninitSize;
	uint32_t	ptrEntryPoint;
	uint32_t	ptrBaseCode;

	// optional
	uint64_t	ptrImageBase;
	uint32_t	sectionAlign;
	uint32_t	fileAlign;
	uint16_t	majorVersion;
	uint16_t	minorVersion;
	uint16_t	majorImageVersion;
	uint16_t	minorImageVersion;
	uint16_t	majorSubVersion;
	uint16_t	minorSubVersion;
	uint32_t	win32value;
	uint32_t	imageSize;
	uint32_t	headersSize;
	uint32_t	checksum;
	uint16_t	subsystem;
	uint16_t	dllCharacteristics;
	uint64_t	stackReserve;
	uint64_t	stackCommit;
	uint64_t	heapReserve;
	uint64_t	heapCommit;
	uint32_t	loaderFlags;
	uint32_t	countRVA;

	data_directory	exportTable;
	data_directory	importTable;
	data_directory	resourceTable;
	data_directory	exceptionTable;
	data_directory	certificateTable;
	data_directory	baseRelocationTable;
	data_directory	debugTable;
	data_directory	architecture;
	data_directory	globalPointer;
	data_directory	TLSTable;
	data_directory	LoadConfigTable;
	data_directory	boundImportTable;
	data_directory	IATable;
	data_directory	delayImportTable;
	data_directory	CLRHeader;
	data_directory	__resv0;


	static bool exists (const Stream &stream)
	{
		const uint16_t magic = stream.at<uint16_t>(0);
		const bool isNT = magic == uint16_t(0x020BU);
		return isNT;
	}
};

struct section_header
{
	char		name[8];
	uint32_t	virtualSize;
	uint32_t	virtualAddress;
	uint32_t	rawSize;
	uint32_t	ptrRawData;
	uint32_t	ptrRelocations;
	uint32_t	ptrLineNumbers;
	uint16_t	numRelocations;
	uint16_t	numLineNumbers;
	uint32_t	characteristics;
};

struct relocation_header
{
	uint32_t	virtualAddress;
	uint32_t	symbolIndex;
	uint16_t	type;
};

struct mb_header
{
	uint32_t		m_Magic;
	uint32_t		m_Flags;
	uint32_t		m_Checksum;

	uint32_t		m_HeaderAddress;
	uint32_t		m_LoadAddress;
	uint32_t		m_LoadEndAddress;
	uint32_t		m_BSSEndAddress;
	uint32_t		m_EntryAddress;

	uint32_t		m_Width;
	uint32_t		m_Height;
	uint32_t		m_Depth;
};

#pragma pack(pop)

#include "common.hpp"

#include <cstdio>

#include <vector>
#include <string>

#include "pe_structs.hpp"

// MMK : For hashing
#include "common/hash.hpp"

using namespace std;

/* KDF Generator
 *
 * In short, this tool takes an input file that is in PE (Portable Executable) format, the format that VC puts out,
 * and turns it into a flat binary.
 *
 * We do this because PE (and ELF, actually) is an encoded format - it is meant to be processed by the kernel and
 * have the constituent sections expanded and copied into their respective memory locations.
 * Our needs, on the other hand, require the binary to _already be in said format_.
 * This processes the PE and pre-expands it so that the binary can be directly loaded by the PBL (Primary Boot Loader).
 *
*/

struct out_section
{
	std::string	name;
	u64			logical_address;
	u64			raw_size;
	u64			virtual_size;
	u8			*raw_data;

	out_section (u64 laddr, u64 rsize, u64 vsize, u8 *rdata) :
		logical_address(laddr), raw_size(rsize), virtual_size(vsize), raw_data(rdata)
	{}
};

struct export_obj
{
	bool		cpp;
	uint64_t	pointer;
	std::string	name;
	export_obj () : cpp(false) {}
};


bool rebase (const char *file, const char *oname)
{
	static const uint32_t BASE = 0x100000U;

	vector<char> output;
	output.resize(8);

	uint32_t lowestSection;
	vector<char> data;
	vector<export_obj> exports;
	export_header *exp_header = nullptr;

	uint64_t baseAddress = 0;

	vector<out_section>	outSections;

	u64 totalSize = 0;
	u64 lTotalSize = 0;
	{
		File fp(file, "rb");
		if (!fp.exists())
		{
			printf("Could not open file %s for rebasing\n", file);
			return false;
		}

		data.resize(fp.getSize());
		fp.read(fp.getSize(), data.data());

		Stream stream(data.data(), data.size());

		uint32_t headerStart = 0;

		if (dos_stub::exists(stream))
		{
			headerStart = stream.at<uint32_t>(60);
		}

		stream.seek(headerStart);

		if (!pe_header::exists(stream))
		{
			printf("Not a valid PE file!\n");
			return false;
		}

		pe_header header = stream.at<pe_header>(4);
		stream.forward(sizeof(pe_header) + 4);

		if (!optional_header_pe32p::exists(stream))
		{
			printf("No optional header...\n");
			return false;
		}

		lowestSection = 0xFFFFFFFFU;
		uint32_t highestSection = 0;
		uint32_t lowestVirtualSection = 0xFFFFFFFFU;
		uint32_t highestVirtualSection = 0;

		optional_header_pe32p optheader;
		memset(&optheader, 0, sizeof(optional_header_pe32p));
		size_t smallerSize = sizeof(optional_header_pe32p) >= header.sizeOptional ?
			header.sizeOptional : sizeof(optional_header_pe32p);
		stream.read(smallerSize, &optheader);

		baseAddress = optheader.ptrImageBase;

		vector<section_header> sections;

		for (size_t i = 0; i < header.numSections; ++i)
		{
			section_header secheader;

			stream.read<section_header>(secheader);

			if (
				secheader.name[0] == '.' &&
				secheader.name[1] == 'r' &&
				secheader.name[2] == 's' &&
				secheader.name[3] == 'r' &&
				secheader.name[4] == 'c'
				) continue;

			if (
				secheader.name[0] == '.' &&
				secheader.name[1] == 'p' &&
				secheader.name[2] == 'd' &&
				secheader.name[3] == 'a' &&
				secheader.name[4] == 't' &&
				secheader.name[5] == 'a'
				) continue;

			/*if (
				secheader.name[0] == '.' &&
				secheader.name[1] == 'C' &&
				secheader.name[2] == 'R' &&
				secheader.name[3] == 'T'
				)
			{
				printf("This binary has a .CRT section - double check for initializers/destructors!\n");
				return false;
			}*/

			sections.push_back(secheader);


			if (secheader.ptrRawData != 0 && secheader.ptrRawData < lowestSection)
				lowestSection = secheader.ptrRawData;

			uint32_t sectionEnd = 0;
			if (secheader.ptrRawData != 0)
			{
				sectionEnd = secheader.ptrRawData + secheader.rawSize;
			}

			if (sectionEnd > highestSection)
				highestSection = sectionEnd;

			sectionEnd = 0;
			if (secheader.virtualAddress != 0)
			{
				sectionEnd = secheader.virtualAddress + secheader.virtualSize;
			}

			if (sectionEnd > highestVirtualSection)
				highestVirtualSection = sectionEnd;

			if (secheader.virtualAddress < lowestVirtualSection)
				lowestVirtualSection = secheader.virtualAddress;

			//uint32_t relocPointer = secheader.ptrRelocations;
		}

		stream.rewind();

		auto getRealOffset = [&sections, &data] (uint32_t logical) -> uint32_t
		{
			for (const section_header &sec : sections)
			{
				if (logical >= sec.virtualAddress && logical < sec.virtualAddress + sec.virtualSize)
				{
					return (sec.ptrRawData + (logical - sec.virtualAddress));
				}
			}
			return -1;
		};

		uint32_t exportOffset = optheader.exportTable.relativeAddress;
		uint32_t nOffset = 0;
		for (const section_header &sec : sections)
		{
			if (exportOffset >= sec.virtualAddress && exportOffset < sec.virtualAddress + sec.virtualSize)
			{
				nOffset = sec.ptrRawData + (exportOffset - sec.virtualAddress);
				break;
			}
		}

		if (nOffset)
		{
			exp_header = (export_header *)(data.data() + nOffset);

			uint32_t offs = getRealOffset(exp_header->addressFunctions);
			const uint32_t *ptrOffsets =	(const uint32_t *)(data.data() + getRealOffset(exp_header->addressFunctions));
			const uint32_t *ptrNames =		(const uint32_t *)(data.data() + getRealOffset(exp_header->addressNames));
			const uint16_t *ptrOrdinals =	(const uint16_t *)(data.data() + getRealOffset(exp_header->addressNameOrdinals));

			for (uint32_t i = 0; i < exp_header->numberNames; ++i)
			{
				const char *name = (const char *)(data.data() + getRealOffset(ptrNames[i]));
				const uint16_t ordinal = ptrOrdinals[i];
				const uint32_t funcPtr = ptrOffsets[ordinal];

				export_obj exp;
				if (name[0] == '?')
					exp.cpp = true;
				exp.name = name;
				exp.pointer = optheader.ptrImageBase + funcPtr;
				exports.push_back(exp);
			}
		}

		// Allocate virtual space for the headers so we can use it for a stack. Yay!
		//output.resize(lowestVirtualSection, 0xABU);

		output.resize(output.size() + 8);
		*(uint64_t *)((((char *)output.data()) + output.size()) - 8) = sections.size();

		section_header &highest = sections[0];
		lTotalSize = (highest.virtualAddress + highest.virtualSize);

		for each (const section_header &sec in sections)
		{
			size_t nRawSize = sec.rawSize;
			if (nRawSize > sec.virtualSize)
				nRawSize = sec.virtualSize;

			out_section osec(optheader.ptrImageBase + sec.virtualAddress, nRawSize, sec.virtualSize, (u8 *)data.data() + sec.ptrRawData);
			osec.name = sec.name;
			if (osec.name[0] == '.')
				osec.name[0] = '_';
			outSections.push_back(osec);

			u64 writeSize = sec.virtualSize;
			if (writeSize > sec.rawSize)
				writeSize = sec.rawSize;

			size_t sz = writeSize;
			sz += 0xFLL;
			sz &= ~0xFLL;

			output.resize(output.size() + 8);
			*(uint64_t *)((((char *)output.data()) + output.size()) - 8) = optheader.ptrImageBase + sec.virtualAddress;
			output.resize(output.size() + 8);
			*(uint64_t *)((((char *)output.data()) + output.size()) - 8) = uint64_t(sec.rawSize);

			output.resize(output.size() + sz);
			memcpy(
				((char *)output.data() + (output.size() - 1)) - sz,
				(char *)data.data() + sec.ptrRawData,
				writeSize
			);

			if (sec.virtualAddress > highest.virtualAddress)
			{
				highest = sec;
				lTotalSize = (highest.virtualAddress + highest.virtualSize);
			}
		}

		totalSize = highestVirtualSection - lowestVirtualSection;
	}

	*(uint64_t *)output.data() = output.size();

	FILE *fp = fopen(oname, "w");
	if (!fp)
	{
		printf("Could not open file for writing: %s\n", oname);
		return false;
	}

	//fwrite((char *)output.data(), 1, output.size(), fp);

	fprintf(fp, "// Header File bin2hex for PE Binary %s\n// Generated %s\n\n#pragma once\n\n", file, __TIMESTAMP__);

	/*fprintf(fp, "static _align(0x10) const u8 kernelData[] = {\n");

	uint64_t offset = 0;
	while (offset < output.size())
	{
		fprintf(fp, "\t");
		for (int i = 0; offset < output.size() && i < 16; ++i)
		{
			uint8_t byte = uint8_t(output[offset]);

			if (offset == output.size() - 1)
			{
				fprintf(fp, "0x%02X", uint32_t(byte));
			}
			else
			{
				fprintf(fp, "0x%02X, ", uint32_t(byte));
			}
			
			offset++;
		}
		fprintf(fp, "\n");
	}

	fprintf(fp, "};\n\n");
	fprintf(fp, "static const u64 kernelSize = %uULL;\n", output.size());*/
	fprintf(fp, "static const u64 kernelBase = 0x%016I64XULL;\n", baseAddress);
	fprintf(fp, "static const u64 kernelSize = %I64uULL;\n", lTotalSize);

	/*u64 hash = getHash((u8 *)output.data(), output.size());

	fprintf(fp, "static const u64 kernelHash = 0x%016I64XULL;\n",
		u32(hash >> 32),
		u32(hash)
	);*/

	fprintf(fp, "struct sectionMap\n{\n\tconst u64\tmSectionLogicalSize;\n\tconst u8\t*mSectionLogicalAddress;\n\t\n\tconst u64\tmSectionRawSize;\n\tconst u8\t*mSectionRawAddress;\n\t\n\tconst u64\tmHash;\n};\n");

	fprintf(fp, "static const u64 numSections = %I64uULL;\n", u64(outSections.size()));

	size_t iter = 0;
	for (const out_section &osec : outSections)
	{
		size_t offset = 0;
		if (osec.raw_size == 0)
			continue;
		else
			fprintf(fp, "static _align(0x10) const u8 sectionData_%u[] = {\n\t", u32(iter++));
		while (offset < osec.raw_size)
		{
			for (int i = 0; offset < osec.raw_size && i < 16; ++i)
			{
				uint8_t byte = osec.raw_data[offset];

				if (offset == osec.raw_size - 1)
				{
					fprintf(fp, "0x%02X", uint32_t(byte));
				}
				else
				{
					fprintf(fp, "0x%02X, ", uint32_t(byte));
				}
			
				offset++;
			}
			if (offset == osec.raw_size)
				fprintf(fp, "\n");
			else
				fprintf(fp, "\n\t");
		}
		fprintf(fp, "};\n");
	}

	fprintf(fp, "\nstatic const sectionMap sectionMaps[numSections] = {\n");
	iter = 0;
	for (const out_section &osec : outSections)
	{
		if (osec.raw_size)
		{
			fprintf(fp,
				"\t{\n\t\t0x%016I64XULL,\n\t\t(u8 *)0x%016I64XULL,\n\t\t0x%016I64XULL,\n\t\tsectionData_%u,\n\t\t0x%016I64XULL\n\t}%c\n",
				osec.virtual_size,
				osec.logical_address,
				osec.raw_size,
				u32(iter++),
				getHash(osec.raw_data, osec.raw_size),
				(iter == outSections.size() - 1) ? ' ' : ','
			);
		}
		else
		{
			fprintf(fp,
				"\t{\n\t\t0x%016I64XULL,\n\t\t(u8 *)0x%016I64XULL,\n\t\t0x%016I64XULL,\n\t\tnullptr,\n\t\t0x%016I64XULL\n\t}%c\n",
				osec.virtual_size,
				osec.logical_address,
				osec.raw_size,
				0ULL,
				(iter == outSections.size() - 1) ? ' ' : ','
			);
		}
	}

	fprintf(fp, "};\n\n");

	iter = 0;
	for (const out_section &osec : outSections)
	{
		fprintf(fp, "static const sectionMap &section_%s =\t\tsectionMaps[%u];\n", osec.name.c_str(), iter++);
	}

//	export_header *header = data.data() + optheader.

	fprintf(fp, "\nnamespace symbols\n{\n");
	for (const export_obj &exp : exports)
	{
		if (exp.cpp)
			fprintf(fp, "\t// Could not export: Not using C-style decoration: %s\n", exp.name.c_str());
		else
			fprintf(fp, "\tstatic void * const ptr_%s =\t\t(void * const)0x%016I64XULL;\n", exp.name.c_str(), exp.pointer);
	}
	fprintf(fp, "}\n");

	fclose(fp);

	return true;
}

int main (int argc, const char **argv)
{
	for (int i = 1; i < argc; i += 2)
	{
		// Process Each File
		bool success = rebase(argv[i], argv[i + 1]);
		if (!success)
		{
			printf("Failed to process %s\n", argv[i]);
			return 1;
		}
	}

	return 0;
}
#include <cstdio>

#include <vector>
#include <string>

#include "pe_structs.hpp"

using namespace std;

bool rebase (const char *file, const char *oname)
{
	static const uint32_t BASE = 0x100000U;

	vector<char> output;

	uint32_t lowestSection;
	vector<char> data;

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

			if (
				secheader.name[0] == '.' &&
				secheader.name[1] == 'C' &&
				secheader.name[2] == 'R' &&
				secheader.name[3] == 'T'
				)
			{
				//printf("This binary has a .CRT section - double check for initializers/destructors!\n");
				//return false;
				continue;
			}

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

		// Allocate virtual space for the headers so we can use it for a stack. Yay!
		output.resize(lowestVirtualSection, 0xABU);

		for each (const section_header &sec in sections)
		{
			uint32_t secend = sec.virtualAddress + sec.virtualSize;

			uint64_t writeSize = sec.virtualSize;
			if (writeSize > sec.rawSize)
				writeSize = sec.rawSize;

			if (output.size() < secend)
				output.resize(secend, 0x00);
			memcpy(
				(char *)output.data() + sec.virtualAddress,
				(char *)data.data() + sec.ptrRawData,
				writeSize
			);
		}

		uint32_t multibootOffset = uint32_t(lowestVirtualSection);
		uint32_t multibootSearch = uint32_t(output.size());
		if (multibootSearch > 0x2000 + lowestVirtualSection)
			multibootSearch = 0x2000 + lowestVirtualSection;

		mb_header *mbheader;

		while (multibootOffset < multibootSearch - 4)
		{
			mb_header &headr = *(mb_header *)((char *)output.data() + multibootOffset);

			if (headr.m_Magic == 0x1BADB002)
			{
				mbheader = &headr;
				goto found;
			}

			multibootOffset++;
		}
		printf("No Multiboot header found in the first 0x1000\n");
		return false;
	found:;

		uint32_t BSS_Size = uint32_t(output.size());
		uint32_t preZeroSize = BSS_Size;
		while (output[preZeroSize - 1] == 0)
		{
			--preZeroSize;
		}

		output.resize(preZeroSize);

		mbheader->m_HeaderAddress = BASE + multibootOffset;
		mbheader->m_LoadAddress = BASE + lowestVirtualSection;
		mbheader->m_EntryAddress = BASE + optheader.ptrEntryPoint;
		mbheader->m_LoadEndAddress = BASE + uint32_t(output.size());
		mbheader->m_BSSEndAddress = BASE + BSS_Size;

	}

	FILE *fp = fopen(oname, "wb");
	if (!fp)
	{
		printf("Could not open file for writing: %s\n", oname);
		return false;
	}

	fwrite((char *)output.data(), 1, output.size(), fp);

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
		}
	}

	return 0;
}
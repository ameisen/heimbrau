#pragma once

#include "common.hpp"

static u64 getHash (const u8 *data, u64 size)
{
	u64 startHash = 0;
	u8 *bytes = (u8 *)&startHash;

	startHash = 0;

	for (u64 i = 0; i < size; ++i)
	{
		u8 byte = data[i];
		bytes[0] += byte;

		for (int ii = 0; ii < 6; ++ii)
		{
			bytes[ii + 1] += bytes[ii];
			bytes[ii + 2] += bytes[ii];
		}
	}

	return startHash;
}
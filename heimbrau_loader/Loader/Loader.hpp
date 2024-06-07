#pragma once

#include "common.hpp"

#include "../Multiboot2/Multiboot2.hpp"

namespace loader
{
	extern __declspec(noreturn) void entry (const multiboot2::info &mbinfo, u64 magic);
}

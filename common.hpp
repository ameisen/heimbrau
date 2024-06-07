#pragma once

typedef unsigned char		u8;
typedef unsigned short		u16;
typedef unsigned int		u32;
typedef unsigned long long	u64;

typedef signed char			s8;
typedef signed short		s16;
typedef signed int			s32;
typedef signed long long	s64;

typedef u64					uptr;
typedef s64					sptr;

typedef u64					usize;

#define _naked				__declspec(naked)
#define _export				__declspec(dllexport)
#define _align(x)			__declspec(align(x))
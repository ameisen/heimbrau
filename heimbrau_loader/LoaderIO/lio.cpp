#include "lio.hpp"

_align(16) lio::character * const lio::s_VideoOut = (lio::character *)0xb8000;
_align(16) lio::character * const lio::s_VideoBackbuffer = lio::s_VideoOut + (80 * 25);

u32 lio::_WriteHandler::s_Count = 0;
u32 lio::_WriteHandler::s_DirtyLines = 0;

u32 lio::s_CurrentX = 0;
u32 lio::s_CurrentY = 0;

lio::character::color lio::s_CurrentColor;

void lio::putc (char c)
{
	_WriteHandler _handler;

	const bool isNewLine = c == '\n' || c == '\r';

	if (s_CurrentX == 80 || isNewLine)
	{
		s_CurrentX = 0;
		++s_CurrentY;
		if (s_CurrentY == 25)
		{
			--s_CurrentY;
			_ShiftUp();
			_WriteHandler::s_DirtyLines |= 0x1FFFFFF; // 0b...25
		}
	}
	if (!isNewLine)
	{
		_WriteHandler::s_DirtyLines |= 1 << s_CurrentY;
		s_VideoBackbuffer[s_CurrentY * 80 + s_CurrentX++].set(c, s_CurrentColor);
	}
}

void lio::puts (const char *s)
{
	_WriteHandler _handler;

	while (*s)
	{
		putc(*s++);
	}
}
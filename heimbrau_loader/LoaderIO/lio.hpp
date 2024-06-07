#pragma once

#include "common.hpp"
#include "heimbrau_asm\hb_asm.hpp"

// All bitfields assume that the bits will be in order. This is what VC and GCC do.
#include <cstdint>

namespace lio
{
	class character
	{
	public:
		class color
		{
			union
			{
				struct
				{
					u8	m_ForegroundColor	: 4;
					u8	m_BackgroundColor	: 3;
					u8	m_FlagBit			: 1;
				};
				u8		m_Data;
			};
		public:
			enum color_value
			{
				BLACK =		0x0,
				BLUE,
				GREEN,
				CYAN,
				RED,
				MAGENTA,
				BROWN,
				LIGHT_GRAY,
				DARK_GRAY,
				BRIGHT_BLUE,
				BRIGHT_GREEN,
				BRIGHT_CYAN,
				BRIGHT_RED,
				BRIGHT_MAGENTA,
				BRIGHT_YELLOW,
				WHITE
			};
		public:
			color () : m_Data(0) {}
			color (color_value fg, color_value bg = color_value(BLACK)) : m_ForegroundColor(u8(fg)), m_BackgroundColor(u8(bg)) {}

			inline operator u8 () const { return m_Data; }

			void setForeground (color_value fg)
			{
				m_ForegroundColor = u8(fg);
			}

			void setBackground (color_value bg)
			{
				m_BackgroundColor = u8(bg);
			}
		};

	private:
		union
		{
			struct
			{
				u16	m_Character	: 8;
				u16	m_Color		: 8;
			};
			u16		m_Data;
		};
	public:
		character () : m_Character(0), m_Color(color(color::LIGHT_GRAY, color::BLACK)) {}
		character (char c, color cl = color(color::LIGHT_GRAY, color::BLACK)) : m_Character(c), m_Color(cl) {}

		character operator = (char c) { m_Character = c; m_Color = color(color::LIGHT_GRAY, color::BLACK); return *this; }

		inline void set_character (char c) volatile
		{
			m_Character = c;
		}

		inline void set_color (color c) volatile
		{
			m_Color = c;
		}

		inline void set (char c, color cl = color(color::LIGHT_GRAY, color::BLACK)) volatile
		{
			m_Character = c;
			m_Color = cl;
		}

		inline void clear () volatile
		{
			m_Data = 0;
		}
	};

	extern _align(16) character * const s_VideoOut;
	extern _align(16) character * const s_VideoBackbuffer;
	extern character::color s_CurrentColor;

	static void init ()
	{
		native::memset_16(s_VideoOut, 0, 80 * 25 * 2);
		native::memset_16(s_VideoBackbuffer, 0, 80 * 25 * 2);
		s_CurrentColor = character::color(character::color::LIGHT_GRAY, character::color::BLACK);
	}

	static void setForegroundColor (character::color::color_value fg)
	{
		s_CurrentColor.setForeground(fg);
	}

	static void setBackgroundColor (character::color::color_value bg)
	{
		s_CurrentColor.setBackground(bg);
	}

	static void setColor (character::color col)
	{
		s_CurrentColor = col;
	}

	static void _SwapBackBuffer (u32 dirty)
	{
		if (dirty == 0x1FFFFFF)
		{
			static const usize sc_bufferSizeD8 = (80 * 25 * sizeof(character));
			native::memcpy_16(s_VideoOut, s_VideoBackbuffer, sc_bufferSizeD8);
		}
		else
		{
			// line by line
			//for (int i = 24; i >= 0; --i)
			int baseI = 0;
			int sizeI = 0;
			for (int i = 0; i < 25; ++i)
			{
				if (dirty & (1 << i))
				{
					++sizeI;
				}
				else
				{
					if (sizeI)
					{
						// This line be dirty!
						native::memcpy_16(s_VideoOut + (80 * baseI), s_VideoBackbuffer + (80 * baseI), 80 * sizeof(character) * sizeI);
						sizeI = 0;
					}
					baseI = i + 1;
				}
			}
			if (sizeI)
			{
				// This line be dirty!
				native::memcpy_16(s_VideoOut + (80 * baseI), s_VideoBackbuffer + (80 * baseI), 80 * sizeof(character) * sizeI);
				sizeI = 0;
			}
		}
	}

	static void _ShiftUp ()
	{
		native::memcpy_16(s_VideoBackbuffer, (s_VideoBackbuffer + 80), 80 * 24 * 2);
		native::memset_16(s_VideoBackbuffer + (24 * 80), 0, 160);
	}

	class _WriteHandler
	{
		static u32 s_Count;
	public:
		static u32 s_DirtyLines;

		_WriteHandler () { ++s_Count; }
		~_WriteHandler ()
		{ 
			if (--s_Count == 0)
			{
				_SwapBackBuffer(s_DirtyLines);
				s_DirtyLines = 0;
			}
		}
	};

	extern u32 s_CurrentX;
	extern u32 s_CurrentY;

	void putc (char c);
	void puts (const char *s);
	s32 printf (const char* format, ...);
	s32 sprintf (char *out, const char* format, ...);
}
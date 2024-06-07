#include "lio.hpp"
#include <stdarg.h>

/*
	The venerable printf:

	%[flags][width][.precision][length]specifier

	valid specifiers:
	c - char
	d or i - signed integer
	e - scientific notation (mantise/exponent) using e character
	E - scientific notation (mantise/exponent) using E character
	f - float
	g - the shorter of %e or %f
	G - the shorter of %E or %f
	o - signed octal
	s - char*
	u - unsigned integer
	x - unsigned hexadecimal integer
	X - unsigned hexadecimal integer (caps)
	p - pointer address, caps hex (B800:0000)
	n - uses the var to store the number of characters written so far
	% - prints a %
	k - color in the format of XX or X.

	valid flags:
	- - left-justify within the given field width
	+ - forces the sign to be shown even for positive integers
	[space] - same as +, but uses a space for positive integers instead of +
	# - used with o, x or X, for nonzero values, precedes with 0, 0x, or 0X
		used with e, E, and f, forces a decimal point even if no digits follow
		used with g or G, as with e or E but trailing zeros are not removed
	0 - left-pads the number with 0 instead of space if padding is specified

	valid width:
	[number] - minimum number of characters to be printed.
		padded with spaces if length is smaller, otherwise it overflows.
	* - width is not specified in the format string, but with an extra
		integer parameter preceding the argument to be formatted

	valid .precision:
	.[number] - for integers (d, i, o, u, x, X) specifies the minimum number
		of digits to be written, padded with leading zeroes, or overflows.
		if the precision is 0, no character is written for the value 0.
	.* - same as * for width

	valid length:
	h - the argument is interpreted as a short int or unsigned short int
		(applies only to i, d, o, u, x, and X)
	l - the argument is interpreted as a long int or unsigned long int
		(for i, d, o, u, x, and X) or as a wchar_t or wchar_t* for c and s.
	L - the argument is interpreted as a long double (for e, E, f, g, and G).

	returns the number of characters written, or -1 on error.
*/

s32 lio::printf(const char* format, ...)
{
	static const char * const hex_digits_upper = "0123456789ABCDEF";
	static const char * const hex_digits_lower = "0123456789abcdef";

	lio::_WriteHandler _handler;
	_align(16) char buffer[1024];
	native::memset_16(buffer, '\0', 1024);

	va_list args;
	va_start(args, format);
	int32_t written = 0;
	uint32_t pos = 0;

	while(format[pos] != '\0')
	{
		// look for a formatting marker
		while(format[pos] != '%' && format[pos] != '\0')
		{
			lio::putc(format[pos++]);
			written++;
		}

		if(format[pos] == '\0')
		{
			break;
		}

		++pos;

		// handle the escape character first
		if(format[pos] == '%')
		{
			lio::putc('%');
			pos++;
			written++;
			continue;
		}

		// %[flags][width][.precision][length]specifier

		// valid flags: '-', '+', ' ', '#', '0'
		bool left_justify = false;
		bool force_sign = false;
		bool space_positive = false;
		bool hex_indicators = false;
		bool pad_with_zeroes = false;

		bool done = false;

		while(!done)
		{
			switch(format[pos])
			{
			case '-':
				left_justify = true;
				pos++;
				break;
			case '+':
				force_sign = true;
				pos++;
				break;
			case ' ':
				space_positive = true;
				pos++;
				break;
			case '#':
				hex_indicators = true;
				pos++;
				break;
			case '0':
				pad_with_zeroes = true;
				pos++;
				break;
			default:
				done = true;
				break;
			}
		}

		// for width, either * or a number
		int32_t min_width = 0;
		if(format[pos] == '*')
		{
			min_width = va_arg(args, int32_t);
			pos++;
		}
		else
		{
			char c = format[pos];
			while(c >= '0' && c <= '9')
			{
				min_width *= 10;
				min_width += (int32_t)(c - '0');
				c = format[++pos];
			}
		}

		// for precision, a ., then either * or a number (or nothing)
		bool precision_specified = false;
		int32_t precision = 1;
		if(format[pos] == '.')
		{
			precision_specified = true;
			precision = 0;
			pos++;
			if(format[pos] == '*')
			{
				precision = va_arg(args, int32_t);
				pos++;
			}
			else
			{
				char c = format[pos];
				while(c >= '0' && c <= '9')
				{
					precision *= 10;
					precision += (int32_t)(c - '0');
					c = format[++pos];
				}
			}
		}

		// length can be h, l, or L
		bool force_short = false;
		bool force_long = false;
		bool force_long_double = false;

		switch(format[pos])
		{
		case 'h':
			force_short = true;
			pos++;
			break;
		case 'l':
			force_long = true;
			pos++;
			break;
		case 'L':
			force_long_double = true;
			pos++;
			break;
		}

		int32_t buffer_len = 0;

		// The way this will work is we'll put integers in the buffer backwards.
		// This is easier to manage and requires fewer copies.
		bool is_integer = false;

		// now figure out what the specifier is.
		char specifier = format[pos];
		switch(specifier)
		{
		case 'c':
			if(force_long)
			{
				// should interpret as wchar_t...
				return -1;
			}
			else
			{
				buffer[buffer_len++] = va_arg(args, char);
			}
			break;
		case 'e':
		case 'E':
		case 'g':
		case 'G':
		case 'f':
			{
				// need to implement floating point detection stuff in kernel
				// double value = 0.0;
				// figure out how to detect if a float was passed instead of a double.
				// floats should be promoted to double
			}
			break;
		case 'k':
			{
				const uint8_t colorval = va_arg(args, uint8_t);
				union
				{
					uint8_t color;
					struct
					{
						uint8_t foreground : 4;
						uint8_t background : 4;
					};
				};
				color = colorval;
				lio::setBackgroundColor(lio::character::color::color_value(background));
				lio::setForegroundColor(lio::character::color::color_value(foreground));
			}
			break;
		case 'd':
		case 'i':
		case 'o':
		case 'u':
		case 'x':
		case 'X':
		case 'p':
			{
				is_integer = true;
				bool negative = false;
				uint64_t value = 0;

				bool is_hex = (specifier == 'x' || specifier == 'X');

				if(specifier == 'd' || specifier == 'i' || specifier == 'o')
				{
					int64_t signed_value = 0;
					if(force_short)
					{
						signed_value = va_arg(args, int16_t);
					}
					else if(force_long)
					{
						signed_value = va_arg(args, int32_t);
					}
					else if (force_long_double)
					{
						signed_value = va_arg(args, int64_t);
					}
					else
					{
						signed_value = va_arg(args, int32_t);
					}

					if(signed_value < 0)
					{
						negative = true;
						signed_value = -signed_value;
					}

					value = (int64_t)signed_value;
				}
				else if(specifier == 'p')
				{
					value = (uint64_t)va_arg(args, void *);
				}
				else
				{
					if(force_short)
					{
						value = va_arg(args, uint16_t);
					}
					else if(force_long)
					{
						value = va_arg(args, uint32_t);
					}
					else if (force_long_double)
					{
						value = va_arg(args, uint64_t);
					}
					else
					{
						value = va_arg(args, uint64_t);
					}
				}

				if(precision > 0 || value > 0)
				{
					if(specifier == 'o')
					{
						// output in octal
						va_end(args);
						return -1;
					}
					else if(is_hex || specifier == 'p')
					{
						if(specifier == 'p' && precision < 8)
						{
							precision = 8;
						}

						// we'll use this to toggle the case of the hex letters.
						const char* hex_digits = (specifier == 'x' ? hex_digits_lower : hex_digits_upper);

						while(value > 0)
						{
							buffer[buffer_len++] = hex_digits[value & 0xF];
							value >>= 4;
						}

						while(buffer_len < precision)
						{
							buffer[buffer_len++] = '0';
						}
							
						if(hex_indicators)
						{
							buffer[buffer_len++] = (specifier == 'x' ? 'x' : 'X');
							buffer[buffer_len++] = '0';
						}
					}
					else
					{
						while(value > 0)
						{
							buffer[buffer_len++] = (char)('0' + (value % 10));
							value /= 10;
						}

						if(buffer_len < precision)
						{
							while(buffer_len < precision)
							{
								buffer[buffer_len++] = '0';
							}
						}

						if(negative)
						{
							buffer[buffer_len++] = '-';
						}
						else if(force_sign)
						{
							buffer[buffer_len++] = '+';
						}
						else if(space_positive)
						{
							buffer[buffer_len++] = ' ';
						}
					}
				}
			}
			break;
		case 's':
			if(force_long)
			{
				// should interpret as wchar_t*...
				va_end(args);
				return -1;
			}
			else
			{
				const char* str = va_arg(args, const char *);
				int64_t str_pos = 0;
				if(precision_specified)
				{
					while(str_pos < precision && str[str_pos] != '\0')
					{
						buffer[buffer_len++] = str[str_pos++];
					}
				}
				else
				{
					while(str[str_pos] != '\0')
					{
						buffer[buffer_len++] = str[str_pos++];
					}
				}
			}
			break;
		case 'n':
			{
				int32_t* dest = va_arg(args, int32_t *);
				*dest = written;
			}
			break;
		default:
			// unknown specifier
			va_end(args);
			return -1;
		}

		for(int32_t i = buffer_len; i < min_width; ++i)
		{
			// if we're left-justifying the value, add spaces to the end
			// of the buffer
			if(left_justify && !is_integer)
			{
				if (pad_with_zeroes)
					buffer[i] += '0';
				else
					buffer[i] += ' ';
				buffer_len++;
			}
			else
			{
				if (pad_with_zeroes)
					lio::putc('0');
				else
					lio::putc(' ');
				written++;
			}
		}

		if(is_integer)
		{
			// remember, numbers we write out backward.
			while(buffer_len-- > 0)
			{
				lio::putc(buffer[buffer_len]);
				buffer[buffer_len] = '\0'; // clear out the piece of the buffer we used
				written++;
			}
		}
		else
		{
			for(int32_t i = 0; i < buffer_len; ++i)
			{
				lio::putc(buffer[i]);
				buffer[i] = '\0'; // clear out the piece of the buffer we used
				written++;
			}
		}

		pos++;
	}

	va_end(args);
	return written;
}

s32 lio::sprintf(char *out, const char* format, ...)
{
	static const char * const hex_digits_upper = "0123456789ABCDEF";
	static const char * const hex_digits_lower = "0123456789abcdef";

	lio::_WriteHandler _handler;
	_align(16) char buffer[1024];
	native::memset_16(buffer, '\0', 1024);

	va_list args;
	va_start(args, format);
	int32_t written = 0;
	uint32_t pos = 0;

	while(format[pos] != '\0')
	{
		// look for a formatting marker
		while(format[pos] != '%' && format[pos] != '\0')
		{
			if (out)
			{
				*out++ = format[pos++];
			}
			written++;
		}

		if(format[pos] == '\0')
		{
			break;
		}

		++pos;

		// handle the escape character first
		if(format[pos] == '%')
		{
			if (out)
			{
				*out++ = '%';
			}
			pos++;
			written++;
			continue;
		}

		// %[flags][width][.precision][length]specifier

		// valid flags: '-', '+', ' ', '#', '0'
		bool left_justify = false;
		bool force_sign = false;
		bool space_positive = false;
		bool hex_indicators = false;
		bool pad_with_zeroes = false;

		bool done = false;

		while(!done)
		{
			switch(format[pos])
			{
			case '-':
				left_justify = true;
				pos++;
				break;
			case '+':
				force_sign = true;
				pos++;
				break;
			case ' ':
				space_positive = true;
				pos++;
				break;
			case '#':
				hex_indicators = true;
				pos++;
				break;
			case '0':
				pad_with_zeroes = true;
				pos++;
				break;
			default:
				done = true;
				break;
			}
		}

		// for width, either * or a number
		int32_t min_width = 0;
		if(format[pos] == '*')
		{
			min_width = va_arg(args, int32_t);
			pos++;
		}
		else
		{
			char c = format[pos];
			while(c >= '0' && c <= '9')
			{
				min_width *= 10;
				min_width += (int32_t)(c - '0');
				c = format[++pos];
			}
		}

		// for precision, a ., then either * or a number (or nothing)
		bool precision_specified = false;
		int32_t precision = 1;
		if(format[pos] == '.')
		{
			precision_specified = true;
			precision = 0;
			pos++;
			if(format[pos] == '*')
			{
				precision = va_arg(args, int32_t);
				pos++;
			}
			else
			{
				char c = format[pos];
				while(c >= '0' && c <= '9')
				{
					precision *= 10;
					precision += (int32_t)(c - '0');
					c = format[++pos];
				}
			}
		}

		// length can be h, l, or L
		bool force_short = false;
		bool force_long = false;
		bool force_long_double = false;

		switch(format[pos])
		{
		case 'h':
			force_short = true;
			pos++;
			break;
		case 'l':
			force_long = true;
			pos++;
			break;
		case 'L':
			force_long_double = true;
			pos++;
			break;
		}

		int32_t buffer_len = 0;

		// The way this will work is we'll put integers in the buffer backwards.
		// This is easier to manage and requires fewer copies.
		bool is_integer = false;

		// now figure out what the specifier is.
		char specifier = format[pos];
		switch(specifier)
		{
		case 'c':
			if(force_long)
			{
				// should interpret as wchar_t...
				return -1;
			}
			else
			{
				buffer[buffer_len++] = va_arg(args, char);
			}
			break;
		case 'e':
		case 'E':
		case 'g':
		case 'G':
		case 'f':
			{
				// need to implement floating point detection stuff in kernel
				// double value = 0.0;
				// figure out how to detect if a float was passed instead of a double.
				// floats should be promoted to double
			}
			break;
		case 'k':
			{
				const uint8_t colorval = va_arg(args, uint8_t);
				union
				{
					uint8_t color;
					struct
					{
						uint8_t foreground : 4;
						uint8_t background : 4;
					};
				};
				color = colorval;
				lio::setBackgroundColor(lio::character::color::color_value(background));
				lio::setForegroundColor(lio::character::color::color_value(foreground));
			}
			break;
		case 'd':
		case 'i':
		case 'o':
		case 'u':
		case 'x':
		case 'X':
		case 'p':
			{
				is_integer = true;
				bool negative = false;
				uint64_t value = 0;

				bool is_hex = (specifier == 'x' || specifier == 'X');

				if(specifier == 'd' || specifier == 'i' || specifier == 'o')
				{
					int64_t signed_value = 0;
					if(force_short)
					{
						signed_value = va_arg(args, int16_t);
					}
					else if(force_long)
					{
						signed_value = va_arg(args, int32_t);
					}
					else if (force_long_double)
					{
						signed_value = va_arg(args, int64_t);
					}
					else
					{
						signed_value = va_arg(args, int32_t);
					}

					if(signed_value < 0)
					{
						negative = true;
						signed_value = -signed_value;
					}

					value = (int64_t)signed_value;
				}
				else if(specifier == 'p')
				{
					value = (uint64_t)va_arg(args, void *);
				}
				else
				{
					if(force_short)
					{
						value = va_arg(args, uint16_t);
					}
					else if(force_long)
					{
						value = va_arg(args, uint32_t);
					}
					else if (force_long_double)
					{
						value = va_arg(args, uint64_t);
					}
					else
					{
						value = va_arg(args, uint64_t);
					}
				}

				if(precision > 0 || value > 0)
				{
					if(specifier == 'o')
					{
						// output in octal
						va_end(args);
						if (out) *out = '\0';
						return -1;
					}
					else if(is_hex || specifier == 'p')
					{
						if(specifier == 'p' && precision < 8)
						{
							precision = 8;
						}

						// we'll use this to toggle the case of the hex letters.
						const char* hex_digits = (specifier == 'x' ? hex_digits_lower : hex_digits_upper);

						while(value > 0)
						{
							buffer[buffer_len++] = hex_digits[value & 0xF];
							value >>= 4;
						}

						while(buffer_len < precision)
						{
							buffer[buffer_len++] = '0';
						}
							
						if(hex_indicators)
						{
							buffer[buffer_len++] = (specifier == 'x' ? 'x' : 'X');
							buffer[buffer_len++] = '0';
						}
					}
					else
					{
						while(value > 0)
						{
							buffer[buffer_len++] = (char)('0' + (value % 10));
							value /= 10;
						}

						if(buffer_len < precision)
						{
							while(buffer_len < precision)
							{
								buffer[buffer_len++] = '0';
							}
						}

						if(negative)
						{
							buffer[buffer_len++] = '-';
						}
						else if(force_sign)
						{
							buffer[buffer_len++] = '+';
						}
						else if(space_positive)
						{
							buffer[buffer_len++] = ' ';
						}
					}
				}
			}
			break;
		case 's':
			if(force_long)
			{
				// should interpret as wchar_t*...
				va_end(args);
				if (out) *out = '\0';
				return -1;
			}
			else
			{
				const char* str = va_arg(args, const char *);
				int64_t str_pos = 0;
				if(precision_specified)
				{
					while(str_pos < precision && str[str_pos] != '\0')
					{
						buffer[buffer_len++] = str[str_pos++];
					}
				}
				else
				{
					while(str[str_pos] != '\0')
					{
						buffer[buffer_len++] = str[str_pos++];
					}
				}
			}
			break;
		case 'n':
			{
				int32_t* dest = va_arg(args, int32_t *);
				*dest = written;
			}
			break;
		default:
			// unknown specifier
			va_end(args);
			if (out) *out = '\0';
			return -1;
		}

		for(int32_t i = buffer_len; i < min_width; ++i)
		{
			// if we're left-justifying the value, add spaces to the end
			// of the buffer
			if(left_justify && !is_integer)
			{
				if (pad_with_zeroes)
					buffer[i] += '0';
				else
					buffer[i] += ' ';
				buffer_len++;
			}
			else
			{
				if (pad_with_zeroes)
					if (out)
					{
						*out++ = '0';
					}
				else
					if (out)
					{
						*out++ = ' ';
					}
				written++;
			}
		}

		if(is_integer)
		{
			// remember, numbers we write out backward.
			while(buffer_len-- > 0)
			{
				if (out)
				{
					*out++ = buffer[buffer_len];
				}
				buffer[buffer_len] = '\0'; // clear out the piece of the buffer we used
				written++;
			}
		}
		else
		{
			for(int32_t i = 0; i < buffer_len; ++i)
			{
				if (out)
				{
					*out++ = buffer[i];
				}
				buffer[i] = '\0'; // clear out the piece of the buffer we used
				written++;
			}
		}

		pos++;
	}

	va_end(args);
	if (out) *out = '\0';
	return written;
}

enum sprintf_flags
{
	sprintf_left_justify = 1 << 0,
	sprintf_force_plus_minus = 1 << 1,
	sprintf_fill_missing_sign = 1 << 2,
	sprintf_hashtag = 1 << 3,
	sprintf_left_pad_zero = 1 << 4,
};

typedef void(*_sprint_type_print)(void *p_value, char *buffer, int *write_index, int buffer_length, int flags, int precision);
#define _SPRINTF_PRINTER(_func_name_) void _func_name_ (void *p_value, char *buffer, int *write_index, int buffer_length, int flags, int precision)

void _sprintf_base(char *buffer, int buffer_len, int *b, void *value, int flags, int width, int precision, 
	_sprint_type_print print, _sprint_type_print print_sign)
{
	
	int sign_len = 0;
	if (print_sign)
	{
		print_sign(value, (char *)0, &sign_len, -1, flags, precision);
		width -= sign_len;
	}
	if (print_sign && (flags & sprintf_left_pad_zero)) // if we're padding with zero the sign needs to be before
	{
		print_sign(value, buffer, b, buffer_len, flags, precision);
	}

	int len = 0;
	print(value, (char *)0, &len, -1, flags, precision);
	len = min(buffer_len - *b, len);  // cap to end of buffer
	
	if (width != -1 && !(flags & sprintf_left_justify)) // left pad
	{
		int padding = min(buffer_len - *b, width - len); // cap
		char pad = flags & sprintf_left_pad_zero ? '0' : ' ';
		for (int i = 0; i < padding; i++)
		{
			buffer[(*b)++] = pad;
		}
	}

	if (print_sign && !(flags & sprintf_left_pad_zero)) //if we're padding with spaces it should be after
	{
		print_sign(value, buffer, b, buffer_len, flags, precision);
	}

	print(value, buffer, b, buffer_len, flags, precision);

	if (width != -1 && (flags & sprintf_left_justify)) // right pad
	{
		int padding = min(buffer_len - *b, width - len); // cap
		for (int i = 0; i < padding; i++)
		{
			buffer[(*b)++] = ' ';
		}
	}
}

_SPRINTF_PRINTER(_sprintf_integer_sign_print)
{
	uintmax_t value = *(uintmax_t *)p_value;
	if (buffer_length <= *write_index && buffer_length != -1)return;
	if ((intmax_t)value < 0) { // write negative sign if needed
		if(buffer) buffer[*write_index]='-';
		++*write_index;
	}
	else if (flags & sprintf_force_plus_minus) // write positive if flag is set
	{
		if (buffer) buffer[*write_index] = '+';
		++*write_index;
	}
	else if (flags & sprintf_fill_missing_sign) // write space if specified
	{
		if (buffer) buffer[*write_index] = '*';
		++*write_index;
	}
}

_SPRINTF_PRINTER(_sprintf_integer_print)
{
	uintmax_t value = *(uintmax_t *) p_value;
	uintmax_t value_cpy = value;
	int len = 0;
	do
	{
		value_cpy /= 10;
		++len;
	} while (value_cpy);
	*write_index += len - 1;
	do // write number backwards
	{
		if (buffer_length <= *write_index && buffer_length != -1)return;
		if(buffer) buffer[*write_index] = value % 10 + '0';
		--*write_index;
		value /= 10;
	} while (value);
	*write_index += len+1;
}

_SPRINTF_PRINTER(_sprintf_integer_signed_print)
{
	intmax_t sval = *(intmax_t *)p_value;
	sval = sval < 0 ? -sval : sval; // abs
	_sprintf_integer_print(&sval, buffer, write_index, buffer_length, flags, precision);
}

_SPRINTF_PRINTER(hex_integer_sign_print_lower)
{ // hex prefix behaves as a sign
	uintmax_t value = *(uintmax_t *)p_value;
	if (!(flags & sprintf_hashtag))return;
	if (buffer_length <= *write_index && buffer_length != -1)return;
	if(buffer) buffer[*write_index] = '0';
	++*write_index;
	if (buffer_length <= *write_index && buffer_length != -1)return;
	if (buffer) buffer[*write_index] = 'x';
	++*write_index;
}

_SPRINTF_PRINTER(hex_integer_sign_print_upper)
{// hex prefix behaves as a sign
	uintmax_t value = *(uintmax_t *)p_value;
	if (!(flags & sprintf_hashtag))return;
	if (buffer_length <= *write_index && buffer_length != -1)return;
	if (buffer) buffer[*write_index] = '0';
	++*write_index;
	if (buffer_length <= *write_index && buffer_length != -1)return;
	if (buffer) buffer[*write_index] = 'X';
	++*write_index;
}

_SPRINTF_PRINTER(_sprintf_hex_print_upper)
{
	uintmax_t value = *(uintmax_t *)p_value;
	uintmax_t value_cpy = value;
	int len = 0;
	do
	{
		value_cpy >>= 4;
		++len;
	} while (value_cpy);
	*write_index += len - 1;
	do // write number backwards
	{
		int curr = value & 0x0f;
		if (buffer_length <= *write_index && buffer_length != -1)return;
		if (buffer) buffer[*write_index] = (curr <= 9) ? curr + '0' : curr - 10 + 'A';
		--*write_index;
		value >>= 4;
	} while (value);
	*write_index += len + 1;
}

_SPRINTF_PRINTER(_sprintf_hex_print_lower)
{
	uintmax_t value = *(uintmax_t *)p_value;
	uintmax_t value_cpy = value;
	int len = 0;
	do
	{
		value_cpy >>= 4;
		++len;
	} while (value_cpy);
	*write_index += len - 1;
	do // write number backwards
	{
		int curr = value & 0x0f;
		if (buffer_length <= *write_index && buffer_length != -1)return;
		if (buffer) buffer[*write_index] = (curr <= 9) ? curr + '0' : curr - 10 + 'a';
		--*write_index;
		value >>= 4;
	} while (value);
	*write_index += len + 1;
}

_SPRINTF_PRINTER(_sprintf_octal_print)
{
	uintmax_t value = *(uintmax_t *)p_value;
	uintmax_t value_cpy = value;
	int len = 0;
	do //get length
	{
		value_cpy >>= 3;
		++len;
	} while (value_cpy);

	*write_index += len - 1;
	do // write number backwards
	{
		int curr = value & 0x07;
		if (buffer_length <= *write_index && buffer_length != -1)return;
		if (buffer) buffer[*write_index] = curr + '0';
		--*write_index;
		value >>= 3;
	} while (value);
	*write_index += len + 1;
}

enum FloatFlag
{
	float_default,
	float_denormalized,
	float_inf,
	float_nan,
	float_zero,
};

void extract_double_values(double value, long long int *mantissa, long int *exp, bool *sign, FloatFlag *flag)
{
	union //strict aliasing rules...
	{
		double value_cpy;
		long long int cast_value;
	};
	value_cpy = value;
	int raw_exp = (cast_value >> 52) & (1<<12-1); // shift away mantisa and mask away sign
	*mantissa = cast_value & ((1ull<<53)-1);
	*sign = (cast_value & (1ull<<63)) != 0;
	*exp = raw_exp - 1023;
	if (!raw_exp)
	{
		*flag = float_zero;
	}
	else if (!((~raw_exp)&((1<<12)-1)))
	{
		if (*mantissa) *flag = float_nan;
		else *flag = float_inf;
	}
	else
	{
		//if (*mantissa & 0x0008000000000000)
			//*flag = float_denormalized;
		//else
			*flag = float_default;
	}
}


enum WriteError
{
	write_error_success = 0,
	write_error_not_enough_space = 1,
	write_error_invalid_buffer = 2,
};

inline WriteError maybe_write(char *buffer, int buffer_length, int write_index, char value)
{
	if (buffer_length <= write_index && buffer_length != -1) return write_error_not_enough_space;
	if (buffer)
	{
		buffer[write_index] = value;
		return write_error_invalid_buffer;
	}
	return write_error_success;
}


WriteError _sprintf_print_string(char *buffer, int buffer_length, int *write_index, char *string)
{
	WriteError err = write_error_success;
	int len = strlen(string);
	for (int i = 0; i < len; i++)
	{
		WriteError error = maybe_write(buffer, buffer_length, *write_index, string[i]);
		if (error == write_error_not_enough_space) return err;
		err = (err == write_error_success) ? error : err; // err|=error;
		++*write_index;
	}
	return err;
}

_SPRINTF_PRINTER(_sprintf_double_print_lowercase)
{
	WriteError err;
	double value = *(double *)p_value;
	long long int mantissa; long int exp; bool sign;
	FloatFlag float_flag;
	extract_double_values(value, &mantissa, &exp, &sign, &float_flag);
	if (float_flag == float_inf)
	{
		_sprintf_print_string(buffer, buffer_length, write_index, "infinite");
	}
	else if (float_flag == float_nan)
	{
		_sprintf_print_string(buffer, buffer_length, write_index, "NaN");
	}
	else if (float_flag == float_denormalized)
	{
		_sprintf_print_string(buffer, buffer_length, write_index, "denormalized");
	}
	else
	{
		{//float test
			union
			{
				float ff;
				int x;
			};
			ff = value;
			unsigned int sign = x >> 31;            // get the sign
			int exp = ((x >> 23) & 0xff) - 127;     // get the exponent
			unsigned int man = x & ((1 << 23) - 1); // get the mantissa
			man |= 1 << 23; // set the implicit bit in the mantissa (assumed normalized...)
			
			if (exp >= 0) 
			{
				// if shifting an int with more than the bits in the number would be 0 the if would not be neccssary (it's undefined)

				long long int pre_point_digit = man >> (23 - exp);
				_sprintf_integer_print(&pre_point_digit, buffer, write_index, buffer_length, flags, precision);
			}
			else
			{
				maybe_write(buffer, buffer_length, *write_index, '0');
				++*write_index;
			}
			{ // print the dot
				maybe_write(buffer, buffer_length, *write_index, '.');
				++*write_index;
			}

			unsigned int frac = man & ((1 << (23 - exp)) - 1);
			unsigned int base;

			double log_two_of_ten = 3.32192809489; // log2(10);
			int guaranteed_zeros = -(((double)(exp+1)) / log_two_of_ten);
			if (!guaranteed_zeros)
			{
				base = 1 << (23 - exp);
			}
			else
			{
				base = 1 << (23 - exp) - guaranteed_zeros;
				for (int i = 0; i < guaranteed_zeros; i++)
				{
					maybe_write(buffer, buffer_length, *write_index, '0');
					++*write_index;
					base /= 5;
				}
			}
			
			//2^n-1 / 10 ^ k
			//2^(n-k) / 5^k

			int c = 0;
			while (frac != 0 && c++ < precision)
			{
				frac *= 10;
				long long int tt = frac / base;
				maybe_write(buffer, buffer_length, *write_index, '0'+tt);
				++*write_index;
				frac %= base;
			}
		}
	}
}


_SPRINTF_PRINTER(_sprintf_double_sign_print)
{
	double value = *(double *)p_value;
	if (buffer_length <= *write_index && buffer_length != -1)return;
	if (value < 0) { // write negative sign if needed
		if (buffer) buffer[*write_index] = '-';
		++*write_index;
	}
	else if (flags & sprintf_force_plus_minus) // write positive if flag is set
	{
		if (buffer) buffer[*write_index] = '+';
		++*write_index;
	}
	else if (flags & sprintf_fill_missing_sign) // write space if specified
	{
		if (buffer) buffer[*write_index] = '*';
		++*write_index;
	}
}

_SPRINTF_PRINTER(_sprintf_double_print_scientific_lowercase)
{
	double value = *(double *)p_value;
	long long int mantissa; long int exp; bool sign;
	FloatFlag float_flag;
	extract_double_values(value, &mantissa, &exp, &sign, &float_flag);
	if (float_flag == float_inf)
	{
		const char *inf = "infinite";
		for (int i = 0; i < strlen(inf); i++)
		{
			buffer[*write_index] = inf[i];
		}
	}
	else if (float_flag == float_nan)
	{
		const char *NaN = "NaN";
		for (int i = 0; i < strlen(NaN); i++)
		{
			buffer[*write_index] = NaN[i];
		}
	}
	else if (float_flag == float_denormalized)
	{
		const char *denormalized = "denormalized";
		for (int i = 0; i < strlen(denormalized); i++)
		{
			buffer[*write_index] = denormalized[i];
		}

	}
	else
	{
		double log_10_of_2 = 0.301029995664;
		double b10_exp = log_10_of_2 * exp;
		double b10_exp_round = round(b10_exp);
		double significant = (((double)mantissa) / ((double)0x000fffffffffffff) + 1) * pow(10.0, b10_exp - b10_exp_round);
		_sprintf_base(buffer, buffer_length, write_index, &significant, sprintf_force_plus_minus, 0, precision, _sprintf_double_print_lowercase, _sprintf_double_sign_print);
		WriteError err = maybe_write(buffer, buffer_length, *write_index, 'e');
		if (err == write_error_not_enough_space) return;
		_sprintf_base(buffer, buffer_length, write_index, &b10_exp_round, sprintf_force_plus_minus, 0, precision, _sprintf_double_print_lowercase, _sprintf_double_sign_print);

	}
}


enum sprintf_length_modifier
{
	sprintf_length_none,
	sprintf_length_h,
	sprintf_length_hh,
	sprintf_length_l,
	sprintf_length_ll,
	sprintf_length_j,
	sprintf_length_z,
	sprintf_length_t,
	sprintf_length_L,
};

int DHSTR_snprintf(char *buffer, int buffer_len, char *format, ...)
{
#ifdef NULL_TEMINATE_SNPRINTF
	--buffer_len; 
#endif
	va_list args;
	va_start(args, format);
	int f = 0;
	int b = 0;
	while (format[f] && b < buffer_len)
	{
		if (format[f] == '%')
		{
			++f;
			int flags=0;
			for (;;) // get flags
			{
				switch (format[f])
				{
				case '-':
					flags |= sprintf_left_justify;
					break;
				case '+':
					flags |= sprintf_force_plus_minus;
					break;
				case ' ':
					flags |= sprintf_fill_missing_sign;
					break;
				case '#':
					flags |= sprintf_hashtag;
					break;
				case '0':
					flags |= sprintf_left_pad_zero;
					break;
				default:
					goto end_flags;
				}
				++f;
			}
			end_flags:

			//get width 
			int width=-1;
			if (format[f] == '*')
			{
				width = va_arg(args, int);
			}
			else
			{
				if (format[f] >= '0' && format[f] < '9') width = 0;
				while (format[f] >= '0' && format[f] < '9')
				{
					width *= 10;
					width += format[f++] - '0';
				}
			}
			
			//get precision
			int precision = 6;
			if (format[f] == '.')
			{
				f++;
				if (format[f] == '*')
				{
					precision = va_arg(args, int);
				}
				else
				{
					if (format[f] >= '0' && format[f] < '9') precision = 0;
					while (format[f] >= '0' && format[f] < '9')
					{
						precision *= 10;
						precision += format[f++] - '0';
					}
				}
			}
			
			sprintf_length_modifier length = sprintf_length_none;
			//get length
			{
				char c_0 = format[f + 0];
				char c_1 = format[f + 1];
				if (c_0 == 'h')
				{
					if (c_1 == 'h')
					{
						length = sprintf_length_hh;
						f += 2;
					}
					else
					{
						length = sprintf_length_h;
						f += 1;
					}
				}
				else if (c_0 == 'l')
				{
					if (c_1 == 'l')
					{
						length = sprintf_length_ll;
						f += 2;
					}
					else
					{
						length = sprintf_length_l;
						f += 1;
					}
				}
				else if (c_0 == 'L')
				{
					length = sprintf_length_L;
					f += 1;
				}
				else if (c_0 == 'j')
				{
					length = sprintf_length_j;
					f += 1;
				}
				else if (c_0 == 'z')
				{
					length = sprintf_length_z;
					f += 1;
				}
				else if (c_0 == 't')
				{
					length = sprintf_length_t;
					f += 1;
				}
			}
			union
			{
				intmax_t integer_value;
				long double float_value;
			};
			void *value = &float_value;

			switch (format[f])
			{
			case 'c':
				integer_value = va_arg(args, int); // default argument promotion stuff, ask K&R
				break;
			case 'i':
			case 'd':
			case 'u':
			case 'x':
			case 'X':
			case 'o':
				switch (length)
				{
				case sprintf_length_none:
					integer_value = va_arg(args, int); 
					break;
				case sprintf_length_hh:
					integer_value = va_arg(args, int); // default argument promotion
					break;
				case sprintf_length_h:
					integer_value = va_arg(args, int); // default argument promotion
					break;
				case sprintf_length_l:
					integer_value = va_arg(args, long int);
					break;
				case sprintf_length_ll:
					integer_value = va_arg(args, long long int);
					break;
				case sprintf_length_j:
					integer_value = va_arg(args, intmax_t);
					break;
				case sprintf_length_z:
					integer_value = va_arg(args, size_t);
					break;
				case sprintf_length_t:
					integer_value = va_arg(args, ptrdiff_t);
					break;
				default:
					assert(false && "sprintf cannot combine length modifier with the provided specifier");
					break;
				}
				break;
			case 'p':
			case 's':
			case 'n':
				integer_value = (intmax_t)va_arg(args, void *);
				break;
			case 'f':
			case 'F':
			case 'e':
			case 'E':
			case 'g':
			case 'G':
			case 'a':
			case 'A':
				switch (length)
				{
				case sprintf_length_L:
					//float_value = va_arg(args, long double);
				case sprintf_length_none:
					float_value = va_arg(args, double); // float aint existing, (default argument promotion)
					break;
				}
				break;

			}
			switch (format[f])
			{
			case '%':
				buffer[b++] = '%';
				break;

			case 'c':
				buffer[b++] = (char)integer_value;
				break;

			case 'i':
			case 'd':
			{
				_sprintf_base(buffer, buffer_len, &b, value, flags, width, precision, _sprintf_integer_signed_print, _sprintf_integer_sign_print);
			}break;
			case 'u':
			{
				_sprintf_base(buffer, buffer_len, &b, value, flags, width, precision, _sprintf_integer_print, _sprintf_integer_sign_print);
			}break;

			case 'p': 
			{
				flags |= sprintf_hashtag | sprintf_left_pad_zero;// force the correct flags
				_sprintf_base(buffer, buffer_len, &b, value, flags, width, precision, _sprintf_hex_print_upper, hex_integer_sign_print_lower);
				break;
			}
			case 'x':
			{
				_sprintf_base(buffer, buffer_len, &b, value, flags, width, precision, _sprintf_hex_print_lower, hex_integer_sign_print_lower);
			} break;

			case 'X':
			{
				_sprintf_base(buffer, buffer_len, &b, value, flags, width, precision, _sprintf_hex_print_upper, hex_integer_sign_print_upper);
			} break;

			case 'o':
			{
				_sprintf_base(buffer, buffer_len, &b, value, flags, width, precision, _sprintf_octal_print, 0);
			} break;
			case 'f':
			{
				_sprintf_base(buffer, buffer_len, &b, value, flags, width, precision, _sprintf_double_print_lowercase, _sprintf_double_sign_print);
			} break;
			case 'e':
			{
				_sprintf_base(buffer, buffer_len, &b, value, flags, width, precision, _sprintf_double_print_scientific_lowercase, _sprintf_double_sign_print);
			} break;

			// case f float
			// case F float uppercase
			// case e float science
			// case E float science uppercase
			// case g shortest reprecentation
			// case G shortest reprecentation uppercase
			// case a hex floating point 
			// case A hex floating point uppercase
			// case s string 
			// case n written so far
			}
		f++;
		}

		buffer[b++] = format[f++];
	}
	va_end(args);
#ifdef NULL_TEMINATE_SNPRINTF
	buffer[b] = 0; // do null term
#endif
	return b+1;
}




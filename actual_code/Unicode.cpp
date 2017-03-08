#include "stdint.h"
void utf8_byte_info(unsigned char byte, int *_out_num_more_bytes, int *_out_num_valid_bits)
{
	int num_lo= __lzcnt16((uint16_t)~byte) - 8;
	*_out_num_valid_bits = 7 - num_lo;
	if (num_lo == 0)
	{
		*_out_num_more_bytes = 0;
	}
	else if (num_lo == 1)
	{
		*_out_num_more_bytes = 1;
	}
	else
	{
		*_out_num_more_bytes = num_lo - 1;
	}
}

bool is_initial_byte(unsigned char byte)
{
	int num_lo = __lzcnt16((uint16_t)~byte) - 8;
	return num_lo != 1;
}

void pack(int32_t *_in_out_packed, int num_bits, int value)
{
	*_in_out_packed <<= num_bits;
	*_in_out_packed |= value & ((1<<num_bits)-1);
}
//not right
void pack_r(int32_t *_in_out_packed, int num_bits, int value)
{
	*_in_out_packed |= (value & ((1 << num_bits) - 1))<<num_bits;
}

struct UTF8_ACK
{
	int32_t ackumulated_value;
	int num_left;
};

enum UnicodeCode
{
	unicode_error = -1,
	unicode_stored = 0,
	unicode_pushed = 1,
};

UnicodeCode nextCodepoint(char32_t *codepoint_out, UTF8_ACK *ack, char new_byte)
{
	int valid_bits,more_bytes;
	utf8_byte_info(new_byte, &more_bytes, &valid_bits);
	if (!ack->num_left)
	{
		if (!is_initial_byte(new_byte)) return unicode_error;
		pack(&ack->ackumulated_value, valid_bits, new_byte);
		ack->num_left = more_bytes;
		return unicode_stored;
	}
	else
	{
		pack(&ack->ackumulated_value, valid_bits, new_byte);
		--ack->num_left;
		if (!ack->num_left)
		{
			*codepoint_out = (char32_t)ack->ackumulated_value;
			*ack = {};
		}
	}
}

UnicodeCode to_utf32(char *utf8_buffer, int utf8_len, char32_t *utf32_buffer, int utf32_len)
{
	UTF8_ACK ack = {};
	utf32_index = 0;
	for (int i = 0; i < utf8_len; i++)
	{
		char32_t codepoint;
		UnicodeCode code = nextCodepoint(&codepoint, &ack, utf8_buffer[i]);
		if (code == unicode_pushed)
		{
			if (!utf32_len > utf32_index) return unicode_error;
			utf32_buffer[utf32_index++] = codepoint;
		}
		else
		{
			if (code != unicode_stored)
			{
				return unicode_error;
			}
		}
	}

}






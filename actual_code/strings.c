#define DEFINE_strlen(type)\
 int DHSTR_strlen(type *string)\
{\
	int counter = 0;\
	while (string[counter])\
	{\
		++counter;\
	}\
	return counter;\
}

DEFINE_strlen(char);
DEFINE_strlen(wchar_t);
DEFINE_strlen(char16_t);
DEFINE_strlen(char32_t);

#define DEFINE_CRTCHARFROM(type_to,type_from)\
 type_to *create##type_to##From##type_from(type_from *string)\
{\
	int length = DHSTR_strlen(string);\
	type_to *newString = ((type_to *)alloc_(sizeof(type_from)*(length+1),DHMA_STRING(create##type_to##From##type_from))); \
	\
	for (int i = 0; i < length; i++)\
	{\
		newString[i] = string[i];\
	}\
	newString[length] = 0;\
	return newString;\
}

DEFINE_CRTCHARFROM(char16_t, wchar_t);
DEFINE_CRTCHARFROM(wchar_t, char16_t);
DEFINE_CRTCHARFROM(char, char16_t);
DEFINE_CRTCHARFROM(char16_t, char);

#define DEFINE_MergeStrings(type)\
type* mergeStrings(type *a, type *b)\
{\
	int aLength = DHSTR_strlen(a);\
	int bLength = DHSTR_strlen(b);\
	type *newString = (type *)alloc_(sizeof(type)*(aLength +bLength + 1),"mergeStrings");\
	for (int i= 0; i < aLength; i++)\
	{\
		newString[i] = a[i];\
	}\
	for (int i = 0; i < bLength; i++)\
	{\
		newString[i+aLength] = b[i];\
	}\
	newString[aLength + bLength] = 0;\
	return newString;\
}
DEFINE_MergeStrings(char);
DEFINE_MergeStrings(wchar_t);
DEFINE_MergeStrings(char16_t);

#define DEFINE_Append(type)\
 void append(type **a, type *b)\
{\
	type *ret = mergeStrings(*a, b);\
	free_(*a);\
	*a = ret;\
}
DEFINE_Append(char);
DEFINE_Append(wchar_t);
DEFINE_Append(char16_t);

#define DEFINE_getLast(type)\
 type *getLast(type *string, type target,bool *success)\
{\
	type *lastDot = (type *)0;\
	while (*string)\
	{\
		if (*string == target)lastDot = string;\
		++string;\
	}\
	*success = lastDot;\
	if (lastDot)\
		return lastDot; \
	else\
		return string; \
}

DEFINE_getLast(char)
DEFINE_getLast(char16_t)
DEFINE_getLast(wchar_t)

 char16_t *copyUntil(char16_t *string, char16_t *last_exclusive)
{
	int length = last_exclusive - string; // is this right??;
	char16_t *newString =(char16_t *) alloc_((length+1)*sizeof(char16_t),"getToAsString"); // +1 == '\0'
	for (int i = 0; i < length; i++)
	{
		newString[i] = string[i];
	}
	newString[length] = 0;
	return newString;
}

 char16_t *copyFrom(char16_t *string, char16_t *first_inclusive)
{
	int length = string+DHSTR_strlen(string)-first_inclusive-1; // is this right??;
	char16_t *newString = (char16_t *)alloc_((length + 1) * sizeof(char16_t), "getToAsString"); // +1 == '\0'
	for (int i = 0; i < length; i++)
	{
		newString[i] = first_inclusive[i+1];
	}
	newString[length] = 0;
	return newString;
}

 char16_t *getLastDot(char16_t *string,bool *success)
{
	return getLast(string, '.',success);
}

#define DEFINE_strcmp(type)\
 bool DH_strcmp(type *a, type *b)\
{\
	while (*a && *b)\
	{\
		if (*a != *b) return false;\
		++a;\
		++b;\
	}\
	return !(*a) && !(*b);\
}
DEFINE_strcmp(char);
DEFINE_strcmp(char16_t);
DEFINE_strcmp(wchar_t);

#define DEFINE_strcmp_init(type)\
 bool DH_strcmp_init(type *a, type *b)\
{\
	while (*a && *b)\
	{\
		if (*a != *b) return false;\
		++a;\
		++b;\
	}\
	return true;\
}
DEFINE_strcmp_init(char);
DEFINE_strcmp_init(char16_t);
DEFINE_strcmp_init(wchar_t);



#define DEFINE_strcmpWeak(type)\
 bool strcmpWeak(type *shorter, type *longer)\
{\
	while (*shorter)\
	{\
		if (!*longer)assert(false);\
		if (*shorter != *longer) return false;\
		++shorter;\
		++longer;\
	}\
	return true;\
}

DEFINE_strcmpWeak(char);
DEFINE_strcmpWeak(char16_t);
DEFINE_strcmpWeak(wchar_t);

#define DEFINE_STRCPY(type)\
 type *strcpy(type *string)\
{\
	int length = DHSTR_strlen(string);\
	type *newString = ((type *)alloc_(sizeof(type)*(length+1),DHMA_STRING(strcpy type))); \
	\
	for (int i = 0; i < length; i++)\
	{\
		newString[i] = string[i];\
	}\
	newString[length] = 0;\
	return newString;\
}

DEFINE_STRCPY(char);
DEFINE_STRCPY(char16_t);
DEFINE_STRCPY(wchar_t);

typedef const char cchar;
typedef const char16_t cchar16;
typedef const wchar_t cwchar;

#define DEFINE_STRCONTAINS(type)\
 bool strContains(type *string, type *target)\
{\
	int length = DHSTR_strlen(string);\
	int target_length = DHSTR_strlen(target);\
	\
	for (int i = 0; i < length; i++)\
	{\
		for(int j = 0; j < target_length;j++)\
			if (string[i] == target[j])return true; \
	}\
	return false;\
}

DEFINE_STRCONTAINS(char);
DEFINE_STRCONTAINS(char16_t);
DEFINE_STRCONTAINS(wchar_t);


bool trimEnd(char *string, char *cmp)
{
	char *string_start = string;
	char *cmp_start = cmp;

	//goto end
	while (*string++);
	while (*cmp++);
	while (*--string == *--cmp)
	{
		if (cmp == cmp_start) { *string = 0; return true;};
		if (string == string_start) return false;
	}
	return false;
}
bool startsWith(char *string, char *cmp)
{
	--string;
	--cmp;
	while (*++string == *++cmp)
	{
		if (!cmp) { return true; };
		if (!string) return false;
	}
	return false;
}


// non_ctring stuff.. probs better

struct DHSTR_String
{
	char *start;
	size_t length;
};


#define DHSTR_CPY(string,alloc)(DHSTR_cpy(string,alloc(string.length),string.length));
DHSTR_String DHSTR_cpy(DHSTR_String string, void *buffer, size_t buffer_len)
{
	assert(buffer_len >= string.length);
	char *c_buffer = (char *)buffer;
	for (int i = 0; i < string.length; i++)
	{
		c_buffer[i] = string.start[i];
	}
	DHSTR_String ret = {};
	ret.start = c_buffer;
	ret.length = string.length;
	return ret;
}
//functions that take allocators are instead defined as macros to allow for stack allocations with alloca
#define DHSTR_COMBINED_LENGTH(...)(DHSTR_multi_length(DHMA_NUMBER_OF_ARGS(__VA_ARGS__),__VA_ARGS__))
int DHSTR_multi_length(int number_of_arguments, ...)
{
	va_list args;
	va_start(args, number_of_arguments);
	int ack = 0;
	for (int i = 0; i < number_of_arguments; i++)
	{
		ack += va_arg(args, DHSTR_String).length;
	}
	va_end(args);
	return ack;
}




#define DHSTR_MERGE_MULTI(alloc,...)(DHSTR_multi_merge(alloc(DHSTR_COMBINED_LENGTH(__VA_ARGS__)),DHSTR_COMBINED_LENGTH(__VA_ARGS__),DHMA_NUMBER_OF_ARGS(__VA_ARGS__),__VA_ARGS__));
DHSTR_String DHSTR_multi_merge(void*buffer, int buffer_len, int number_of_arguments,...)
{
	
	va_list args;
	va_start(args, number_of_arguments);
	int index=0;
	char *c_buffer= (char *)buffer;
	for (int i = 0; i < number_of_arguments; i++)
	{
		DHSTR_String string = va_arg(args, DHSTR_String);
		for (int j = 0; j < string.length;j++)
		{
			assert(index < buffer_len);
			c_buffer[index++] = string.start[j];
		}
	}
	va_end(args);
	DHSTR_String ret;
	ret.length = index;
	ret.start = c_buffer;
	return ret;
}

#define DHSTR_COMBINED_STRLEN(...)(DHSTR_combined_strlen(DHMA_NUMBER_OF_ARGS(__VA_ARGS__),__VA_ARGS__))
int DHSTR_combined_strlen(int number_of_arguments, ...)
{
	va_list args;
	va_start(args, number_of_arguments);
	int ack = 0;
	for (int i = 0; i < number_of_arguments; i++)
	{
		ack += DHSTR_strlen(va_arg(args, char *));
	}
	va_end(args);
	return ack;
}

#define DHSTR_MERGE(a,b,alloc)(DHSTR_merge(alloc(a.length+b.length),a.length+b.length,a,b));
DHSTR_String DHSTR_merge(void*buffer, size_t buffer_len, DHSTR_String a, DHSTR_String b)
{
	assert(buffer_len >= a.length + b.length);
	char *c_buffer = (char *)buffer;
	for (int i = 0; i < a.length; i++)
	{
		c_buffer[i] = a.start[i];
	}
	for (int i = 0; i < b.length; i++)
	{
		c_buffer[i + a.length] = b.start[i];
	}
	DHSTR_String ret = {};
	ret.start = c_buffer;
	ret.length = a.length + b.length;
	return ret;
}



enum StringEqMode
{
	string_eq_length_dont_care,
	string_eq_length_matter,
};

bool DHSTR_eq(DHSTR_String a, DHSTR_String b, StringEqMode mode)
{
	if (mode == string_eq_length_matter && a.length != b.length)return false;
	for (int i = 0; i < a.length; i++)
	{
		if (a.start[i] != b.start[i])return false;
	}
	return true;
}

enum StringCmpMode
{
	string_cmp_length_dont_care,
	string_cmp_longer_smaller,
	string_cmp_longer_bigger,
};

bool DHSTR_cmp(DHSTR_String a, DHSTR_String b, StringCmpMode mode)
{
	if (a.length != b.length && mode != string_cmp_length_dont_care)
	{
		return ((a.length > b.length) ^ (string_cmp_longer_smaller)) ? 1 : -1;
	}
	for (int i = 0; i < a.length; i++)
	{
		if (a.start[i] != b.start[i]) return a.start[i] - b.start[i];
	}
	return 0;
}

DHSTR_String DHSTR_subString(DHSTR_String a, int start, int end)
{
	assert(start <= end);
	assert(start >= 0);
	assert(end <= a.length);
	
	DHSTR_String ret;
	ret.start = a.start + start;
	ret.length = end - start;
	return ret;
}

#define DHSTR_INDEX_OF_LAST -1
#define DHSTR_INDEX_OF_FIRST 0

// finding the nth instnace of a given character. use -1 to indicate last
int DHSTR_index_of(DHSTR_String a, char character, int nth, bool *success)
{
	int ack = 0;
	int last_index=-1;
	for (int i = 0; i < a.length; i++)
	{
		if (a.start[i] == character )
		{
			if (nth == ack++)
			{
				*success = true;
				return i;
			}
			if (nth == -1)
			{
				last_index = i;
			}
		}
	}
	if (nth == -1 && last_index != -1)
	{
		*success = true;
		return last_index;
	}
	else
	{
		*success = false;
		return -1;
	}
}

bool DHSTR_eq_start(DHSTR_String a, DHSTR_String b)
{
	for (int i = 0; i < min(a.length,b.length); i++)
	{
		if (a.start[i] != b.start[i])return false;
	}
	return true;
}

bool DHSTR_eq_end(DHSTR_String a, DHSTR_String b)
{
	for (int i = min(a.length, b.length)-1; i >=0 ; i--)
	{
		if (a.start[i] != b.start[i])return false;
	}
	return true;
}

// while wchar_t is retarded, some apis require it. Like fucking windows at times. (It's fucking defined to be char16_t on windows, like come the fuck on.) 
// so we're supporting it. reluctantly.

constexpr size_t constexpr_strlen(char *string)
{
	return *string ? 1 + constexpr_strlen(string + 1):0;
}

//both bellow is the same (although the while is actually producing slightly better asm) I was thinking they'd produce the same. (msvc)
constexpr int hash_string_constexpr(char *string, int s = 0) //allright because cpps constexpr is retarded we're not allowed to use an ackumulator plus loop....stupid cpp...
{
	return *string ? hash_string_constexpr(string + 1, s * 31 + *string) : s;
}

int hash_string(char *string)
{
	int ack = *string;
	while (*++string)
	{
		ack = ack * 31 + *string;
	}
	return ack;
}

int hash_string(DHSTR_String s)
{
	int ack = 0;
	for (int i = 0; i < s.length; i++)
	{
		ack = ack * 31 + s.start[i];
	}
	return ack;
}

constexpr DHSTR_String DHSTR_makeString_(char *string, size_t len)
{
	return { string, len };
}
//#include "..\libs\ConvertUTF.h"
#define DHSTR_STRING_FROM_UTF16(allocator,string) DHSTR_makeString_(string,allocator(DHSTR_strlen(string)*4),DHSTR_strlen(string)*4)
DHSTR_String DHSTR_makeString_(char16_t *string, void *buffer, int bufferlen)
{
	int32_t errors;
	size_t new_len_in_bytes = utf16toutf8((uint16_t *)string, DHSTR_strlen(string) * sizeof(char16_t), (char *) buffer, bufferlen, &errors);
	assert(errors == UTF8_ERR_NONE);
	return DHSTR_makeString_((char *)buffer, new_len_in_bytes); 
}

#define DHSTR_STRING_FROM_UTF32(allocator,string) DHSTR_makeString_(string,allocator(DHSTR_strlen(string)*4),DHSTR_strlen(string)*4)
DHSTR_String DHSTR_makeString_(char32_t *string, void *buffer, int bufferlen)
{
	int32_t errors;
	size_t new_len_in_bytes = utf32toutf8((uint32_t *)string, DHSTR_strlen(string) * sizeof(char32_t), (char *)buffer, bufferlen, &errors);
	assert(errors == UTF8_ERR_NONE);
	return DHSTR_makeString_((char *)buffer, new_len_in_bytes);
}

#define DHSTR_STRING_FROM_WCHART(allocator,string) DHSTR_makeString_(string,allocator(DHSTR_strlen(string)*4),DHSTR_strlen(string)*4)
DHSTR_String DHSTR_makeString_(wchar_t *string, void *buffer, int bufferlen)
{
	if (sizeof(wchar_t) == sizeof(char16_t))return DHSTR_makeString_((char16_t *)string, buffer, bufferlen);
	if (sizeof(wchar_t) == sizeof(char32_t))return DHSTR_makeString_((char32_t *)string, buffer, bufferlen);
	assert(false && "wchar_t has a fucked up size");
}


#define DHSTR_UTF16_FROM_STRING(string,alloc)(DHSTR_utf16FromString(string,alloc(string.length*4+sizeof(char16_t)),string.length*4+sizeof(char16_t)))
//null terminated utf16 string
char16_t *DHSTR_utf16FromString(DHSTR_String string, void *buffer, int bufferlen)
{
	int32_t errors;
	size_t new_len_in_bytes = utf8toutf16(string.start,string.length, (uint16_t *)buffer, bufferlen, &errors);
	assert(errors == UTF8_ERR_NONE);
	assert(new_len_in_bytes < bufferlen);
	((char16_t *)buffer)[new_len_in_bytes / sizeof(char16_t)] = 0;
	return (char16_t *)buffer;
}

#define DHSTR_UTF32_FROM_STRING(string,alloc)(DHSTR_utf32FromString(string,alloc(string.length*4+sizeof(char32_t)),string.length*4+sizeof(char32_t)))
//null terminated utf32 string
char32_t *DHSTR_utf32FromString(DHSTR_String string,void *buffer, int bufferlen)
{
	int32_t errors;
	size_t new_len_in_bytes = utf8toutf32(string.start, string.length, (uint32_t *)buffer, bufferlen, &errors);
	assert(errors == UTF8_ERR_NONE);
	assert(new_len_in_bytes < bufferlen);
	((char32_t *)buffer)[new_len_in_bytes / sizeof(char32_t)] = 0;
	return (char32_t *)buffer;
}


#define DHSTR_WCHART_FROM_STRING(string,alloc)(DHSTR_wchar_tFromString(string,alloc(string.length*4+sizeof(wchar_t)),string.length*4+sizeof(wchar_t)))
wchar_t *DHSTR_wchar_tFromString(DHSTR_String string, void *buffer, int bufferlen)
{
	if (sizeof(wchar_t) == sizeof(char16_t))return (wchar_t *)DHSTR_utf16FromString(string, buffer, bufferlen);
	if (sizeof(wchar_t) == sizeof(char32_t))return (wchar_t *)DHSTR_utf32FromString(string, buffer, bufferlen);
	assert(false && "wchar_t has a fucked up size");
}



#define DHSTR_UTF8_FROM_STRING(string,alloc)(DHSTR_utf8FromString(string,alloc(string.length+sizeof(char)),string.length+sizeof(char)))
//null terminated utf8 string
char *DHSTR_utf8FromString(DHSTR_String string, void* buffer, size_t buffer_len)
{
	assert(buffer_len > string.length);
	char *c_buffer = (char *)buffer;
	for (int i = 0; i < string.length; i++)
	{
		c_buffer[i] = string.start[i];
	}
	c_buffer[string.length] = 0;
	return c_buffer;
}


// a utf8-character cannot take more than 4 bytes. However does it overlapp with utf16 characters taking more than one ?
#define DHSTR_MAKE_STRING(string) DHSTR_makeString_(string,constexpr_strlen(string))

#define NULL_TEMINATE_SNPRINTF

enum sprintf_flags
{
	sprintf_left_justify = 1 << 0,
	sprintf_force_plus_minus = 1 << 1,
	sprintf_fill_missing_sign = 1 << 2,
	sprintf_hashtag = 1 << 3,
	sprintf_left_pad_zero = 1 << 4,
};

typedef int(*_sprint_type_length)(void *p_value, int flags, int precision);
typedef void (*_sprint_type_print)(void *p_value, char *start, int truncated_length, int flags, int precision);
#define _SPRINTF_PRINTER(_func_name_) void _func_name_ (void *p_value, char *start, int trunc_len, int flags, int precision)

void _sprintf_base_int(char *buffer, int buffer_len, int *b, void *value, int flags, int width, int precision, 
	_sprint_type_length length, _sprint_type_print print,
	_sprint_type_length length_sign, _sprint_type_print print_sign)
{
	
	int sign_len = 0;
	if (length_sign)
	{
		sign_len = length_sign(value, flags, precision);
		width -= sign_len;
	}
	if (sign_len && (flags & sprintf_left_pad_zero)) // if we're padding with zero the sign needs to be before
	{
		print_sign(value, &buffer[*b], sign_len, flags, precision);
		*b += sign_len;
	}

	int len = length(value,flags, precision);
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

	if (sign_len && !(flags & sprintf_left_pad_zero)) //if we're padding with spaces it should be after
	{
		print_sign(value, &buffer[*b], sign_len, flags, precision);
		*b += sign_len;
	}

	print(value, &buffer[*b], len,flags, precision);
	*b += len;  // set to end;

	if (width != -1 && (flags & sprintf_left_justify)) // right pad
	{
		int padding = min(buffer_len - *b, width - len); // cap
		for (int i = 0; i < padding; i++)
		{
			buffer[(*b)++] = ' ';
		}
	}
}

int _sprintf_integer_sign_len(void *p_value, int flags, int precision)
{
	uintmax_t value = *(uintmax_t *)p_value;
	if ((intmax_t)value < 0) { // write negative sign if needed
		return 1;
	}
	else if (flags & sprintf_force_plus_minus) // write positive if flag is set
	{
		return 1;
	}
	else if (flags & sprintf_fill_missing_sign) // write space if specified
	{
		return 1;
	}
	return 0;
}

_SPRINTF_PRINTER(_sprintf_integer_sign_print)
{
	uintmax_t value = *(uintmax_t *)p_value;
	if (!trunc_len)return;
	if ((intmax_t)value < 0) { // write negative sign if needed
		*start= '-';
	}
	else if (flags & sprintf_force_plus_minus) // write positive if flag is set
	{
		*start = '+';
	}
	else if (flags & sprintf_fill_missing_sign) // write space if specified
	{
		*start = ' ';
	}

}

int _sprintf_integer_len(void *p_value, int flags, int precision)
{
	uintmax_t value = *(uintmax_t *)p_value;
	int len = 1;
	while (value /= 10) ++len; // number of digits
	return len;
}

_SPRINTF_PRINTER(_sprintf_integer_print)
{
	uintmax_t value = *(uintmax_t *) p_value;
	for (int i = trunc_len - 1; i >= 0; i--) // write number backwards
	{
		start[i] = value % 10 + '0';
		value /= 10;
	}
}

int _sprintf_integer_signed_len(void *p_value, int flags, int precision)
{
	intmax_t sval = *(intmax_t *)p_value;
	sval = sval < 0 ? -sval : sval; // abs
	return _sprintf_integer_len(p_value, flags,precision);
}

_SPRINTF_PRINTER(_sprintf_integer_signed_print)
{
	intmax_t sval = *(intmax_t *)p_value;
	sval = sval < 0 ? -sval : sval; // abs
	_sprintf_integer_print(p_value, start, trunc_len,flags, precision);
}

void _sprintf_integer_unsigned(char *buffer, int buffer_len, int *b, uintmax_t value, int flags, int width, int precision)
{
	_sprintf_base_int(buffer, buffer_len, b, &value, flags, width, precision, _sprintf_integer_len, _sprintf_integer_print,0,0);
}

void _sprintf_integer_signed(char *buffer, int buffer_len, int *b, uintmax_t value, int flags, int width, int precision)
{
	_sprintf_base_int(buffer, buffer_len, b, &value, flags, width, precision,_sprintf_integer_signed_len, _sprintf_integer_signed_print, _sprintf_integer_sign_len, _sprintf_integer_sign_print);
}

int _sprintf_hex_len(void * p_value, int flags, int precision)
{
	uintmax_t value = *(uintmax_t *)p_value;
	int len = 1;
	while (value >>= 4) ++len; // number of digits
	return len;
}

int hex_integer_sign_len(void *p_value, int flags, int precision)
{
	uintmax_t value = *(uintmax_t *)p_value;
	return flags & sprintf_hashtag ? 2 : 0;
}

_SPRINTF_PRINTER(hex_integer_sign_print_lower)
{ // hex prefix behaves as a sign
	uintmax_t value = *(uintmax_t *)p_value;
	if (!(flags & sprintf_hashtag)||trunc_len < 2)return;
	start[0] = '0';
	start[1] = 'x';
}

void hex_integer_sign_print_upper(void *p_value, char *start, int trunc_len, int flags, int precision)
{// hex prefix behaves as a sign
	uintmax_t value = *(uintmax_t *)p_value;
	if (!(flags & sprintf_hashtag)|| trunc_len < 2)return;
	start[0] = '0';
	start[1] = 'X';
}

_SPRINTF_PRINTER(_sprintf_hex_print_upper)
{
	uintmax_t value = *(uintmax_t *)p_value;
	for (int i = trunc_len - 1; i >= 0; i--) // write number backwards
	{
		int curr = value & 0x0f;
		if(curr <= 9)
			start[i] = curr % 10 + '0';
		else
			start[i] = curr - 10  + 'A';
		value >>= 4;
	}
}

_SPRINTF_PRINTER(_sprintf_hex_print_lower)
{
	uintmax_t value = *(uintmax_t *)p_value;
	for (int i = trunc_len - 1; i >= 0; i--) // write number backwards
	{
		int curr = value & 0x0f;
		if (curr <= 9)
			start[i] = curr % 10 + '0';
		else
			start[i] = curr - 10 + 'a';
		value >>= 4;
	}
}

int _sprintf_octal_len(void *p_value, int flags, int precision)
{
	uintmax_t value = *(uintmax_t *)p_value;
	int len = 1;
	while (value >>= 3) ++len; // number of digits
	return len;
}

_SPRINTF_PRINTER(_sprintf_octal_print)
{
	uintmax_t value = *(uintmax_t *)p_value;
	for (int i = trunc_len - 1; i >= 0; i--) // write number backwards
	{
		int curr = value & 0x07;
		start[i] = curr % 10 + '0';
		value >>= 3;
	}
}

void _sprintf_hex_lower(char *buffer, int buffer_len, int *b, uintmax_t value, int flags, int width, int precision)
{
	_sprintf_base_int(buffer, buffer_len, b, &value, flags, width, precision, _sprintf_hex_len, _sprintf_hex_print_lower, hex_integer_sign_len, hex_integer_sign_print_lower);
}

void _sprintf_hex_upper(char *buffer, int buffer_len, int *b, uintmax_t value, int flags, int width, int precision)
{
	_sprintf_base_int(buffer, buffer_len, b, &value, flags, width, precision, _sprintf_hex_len, _sprintf_hex_print_upper, hex_integer_sign_len, hex_integer_sign_print_upper);
}

void _sprintf_octal(char *buffer, int buffer_len, int *b, uintmax_t value, int flags, int width, int precision)
{
	_sprintf_base_int(buffer, buffer_len, b, &value, flags, width,precision, _sprintf_octal_len, _sprintf_octal_print, 0, 0);
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
	int raw_exp = (cast_value >> 52) & 0x7ff; // shift away mantisa and mask away sign
	*mantissa = cast_value & 0x000fffffffffffff;
	*sign = (cast_value & 0x8000000000000000) != 0;
	*exp = raw_exp - 1023;
	if (!raw_exp)
	{
		*flag = float_zero;
	}
	else if (!((~raw_exp)&0x7ff))
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

void _print_double(char *buffer, int buffer_length, double value, int precision)
{
	double pre_dot_digits__d = value <= 0 ? 1 : floor(log10(value)+1);
	int pre_dot_digits = (int)pre_dot_digits__d;
	int dot = 1;
	value = fabs(value);
	float vp = value;
	for (int i = 0; i < precision;i++)vp *= 10;
	for (int i = 1; i <= precision; i++)
	{
		buffer[precision + dot + pre_dot_digits - i] = '0'+((int)vp) % 10;
		vp /= 10;
	}
	buffer[pre_dot_digits + dot-1] = '.';
	for (int i = 1; i <= pre_dot_digits;i++)
	{
		buffer[pre_dot_digits - i] = '0' + ((int)value) % 10;
		value /= 10;
	}
}

_SPRINTF_PRINTER(_sprintf_double_print_lowercase)
{
	double value = *(double *)p_value;
	long long int mantissa; long int exp; bool sign;
	FloatFlag float_flag;
	extract_double_values(value, &mantissa, &exp, &sign, &float_flag);
	if (float_flag == float_inf)
	{
		start[0] = 'i';
		start[1] = 'n';
		start[2] = 'f';
		start[3] = 'i';
		start[4] = 'n';
		start[5] = 'i';
		start[6] = 't';
		start[7] = 'e';
	}
	else if (float_flag == float_nan)
	{
		start[0] = 'N';
		start[1] = 'a';
		start[2] = 'N';
	}
	else if (float_flag == float_denormalized)
	{
		start[0] =  'd';
		start[1] =  'e';
		start[2] =  'n';
		start[3] =  'o';
		start[4] =  'r';
		start[5] =  'm';
		start[6] =  'a';
		start[7] =  'l';
		start[8] =  'i';
		start[9] =  'z';
		start[10] = 'e';
		start[11] = 'd';
	}
	else
	{
		_print_double(start, trunc_len, value, precision);
	}
}

int _sprintf_double_len(void *p_value, int flags, int precision)
{
	double value = *(double *)p_value;
	long long int mantissa; long int exp; bool sign;
	FloatFlag float_flag;
	extract_double_values(value, &mantissa, &exp, &sign,&float_flag);
	if (float_flag == float_inf)
	{
		return 8; // "infinite" is 8 character
	}
	else if (float_flag == float_nan)
	{
		return 3;
	}
	else if (float_flag == float_denormalized)
	{
		return 12; //denormalized
	}
	else
	{
		int pre_dot_digits = value <= 0 ? 1 : floor(log10(value) + 1);
		return pre_dot_digits + 1 + precision;
	}
}

int _sprintf_double_sign_len(void *p_value, int flags,int precision)
{
	double value = *(double *)p_value;
	if ((intmax_t)value < 0 || (flags & sprintf_force_plus_minus) || (flags & sprintf_fill_missing_sign)) // write space if specified
	{
		return 1;
	}
	return 0;
}

_SPRINTF_PRINTER(_sprintf_double_sign_print)
{
	double value = *(double *)p_value;
	if (!trunc_len)return;
	if ((intmax_t)value < 0) { // write negative sign if needed
		*start = '-';
	}
	else if (flags & sprintf_force_plus_minus) // write positive if flag is set
	{
		*start = '+';
	}
	else if (flags & sprintf_fill_missing_sign) // write space if specified
	{
		*start = ' ';
	}
}

void _sprintf_float(char *buffer, int buffer_len, int *b, double value, int flags, int width, int precision)
{
	_sprintf_base_int(buffer, buffer_len, b, &value, flags, width, precision,_sprintf_double_len, _sprintf_double_print_lowercase, _sprintf_double_sign_len, _sprintf_double_sign_print);
}

_SPRINTF_PRINTER(_sprintf_double_print_scientific_lowercase)
{
	double value = *(double *)p_value;
	long long int mantissa; long int exp; bool sign;
	FloatFlag float_flag;
	extract_double_values(value, &mantissa, &exp, &sign, &float_flag);
	if (float_flag == float_inf)
	{
		start[0] = 'i';
		start[1] = 'n';
		start[2] = 'f';
		start[3] = 'i';
		start[4] = 'n';
		start[5] = 'i';
		start[6] = 't';
		start[7] = 'e';
	}
	else if (float_flag == float_nan)
	{
		start[0] = 'N';
		start[1] = 'a';
		start[2] = 'N';
	}
	else if (float_flag == float_denormalized)
	{
		start[0] = 'd';
		start[1] = 'e';
		start[2] = 'n';
		start[3] = 'o';
		start[4] = 'r';
		start[5] = 'm';
		start[6] = 'a';
		start[7] = 'l';
		start[8] = 'i';
		start[9] = 'z';
		start[10] = 'e';
		start[11] = 'd';
	}
	else
	{
		double log_10_of_2 = 0.301029995664;
		double b10_exp = log_10_of_2 * exp;
		double b10_exp_round = round(b10_exp);
		double significant = (((double)mantissa) / ((double)0x000fffffffffffff) + 1) * pow(10.0, b10_exp - b10_exp_round);
		int i = 0;
		_sprintf_float(start, trunc_len, &i, significant, sprintf_force_plus_minus, 0, precision);
		start += i;
		*start = 'e';
		++start;
		int q = 0;
		_sprintf_float(start, trunc_len, &q, b10_exp_round, sprintf_force_plus_minus, 0, precision);
	}
}

void _sprintf_float_science(char *buffer, int buffer_len, int *b, double value, int flags, int width, int precision)
{
	_sprintf_base_int(buffer, buffer_len, b, &value, flags, width, precision, _sprintf_double_len, _sprintf_double_print_scientific_lowercase, _sprintf_double_sign_len, _sprintf_double_sign_print);
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
					if (format[f] >= '0' && format[f] < '9') width = 0;
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
			intmax_t integer_value;
			long double float_value;
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
				_sprintf_integer_signed(buffer, buffer_len, &b, integer_value, flags, width, precision);
			}break;
			case 'u':
			{
				_sprintf_integer_unsigned(buffer, buffer_len, &b, integer_value, flags, width, precision);
			}break;

			case 'p': 
			{
				flags |= sprintf_hashtag | sprintf_left_pad_zero;// force the correct flags
				width = sizeof(void *)*2+2; //+2 for the ox
				_sprintf_hex_lower(buffer, buffer_len, &b, integer_value, flags, width, precision);
				break;
			}
			case 'x':
			{
				_sprintf_hex_lower(buffer, buffer_len, &b, integer_value, flags, width, precision);
			} break;

			case 'X':
			{
				_sprintf_hex_upper(buffer, buffer_len, &b, integer_value, flags, width, precision);
			} break;

			case 'o':
			{
				_sprintf_octal(buffer, buffer_len, &b, integer_value, flags, width, precision);
			} break;
			case 'f':
			{
				_sprintf_float(buffer, buffer_len, &b, float_value, flags, width, precision);
			} break;
			case 'e':
			{
				_sprintf_float_science(buffer, buffer_len, &b, float_value, flags, width, precision);
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
void test_the_printf()
{
	char buffer[1000];
	double d = -1;
	OutputDebugString("correct\n");
	snprintf(buffer, ARRAY_LENGTH(buffer), "Hello, I'm '%e' years old.\n", d);
	OutputDebugString(buffer);
	snprintf(buffer, ARRAY_LENGTH(buffer), "Hello, I'm aaaaaaaaa'%e' years old.\n", 200.0);
	OutputDebugString(buffer);
	OutputDebugString("mine\n");
	DHSTR_snprintf(buffer, ARRAY_LENGTH(buffer), "Hello, I'm '%e' years old.\n", d);
	OutputDebugString(buffer);
	DHSTR_snprintf(buffer, ARRAY_LENGTH(buffer), "Hello, I'm aaaaaa '%e' years old.\n", 200.0);
	OutputDebugString(buffer);
}







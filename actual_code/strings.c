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


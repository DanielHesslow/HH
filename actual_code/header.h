#ifndef HEADER
#define HEADER
//although the actual name of the marker where text is to be inserted is a called a cursor I call it caret to disambigueate between that and the mouse cursor.
// I don't know if that is a good decision but that is how I do it. well sometimes I don't so now it's real wierd.

typedef int8_t 	int8; 
typedef int16_t int16; 
typedef int32_t int32; 
typedef int64_t int64; 

typedef uint8_t  uint8; 
typedef uint16_t uint16;
typedef uint32_t uint32; 
typedef uint64_t uint64;

#define internal 		static
#define local_persist 	static
#define global_variable static
struct TextBuffer;
struct Data;



// 		---	     HISTORY

// further more there should be event markers so you can remove last 'event'
// you don't care if the cut first moves to the right then deletes the word 
// or if it's the other way around 
// it's the result that is important.
// appending enter should also add an even marker... Never remove more than a line of text in one undo.

struct Location
{
	int line;
	int column;
};


struct Rect
{
	int x;
	int y;
	int width;
	int height;
};

#include "History.h"
#include "MultiGapBuffer.h"

struct ColorChange
{
	int index;
	int color;
};

struct Bitmap
{
	void* memory;
	int width;
	int height;
	int stride;
	int bytesPerPixel;
};


#define DHEN_NAME InputType
#define DHEN_PREFIX input
#define DHEN_VALUES X(character) X(key) X(mouseWheel)
#include "enums.h"
#undef DHEN_VALUES
#undef DHEN_NAME
#undef DHEN_PREFIX


#define DHEN_NAME ENUM_NAME
#define DHEN_VALUES X(enumCHAR) X(emumMOUSEWHEEL)
#include "enums.h"
#undef DHEN_VALUES
#undef DHEN_NAME

struct Input
{
	InputType inputType;
	union
	{
		char character;
		uint16_t VK_Code;
		int16_t mouseWheel;
	};
	bool caps;
	bool control;
	bool shift;
	bool alt;
};


enum AST_Type
{
	ast_type,			// bools ints etc.
	ast_keyword,		// if else case etc.
	ast_struct,			// structs, enums, unions
	ast_function,		// functions
};


struct identifier
{
	AST_Type type;
	int hashedName;
};

struct CharBitmap
{
	uint8_t *memory;
	int colorStride;
	int height;
	int width;
	
	char character;
	float scale;
	int xOff;
	int yOff;
};



typedef char16_t *PCHAR16;

DEFINE_DynamicArray(identifier)

DEFINE_DynamicArray(ColorChange)
DEFINE_DynamicArray(CharBitmap)
DEFINE_DynamicArray(PCHAR16)




//---Bindings
enum Mods
{
	mod_none = 0,
	mod_alt = 1 << 1,
	mod_control = 1 << 2,
	mod_shift = 1 << 3,
	mod_shift_left = 1 << 4 | mod_shift,
	mod_shift_right = 1 << 5 | mod_shift,
	mod_control_left = 1 << 6 | mod_control,
	mod_control_right = 1 << 7 | mod_control,
	mod_alt_right = 1 << 8 | mod_alt,
	mod_alt_left = 1 << 9 | mod_alt,
};

Mods operator | (Mods a, Mods b) { return static_cast<Mods>(static_cast<int>(a) | static_cast<int>(b)); }

struct Argument
{
	Mods mods;
};


enum ModMode
{
	atLeast,
	atMost,
	precisely,
};

enum FuncType
{
	Arg_TextBuffer,
	Arg_TextBuffer_Mods,
	Arg_Data,

};

struct KeyBinding
{
	char VK_Code;
	Mods mods;
	ModMode modMode;
	FuncType funcType;
	union //  appearently this is implementation defined behaviour... well what eves...
	{
		void(*func_TextBuffer_Mods)(TextBuffer *buffer, Mods mods);
		void(*func_TextBuffer)(TextBuffer *buffer);
		void(*func_Data)(Data *data);
		void *funcP;
	};
	int last_event_peeked;
};

DEFINE_DynamicArray(KeyBinding);

enum BufferType
{
	commandLineBuffer,
	regularBuffer,
	ASTBuffer,
	HistoryBuffer,
};

struct BindingIdentifier
{
	int id;
	void *function;
};

bool bind_ident_eq(BindingIdentifier a, BindingIdentifier b)
{
	return a.id == b.id && a.function == b.function;
}
typedef void *PVOID;

unsigned int silly_bind_ident_hash(BindingIdentifier ident) {
	return silly_hash(ident.id) * 31 + silly_hash((int)(long long)(ident.function));
}

unsigned int silly_hash_void_ptr(void *ptr) {
	return silly_hash((int)(long long)(ptr));
}
unsigned int ptr_eq(void *a, void *b) {
	return a == b;
}

struct HistoryChangeTracker
{
	int next_index;
};

DEFINE_HashTable(BindingIdentifier, PVOID, silly_bind_ident_hash, bind_ident_eq)
DEFINE_HashTable(PVOID, HistoryChangeTracker, silly_hash_void_ptr, ptr_eq)


struct BackingBuffer
{
	MultiGapBuffer *buffer;
	History history;
	DynamicArray_int lineSumTree;
	int lines;
	HashTable_BindingIdentifier_PVOID bindingMemory;
	HashTable_PVOID_HistoryChangeTracker binding_next_change;
	uint64_t ref_count;
	DHSTR_String path; 
	DH_Allocator allocator;
};



struct CursorInfo
{
	int prefered_x;
	bool is_dirty;
};

DEFINE_DynamicArray(CursorInfo);



int sillyhash_char(char c)
{
	return silly_hash((int)c);
}
int char_eq(char a, char b)
{
	return a == b;
}
typedef long long unsigned int ulli;
internal inline bool lli_eq(ulli a, ulli b)
{
	return a == b;
}
internal inline bool int_eq(int a, int b)
{
	return a == b;
}

int silly_hash_char32_t(char32_t c) { return silly_hash((int)c); }
bool char32_t_eq(char32_t a, char32_t b) { return a == b; }
DEFINE_HashTable(ulli, CharBitmap, silly_hash_lli, lli_eq);
DEFINE_HashTable(char32_t, int, silly_hash_char32_t, int_eq);

struct Typeface
{
	struct Font
	{
		stbtt_fontinfo *font_info;
		HashTable_ulli_CharBitmap cachedBitmaps;
		HashTable_char32_t_int cachedGlyphs;
		int ascent, descent, lineHeight, lineGap;
	}Light, DemiLight, Regular, DemiBold, Bold, Black, Italic, BoldItalic;// use
};


struct ContextCharWidthHook
{
	typedef int (*CharWidthHookFunc)(TextBuffer *buffer, MGB_Iterator it, char32_t current_char, char32_t next_char, Typeface::Font *typeface, float scale);
	CharWidthHookFunc func;
	char character;
};


enum CharRenderingChangeType
{
	rendering_change_scale,
	rendering_change_color,
	rendering_change_background_color,
	rendering_change_highlight_color,
	rendering_change_font,
};

struct CharRenderingChange
{
	Location location;
	void *owner;
	CharRenderingChangeType type;
	union
	{
		float scale;
		int color;
		int background_color;
		int highlight_color;
		Typeface::Font *font;
	};
};
inline int cmp_int(int a, int b)
{
	return (a > b) - (a < b); //+1,-1, 0 are possible return values
}

inline int cmpCharRenderingChange(void *v_a, void *v_b)
{
	CharRenderingChange a = *(CharRenderingChange *)v_a;
	CharRenderingChange b = *(CharRenderingChange *)v_b;
	return cmp_int(a.location.line, b.location.line) << 1 | cmp_int(a.location.column, b.location.column);
}
DEFINE_Complete_ORD_DynamicArray(CharRenderingChange, cmpCharRenderingChange);




DEFINE_DynamicArray(ContextCharWidthHook);


struct CharRenderingInfo
{
	int color;
	int background_color;
	int highlight_color;
	int x;
	int y;
	float scale;
	Typeface::Font *font;
};


struct TextBuffer
{
	BufferType bufferType;
	BackingBuffer *backingBuffer;

	DynamicArray_KeyBinding KeyBindings;

	int lines;
	int caretX;

	DHSTR_String fileName;

	DynamicArray_int ownedCarets_id;
	DynamicArray_int ownedSelection_id;
	DynamicArray_CursorInfo cursorInfo;
	DynamicArray_ContextCharWidthHook contextCharWidthHook;
	HashTable_BindingIdentifier_PVOID bindingMemory;
	ORD_DynamicArray_CharRenderingChange rendering_changes;
	DH_Allocator allocator;
	CharRenderingInfo initial_rendering_state;
	int textBuffer_id;
	//reg_buffer specific renderthings. need to be here becuase no matter the view we're in we need to keep our info consitant
	//(if we will ever tab back to the old view). If I'm switching to ast view and back I don't want the position to change!

	// I mean, sure but we could keep it somewhere else in either case. like in the binding_memory for example.
	// that just means that we're not allowed to union the shit together...
	float lastWindowLineOffset;
	int dx;
	uint64_t lastAction;
};

bool ownsCaret(TextBuffer *buffer, int index)
{
	for (int i = 0; i < buffer->ownedCarets_id.length; i++)
	{
		if (buffer->ownedCarets_id.start[i] == index)
		{
			return true;
		}
	}
	return false;
}


DEFINE_DynamicArray(TextBuffer);

struct MenuItem
{
	DHSTR_String name;
	int ordering; 
	void *data;
};

DEFINE_DynamicArray(MenuItem);

struct Data
{
	TextBuffer *commandLine;
	DynamicArray_TextBuffer textBuffers;
	int activeTextBufferIndex;
	bool isCommandlineActive;
	bool updateAllBuffers;
	DynamicArray_MenuItem menu;
	int activeMenuItem;
	bool eatNext;
};


enum MoveType
{
	move_caret_selection,
	move_caret_insert,
	move_caret_both,
};


enum log_
{
	do_log,
	no_log,
};

enum select_
{
	do_select,
	no_select,
};
struct MGB_Iterator;

internal bool 		isLineBreak(char32_t codepoint);
internal void 		appendCharacter(TextBuffer *textBuffer, char character, int caretId, bool log = true);
internal void		removeCharacter(TextBuffer *textBuffer, bool log = true);


internal bool 		deleteCharacter(TextBuffer *textBuffer, int caretId,bool log = true);
internal void 		unDeleteCharacter(TextBuffer *textBuffer, char character);
internal int 		getCharacterWidth_std(char32_t currentChar, char32_t nextChar, Typeface::Font *font, float scale);
internal int		getCharacterWidth(TextBuffer *buffer, MGB_Iterator it, char32_t current_char, char32_t next_char, Typeface::Font *font, float scale);
//internal bool 		getNextCharacter(MultiGapBuffer *buffer, char **character);
internal void		renderRect(Bitmap bitmap, int xOffset, int yOffset, int width, int height, int color);
internal void		renderRectFullColor(Bitmap bitmap, int xOffset, int yOffset, int width, int height, int color);
internal bool		isWordDelimiter(char i);
internal bool		isFuncDelimiter(char i);
internal void		initTextBuffer(TextBuffer *textBuffer);
internal char *getRestAsString(MultiGapBuffer *buffer, MGB_Iterator it);
internal void		CloseCommandLine(Data *data);
void setNoSelection(TextBuffer *textBuffer, log_ log);
internal void removeSelection(TextBuffer *textBuffer);
internal MGB_Iterator getIteratorAtLine(TextBuffer *textBuffer, int line);
internal int getLineFromCaret(TextBuffer *buffer, int caretId);
internal void gotoStartOfLine(TextBuffer *textBuffer, int line, select_ selection, int caretIdIndex);
internal bool validSelection(TextBuffer *buffer);
internal void moveWhile(TextBuffer *textBuffer, select_ selection, log_ log, bool setPrefX, Direction dir, bool(*func)(char, int *));
internal bool whileSameLine(char character, int *state);
internal void move(TextBuffer *textBuffer, Direction dir, log_ log, select_ selection);
internal bool move(TextBuffer *textBuffer, Direction dir, int caretIdIndex, log_ log, select_ selection);
enum MoveMode
{
	movemode_byte,
	movemode_codepoint,
	movemode_grapheme_cluster,
};
internal bool move_nc(TextBuffer *textBuffer, Direction dir, int caretIdIndex, log_ log, select_ selection, MoveMode mode);
internal bool move_llnc(TextBuffer *textBuffer, Direction dir, int caretIdIndex, bool log, MoveType type, MoveMode mode);


// should be in the win32 header.
// it doens't work though :(
internal void copy(TextBuffer *textBuffer);
internal void paste(TextBuffer *buffer);


struct AvailableFont
{
	DHSTR_String path;
	DHSTR_String name;
};
int cmp_font(void *va, void *vb)
{
	DHSTR_String a = ((AvailableFont *)va)->name;
	DHSTR_String b = ((AvailableFont *)vb)->name;
	return DHSTR_cmp(a, b, string_cmp_longer_bigger);
}
DEFINE_Complete_ORD_DynamicArray(AvailableFont,cmp_font);
struct AvailableTypeface
{
	AvailableFont *members;
	int member_len;
	char *typefaceName;
};
DEFINE_DynamicArray(AvailableTypeface);
DynamicArray_AvailableTypeface availableTypefaces = DHDS_constructDA(AvailableTypeface, 250, default_allocator);
static ORD_DynamicArray_AvailableFont availableFonts = DHDS_ORD_constructDA(AvailableFont, 500, default_allocator);


// this is a 'generic' view of a buffer with all the data the Lexer needs
// it might be a file, a GapBuffer, a string or whatever
struct LBuffer
{
	char *(*getNextCharacter)(LBuffer buffer, void *iterator, bool *success);
	void *(*getFirst)(LBuffer buffer);
	void *data;
};

internal int hash(const char *string)
{
	int hash = 0;
	while (*string)
	{
		hash = hash * 101 + *(string++);
	}
	return hash;
}

#endif

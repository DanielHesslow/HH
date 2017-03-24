#ifndef HEADER
#define HEADER
//although the actual name of the marker where text is to be inserted is a called a cursor I call it caret to disambigueate between that and the mouse cursor.
// I don't know if that is a good decision but that is how I do it. well sometimes I don't so now it's real wierd.
#define external extern "C" __declspec(dllexport)

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

#include "core_heder.h"

struct Rect
{
	int x;
	int y;
	int width;
	int height;
};
enum Direction
{
	dir_right,
	dir_left,
};
#define DA_TYPE int
#define DA_NAME DA_int
#include "DynamicArray.h"


#include "History.h"
#include "MultiGapBuffer.h"
#include "api.h"
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


#include "Layout.h"


#define DHEN_NAME InputType
#define DHEN_PREFIX input
#define DHEN_VALUES X(keyboard) X(mouseWheel)
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
	char *utf8;
	int utf8_len;
	uint16_t VK_Code;

	union
	{
		int16_t mouseWheel;
	};
	bool caps;
	bool control_left;
	bool control_right;
	bool shift_left;
	bool shift_right;
	bool alt_left;
	bool alt_right;
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

#define DA_TYPE identifier
#define DA_NAME DA_identifier
#include "DynamicArray.h"
#define DA_TYPE ColorChange
#define DA_NAME DA_ColorChange
#include "DynamicArray.h"
#define DA_TYPE CharBitmap
#define DA_NAME DA_CharBitmap
#include "DynamicArray.h"
#define DA_TYPE char16_t *
#define DA_NAME DA_Pchar16
#include "DynamicArray.h"
#define DA_TYPE TextBuffer *
#define DA_NAME DA_PTextBuffer
#include "DynamicArray.h"




//---Bindings

struct Argument
{
	Mods mods;
};




enum FuncType
{
	Arg_Mods,
	Arg_Void,
};

struct KeyBinding
{
	char VK_Code;
	Mods mods;
	ModMode modMode;
	FuncType funcType;
	union //  appearently this is implementation defined behaviour... well what eves...
	{
		void(*func_Mods)(Mods mods);
		void(*func)();
		void *funcP;
	};
	int last_event_peeked;
};

#define DA_TYPE KeyBinding
#define DA_NAME DA_KeyBinding
#include "DynamicArray.h"


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

unsigned long long silly_hash(long long i) {
	return (unsigned long long)i * (unsigned long long int)2654435761;
}

unsigned int silly_bind_ident_hash(BindingIdentifier ident) {
	return silly_hash(ident.id) * 31 + silly_hash((int)(long long)(ident.function));
}

unsigned int silly_hash_void_ptr(void *ptr) {
	return silly_hash((int)(long long)(ptr));
}
	
bool ptr_eq(void *a, void *b) {
	return a == b;
}

struct HistoryChangeTracker
{
	int next_index;
	bool move_change;
};

#define HT_ALLOC(num_bytes) allocator->alloc(num_bytes).mem;
#define HT_FREE(ptr,num_bytes) allocator->free(ptr,num_bytes);
#define HT_ALLOCATOR DH_Allocator *

// @LEAK @CLEANUP use allocator!
#define HT_NAME HashTable_BindingIdentifier_PVOID 
#define HT_HASH(x) silly_bind_ident_hash(x)
#define HT_KEY BindingIdentifier
#define HT_VALUE void *
#define HT_EQUAL(a,b) bind_ident_eq(a,b)
#include "L:\HashTable.h"

#define HT_NAME HashTable_PVOID_HistoryChangeTracker 
#define HT_HASH(x) silly_hash_void_ptr(x)
#define HT_KEY void *
#define HT_VALUE HistoryChangeTracker
#include "L:\HashTable.h"

struct CommonBuffer
{
	HashTable_BindingIdentifier_PVOID bindingMemory;
	DH_SlowTrackingArena *allocator;
};

struct BackingBuffer
{
	CommonBuffer commonBuffer;
	DH_SlowTrackingArena *allocator;
	MultiGapBuffer *buffer;
	History history;
	DA_int lineSumTree;
	int lines;
	HashTable_PVOID_HistoryChangeTracker binding_next_change;
	DA_PTextBuffer textBuffers;
	String path; 
};


struct CursorInfo
{
	int prefered_x;
	bool is_dirty;
};

#define DA_TYPE CursorInfo
#define DA_NAME DA_CursorInfo
#include "DynamicArray.h"

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

#define HT_NAME HashTable_ulli_CharBitmap
#define HT_KEY unsigned long long
#define HT_VALUE CharBitmap
#define HT_HASH(x) silly_hash(x);
#include "L:\HashTable.h"

#define HT_NAME HashTable_char32_int
#define HT_KEY char32_t
#define HT_VALUE int
#define HT_HASH(x) silly_hash(x);
#include "L:\HashTable.h"


struct Typeface
{
	struct Font
	{
		stbtt_fontinfo font_info;
		HashTable_ulli_CharBitmap cachedBitmaps;
		HashTable_char32_int cachedGlyphs;
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


//
//-- color
//




struct CharRenderingChange
{
	Location location;
	void *owner;
	CharRenderingChangeType type;
	union
	{
		float scale;
		Color color;
		Typeface::Font *font;
	} raw;

	union
	{
		float scale;
		int color;
		Typeface::Font *font;
	} applied;

	bool start;
};



#define DA_NAME ORD_DA_CharRenderingChange
#define DA_TYPE CharRenderingChange
#define DA_ORDER(a,b) a.location.line < b.location.line || (a.location.line == b.location.line && a.location.column < b.location.column)
#include "DynamicArray.h"

#define DA_NAME DA_ContextCharWidthHook
#define DA_TYPE ContextCharWidthHook
#include "DynamicArray.h"



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


typedef void(*RenderingModifier)(TextBuffer *textBuffer);
#define DA_NAME DA_RenderingModifier
#define DA_TYPE RenderingModifier
#include "DynamicArray.h"


struct TextBuffer
{
	CommonBuffer commonBuffer;
	BufferType bufferType;
	BackingBuffer *backingBuffer;
	DA_RenderingModifier renderingModifiers;

	DA_KeyBinding KeyBindings;

	int lines;

	String fileName;

	DA_int ownedCarets_id;
	DA_int ownedSelection_id;
	DA_CursorInfo cursorInfo;
	DA_ContextCharWidthHook contextCharWidthHook;
	ORD_DA_CharRenderingChange rendering_changes;
	DH_SlowTrackingArena *allocator;
	CharRenderingInfo initial_rendering_state;
	int textBuffer_id;
	//reg_buffer specific renderthings. need to be here becuase no matter the view we're in we need to keep our info consitant
	//(if we will ever tab back to the old view). If I'm switching to ast view and back I don't want the position to change!

	// I mean, sure but we could keep it somewhere else in either case. like in the binding_memory for example.
	// that just means that we're not allowed to union the shit together...
	float lastWindowLineOffset;
	int dx;
	uint64_t lastAction;
	bool rendering_change_is_dirty;
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




struct MenuItem
{
	String name;
	void *data;
};

#define DA_NAME DA_MenuItem
#define DA_TYPE MenuItem
#include "DynamicArray.h"

struct Menu
{
	DA_MenuItem items;
	int active_item;
	DH_SlowTrackingArena *allocator; //free on close
};

struct Data
{
	TextBuffer *commandLine;
	DA_PTextBuffer textBuffers;
	int activeTextBufferIndex;
	bool isCommandlineActive;
	bool updateAllBuffers;
	Menu menu;
	bool eatNext;
	Layout *layout;
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
internal void       appendCharacter(TextBuffer *textBuffer, char character, int caretIdIndex, bool log = true);
internal void		removeCharacter(TextBuffer *textBuffer, bool log = true);
internal bool       removeCharacter(TextBuffer *textBuffer, int caretIdIndex, bool log = true);
internal void		unDeleteCharacter(TextBuffer *textBuffer, char character, int caretIdIndex, bool log=true);
internal void		markPreferedCaretXDirty(TextBuffer *textBuffer, int caretIdIndex);


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
internal char *getRestAsString(MultiGapBuffer *buffer, MGB_Iterator it, DH_Allocator * allocator);
internal void		CloseCommandLine(Data *data);
void setNoSelection(TextBuffer *textBuffer, log_ log);
internal void removeSelection(TextBuffer *textBuffer);
internal void removeSelection(TextBuffer *textBuffer, int caretIdIndex);
internal MGB_Iterator iteratorFromLine(BackingBuffer *backingBuffer, int line);
internal int getLineFromCaret(BackingBuffer *buffer, int caretId);
internal void gotoStartOfLine(TextBuffer *textBuffer, int line, select_ selection, int caretIdIndex);
internal bool validSelection(TextBuffer *buffer);
internal void moveWhile(TextBuffer *textBuffer, select_ selection, log_ log, bool setPrefX, Direction dir, bool(*func)(char, int *));
internal bool whileSameLine(char character, int *state);
//internal void move(TextBuffer *textBuffer, Direction dir, log_ log, select_ selection);
//internal bool move(TextBuffer *textBuffer, Direction dir, int caretIdIndex, log_ log, select_ selection);
//internal void moveV_nc(TextBuffer *textBuffer, select_ selection, int caretIdIndex, bool up);
internal int getLineStart(BackingBuffer *backingBuffer, int line);
void removeOwnedEmptyCarets(TextBuffer *textBuffer);

void removeCaret(TextBuffer *buffer, int caretIdIndex, bool log);
void AddCaret(TextBuffer *buffer, int pos);

bool getActiveBuffer(Data *data, TextBuffer **activeBuffer_out);

enum MoveMode
{
	movemode_byte,
	movemode_codepoint,
	movemode_grapheme_cluster,
};

internal bool move_nc(TextBuffer *textBuffer, Direction dir, int caretIdIndex, log_ log, select_ selection, MoveMode mode);
internal bool move_llnc(TextBuffer *textBuffer, Direction dir, int caretIdIndex, bool log, MoveType type, MoveMode mode);
internal bool move_llnc_(TextBuffer *textBuffer, Direction dir, int caretId, bool log, MoveMode mode);


// should be in the win32 header.
// it doens't work though :(
internal void copy(TextBuffer *textBuffer);
internal void paste(TextBuffer *buffer);


struct AvailableFont
{
	String name;
	String path;
};
#if 0
int cmp_font(void *va, void *vb)
{
	String a = ((AvailableFont *)va)->name;
	String b = ((AvailableFont *)vb)->name;
	return DHSTR_cmp(a, b, string_cmp_longer_bigger);
}
#endif

#define DA_TYPE AvailableFont
#define DA_NAME DA_ORD_AvailableFont
//#define DA_ORDER(a,b) (DHSTR_cmp(a.name,b.name,string_cmp_longer_bigger) < 0)    //@CLEANUP ???
#include "DynamicArray.h"

struct AvailableTypeface
{
	AvailableFont *members;
	int member_len;
	char *typefaceName;
};

#define DA_NAME DA_AvailableTypeface
#define DA_TYPE AvailableTypeface
#include "DynamicArray.h"

DA_AvailableTypeface availableTypefaces = DA_AvailableTypeface::make(general_allocator);
static DA_ORD_AvailableFont availableFonts = DA_ORD_AvailableFont::make(general_allocator);


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

#ifndef API_HEADER
#define API_HEADER
#include "core_heder.h"



#ifdef TYPE_SAFETY
struct ViewHandle
{
	ViewHandle view_handle;
};

struct BufferHandle
{
	ViewHandle view_handle;
};

#else
#define BufferHandle void *
#define ViewHandle void *
#endif


enum HandleType
{
	handle_type_view,
	handle_type_buffer,
};

enum CallBackTime 
{ 
	callback_pre_render 
};

struct TextIterator
{
	struct
	{
		int block_index;
		int sub_index;
	}current,start;
	BufferHandle buffer_handle;
};


struct ViewIterator
{
	BufferHandle buffer_handle;
	int next;
};


typedef void(*StringFunction)(char *string_remaining, int string_length, void **user_data);
	
#define ALL_CURSORS -1

enum API_MoveMode
{
	move_mode_grapheme_cluster,
	move_mode_codepoint,
	move_mode_byte,
	move_mode_line,
};




struct RenderingState
{
	void *font_handle;
	Color text_color;
	Color background_color;
	Color highglight_color;
	float scale;
};


// feel free to add more modes, just keep these at the top.
enum BufferModes
{
	buffer_mode_commandline = 0,
	buffer_mode_defualt	    = 1,
};


struct API
{
	struct
	{
		void (*registerCallBack)(CallBackTime time, ViewHandle view_handle, void (*function)(ViewHandle handle));
		void (*bindKey_mods)(ViewHandle view_handle, char VK_Code, ModMode filter, Mods mods, void(*func)(Mods mods));
		void (*bindKey)(ViewHandle view_handle, char VK_Code, ModMode filter, Mods mods, void(*func)());
		void (*bindCommand)(char *name, StringFunction select, StringFunction charDown, void (*interupt)(void **user_data));
	}callbacks;
	
	struct
	{
		ViewHandle(*getActiveViewHandle)();	 // does not include the commandline, may return null if no buffer is open
		ViewHandle(*getFocusedViewHandle)();    // includes the commandline
		ViewHandle(*getCommandLineViewHandle)();
		ViewHandle(*getViewHandleFromIndex)(int);
		BufferHandle(*getActiveBufferHandle)();	 // does not include the commandline, may return null if no buffer is open
		BufferHandle(*getFocusedBufferHandle)();    // includes the commandline
		BufferHandle(*getCommandLineBufferHandle)();
		BufferHandle(*getBufferHandleFromViewHandle)(ViewHandle view_handle);
	}handles;

	struct
	{
		void **(*getFunctionInfo) (void *handle, void *function);
		void *(*allocateMemory)  (void *handle, size_t size);
		void  (*deallocateMemory)(void *handle, void *memory);
	}memory;

	struct
	{
		ViewHandle(*open)();
		void (*close)();
		void (*clear)();
		void (*feed)(char *string, int string_length);
		void (*executeCommand)();
	} commandLine;
	
	struct
	{
		bool (*moveWhile)(ViewHandle view_handle, int direction, int cursor_index, bool select, bool(*function)(char character, void **user_data));
		bool (*removeWhile)(ViewHandle view_handle, int direction, int cursor_index, bool(*function)(char character, void **user_data));
		int  (*move)(ViewHandle view_handle, int direction, int cursor_index, bool select, API_MoveMode mode);
		char (*get_byte)(ViewHandle handle, int cursor_index, int direction);
		char32_t (*get_codepoint)(ViewHandle handle, int cursor_index, int direction);

		bool (*moveToLocation)(ViewHandle view_handle, int cursor_index, bool select, int line, int column);

		int  (*add)(ViewHandle view_handle, int line, int column);
		bool (*remove)(ViewHandle view_handle, int cursor_index);

		void (*append_codepoint)(ViewHandle view_handle, int cursor_index, int direction, char32_t character);
		void (*remove_codepoint)(ViewHandle view_handle, int cursor_index, int direction);
		int  (*selection_length)(ViewHandle view_handle, int cursor_index);
		int  (*delete_selection)(ViewHandle view_handle, int cursor_index);
		int  (*number_of_cursors)(ViewHandle view_handle);

	} cursor;

	struct
	{
		ViewHandle(*createFromFile)(char *path, int path_length);
		void (*clone_view)(ViewHandle view_handle);
		void  (*close)(ViewHandle view_handle);
		void(*setActive)(ViewHandle view_handle);  //excluding commandline
		void(*setFocused)(ViewHandle view_handle); //including commandline

		void(*set_type)(ViewHandle view_handle, int type);
		int(*get_type)(ViewHandle view_handle);

	}view;
	
	struct
	{
		void(*remove)(ViewHandle view_handle, void *function);
		void(*highlight_color)	(ViewHandle view_handle, void *function, Location start_inclusive, Location end_exlusive, Color color);
		void(*background_color)	(ViewHandle view_handle, void *function, Location start_inclusive, Location end_exlusive, Color color);
		void(*text_color)		(ViewHandle view_handle, void *function, Location start_inclusive, Location end_exlusive, Color color);
		void(*scale)			(ViewHandle view_handle, void *function, Location start_inclusive, Location end_exlusive, float scale);
		void(*font)			(ViewHandle view_handle, void *function, Location start_inclusive, Location end_exlusive, char *font_name, int font_length);
		RenderingState(*get_initial_rendering_state)(void *view_handle);
		void		(*set_initial_rendering_state)(void *view_handle, RenderingState state);

	}markup;

	struct
	{
		ViewIterator (*view_iterator)(BufferHandle buffer_handle);
		bool(*next)(ViewIterator *it,ViewHandle *view_handle);
		int (*numLines)(BufferHandle buffer_handle);
		bool(*save)(BufferHandle buffer_handle);
	}buffer;
	
	struct
	{
		// do we need to be able to do this on an view basis?
		// it might be sort of handy but it's the buffer that changes,
		// any fuction that cares need to handle that?
		bool (*next_change)(BufferHandle buffer_handle, void *function, BufferChange *bufferChange);
		void (*mark_all_read)(BufferHandle buffer_handle, void *function);
		bool (*has_moved)(BufferHandle buffer_handle, void *function);
	}changes;

	struct
	{
		TextIterator (*make)(BufferHandle buffer_handle);
		TextIterator (*make_from_location)(BufferHandle buffer_handle,Location loc);
		TextIterator (*make_from_cursor)(BufferHandle buffer_handle, int cursor);
		char (*get_byte)(TextIterator it, int direction);
		char32_t (*get_codepoint)(TextIterator it, int direction);
		int(*move)(TextIterator *it, int direction, API_MoveMode mode, bool wrap);
	}text_iterator;

	struct
	{
		Location (*from_iterator)(BufferHandle buffer_handle, TextIterator text_iterator);
	}Location;
	struct
	{
		int (*byteIndexFromLine)(BufferHandle buffer_handle, int line);
		int (*lineFromByteIndex)(BufferHandle buffer_handle, int byte_index);
		void (*openRedirectedCommandPrompt)(char *command, int command_length);
	}misc;

	struct
	{
		
	}layout;

	struct
	{

	}_internal;

	struct
	{
		void(*copy) (ViewHandle view_handle);
		void(*paste)(ViewHandle view_handle);
		void(*cut)  (ViewHandle view_handle);
	}clipboard;
};

//API getAPi();
//#include "api.cpp"

#endif
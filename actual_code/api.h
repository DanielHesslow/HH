#ifndef API_HEADER
#define API_HEADER
#include "core_heder.h"
#include "DH_MacroAbuse.h"


#define BufferHandle void *
#define ViewHandle void *
#include "windows.h"

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
	}current, start;
	BufferHandle buffer_handle;
};

struct API_MenuItem
{
	char *name;
	int name_length;
	void *data;
};

struct ViewIterator
{
	BufferHandle buffer_handle;
	int next;
};


typedef void(*StringFunction)(char *string_remaining, int string_length, void **user_data);

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
	buffer_mode_default = 1,
};


#ifndef API
#define API 
#endif


#ifndef HEADER_ONLY
#if 0
#define CREATE_LAYOUT(layout_type, width, ...) createLayout(layout_type, width, DHMA_NUMBER_OF_ARGS(__VA_ARGS__),__VA_ARGS__)
#endif

struct Divider
{
	float relative_position;
	int width;
};

enum LayoutType
{
	layout_type_x,
	layout_type_y,
	layout_type_z,
};


struct Layout
{
	LayoutType type;
	Layout *children[10];
	int number_of_children;
	Divider dividers[9];
	int favourite_child;
	//know that these may not be able to tell the future
	int last_width;
	int last_height;
};



//this defines every function as a function_pointer which gets loaded on startup 
#define API_FUNC(ret, name, params) ret(*name)params = (ret(*)params)GetProcAddress(GetModuleHandle(0),#name)

API_FUNC(ViewIterator, view_iterator_from_buffer_handle, (BufferHandle buffer_handle));
API_FUNC(bool, view_iterator_next, (ViewIterator *iterator, ViewHandle *view_handle));
API_FUNC(bool, save, (ViewHandle handle));
API_FUNC(int, num_lines, (BufferHandle handle));
API_FUNC(void, copy, (ViewHandle view_handle));
API_FUNC(void, paste, (ViewHandle view_handle));
API_FUNC(void, cut, (ViewHandle view_handle));
API_FUNC(ViewHandle, commandline_open, ());
API_FUNC(void, commandline_close, ());
API_FUNC(void, commandline_clear, ());
API_FUNC(void, commandline_feed, (char *string, int string_length));
API_FUNC(void, commandline_execute_command, ());
API_FUNC(int, cursor_move, (ViewHandle view_handle, int direction, int cursor_index, bool select, API_MoveMode mode));
API_FUNC(bool, cursor_move_to_location, (ViewHandle view_handle, int cursor_index, bool select, int line, int column));
API_FUNC(int, cursor_add, (ViewHandle view_handle, Location loc));
API_FUNC(bool, cursor_remove, (ViewHandle view_handle, int cursor_index));
API_FUNC(void, append_byte, (ViewHandle view_handle, int cursor_index, int direction, char character));
API_FUNC(void, cursor_remove_codepoint, (ViewHandle view_handle, int cursor_index, int direction));
API_FUNC(int, selection_length, (ViewHandle view_handle, int cursor_index));
API_FUNC(int, delete_selection, (ViewHandle view_handle, int cursor_index));
API_FUNC(int, num_cursors, (ViewHandle view_handle));
API_FUNC(void, register_callback, (CallBackTime time, ViewHandle view_handle, void(*function)(ViewHandle handle)));
API_FUNC(void, bind_key_mods, (ViewHandle view_handle, char VK_Code, ModMode filter, Mods mods, void(*func)(Mods mods)));
API_FUNC(void, bind_key, (ViewHandle view_handle, char VK_Code, ModMode filter, Mods mods, void(*func)()));
API_FUNC(void, bind_command, (char *name, StringFunction select, StringFunction charDown, void(*interupt)(void **user_data)));
API_FUNC(ViewHandle, view_handle_from_index, (int index));
API_FUNC(ViewHandle, view_handle_active, ());	 // does not include the commandline, may return null if no buffer is open);
API_FUNC(ViewHandle, view_handle_cmdline, ());
API_FUNC(ViewHandle, view_handle_focused, ());    // includes the commandline);
API_FUNC(BufferHandle, buffer_handle_active, ());	 // does not include the commandline, may return null if no buffer is open);
API_FUNC(BufferHandle, buffer_handle_focused, ());    // includes the commandline);
API_FUNC(BufferHandle, buffer_handle_cmdline, ());
API_FUNC(BufferHandle, buffer_handle_from_view_handle, (ViewHandle view_handle));
API_FUNC(Location, location_from_iterator, (BufferHandle bufferHandle, TextIterator it));
API_FUNC(Location, location_from_cursor, (ViewHandle viewHandle, int cursor_index));
API_FUNC(void **, function_info_ptr, (void *handle, void *function));
API_FUNC(void *, memory_alloc, (void *handle, size_t size));
API_FUNC(void, memory_free, (void *handle, void *memory));
API_FUNC(int, byte_index_from_line, (BufferHandle buffer_handle, int line));
API_FUNC(int, line_from_byte_index, (BufferHandle buffer_handle, int byte_index));
API_FUNC(TextIterator, text_iterator_start, (BufferHandle buffer_handle));
API_FUNC(TextIterator, text_iterator_from_location, (BufferHandle buffer_handle, Location loc));
API_FUNC(TextIterator, text_iterator_from_cursor, (ViewHandle view_handle, int cursor_index));
API_FUNC(char, text_iterator_get_byte, (TextIterator it, int dir));
API_FUNC(char32_t, text_iterator_get_codepoint, (TextIterator it, int dir));
API_FUNC(char, cursor_get_byte, (ViewHandle view_handle, int cursor, int dir));
API_FUNC(char32_t, cursor_get_codepoint, (ViewHandle view_handle, int cursor, int dir));
API_FUNC(int, text_iterator_move, (TextIterator *it, int direction, API_MoveMode mode, bool wrap));
API_FUNC(void, markup_remove, (ViewHandle view_handle, void *function));
API_FUNC(void, markup_highlight_color, (ViewHandle view_handle, void *function, Location start_inclusive, Location end_exclusive, Color color));
API_FUNC(void, markup_background_color, (ViewHandle view_handle, void *function, Location start_inclusive, Location end_exclusive, Color color));
API_FUNC(void, markup_text_color, (ViewHandle view_handle, void *function, Location start_inclusive, Location end_exclusive, Color color));
API_FUNC(void, markup_scale, (ViewHandle view_handle, void *function, Location start_inclusive, Location end_exclusive, float scale));
API_FUNC(void, markup_font, (ViewHandle view_handle, void *function, Location start_inclusive, Location end_exclusive, char *font_name, int font_length));
API_FUNC(RenderingState, markup_get_initial_rendering_state, (void *view_handle));
API_FUNC(void, markup_set_initial_rendering_state, (void *view_handle, RenderingState state));
API_FUNC(void *, open_view, (char *path, int path_length));
API_FUNC(ViewHandle, view_from_buffer_handle, (BufferHandle buffer_handle));
API_FUNC(void, view_clone, (ViewHandle view_handle));
API_FUNC(void, view_close, (ViewHandle view_handle));
API_FUNC(void, view_set_focused, (ViewHandle view_handle));
API_FUNC(void, view_set_type, (ViewHandle view_handle, int type));
API_FUNC(int, view_get_type, (ViewHandle view_handle));
API_FUNC(void *, open_redirected_terminal, (char *command, int command_length));
API_FUNC(bool, menu_get_active, (API_MenuItem *_out_MenuItem));
API_FUNC(void, undo, (ViewHandle handle));
API_FUNC(void, redo, (ViewHandle handle));
API_FUNC(void, menu_clear, ());
API_FUNC(void, menu_add,(API_MenuItem item));
API_FUNC(void, menu_move_active,(int dir));
API_FUNC(void, history_next_leaf, (ViewHandle view_handle));
API_FUNC(void, history_insert_waypoint,(ViewHandle handle));

API_FUNC(void, move_layout, (int dir,LayoutType type));
#if 0
API_FUNC(void, set_layout, (Layout *layout));
API_FUNC(int, get_num_views, ());
API_FUNC(void, createLayout, ());
#endif

#endif //HEADER ONLY



#endif 



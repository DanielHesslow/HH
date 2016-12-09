// BACKEND API LAYER

#define API extern "C" __declspec(dllexport)
#define external extern "C" __declspec(dllexport)

#define HEADER_ONLY
//#include "api.h"
#include "api_2.h"
#include "header.h"

struct CommandInfo
{
	char *name;
	int hashedName;
	void *user_data;
	void(*func)(char *str, int strlen, void **user_data);
	void(*charDown)(char *str, int strlen, void **user_data);
};


void *MenuAlloc(size_t bytes)
{
	return global_data->menu.allocator.alloc(bytes, "allocation", global_data->menu.allocator.data);
}

external void menu_clear()
{
	arena_clear(global_data->menu.allocator);
	global_data->menu.items.length = 0;
}

MenuItem MenuItemFromApiMenuItem(API_MenuItem item)
{
	MenuItem ret = {};
	ret.data = item.data;
	ret.name = {item.name,(size_t)item.name_length};
	return ret;
}

API_MenuItem ApiMenuItemFromMenuItem(MenuItem item)
{
	API_MenuItem ret = {};
	ret.data = item.data;
	ret.name = item.name.start;
	ret.name_length= item.name.length;
	return ret;
}

external void menu_add(API_MenuItem item)
{
	char *str = (char *)MenuAlloc(item.name_length);
	memcpy(str, item.name, item.name_length);
	MenuItem item_ = {};
	item_.data = item.data;
	item_.name = { str,(size_t)item.name_length };
	Add(&global_data->menu.items, item_);
}

static void *user_data_sort;
int(*comparator_sort)(API_MenuItem a, API_MenuItem b, void *user_data);
int comp(const void *a, const void *b)
{
	API_MenuItem a_ = ApiMenuItemFromMenuItem(*(MenuItem *)a);
	API_MenuItem b_ = ApiMenuItemFromMenuItem(*(MenuItem *)b);
	return comparator_sort(a_, b_, user_data_sort);
}

void menu_sort(int(*cmp)(API_MenuItem a, API_MenuItem b, void *user_data), void *user_data)
{
	comparator_sort = cmp;
	user_data_sort = user_data;
	qsort(&global_data->menu.items.start, global_data->menu.items.length, sizeof(MenuItem), comp);
}

external void menu_move_active(int dir)
{
	global_data->menu.active_item += dir;
	global_data->menu.active_item  = clamp(global_data->menu.active_item, 0, global_data->menu.items.length);
}

void menu_disable_active()
{
	global_data->menu.active_item = 0;
}


DEFINE_DynamicArray(CommandInfo)
static DynamicArray_CommandInfo commands = constructDynamicArray_CommandInfo(100, "commandBindings");


internal void bindCommand(char *name, StringFunction func)
{
	CommandInfo commandInfo = {};
	commandInfo.name = name;
	commandInfo.hashedName = hash(name);
	commandInfo.func = func;
	Add(&commands, commandInfo);
}

internal void bindCommand(char *name, StringFunction func, StringFunction charDown)
{
	CommandInfo commandInfo = {};
	commandInfo.name = name;
	commandInfo.hashedName = hash(name);
	commandInfo.func = func;
	commandInfo.charDown = charDown;
	Add(&commands, commandInfo);
}


internal CommandInfo findMatchingCommand(MultiGapBuffer *buffer, DynamicArray_CommandInfo commands, bool *success)
{
	CommandInfo allocationInfo = {};
	MGB_Iterator it = getIterator(buffer);

	int ignore;

	int hash = getNextTokenHash(buffer, &it, &ignore);
	for (int i = 0; i<commands.length; i++)
	{
		if (commands.start[i].hashedName == hash)//make sure the string is the same as well!
		{
			allocationInfo = commands.start[i];
			*success = true;
			return allocationInfo;
		}
	}
	*success = false;
	return allocationInfo;
}


// these two functions is the same. (one token diff)
// merge them? @cleanup
internal void keyDown_CommandLine(Data *data)
{
	MultiGapBuffer *buffer = data->commandLine->backingBuffer->buffer;

	bool success;
	CommandInfo command = findMatchingCommand(buffer, commands, &success);
	if (!success) return;

	MGB_Iterator rest = getIterator(buffer, DHSTR_strlen(command.name));

	int ignore;
	stripInitialWhite(buffer, &rest, &ignore);
	char *remainingString = getRestAsString(buffer, rest);
	if (command.charDown)
		command.charDown(remainingString, strlen(remainingString),&command.user_data);
	free_(remainingString);
}

bool matchMods(Mods mods, Mods filter, ModMode mode)
{
	if (mode == atMost)
	{
		return !(mods & ~filter);
	}
	else if (mode == atLeast)
	{
		return (mods & filter) == filter;
	}
	else if (mode == precisely)
	{
		return (mods == filter);
	}
	else assert(false);
	return false;
}

global_variable DynamicArray_KeyBinding *bindings = (DynamicArray_KeyBinding *)0;
//external API getAPI();







void _setLocalBindings(TextBuffer *textBuffer)
{
	if (!setBindingsLocal)return;
	assert(bindings == 0); // this is fine till we're using mutple threads, assert to make sure!
	bindings = &textBuffer->KeyBindings;
	setBindingsLocal(textBuffer);
	bindings = (DynamicArray_KeyBinding *)0;
}

KeyBinding bindKeyBase(char VK_Code, ModMode modMode, Mods mods, void *func)
{
	KeyBinding binding = {};
	binding.VK_Code = VK_Code;
	binding.mods = mods;
	binding.funcP = func;
	binding.modMode = modMode;
	return binding;
}

internal void executeBindingFunction(KeyBinding binding, TextBuffer *buffer, Data *data, Mods currentMods)
{
	if (binding.funcType == Arg_Mods)
	{
		binding.func_Mods(currentMods);
	}
	else if (binding.funcType == Arg_Void)
	{
		binding.func();
	}
	else assert(false);
}


void bindKey(char VK_Code, ModMode modMode, Mods mods, void(*func)())
{
	KeyBinding binding = bindKeyBase(VK_Code, modMode, mods, (void *)func);
	binding.funcType = Arg_Void;
	Add(bindings, binding);
}

void bindKeyMods(char VK_Code, ModMode modMode, Mods mods, void(*func)(Mods mods))
{
	KeyBinding binding = bindKeyBase(VK_Code, modMode, mods, (void *)func);
	binding.funcType = Arg_Mods;
	Add(bindings, binding);
}

























external ViewIterator view_iterator_from_buffer_handle(BufferHandle buffer_handle)
{
	ViewIterator ret;
	ret.buffer_handle = buffer_handle;
	ret.next = 0;
	return ret;
}

external bool view_iterator_next(ViewIterator *iterator, ViewHandle *view_handle)
{
	BackingBuffer *backingBuffer = (BackingBuffer *)iterator->buffer_handle;
	if (iterator->next >= backingBuffer->textBuffers.length)
	{
		view_handle = 0;
		return false;
	}
	*view_handle = backingBuffer->textBuffers.start[iterator->next++];
	return true;
}

bool textBuffer_index(TextBuffer *textBuffer, int *_out_index)
{
	for (int i = 0; i < global_data->textBuffers.length; i++)
	{
		if (global_data->textBuffers.start[i] == textBuffer)
		{
			*_out_index = i;
			return true;
		}
	}
	*_out_index = -1;
	return false;
}

external bool save(ViewHandle handle)
{
	saveFile((TextBuffer *)handle);
	return true; // heh
}
external int num_lines(BufferHandle handle)
{
	return getLines((BackingBuffer *)handle);
}



external void copy(ViewHandle view_handle)
{
	copy((TextBuffer *)view_handle);
}

external void paste(ViewHandle view_handle)
{
	paste((TextBuffer *)view_handle);
}

external void cut(ViewHandle view_handle)
{
	cut((TextBuffer *)view_handle);
}

external ViewHandle commandline_open()
{
	global_data->isCommandlineActive = true;
	return global_data->commandLine;
}

external void commandline_close()
{
	global_data->isCommandlineActive = false;
}

void textBuffer_clear(TextBuffer *buffer)
{
	MultiGapBuffer *mgb = buffer->backingBuffer->buffer;
	for (int i = 0; i < mgb->blocks.length; i++)
	{
		mgb->blocks.start[i].length = 0;
	}
}

external void commandline_clear()
{
	textBuffer_clear(global_data->commandLine);
}

external void commandline_feed(char *string, int string_length)
{
	for (int i = 0; i < string_length; i++)
	{
		appendCharacter(global_data->commandLine, string[i], 0, true);
	}
	//keyDown_CommandLine(global_data);
}

external void commandline_execute_command()
{
	global_data->eatNext = true;

	MultiGapBuffer *mgb = global_data->commandLine->backingBuffer->buffer;

	bool success;
	CommandInfo command = findMatchingCommand(mgb, commands, &success);
	if (!success) return;

	MGB_Iterator rest = getIterator(mgb, DHSTR_strlen(command.name));

	int ignore;
	stripInitialWhite(mgb, &rest, &ignore);
	char *remainingString = getRestAsString(mgb, rest);
	if (command.func)
		command.func(remainingString, strlen(remainingString), &command.user_data);

	free_(remainingString);
	global_data->updateAllBuffers = true;
}

MoveMode MoveMode_from_API_MoveMode(API_MoveMode mode)
{
	//fucking confusing why not use the same internally maybe?
	switch (mode)
	{
	case move_mode_grapheme_cluster: return movemode_grapheme_cluster;
	case move_mode_codepoint: return movemode_codepoint;
	case move_mode_byte: return movemode_byte;
	default: assert(false && "movemode aint existing"); //TODO: report error rather than assert or whatever
	}
	return movemode_byte;
}
external TextIterator text_iterator_from_cursor(ViewHandle view_handle, int cursor_index);
external int cursor_move(ViewHandle view_handle, int direction, int cursor_index, bool select, API_MoveMode mode)
{
	int ack = 0;
	TextBuffer *buffer = (TextBuffer *)view_handle;
	int state = 0;

	for (int i = 0; i < abs(direction); i++)
	{
		if (mode == move_mode_line)
		{
			//TODO MAKE MOVE V RETURN SUCCESS
			moveV_nc(buffer, select ? do_select : no_select, cursor_index, direction < 0);
		}
		else
		{
			ack += move_llnc_(buffer, direction > 0 ? dir_right : dir_left, buffer->ownedCarets_id.start[cursor_index], true, MoveMode_from_API_MoveMode(mode));
		}
	}
	if (!select)
	{
		setNoSelection(buffer, cursor_index, do_log);
	}
	if (mode != move_mode_line)
		markPreferedCaretXDirty(buffer, cursor_index);
	
	return ack; //return number of places the cursors have moved together.
}


struct StringAckumulator
{
	int length;
	
	char *buffer;

	int capacity;
	
	void clear()
	{
		length = 0;
	}

	void push(char c, int dir)
	{
		assert(dir == 1 || dir == -1);
		
		length += dir;
		if (abs(length)> capacity)
		{
			//need to grow..
			assert(false);
			return;
		}
		if (dir < 0 && length > 0 || dir > 0 && length < 0)
		{
			assert(buffer[length > 0 ? length : (capacity + length)] == c);
		}
		else
		{
			buffer[length > 0? length : (capacity + length)] = c;
		}
	}

	int get_ackumulated(char **string)
	{
		if (length >= 0)
		{
			*string = buffer;
			return length;
		}
		else
		{
			*string = buffer + capacity + length;
			return -length;
		}
	}
	int get_length()
	{
		return abs(length);
	}
};


internal void removeAllButOneCaret(TextBuffer *buffer)
{
	for (int i = 1; i < buffer->ownedCarets_id.length; )
	{
		removeCaret(buffer, 1, true);
	}
}

external bool cursor_move_to_location(ViewHandle view_handle, int cursor_index, bool select, int line, int column)
{
	TextBuffer *buffer = (TextBuffer *)view_handle;
	

	gotoStartOfLine(buffer, line, select ? do_select : no_select, cursor_index);
	for (int i = 0; i < column; i++)
		move(buffer, dir_right, do_log, do_select);

	return true; //TODO RETURN CORRECT
}

external int cursor_add(ViewHandle view_handle, Location loc)
{
	TextBuffer *buffer = (TextBuffer *)view_handle;
	AddCaret(buffer, getLineStart(buffer->backingBuffer, loc.line) + loc.column);
	return buffer->ownedCarets_id.length - 1;
}

external bool cursor_remove(ViewHandle view_handle, int cursor_index)
{
	TextBuffer *buffer = (TextBuffer *)view_handle;

	removeCaret(buffer, cursor_index, true);
	return true; //TODO we can totes fail 
}

external void append_codepoint(ViewHandle view_handle, int cursor_index, int direction, char32_t character)
{
	TextBuffer *buffer = (TextBuffer *)view_handle;
	int state = 0;

	for (int i = 0; i < direction; i++)
		unDeleteCharacter(buffer, character, cursor_index);

	for (int i = 0; i < -direction; i++)
		appendCharacter(buffer, character, cursor_index);
}

external void cursor_remove_codepoint(ViewHandle view_handle, int cursor_index, int direction)
{
	TextBuffer *buffer = (TextBuffer *)view_handle;
	int state = 0;

	for (int i = 0; i < direction; i++)
		deleteCharacter(buffer, cursor_index);

	for (int i = 0; i < -direction; i++)
		removeCharacter(buffer, cursor_index);
}

external int selection_length(ViewHandle view_handle, int cursor_index)
{
	TextBuffer *buffer = (TextBuffer *)view_handle;
	MultiGapBuffer *mgb = buffer->backingBuffer->buffer;
	int ack = 0;
	
	int state = 0;

	int selPos = getCaretPos(mgb, buffer->ownedSelection_id.start[cursor_index]);
	int carPos = getCaretPos(mgb, buffer->ownedCarets_id.start[cursor_index]);
	ack += abs(selPos - carPos);
	
	return ack;
}

external int delete_selection(ViewHandle view_handle, int cursor_index)
{
	TextBuffer *buffer = (TextBuffer *)view_handle;
	int selection_len = selection_length(view_handle, cursor_index);
	removeSelection(buffer, cursor_index);
	setNoSelection(buffer, do_log);
	return selection_len; //TODO FIXME
}

external int num_cursors(ViewHandle view_handle)
{
	TextBuffer *buffer = (TextBuffer *)view_handle;
	return buffer->ownedCarets_id.length;

}



	
external void register_callback (CallBackTime time, ViewHandle view_handle, void (*function)(ViewHandle handle))
{
	TextBuffer *buffer = (TextBuffer *)view_handle;
	switch(time)
	{
	case callback_pre_render:
		Add(&buffer->renderingModifiers, (RenderingModifier)function);
		break;
	default:
		assert(false && "un handeled callbacktime");
	}
}

external void bind_key_mods(ViewHandle view_handle, char VK_Code, ModMode filter, Mods mods, void(*func)(Mods mods))
{
	TextBuffer *buffer = (TextBuffer *)view_handle;
	bindKeyMods(VK_Code, filter, mods, func);
}

external void bind_key (ViewHandle view_handle, char VK_Code, ModMode filter, Mods mods, void(*func)())
{
	TextBuffer *buffer = (TextBuffer *)view_handle;
	bindKey(VK_Code, filter, mods, func);
}

external void bind_command(char *name, StringFunction select, StringFunction charDown, void(*interupt)(void **user_data))
{
	bindCommand(name, select, charDown);
}


external ViewHandle view_handle_from_index(int index)
{
	if (index >= 0 && global_data->textBuffers.length > index)
		return global_data->textBuffers.start[index];
	return 0;
}

external ViewHandle view_handle_active()	 // does not include the commandline, may return null if no buffer is open
{
	return view_handle_from_index(global_data->activeTextBufferIndex);
}

external ViewHandle view_handle_cmdline()
{
	return global_data->commandLine;
}

external ViewHandle view_handle_focused()    // includes the commandline
{
	if (global_data->isCommandlineActive) return view_handle_cmdline();
	else return view_handle_active();
}

external BufferHandle buffer_handle_active()	 // does not include the commandline, may return null if no buffer is open
{
	return((TextBuffer *)view_handle_active())->backingBuffer;
}
external BufferHandle buffer_handle_focused()    // includes the commandline
{
	return((TextBuffer *)view_handle_focused())->backingBuffer;
}
external BufferHandle buffer_handle_cmdline()
{
	return((TextBuffer *)view_handle_cmdline())->backingBuffer;
}
external BufferHandle buffer_handle_from_view_handle(ViewHandle view_handle)
{
	return ((TextBuffer *)view_handle)->backingBuffer;
}

external Location location_from_iterator(BufferHandle bufferHandle, TextIterator it)
{
	BackingBuffer *bb = (BackingBuffer *)bufferHandle;
	int pos = getIteratorPos(bb->buffer, { it.current.block_index,it.current.sub_index });
	int line = getLine(bb, pos);
	int line_start = getLineStart(bb, line);
	return{ line, pos - line_start };
}

external Location location_from_cursor(ViewHandle viewHandle, int cursor_index)
{
	TextBuffer *tb = (TextBuffer *)viewHandle;
	return getLocationFromCaret(tb, tb->ownedCarets_id.start[cursor_index]);

}

external void **function_info_ptr(void *handle, void *function)
{
	BindingIdentifier bi = { 1, function };
	void **data=0;
	if (lookup(&((CommonBuffer *)handle)->bindingMemory, bi, &data))
		return data;
	else
		return insert(&((CommonBuffer *)handle)->bindingMemory, bi, data);
}

external void *memory_alloc(void *handle, size_t size)
{
	CommonBuffer *cb = (CommonBuffer *)handle;
	return cb->allocator.alloc(size, "user_space_allocation", cb->allocator.data);
}

external void memory_free(void *handle, void *memory)
{
	CommonBuffer *cb = (CommonBuffer *)handle;
	cb->allocator.free(memory, cb->allocator.data);
}

external int byte_index_from_line(BufferHandle buffer_handle, int line)
{
	return 0; //@fixme
}
external int line_from_byte_index(BufferHandle buffer_handle, int byte_index)
{
	return 0; //@fixme
}


TextIterator _mk_it(MGB_Iterator mgbit, BufferHandle handle)
{
	TextIterator it = {};
	it.current.block_index = mgbit.block_index;
	it.start.block_index = mgbit.block_index;

	it.current.sub_index = mgbit.sub_index;
	it.start.sub_index = mgbit.sub_index;
	it.buffer_handle = handle;
	return it;
}

external TextIterator text_iterator_start(BufferHandle buffer_handle)
{
	MultiGapBuffer *mgb = ((BackingBuffer *)buffer_handle)->buffer;
	return _mk_it(getIterator(mgb),buffer_handle);
}

external TextIterator text_iterator_from_location(BufferHandle buffer_handle, Location loc)
{
	BackingBuffer *backingBuffer = ((BackingBuffer *)buffer_handle);
	return _mk_it(iteratorFromLocation(backingBuffer, loc),buffer_handle);
}

external TextIterator text_iterator_from_cursor(ViewHandle view_handle, int cursor_index)
{
	TextBuffer *tb = (TextBuffer *)view_handle;
	return _mk_it(getIteratorFromCaret(tb->backingBuffer->buffer, tb->ownedCarets_id.start[cursor_index]),tb->backingBuffer);
}

external char text_iterator_get_byte(TextIterator it, int dir)
{
	MGB_Iterator _it = { it.current.block_index, it.current.sub_index };
	MultiGapBuffer *mgb = ((BackingBuffer *)it.buffer_handle)->buffer;
	if (dir > 0)
	{
		return *getCharacter(mgb, _it);
	}
	else
	{
		MoveIterator(mgb,&_it, -1);
		return *getCharacter(mgb,_it);
	}
}



#include "intrin.h"
#include "stdint.h"

external char32_t text_iterator_get_codepoint(TextIterator it, int dir)
{
	assert(dir == 1 || dir == -1);
	//we better not be in the middle of a codepoint
	MGB_Iterator _it = { it.current.block_index, it.current.sub_index };
	MultiGapBuffer *mgb = ((BackingBuffer *)it.buffer_handle)->buffer;
	unsigned char bytes[8];
	int len = 0;
	unsigned char *start = 0;
	if (dir > 0)
	{
		bytes[len++] = *getCharacter(mgb, _it);
		int num_bytes = __lzcnt16((uint16_t)~bytes[0])-8;
		for (int i = 0; i < num_bytes-1; i++)
		{
			if (!MoveIterator(mgb, &_it, 1))return 0;
			bytes[len++] = *getCharacter(mgb, _it);
		}
			 
		start = bytes;
	}
	else
	{
		for (int i = 0; i < 8; i++)
		{
			if (!MoveIterator(mgb, &_it, -1))return 0;
			++len;
			bytes[sizeof(bytes) - len] = *getCharacter(mgb, _it);
			int num_lo = __lzcnt16((uint16_t)~bytes[sizeof(bytes) - len]) - 8;
			if (num_lo != 1) goto success; //is this correct?
		}
		assert(false && "tried to read utf-8 larger than 8 bytes long ...");
		success:
		start = &bytes[sizeof(bytes)-len];
	}


	char32_t result = 0;

	for (int i = 0; i < len; i++)
	{
		if (i == 0)
		{
			if (len == 1)
			{
				result = start[0] & 0x7f;
			}
			else
			{
				result = (start[0] << (len + 1)) >> (len + 1);
			}
		}
		else
		{
			result = (result << 6) | ((start[i*dir] << 2) >> 2);
		}
	}
	return result;
}


external char cursor_get_byte(ViewHandle view_handle, int cursor, int dir)
{
	return text_iterator_get_byte(text_iterator_from_cursor(view_handle, cursor), dir);
}

external char32_t cursor_get_codepoint(ViewHandle view_handle, int cursor, int dir)
{
	return text_iterator_get_codepoint(text_iterator_from_cursor(view_handle, cursor), dir);
}


external int text_iterator_move(TextIterator *it, int direction, API_MoveMode mode, bool wrap)
{
	MultiGapBuffer *mgb = ((BackingBuffer *)it->buffer_handle)->buffer;
	switch (mode)
	{
	case move_mode_byte:
		return MoveIterator(mgb, (MGB_Iterator*)it, direction); //TODO return correct amount
		break;
	}
	assert(false);
	return 0;
}


// --- Markup
external void markup_remove (ViewHandle view_handle, void *function)
{
	TextBuffer *tb = (TextBuffer *)view_handle;
	remove_rendering_changes(tb, function);
}

external void markup_highlight_color(ViewHandle view_handle, void *function, Location start_inclusive, Location end_exclusive, Color color)
{
	TextBuffer *tb = (TextBuffer *)view_handle;
	change_rendering_highlight_color(tb,start_inclusive,end_exclusive,color,function);
}

external void markup_background_color(ViewHandle view_handle, void *function, Location start_inclusive, Location end_exclusive, Color color)
{
	TextBuffer *tb = (TextBuffer *)view_handle;
	change_rendering_background_color(tb, start_inclusive, end_exclusive, color, function);
}

external void markup_text_color(ViewHandle view_handle, void *function, Location start_inclusive, Location end_exclusive, Color color)
{
	TextBuffer *tb = (TextBuffer *)view_handle;
	change_rendering_color(tb, start_inclusive, end_exclusive, color, function);
}

external void markup_scale(ViewHandle view_handle, void *function, Location start_inclusive, Location end_exclusive, float scale)
{
	TextBuffer *tb = (TextBuffer *)view_handle;
	change_rendering_scale(tb, start_inclusive, end_exclusive, scale,function);
}

external void markup_font(ViewHandle view_handle, void *function, Location start_inclusive, Location end_exclusive, char *font_name, int font_length)
{
	TextBuffer *tb = (TextBuffer *)view_handle;
	DHSTR_String font_str= DHSTR_makeString_(font_name, font_length);
	Typeface::Font *font = getFont(font_str);
	change_rendering_font(tb, start_inclusive, end_exclusive, font, function);
}

RenderingState RenderingStateFromCharRenderingInfo(CharRenderingInfo info)
{
	Color color;
	RenderingState rs = {};
	rs.background_color = colorFromInt(info.background_color);
	rs.highglight_color = colorFromInt(info.highlight_color);
	rs.text_color = colorFromInt(info.color);
	rs.scale = info.scale;
	//TODO FONT HANDLE??
	return rs;
}

CharRenderingInfo CharRenderingInfoFromRenderingState(RenderingState state)
{
	CharRenderingInfo info = {};
	info.background_color = (int)state.background_color;
	info.color= (int)state.text_color;
	info.highlight_color = (int)state.highglight_color;
	info.scale = state.scale;
	return info;
}

external RenderingState markup_get_initial_rendering_state(void *view_handle)
{
	TextBuffer *tb = (TextBuffer *)view_handle;
	RenderingState rs = {};
	return RenderingStateFromCharRenderingInfo(tb->initial_rendering_state);
}

external void markup_set_initial_rendering_state(void *view_handle, RenderingState state)
{
	TextBuffer *tb = (TextBuffer *)view_handle;
	tb->initial_rendering_state = CharRenderingInfoFromRenderingState(state);
}


// --- view
external void *open_view(char *path, int path_length)
{
	DHSTR_String path_string = DHSTR_makeString_(path, path_length);
	bool success;
	TextBuffer *buffer = openFileIntoNewBuffer(path_string, &success);
	if (success)
	{
		Add(&global_data->textBuffers, buffer);
		global_data->activeTextBufferIndex = global_data->textBuffers.length - 1;
		return &global_data->textBuffers.start[global_data->textBuffers.length - 1];
	}
	return 0;
}

external ViewHandle view_from_buffer_handle(BufferHandle buffer_handle)
{
	BackingBuffer *backingBuffer = (BackingBuffer*)buffer_handle;
	TextBuffer *buffer = createTextBufferFromBackingBuffer(backingBuffer, DHSTR_MAKE_STRING("hello"));
	Add(&global_data->textBuffers, buffer);
	return &global_data->textBuffers.start[global_data->textBuffers.length - 1];
}



external void view_clone(ViewHandle view_handle)
{
	assert(false && "na man, no time to implement atm");
}

external void view_close(ViewHandle view_handle)
{
	TextBuffer *tb = ((TextBuffer*)view_handle);
	int index;
	assert(textBuffer_index(tb, &index));
	freeTextBuffer(tb);
	Remove(&global_data->textBuffers, index);
	global_data->activeTextBufferIndex = 0;
	global_data->updateAllBuffers = true;
}

external void view_set_focused(ViewHandle view_handle)
{
	TextBuffer *tb = ((TextBuffer*)view_handle);
	if (tb == global_data->commandLine)
	{
		global_data->isCommandlineActive = true;
	}
	else
	{
		int target_index;
		assert(textBuffer_index(tb, &target_index));
		assert(target_index >= 0 && target_index < global_data->textBuffers.length);
		global_data->activeTextBufferIndex = target_index;
	}
}

external void view_set_type(ViewHandle view_handle, int type)
{
	TextBuffer *tb = ((TextBuffer*)view_handle);
	tb->bufferType = (BufferType)type;
}

external int view_get_type(ViewHandle view_handle)
{
	TextBuffer *tb = ((TextBuffer*)view_handle);
	return (int)tb->bufferType;
}

external void *open_redirected_terminal (char *command, int command_length)
{
	return platform_execute_command(command,command_length);
}


external bool menu_get_active(API_MenuItem *_out_MenuItem)
{
	Menu menu = global_data->menu;
	if (!menu.active_item)return false;
	else
	{
		if (_out_MenuItem) *_out_MenuItem = ApiMenuItemFromMenuItem(global_data->menu.items.start[global_data->menu.active_item - 1]);
		return true;
	}
}
#include "History_2.h"

external void undo (ViewHandle handle)
{
	TextBuffer *tb = (TextBuffer *)handle;
	undo(tb->backingBuffer);
}


external void redo(ViewHandle handle)
{
	TextBuffer *tb = (TextBuffer *)handle;
	int branches = num_branches(&tb->backingBuffer->history_2, tb->backingBuffer->history_2.state.location);
	redo(tb->backingBuffer,branches-1);
}




#if 0
external API getAPI()
{
	API api = {};
	api.buffer.view_iterator = view_iterator_from_buffer_handle;
	api.buffer.next          = view_iterator_next;
	api.buffer.save          = save;
	api.buffer.numLines      = num_lines;

	api.clipboard.copy  = copy;
	api.clipboard.cut   = cut;
	api.clipboard.paste = paste;

	api.commandLine.clear          = commandline_clear;
	api.commandLine.close          = commandline_close;
	api.commandLine.executeCommand = commandline_execute_command;
	api.commandLine.feed           = commandline_feed;
	api.commandLine.open           = commandline_open;

	api.cursor.append_codepoint  = append_codepoint;
	api.cursor.delete_selection  = delete_selection;
	api.cursor.add               = cursor_add;
	api.cursor.remove            = cursor_remove;
	api.cursor.move	             = cursor_move;
	api.cursor.moveToLocation    = cursor_move_to_location;
	api.cursor.number_of_cursors = num_cursors;
	api.cursor.remove_codepoint  = cursor_remove_codepoint;
	api.cursor.selection_length  = selection_length;
	api.cursor.get_byte = cursor_get_byte;
	api.cursor.get_codepoint = cursor_get_codepoint;

	api.callbacks.bindCommand = bind_command;
	api.callbacks.bindKey = bind_key;
	api.callbacks.bindKey_mods = bind_key_mods;
	api.callbacks.registerCallBack = register_callback;

	api.handles.getActiveViewHandle = view_handle_active;	 // does not include the commandline, may return null if no buffer is open
	api.handles.getFocusedViewHandle = view_handle_focused;    // includes the commandline
	api.handles.getCommandLineViewHandle = view_handle_cmdline;
	api.handles.getViewHandleFromIndex = view_handle_from_index;
	api.handles.getActiveBufferHandle = buffer_handle_active;	 // does not include the commandline, may return null if no buffer is open
	api.handles.getFocusedBufferHandle = buffer_handle_focused;    // includes the commandline
	api.handles.getCommandLineBufferHandle = buffer_handle_cmdline;
	api.handles.getBufferHandleFromViewHandle = buffer_handle_from_view_handle;

	api.Location.from_iterator = location_from_iterator;
	api.Location.from_cursor = location_from_cursor;


	api.memory.allocateMemory = memory_alloc;
	api.memory.deallocateMemory = memory_free;
	api.memory.getFunctionInfo = function_info_ptr;

	api.misc.byteIndexFromLine = byte_index_from_line;
	api.misc.lineFromByteIndex = line_from_byte_index;
	api.misc.openRedirectedCommandPrompt = open_redirected_terminal; //@fixme

	api.misc.addMenuItem = menu_add;
	api.misc.sortMenu= menu_sort;
	api.misc.move_active_menu = menu_move_active;
	api.misc.disable_active_menu= menu_disable_active;
	api.misc.clearMenu= menu_clear;
	api.misc.get_active_menu_item = menu_get_active;
	api.misc.undo = undo;
	api.misc.redo = redo;

	api.text_iterator.make = text_iterator_start;
	api.text_iterator.make_from_cursor = text_iterator_from_cursor;
	api.text_iterator.make_from_location = text_iterator_from_location;
	api.text_iterator.move = text_iterator_move;
	api.text_iterator.get_byte= text_iterator_get_byte;
	api.text_iterator.get_codepoint= text_iterator_get_codepoint;

	api.markup.background_color = markup_background_color;
	api.markup.font = markup_font;
	api.markup.get_initial_rendering_state = markup_get_initial_rendering_state;
	api.markup.set_initial_rendering_state = markup_set_initial_rendering_state;
	api.markup.highlight_color= markup_highlight_color;
	api.markup.remove= markup_remove;
	api.markup.scale= markup_scale;
	api.markup.text_color= markup_text_color;

	api.view.clone_view = view_clone;
	api.view.close = view_close;
	api.view.createFromFile = open_view;
	api.view.get_type= view_get_type;
	api.view.set_type = view_set_type;
	api.view.setFocused = view_set_focused;
	api.view.createFromBufferHandle= view_from_buffer_handle;

	return api;
}

#endif




// BACKEND API LAYER


#include "api.h"
#include "header.h"
Data *global_data;

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
	return global_data->menu.allocator.alloc(bytes, "allocation", 0);
}

void MenuClear()
{
	arena_deallocate_all(global_data->menu.allocator);
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

void MenuAddItem(API_MenuItem item)
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

void MenuSort(int(*cmp)(API_MenuItem a, API_MenuItem b, void *user_data), void *user_data)
{
	comparator_sort = cmp;
	user_data_sort = user_data;
	qsort(&global_data->menu.items.start, global_data->menu.items.length, sizeof(MenuItem), comp);
}

void MenuMoveActiveItem(int dir)
{
	global_data->menu.active_item += dir;
	clamp(global_data->menu.active_item, 0, global_data->menu.items.length);
}
void MenuDisableActiveItem()
{
	global_data->menu.active_item = 0;
}

//just a helper funtction to be able to iterate on all cursors. state is assumed to be initialized to 0
inline bool next_cursor(TextBuffer *buffer, int cursor_index, int *out_cursor_index, int *state)
{
	if (cursor_index == ALL_CURSORS)
	{
		if ((*state)++ < buffer->ownedCarets_id.length)
		{
			*out_cursor_index = *state-1;
			return true;
		}
		else
		{
			*out_cursor_index = -1;
			return false;
		}
	}
	else
	{
		if (!((*state)++))
		{
			*out_cursor_index = cursor_index;
			return true;
		}
		else
		{
			*out_cursor_index = -1;
			return false;
		}
	}
}


DEFINE_DynamicArray(CommandInfo)
static DynamicArray_CommandInfo commands = constructDynamicArray_CommandInfo(100, "commandBindings");

internal void bindCommand(char *name, void(*func)(char *str, int str_len, void **user_data))
{
	CommandInfo commandInfo = {};
	commandInfo.name = name;
	commandInfo.hashedName = hash(name);
	commandInfo.func = func;
	Add(&commands, commandInfo);
}

internal void bindCommand(char *name, void(*func)(char *str, int str_len, void **user_data), void(*charDown)(char *str, int str_len, void **user_data))
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
API getAPI();







void _setLocalBindings(TextBuffer *textBuffer)
{
	if (!setBindingsLocal)return;
	assert(bindings == 0); // this is fine till we're using mutple threads, assert to make sure!
	bindings = &textBuffer->KeyBindings;
	setBindingsLocal(getAPI(), textBuffer);
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


#if 0
void bindKey(char VK_Code, ModMode modMode, Mods mods, void(*func)(TextBuffer *buffer))
{
	KeyBinding binding = bindKeyBase(VK_Code, modMode, mods, (void *)func);
	binding.funcType = Arg_TextBuffer;
	Add(bindings, binding);
}

void bindKey(char VK_Code, ModMode modMode, Mods mods, void(*func)(TextBuffer *buffer, Mods mods))
{
	KeyBinding binding = bindKeyBase(VK_Code, modMode, mods, (void *)func);
	binding.funcType = Arg_TextBuffer_Mods;
	Add(bindings, binding);
}

void bindKey(char VK_Code, ModMode modMode, Mods mods, void(*func)(Data *data))
{
	KeyBinding binding = bindKeyBase(VK_Code, modMode, mods, (void *)func);
	binding.funcType = Arg_Data;
	Add(bindings, binding);
}
#endif

void bindKey(char VK_Code, ModMode modMode, Mods mods, void(*func)())
{
	KeyBinding binding = bindKeyBase(VK_Code, modMode, mods, (void *)func);
	binding.funcType = Arg_Void;
	Add(bindings, binding);
}

void bindKey(char VK_Code, ModMode modMode, Mods mods, void(*func)(Mods mods))
{
	KeyBinding binding = bindKeyBase(VK_Code, modMode, mods, (void *)func);
	binding.funcType = Arg_Mods;
	Add(bindings, binding);
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

// --- END BACKEND_API
























ViewIterator view_iterator_from_buffer_handle(BufferHandle buffer_handle)
{
	ViewIterator ret;
	ret.buffer_handle = buffer_handle;
	ret.next = 0;
	return ret;
}

bool next_view(ViewIterator *iterator, ViewHandle *view_handle)
{
	// slow, what ever.
	int ack = 0;
	for (int i = 0; i < global_data->textBuffers.length; i++)
	{
		if (global_data->textBuffers.start[i].backingBuffer == iterator->buffer_handle)
		{
			if (ack++ == iterator->next)
			{
				++iterator->next;
				*view_handle = &global_data->textBuffers.start[i];
				return true;
			}
		}
	}
	return false;
}

bool save_view(ViewHandle handle)
{
	saveFile((TextBuffer *)handle);
	return true; // heh
}
int lines_from_buffer_handle(BufferHandle handle)
{
	return getLines((BackingBuffer *)handle);
}

bool next_change(BufferHandle buffer_handle, void *function, BufferChange *bufferChange)
{
	return next_history_change((BackingBuffer *)buffer_handle, function, bufferChange);
}

void mark_all_changes_read(BufferHandle buffer_handle, void *function)
{
	fast_forward_history_changes((BackingBuffer *)buffer_handle, function);
}

bool hash_moved_since_last(BufferHandle buffer_handle, void *function)
{
	return has_move_changed((BackingBuffer *)buffer_handle, function);
}

void clipboard_copy(ViewHandle view_handle)
{
	copy((TextBuffer *)view_handle);
}
void clipboard_paste(ViewHandle view_handle)
{
	paste((TextBuffer *)view_handle);
}
void clipboard_cut(ViewHandle view_handle)
{
	cut((TextBuffer *)view_handle);
}

ViewHandle commandline_open()
{
	global_data->isCommandlineActive = true;
	return global_data->commandLine;
}

void commandline_close()
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

void commandline_clear()
{
	textBuffer_clear(global_data->commandLine);
}

void commandline_feed(char *string, int string_length)
{
	for (int i = 0; i < string_length; i++)
	{
		appendCharacter(global_data->commandLine, string[i], 0, true);
	}
}

void commandline_execute_command()
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
	MenuClear();
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
TextIterator make_from_cursor(ViewHandle view_handle, int cursor_index);
int cursor_move(ViewHandle view_handle, int direction, int cursor_index, bool select, API_MoveMode mode)
{
	int ack = 0;
	TextBuffer *buffer = (TextBuffer *)view_handle;
	int state = 0;
	int _cursor_index;
	while (next_cursor(buffer,cursor_index, &_cursor_index, &state))
	{
		for (int i = 0; i < abs(direction); i++)
		{
			if (mode == move_mode_line)
			{
				//TODO MAKE MOVE V RETURN SUCCESS
				moveV_nc(buffer, select ? do_select : no_select, _cursor_index, direction < 0);
			}
			else
			{
				ack += move_llnc_(buffer, direction > 0 ? dir_right : dir_left, buffer->ownedCarets_id.start[_cursor_index], true, MoveMode_from_API_MoveMode(mode));
			}
		}
		if (!select)
		{
			setNoSelection(buffer, _cursor_index, do_log);
		}
		if (mode != move_mode_line)
			markPreferedCaretXDirty(buffer, _cursor_index);
	}
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



//OPTIMAL MOVE:
#if 0
#define FOR_CARET(c) for(int c = 0; c < 100; c++)
{
	FOR_CARET(c)
	{
		c.move_codepoint(10);
		c.move_grapheme_cluster(-10);
		c.move_byte(5); // dangerous, might split codepoint

		while (is_letter(get_codepoint(c,-1)))
		{
			int read = c.move_codepoint(-1);
			if (!read) break;
		}

		TextIterator it = get_it(c);
		while (!is_space(get_byte(it, -1)))
		{
			int read = move_byte(it, -1);
			if (!read)break;
		}
	}
}
#endif


#if 0
bool caret_move_while (ViewHandle view_handle, int direction, int cursor_index, bool select, API_MoveMode mode,bool(*function)(char32_t character, void **user_data))
{

	assert(mode != move_mode_line && "move while does not support line moves atm (maybe)");
	TextBuffer *buffer = (TextBuffer *)view_handle;
	int state = 0;
	int _cursor_index;
	while (next_cursor(buffer, cursor_index, &_cursor_index, &state))
	{
		int caretId = textBuffer->ownedCarets_id.start[_cursor_index];
		void *user_data= 0;
		while (func(charAtDirOfCaret(textBuffer->backingBuffer->buffer, dir, caretId), &user_data))
		{
			if (!move(textBuffer, dir, i, log, selection))
			{
				break;
			}

		}
		if (setPrefX)
			markPreferedCaretXDirty(textBuffer, i);
		else
			setPreferedX(textBuffer, i);
	}
	removeOwnedEmptyCarets(textBuffer);
}
#endif

internal void removeAllButOneCaret(TextBuffer *buffer)
{
	for (int i = 1; i < buffer->ownedCarets_id.length; )
	{
		removeCaret(buffer, 1, true);
	}
}

bool cursor_move_to_location(ViewHandle view_handle, int cursor_index, bool select, int line, int column)
{
	TextBuffer *buffer = (TextBuffer *)view_handle;
	if (cursor_index == ALL_CURSORS)
	{
		removeAllButOneCaret(buffer);
		cursor_index = 0;
	}

	gotoStartOfLine(buffer, line, select ? do_select : no_select, cursor_index);
	for (int i = 0; i < column; i++)
		move(buffer, dir_right, do_log, do_select);

	return true; //TODO RETURN CORRECT
}

int cursor_add(ViewHandle view_handle, int line, int column)
{
	TextBuffer *buffer = (TextBuffer *)view_handle;
	AddCaret(buffer, getLineStart(buffer->backingBuffer, line) + column);
	return buffer->ownedCarets_id.length - 1;
}

bool cursor_remove(ViewHandle view_handle, int cursor_index)
{
	TextBuffer *buffer = (TextBuffer *)view_handle;

	if (cursor_index == ALL_CURSORS) removeAllButOneCaret(buffer);
	else removeCaret(buffer, cursor_index, do_log);
	return true; //TODO we can totes fail 
}

void cursor_append_codepoint(ViewHandle view_handle, int cursor_index, int direction, char32_t character)
{
	TextBuffer *buffer = (TextBuffer *)view_handle;
	int state = 0;
	int _cursor_index;

	while (next_cursor(buffer, cursor_index, &_cursor_index, &state))
	{
		for (int i = 0; i < direction; i++)
			unDeleteCharacter(buffer, character, _cursor_index);

		for (int i = 0; i < -direction; i++)
			appendCharacter(buffer, character, _cursor_index);
	}
}

void cursor_remove_codepoint(ViewHandle view_handle, int cursor_index, int direction)
{
	TextBuffer *buffer = (TextBuffer *)view_handle;
	int state = 0;
	int _cursor_index;

	while (next_cursor(buffer, cursor_index, &_cursor_index, &state))
	{
		for (int i = 0; i < direction; i++)
			deleteCharacter(buffer, _cursor_index);

		for (int i = 0; i < -direction; i++)
			removeCharacter(buffer, _cursor_index);
	}
}

int cursor_selection_length(ViewHandle view_handle, int cursor_index)
{
	TextBuffer *buffer = (TextBuffer *)view_handle;
	MultiGapBuffer *mgb = buffer->backingBuffer->buffer;
	int ack = 0;
	
	int state = 0;
	int _cursor_index;

	while (next_cursor(buffer, cursor_index, &_cursor_index, &state))
	{
		int selPos = getCaretPos(mgb, buffer->ownedSelection_id.start[_cursor_index]);
		int carPos = getCaretPos(mgb, buffer->ownedCarets_id.start[_cursor_index]);
		ack += abs(selPos - carPos);
	}
	
	return ack;
}

int cursor_delete_selection(ViewHandle view_handle, int cursor_index)
{
	TextBuffer *buffer = (TextBuffer *)view_handle;
	if (cursor_index == ALL_CURSORS)
	{
		for (int i = 0; i < buffer->ownedCarets_id.length; i++)
		{
			removeSelection(buffer, i);
		}
		return 2; //TODO FIXME
	}
	else
	{
		removeSelection(buffer, cursor_index);
		return 1; //TODO FIXME
	}
	
}

int  cursor_number_of_cursors(ViewHandle view_handle)
{
	TextBuffer *buffer = (TextBuffer *)view_handle;
	return buffer->ownedCarets_id.length;

}



	
void callbacks_register_callback (CallBackTime time, ViewHandle view_handle, void (*function)(ViewHandle handle))
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

void callbacks_bind_key_mods(ViewHandle view_handle, char VK_Code, ModMode filter, Mods mods, void(*func)(Mods mods))
{
	TextBuffer *buffer = (TextBuffer *)view_handle;
	bindKey(VK_Code, filter, mods, func);
}

void callbacks_bind_key (ViewHandle view_handle, char VK_Code, ModMode filter, Mods mods, void(*func)())
{
	TextBuffer *buffer = (TextBuffer *)view_handle;
	bindKey(VK_Code, filter, mods, func);
}

void callbacks_bind_command(char *name, StringFunction select, StringFunction charDown, void(*interupt)(void **user_data))
{
	bindCommand(name, select, charDown);
}


ViewHandle getViewHandleFromIndex(int index)
{
	if (index >= 0 && global_data->textBuffers.length > index)
		return &global_data->textBuffers.start[index];
	return 0;
}

ViewHandle getActiveViewHandle()	 // does not include the commandline, may return null if no buffer is open
{
	return getViewHandleFromIndex(global_data->activeTextBufferIndex);
}

ViewHandle getCommandLineViewHandle()
{
	return global_data->commandLine;
}

ViewHandle getFocusedViewHandle()    // includes the commandline
{
	if (global_data->isCommandlineActive) return getCommandLineViewHandle();
	else return getActiveViewHandle();
}

BufferHandle getActiveBufferHandle()	 // does not include the commandline, may return null if no buffer is open
{
	return((TextBuffer *)getActiveViewHandle())->backingBuffer;
}
BufferHandle getFocusedBufferHandle()    // includes the commandline
{
	return((TextBuffer *)getFocusedViewHandle())->backingBuffer;
}
BufferHandle getCommandLineBufferHandle()
{
	return((TextBuffer *)getCommandLineViewHandle())->backingBuffer;
}
BufferHandle getBufferHandleFromViewHandle(ViewHandle view_handle)
{
	return ((TextBuffer *)view_handle)->backingBuffer;
}

Location location_from_iterator(BufferHandle bufferHandle, TextIterator it)
{
	BackingBuffer *bb = (BackingBuffer *)bufferHandle;
	int pos = getIteratorPos(bb->buffer, { it.current.block_index,it.current.sub_index });
	int line = getLine(bb, pos);
	int line_start = getLineStart(bb, line);
	return{ line, pos - line_start };
}


void **getFunctionInfo(void *handle, void *function)
{
	BindingIdentifier bi = { 1, function };
	void **data=0;
	if (lookup(&((CommonBuffer *)handle)->bindingMemory, bi, &data))
		return data;
	else
		return insert(&((CommonBuffer *)handle)->bindingMemory, bi, data);
}

void *allocateMemory(void *handle, size_t size)
{
	CommonBuffer *cb = (CommonBuffer *)handle;
	return cb->allocator.alloc(size, "user_space_allocation", cb->allocator.data);
}

void  deallocateMemory(void *handle, void *memory)
{
	CommonBuffer *cb = (CommonBuffer *)handle;
	cb->allocator.free(memory, cb->allocator.data);
}

int byteIndexFromLine(BufferHandle buffer_handle, int line)
{
	return 0; //@fixme
}
int lineFromByteIndex(BufferHandle buffer_handle, int byte_index)
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
TextIterator make(BufferHandle buffer_handle)
{
	MultiGapBuffer *mgb = ((BackingBuffer *)buffer_handle)->buffer;
	return _mk_it(getIterator(mgb),buffer_handle);
}

TextIterator make_from_location(BufferHandle buffer_handle, Location loc)
{
	BackingBuffer *backingBuffer = ((BackingBuffer *)buffer_handle);
	return _mk_it(iteratorFromLocation(backingBuffer, loc),buffer_handle);
}

TextIterator make_from_cursor(ViewHandle view_handle, int cursor_index)
{
	TextBuffer *tb = (TextBuffer *)view_handle;
	assert(cursor_index != ALL_CURSORS && "must supply a specific cursor index to api__iterator.make_from_cursor");


	return _mk_it(getIteratorFromCaret(tb->backingBuffer->buffer, tb->ownedCarets_id.start[cursor_index]),tb->backingBuffer);
}

char get_byte(TextIterator it, int dir)
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

char32_t get_codepoint(TextIterator it, int dir)
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
			if (num_lo == 0 || num_lo != 1) goto success;
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



char cursor_get_byte(ViewHandle view_handle, int cursor, int dir)
{
	return get_byte(make_from_cursor(view_handle, cursor), dir);
}

char32_t cursor_get_codepoint(ViewHandle view_handle, int cursor, int dir)
{
	return get_codepoint(make_from_cursor(view_handle, cursor), dir);
}


int move(TextIterator *it, int direction, API_MoveMode mode, bool wrap)
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
void remove (ViewHandle view_handle, void *function)
{
	TextBuffer *tb = (TextBuffer *)view_handle;
	remove_rendering_changes(tb, function);
}

void highlight_color(ViewHandle view_handle, void *function, Location start_inclusive, Location end_exclusive, Color color)
{
	TextBuffer *tb = (TextBuffer *)view_handle;
	change_rendering_highlight_color(tb,start_inclusive,end_exclusive,color,function);
}

void background_color(ViewHandle view_handle, void *function, Location start_inclusive, Location end_exclusive, Color color)
{
	TextBuffer *tb = (TextBuffer *)view_handle;
	change_rendering_background_color(tb, start_inclusive, end_exclusive, color, function);
}

void text_color(ViewHandle view_handle, void *function, Location start_inclusive, Location end_exclusive, Color color)
{
	TextBuffer *tb = (TextBuffer *)view_handle;
	change_rendering_color(tb, start_inclusive, end_exclusive, color, function);
}

void scale(ViewHandle view_handle, void *function, Location start_inclusive, Location end_exclusive, float scale)
{
	TextBuffer *tb = (TextBuffer *)view_handle;
	change_rendering_scale(tb, start_inclusive, end_exclusive, scale,function);
}

void font(ViewHandle view_handle, void *function, Location start_inclusive, Location end_exclusive, char *font_name, int font_length)
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

RenderingState get_initial_rendering_state(void *view_handle)
{
	TextBuffer *tb = (TextBuffer *)view_handle;
	RenderingState rs = {};
	return RenderingStateFromCharRenderingInfo(tb->initial_rendering_state);
}
void set_initial_rendering_state(void *view_handle, RenderingState state) 
{
	TextBuffer *tb = (TextBuffer *)view_handle;
	tb->initial_rendering_state = CharRenderingInfoFromRenderingState(state);
}




// --- view

void *createFromFile(char *path, int path_length)
{
	DHSTR_String _tmp = DHSTR_makeString_(path, path_length);
	DH_Allocator macro_used_allocator = default_allocator;
	DHSTR_String path_str = DHSTR_CPY(_tmp, ALLOCATE);
	bool success;
	TextBuffer buffer = openFileIntoNewBuffer(path_str, &success);
	if (success)
	{
		Add(&global_data->textBuffers, buffer);
		global_data->activeTextBufferIndex = global_data->textBuffers.length - 1;
		return &global_data->textBuffers.start[global_data->textBuffers.length - 1];
	}
	return 0;
}

ViewHandle createFromBufferHandle(BufferHandle buffer_handle)
{
	BackingBuffer *backingBuffer = (BackingBuffer*)buffer_handle;
	TextBuffer buffer = createTextBufferFromBackingBuffer(backingBuffer, DHSTR_MAKE_STRING("hello"));
	Add(&global_data->textBuffers, buffer);
	return &global_data->textBuffers.start[global_data->textBuffers.length - 1];
}



void clone_view(ViewHandle view_handle)
{
	assert(false && "na man, no time to implement atm");
}

void close(ViewHandle view_handle)
{
	TextBuffer *tb = ((TextBuffer*)view_handle);
	int index = tb - global_data->textBuffers.start;
	freeTextBuffer(*tb);
	Remove(&global_data->textBuffers, index);
	global_data->activeTextBufferIndex = 0;
	global_data->updateAllBuffers = true;
}

void setFocused(ViewHandle view_handle)
{
	TextBuffer *tb = ((TextBuffer*)view_handle);
	if (tb == global_data->commandLine)
	{
		global_data->isCommandlineActive = true;
	}
	else
	{
		int target_index = tb - global_data->textBuffers.start;
		assert(target_index >= 0 && target_index < global_data->textBuffers.length);
		global_data->activeTextBufferIndex = target_index;
	}
}

void set_type(ViewHandle view_handle, int type)
{
	TextBuffer *tb = ((TextBuffer*)view_handle);
	tb->bufferType = (BufferType)type;
}
int get_type(ViewHandle view_handle)
{
	TextBuffer *tb = ((TextBuffer*)view_handle);
	return (int)tb->bufferType;
}

void *openRedirectedCommandPrompt (char *command, int command_length)
{
	return platform_execute_command(command,command_length);
}

API getAPI()
{
	API api = {};
	api.buffer.view_iterator = view_iterator_from_buffer_handle;
	api.buffer.next          = next_view;
	api.buffer.save          = save_view;
	api.buffer.numLines      = lines_from_buffer_handle;

	api.changes.has_moved     = hash_moved_since_last;
	api.changes.mark_all_read = mark_all_changes_read;
	api.changes.next_change   = next_change;
	
	api.clipboard.copy  = clipboard_copy;
	api.clipboard.cut   = clipboard_cut;
	api.clipboard.paste = clipboard_paste;

	api.commandLine.clear          = commandline_clear;
	api.commandLine.close          = commandline_close;
	api.commandLine.executeCommand = commandline_execute_command;
	api.commandLine.feed           = commandline_feed;
	api.commandLine.open           = commandline_open;

	api.cursor.append_codepoint  = cursor_append_codepoint;
	api.cursor.delete_selection  = cursor_delete_selection;
	api.cursor.add               = cursor_add;
	api.cursor.remove            = cursor_remove;
	api.cursor.move	             = cursor_move;
	api.cursor.moveToLocation    = cursor_move_to_location;
	api.cursor.number_of_cursors = cursor_number_of_cursors;
	api.cursor.remove_codepoint  = cursor_remove_codepoint;
	api.cursor.selection_length  = cursor_selection_length;
	api.cursor.get_byte = cursor_get_byte;
	api.cursor.get_codepoint = cursor_get_codepoint;



	api.callbacks.bindCommand = callbacks_bind_command;
	api.callbacks.bindKey = callbacks_bind_key;
	api.callbacks.bindKey_mods = callbacks_bind_key_mods;
	api.callbacks.registerCallBack = callbacks_register_callback;

	api.handles.getActiveViewHandle = getActiveViewHandle;	 // does not include the commandline, may return null if no buffer is open
	api.handles.getFocusedViewHandle = getFocusedViewHandle;    // includes the commandline
	api.handles.getCommandLineViewHandle = getCommandLineViewHandle;
	api.handles.getViewHandleFromIndex = getViewHandleFromIndex;
	api.handles.getActiveBufferHandle = getActiveBufferHandle;	 // does not include the commandline, may return null if no buffer is open
	api.handles.getFocusedBufferHandle = getFocusedBufferHandle;    // includes the commandline
	api.handles.getCommandLineBufferHandle = getCommandLineBufferHandle;
	api.handles.getBufferHandleFromViewHandle = getBufferHandleFromViewHandle;

	api.Location.from_iterator = location_from_iterator;

	api.memory.allocateMemory = allocateMemory;
	api.memory.deallocateMemory = deallocateMemory;
	api.memory.getFunctionInfo = getFunctionInfo;

	api.misc.byteIndexFromLine = byteIndexFromLine;
	api.misc.lineFromByteIndex = lineFromByteIndex;
	api.misc.openRedirectedCommandPrompt = openRedirectedCommandPrompt; //@fixme

	api.misc.addMenuItem = MenuAddItem;
	api.misc.sortMenu= MenuSort;
	api.misc.move_active_menu = MenuMoveActiveItem;
	api.misc.disable_active_menu= MenuDisableActiveItem;


	api.text_iterator.make = make;
	api.text_iterator.make_from_cursor = make_from_cursor;
	api.text_iterator.make_from_location = make_from_location;
	api.text_iterator.move = move;
	api.text_iterator.get_byte= get_byte;
	api.text_iterator.get_codepoint= get_codepoint;


	api.markup.background_color = background_color;
	api.markup.font = font;
	api.markup.get_initial_rendering_state = get_initial_rendering_state;
	api.markup.set_initial_rendering_state = set_initial_rendering_state;
	api.markup.highlight_color= highlight_color;
	api.markup.remove= remove;
	api.markup.scale= scale;
	api.markup.text_color= text_color;

	api.view.clone_view = clone_view;
	api.view.close = close;
	api.view.createFromFile = createFromFile;
	api.view.get_type= get_type;
	api.view.set_type = set_type;
	api.view.setFocused = setFocused;
	api.view.createFromBufferHandle= createFromBufferHandle;

	return api;
}






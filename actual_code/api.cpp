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


//OLD STUFF:

internal void freeMenuItems(Data *data)
{
	for (int i = 0; i < data->menu.length; i++)
	{
		free_(data->menu.start[i].name.start);
		free_(data->menu.start[i].data);
	}
	data->menu.length = 0;
}



struct MenuData
{
	bool isFile;
};

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
	if (!success)
	{
		freeMenuItems(data);
		return;
	}

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
		if (&global_data->textBuffers.start[i].backingBuffer == iterator->buffer_handle)
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
	if (!success)
	{
		freeMenuItems(global_data); //we totes need this to be fixed dude.
		return;
	}

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

int cursor_move(ViewHandle view_handle, int direction, int cursor_index, bool select, API_MoveMode mode)
{
	int ack = 0;
	TextBuffer *buffer = (TextBuffer *)view_handle;
	if (cursor_index == ALL_CURSORS)
	{
		for (int i = 0; i < buffer->ownedCarets_id.length; i++)
		{
			for (int i = 0; i < abs(direction); i++)
			{
				if (mode == move_mode_line)
				{
					//TODO MAKE MOVE V RETURN SUCCESS
					moveV_nc(buffer, select ? do_select : no_select, i, direction < 0);
				}
				else
				{
					ack += move_llnc_(buffer, direction > 0 ? dir_right : dir_left, buffer->ownedCarets_id.start[i], do_log, MoveMode_from_API_MoveMode(mode));
				}
			}
			if (select)
			{
				setNoSelection(buffer, i, do_log);
			}
			if (mode != move_mode_line)
				markPreferedCaretXDirty(buffer, i);
		}
	}
	else
	{
		for (int i = 0; i < abs(direction); i++)
		{
			if (mode == move_mode_line)
			{
				//TODO MAKE MOVE V RETURN SUCCESS
				moveV_nc(buffer, select ? do_select : no_select, cursor_index, direction < 0);
			}
			else
			{
				ack += move_llnc_(buffer, direction > 0 ? dir_right : dir_left, buffer->ownedCarets_id.start[cursor_index], do_log, MoveMode_from_API_MoveMode(mode));
			}
		}
		if (select)
		{
			setNoSelection(buffer, cursor_index, do_log);
		}
		if (mode != move_mode_line)
			markPreferedCaretXDirty(buffer, cursor_index);
	}
	removeOwnedEmptyCarets(buffer);

	return ack; //return number of places the cursors have moved together.
}

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
	for (int i = 0; i < direction; i++)
	{
		TextBuffer *buffer = (TextBuffer *)view_handle;
		if (cursor_index == ALL_CURSORS)
		{
			for (int i = 0; i < buffer->ownedCarets_id.length; i++)
				unDeleteCharacter(buffer, character, cursor_index);
		}
		else
		{
			unDeleteCharacter(buffer, character, cursor_index);
		}
	}
	
	for (int i = 0; i < -direction; i++)
	{
		TextBuffer *buffer = (TextBuffer *)view_handle;
		if (cursor_index == ALL_CURSORS)
		{
			for (int i = 0; i < buffer->ownedCarets_id.length;i++)
				appendCharacter(buffer, character, i);
		}
		else
		{
			appendCharacter(buffer, character, cursor_index);
		}
	}
}

void cursor_remove_codepoint(ViewHandle view_handle, int cursor_index, int direction)
{
	for (int i = 0; i < direction; i++)
	{
		TextBuffer *buffer = (TextBuffer *)view_handle;
		if (cursor_index == ALL_CURSORS)
		{
			for (int i = 0; i < buffer->ownedCarets_id.length; i++)
				deleteCharacter(buffer, cursor_index);
		}
		else
		{
			deleteCharacter(buffer, cursor_index);
		}
	}

	for (int i = 0; i < -direction; i++)
	{
		TextBuffer *buffer = (TextBuffer *)view_handle;
		if (cursor_index == ALL_CURSORS)
		{
			for (int i = 0; i < buffer->ownedCarets_id.length; i++)
				removeCharacter(buffer, i);
		}
		else
		{
			removeCharacter(buffer, cursor_index);
		}
	}
}

int  cursor_selection_length(ViewHandle view_handle, int cursor_index)
{
	TextBuffer *buffer = (TextBuffer *)view_handle;
	MultiGapBuffer *mgb = buffer->backingBuffer->buffer;
	int ack = 0;
	if (cursor_index == ALL_CURSORS)
	{
		int selPos = getCaretPos(mgb, buffer->ownedSelection_id.start[cursor_index]);
		int carPos = getCaretPos(mgb, buffer->ownedCarets_id.start[cursor_index]);
		ack = abs(selPos - carPos);
	}
	else
	{
		for (int i = 0; i < buffer->ownedCarets_id.length;i++)
		{
			int selPos = getCaretPos(mgb, buffer->ownedSelection_id.start[i]);
			int carPos = getCaretPos(mgb, buffer->ownedCarets_id.start[i]);
			ack +=  abs(selPos - carPos);
		}
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
	void **data;
	if (lookup(&((CommonBuffer *)handle)->bindingMemory, bi, &data))
		return data;
	else
		return insert(&((CommonBuffer *)handle)->bindingMemory, bi, data);
}

void *allocateMemory(void *handle, size_t size)
{
	return ((CommonBuffer *)handle)->allocator.alloc(size, "user_space_allocation", 0);
}

void  deallocateMemory(void *handle, void *memory)
{
	((CommonBuffer *)handle)->allocator.free(memory, 0);
}

int byteIndexFromLine(BufferHandle buffer_handle, int line)
{
	return 0; //@fixme
}
int lineFromByteIndex(BufferHandle buffer_handle, int byte_index)
{
	return 0; //@fixme
}


TextIterator _mk_it(MGB_Iterator mgbit)
{
	TextIterator it = {};
	it.current.block_index = mgbit.block_index;
	it.start.block_index = mgbit.block_index;

	it.current.sub_index = mgbit.sub_index;
	it.start.sub_index = mgbit.sub_index;
	return it;
}
TextIterator make(BufferHandle buffer_handle)
{
	MultiGapBuffer *mgb = ((BackingBuffer *)buffer_handle)->buffer;
	return _mk_it(getIterator(mgb));
}

TextIterator make_from_location(BufferHandle buffer_handle, Location loc)
{
	BackingBuffer *backingBuffer = ((BackingBuffer *)buffer_handle);
	return _mk_it(iteratorFromLocation(backingBuffer, loc));
}

TextIterator make_from_cursor(ViewHandle view_handle, int cursor_index)
{
	TextBuffer *tb = (TextBuffer *)view_handle;
	assert(cursor_index != ALL_CURSORS && "must supply a specific cursor index to api__iterator.make_from_cursor");


	return _mk_it(getIteratorFromCaret(tb->backingBuffer->buffer, tb->ownedCarets_id.start[cursor_index]));
}

bool nextByte(TextIterator *it, char *out_result_, bool wrap)
{
	return false; //TODO FIXME
}

bool nextCodepoint(TextIterator *it, char32_t *out_result, bool wrap)
{
	return false; //TODO FIXME
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
	DHSTR_String path_str = DHSTR_makeString_(path, path_length);
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

void clone_view(ViewHandle view_handle)
{
	assert(false && "na man, no time to implement atm");
}

void close(ViewHandle view_handle)
{
	TextBuffer *tb = ((TextBuffer*)view_handle);
	closeTextBuffer(global_data); //closes current...
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

	api.callbacks.bindCommand = callbacks_bind_command;
	api.callbacks.bindKey = callbacks_bind_key;
	api.callbacks.bindKey_mods = callbacks_bind_key_mods;
	api.callbacks.registerCallBack = callbacks_register_callback;

	api.handles.getActiveBufferHandle = getActiveViewHandle;	 // does not include the commandline, may return null if no buffer is open
	api.handles.getFocusedViewHandle = getFocusedViewHandle;    // includes the commandline
	api.handles.getCommandLineViewHandle = getCommandLineViewHandle;
	api.handles.getViewHandleFromIndex = getViewHandleFromIndex;
	api.handles.getActiveBufferHandle = getActiveBufferHandle;	 // does not include the commandline, may return null if no buffer is open
	api.handles.getFocusedBufferHandle = getFocusedBufferHandle;    // includes the commandline
	api.handles.getCommandLineViewHandle = getCommandLineBufferHandle;
	api.handles.getBufferHandleFromViewHandle = getBufferHandleFromViewHandle;

	api.Location.from_iterator = location_from_iterator;

	api.memory.allocateMemory = allocateMemory;
	api.memory.deallocateMemory = deallocateMemory;
	api.memory.getFunctionInfo = getFunctionInfo;

	api.misc.byteIndexFromLine = byteIndexFromLine;
	api.misc.lineFromByteIndex = lineFromByteIndex;
	api.misc.openRedirectedCommandPrompt = 0; //@fixme

	api.text_iterator.make = make;
	api.text_iterator.make_from_cursor = make_from_cursor;
	api.text_iterator.make_from_location = make_from_location;
	api.text_iterator.nextByte = nextByte;
	api.text_iterator.nextCodepoint= nextCodepoint;
		
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
	api.view.setActive= api.view.setActive;


	return api;
}






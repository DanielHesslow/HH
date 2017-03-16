// BACKEND API LAYER

#define API extern "C" __declspec(dllexport)

#define HEADER_ONLY
//#include "api.h"
#include "api.h"
#include "header.h"


struct CommandInfo {
	char *name;
	int hashedName;
	void *user_data;
	void(*func)(char *str, int strlen, void **user_data);
	void(*charDown)(char *str, int strlen, void **user_data);
};

external char cursor_get_byte(ViewHandle view_handle, int cursor, int dir);

void *MenuAlloc(size_t bytes) {
	return global_data->menu.allocator->alloc(bytes).mem;
}

external void menu_clear() {
	global_data->menu.allocator->free_all();
	global_data->menu.items.length = 0;
}

MenuItem MenuItemFromApiMenuItem(API_MenuItem item) {
	MenuItem ret = {};
	ret.data = item.data;
	ret.name = { item.name,item.name_length };
	return ret;
}

API_MenuItem ApiMenuItemFromMenuItem(MenuItem item) {
	API_MenuItem ret = {};
	ret.data = item.data;
	ret.name = item.name.start;
	ret.name_length = item.name.length;
	return ret;
}

external void menu_add(API_MenuItem item) {
	char *str = (char *)MenuAlloc(item.name_length);
	memcpy(str, item.name, item.name_length);
	MenuItem item_ = {};
	item_.data = item.data;
	item_.name = { str,item.name_length };
	global_data->menu.items.add(item_);
}

static void *user_data_sort;
int(*comparator_sort)(API_MenuItem a, API_MenuItem b, void *user_data);
int comp(const void *a, const void *b) {
	API_MenuItem a_ = ApiMenuItemFromMenuItem(*(MenuItem *)a);
	API_MenuItem b_ = ApiMenuItemFromMenuItem(*(MenuItem *)b);
	return comparator_sort(a_, b_, user_data_sort);
}

void menu_sort(int(*cmp)(API_MenuItem a, API_MenuItem b, void *user_data), void *user_data) {
	comparator_sort = cmp;
	user_data_sort = user_data;
	qsort(&global_data->menu.items.start, global_data->menu.items.length, sizeof(MenuItem), comp);
}

external void menu_move_active(int dir) {
	global_data->menu.active_item += dir;
	global_data->menu.active_item = clamp(global_data->menu.active_item, 0, global_data->menu.items.length);
}

void menu_disable_active() {
	global_data->menu.active_item = 0;
}


#define DA_TYPE CommandInfo
#define DA_NAME DA_CommandInfo
#include "DynamicArray.h"
static DA_CommandInfo commands = DA_CommandInfo::make(general_allocator);


internal void bindCommand(char *name, StringFunction func) {
	CommandInfo commandInfo = {};
	commandInfo.name = name;
	commandInfo.hashedName = hash(name);
	commandInfo.func = func;
	commands.add(commandInfo);
}

internal void bindCommand(char *name, StringFunction func, StringFunction charDown) {
	CommandInfo commandInfo = {};
	commandInfo.name = name;
	commandInfo.hashedName = hash(name);
	commandInfo.func = func;
	commandInfo.charDown = charDown;
	commands.add(commandInfo);
}


internal CommandInfo findMatchingCommand(MultiGapBuffer *buffer, DA_CommandInfo commands, bool *success) {
	CommandInfo allocationInfo = {};
	MGB_Iterator it = getIterator(buffer);

	int ignore;

	int hash = getNextTokenHash(buffer, &it, &ignore);
	for (int i = 0; i < commands.length; i++) {
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
internal void keyDown_CommandLine(Data *data) {
	MultiGapBuffer *buffer = data->commandLine->backingBuffer->buffer;

	bool success;
	CommandInfo command = findMatchingCommand(buffer, commands, &success);
	if (!success) return;

	MGB_Iterator rest = getIterator(buffer, strlen(command.name));

	int ignore;
	stripInitialWhite(buffer, &rest, &ignore);
	char *remainingString = getRestAsString(buffer, rest, general_allocator);
	if (command.charDown)
		command.charDown(remainingString, strlen(remainingString), &command.user_data);
	general_allocator->free(remainingString, strlen(remainingString) + 1); // @allocator is not used in string, might change to not use malloc @cleanup @bug
}

bool matchMods(Mods mods, Mods filter, ModMode mode) {
	if (mode == atMost) {
		return !(mods & ~filter);
	} else if (mode == atLeast) {
		return (mods & filter) == filter;
	} else if (mode == precisely) {
		return (mods == filter);
	} else assert(false);
	return false;
}

global_variable DA_KeyBinding *bindings = (DA_KeyBinding *)0;
//external API getAPI();







void _setLocalBindings(TextBuffer *textBuffer) {
	if (!setBindingsLocal)return;
	assert(bindings == 0); // this is fine till we're using mutple threads, assert to make sure!
	bindings = &textBuffer->KeyBindings;
	setBindingsLocal(textBuffer);
	bindings = (DA_KeyBinding *)0;
}

KeyBinding bindKeyBase(char VK_Code, ModMode modMode, Mods mods, void *func) {
	KeyBinding binding = {};
	binding.VK_Code = VK_Code;
	binding.mods = mods;
	binding.funcP = func;
	binding.modMode = modMode;
	return binding;
}

internal void executeBindingFunction(KeyBinding binding, TextBuffer *buffer, Data *data, Mods currentMods) {
	if (binding.funcType == Arg_Mods) {
		binding.func_Mods(currentMods);
	} else if (binding.funcType == Arg_Void) {
		binding.func();
	} else assert(false);
}


void bindKey(char VK_Code, ModMode modMode, Mods mods, void(*func)()) {
	KeyBinding binding = bindKeyBase(VK_Code, modMode, mods, (void *)func);
	binding.funcType = Arg_Void;
	bindings->add(binding);
}

void bindKeyMods(char VK_Code, ModMode modMode, Mods mods, void(*func)(Mods mods)) {
	KeyBinding binding = bindKeyBase(VK_Code, modMode, mods, (void *)func);
	binding.funcType = Arg_Mods;
	bindings->add(binding);
}

























external ViewIterator view_iterator_from_buffer_handle(BufferHandle buffer_handle) {
	ViewIterator ret;
	ret.buffer_handle = buffer_handle;
	ret.next = 0;
	return ret;
}

external bool view_iterator_next(ViewIterator *iterator, ViewHandle *view_handle) {
	BackingBuffer *backingBuffer = (BackingBuffer *)iterator->buffer_handle;
	if (iterator->next >= backingBuffer->textBuffers.length) {
		view_handle = 0;
		return false;
	}
	*view_handle = backingBuffer->textBuffers.start[iterator->next++];
	return true;
}

bool textBuffer_index(TextBuffer *textBuffer, int *_out_index) {
	for (int i = 0; i < global_data->textBuffers.length; i++) {
		if (global_data->textBuffers.start[i] == textBuffer) {
			*_out_index = i;
			return true;
		}
	}
	*_out_index = -1;
	return false;
}

external bool save(ViewHandle handle) {
	saveFile((TextBuffer *)handle);
	return true; // heh
}
external int num_lines(BufferHandle handle) {
	return getLines((BackingBuffer *)handle);
}



external void copy(ViewHandle view_handle) {
	copy((TextBuffer *)view_handle);
}

external void paste(ViewHandle view_handle) {
	paste((TextBuffer *)view_handle);
}

external void cut(ViewHandle view_handle) {
	cut((TextBuffer *)view_handle);
}

external ViewHandle commandline_open() {
	global_data->isCommandlineActive = true;
	return global_data->commandLine;
}

external void commandline_close() {
	if (global_data->textBuffers.length > 0) {
		global_data->isCommandlineActive = false;
	}
}

void textBuffer_clear(TextBuffer *buffer) {
	MultiGapBuffer *mgb = buffer->backingBuffer->buffer;
	for (int i = 0; i < mgb->blocks.length; i++) {
		mgb->blocks.start[i].length = 0;
	}
}

external void commandline_clear() {
	textBuffer_clear(global_data->commandLine);
}

external void commandline_feed(char *string, int string_length) {
	for (int i = 0; i < string_length; i++) {
		appendCharacter(global_data->commandLine, string[i], 0, true);
	}
	//keyDown_CommandLine(global_data);
}

external void commandline_execute_command() {
	global_data->eatNext = true;

	MultiGapBuffer *mgb = global_data->commandLine->backingBuffer->buffer;

	bool success;
	CommandInfo command = findMatchingCommand(mgb, commands, &success);
	if (!success) return;

	MGB_Iterator rest = getIterator(mgb, strlen(command.name));

	int ignore;
	stripInitialWhite(mgb, &rest, &ignore);
	char *remainingString = getRestAsString(mgb, rest, general_allocator);
	if (command.func)
		command.func(remainingString, strlen(remainingString), &command.user_data);

	//free(remainingString); //@Cleanup @String @Allocation
	general_allocator->free(remainingString, strlen(remainingString) + 1);
	global_data->updateAllBuffers = true;
}

MoveMode MoveMode_from_API_MoveMode(API_MoveMode mode) {
	//fucking confusing why not use the same internally maybe?
	switch (mode) {
	case move_mode_grapheme_cluster: return movemode_grapheme_cluster;
	case move_mode_codepoint: return movemode_codepoint;
	case move_mode_byte: return movemode_byte;
	default: assert(false && "movemode aint existing"); //TODO: report error rather than assert or whatever
	}
	return movemode_byte;
}
external TextIterator text_iterator_from_cursor(ViewHandle view_handle, int cursor_index);
external int cursor_move(ViewHandle view_handle, int direction, int cursor_index, bool select, API_MoveMode mode) {
	int ack = 0;
	TextBuffer *buffer = (TextBuffer *)view_handle;
	int state = 0;

	for (int i = 0; i < abs(direction); i++) {
		if (mode == move_mode_line) {
			//TODO MAKE MOVE V RETURN SUCCESS
			moveV_nc(buffer, select ? do_select : no_select, cursor_index, direction < 0);
		} else {
			ack += move_llnc_(buffer, direction > 0 ? dir_right : dir_left, buffer->ownedCarets_id.start[cursor_index], true, MoveMode_from_API_MoveMode(mode));
		}
	}
	if (!select) {
		setNoSelection(buffer, cursor_index, do_log);
	}
	if (mode != move_mode_line)
		markPreferedCaretXDirty(buffer, cursor_index);

	return ack; //return number of places the cursors have moved together.
}

internal void removeAllButOneCaret(TextBuffer *buffer) {
	for (int i = 1; i < buffer->ownedCarets_id.length; ) {
		removeCaret(buffer, 1, true);
	}
}

external bool cursor_move_to_location(ViewHandle view_handle, int cursor_index, bool select, int line, int column) {
	TextBuffer *buffer = (TextBuffer *)view_handle;
	gotoStartOfLine(buffer, line, select ? do_select : no_select, cursor_index);
	cursor_move(buffer, column, cursor_index, select, move_mode_byte);
	return true; //TODO RETURN CORRECT
}

external int cursor_add(ViewHandle view_handle, Location loc) {
	TextBuffer *buffer = (TextBuffer *)view_handle;
	AddCaret(buffer, getLineStart(buffer->backingBuffer, loc.line) + loc.column);
	return buffer->ownedCarets_id.length - 1;
}

external bool cursor_remove(ViewHandle view_handle, int cursor_index) {
	TextBuffer *buffer = (TextBuffer *)view_handle;

	removeCaret(buffer, cursor_index, true);
	return true; //TODO we can totes fail 
}

external void append_byte(ViewHandle view_handle, int cursor_index, int direction, char byte) {
	TextBuffer *buffer = (TextBuffer *)view_handle;
	int state = 0;

	for (int i = 0; i < direction; i++)
		unDeleteCharacter(buffer, byte, cursor_index);

	for (int i = 0; i < -direction; i++)
		appendCharacter(buffer, byte, cursor_index);

	setNoSelection(buffer, do_log);
}

bool utf8_initial_byte(unsigned char byte) {
	int num_leading_ones = __lzcnt16(((uint16_t)~byte) & 0xff) - 8;
	return num_leading_ones != 1;
}

external void cursor_remove_codepoint(ViewHandle view_handle, int cursor_index, int direction) {
	TextBuffer *buffer = (TextBuffer *)view_handle;
	int state = 0;

	for (int i = 0; i < direction; i++) {
		for (;;) {
			deleteCharacter(buffer, cursor_index);
			char next_byte = cursor_get_byte(view_handle, cursor_index, 1);
			if (utf8_initial_byte(next_byte))break;
		}

	}

	for (int i = 0; i < -direction; i++) {
		for (;;) {
			char next_byte = cursor_get_byte(view_handle, cursor_index, -1);
			removeCharacter(buffer, cursor_index);
			if (utf8_initial_byte(next_byte))break;
		}
	}
}

external int selection_length(ViewHandle view_handle, int cursor_index) {
	TextBuffer *buffer = (TextBuffer *)view_handle;
	MultiGapBuffer *mgb = buffer->backingBuffer->buffer;
	int ack = 0;

	int state = 0;

	int selPos = getCaretPos(mgb, buffer->ownedSelection_id.start[cursor_index]);
	int carPos = getCaretPos(mgb, buffer->ownedCarets_id.start[cursor_index]);
	ack += abs(selPos - carPos);

	return ack;
}

external int delete_selection(ViewHandle view_handle, int cursor_index) {
	TextBuffer *buffer = (TextBuffer *)view_handle;
	int selection_len = selection_length(view_handle, cursor_index);
	removeSelection(buffer, cursor_index);
	setNoSelection(buffer, do_log);
	return selection_len; //TODO FIXME
}

external int num_cursors(ViewHandle view_handle) {
	TextBuffer *buffer = (TextBuffer *)view_handle;
	return buffer->ownedCarets_id.length;

}




external void register_callback(CallBackTime time, ViewHandle view_handle, void(*function)(ViewHandle handle)) {
	TextBuffer *buffer = (TextBuffer *)view_handle;
	switch (time) {
	case callback_pre_render:
		buffer->renderingModifiers.add((RenderingModifier)function);
		break;
	default:
		assert(false && "un handeled callbacktime");
	}
}

external void bind_key_mods(ViewHandle view_handle, char VK_Code, ModMode filter, Mods mods, void(*func)(Mods mods)) {
	TextBuffer *buffer = (TextBuffer *)view_handle;
	bindKeyMods(VK_Code, filter, mods, func);
}

external void bind_key(ViewHandle view_handle, char VK_Code, ModMode filter, Mods mods, void(*func)()) {
	TextBuffer *buffer = (TextBuffer *)view_handle;
	bindKey(VK_Code, filter, mods, func);
}

external void bind_command(char *name, StringFunction select, StringFunction charDown, void(*interupt)(void **user_data)) {
	bindCommand(name, select, charDown);
}


external ViewHandle view_handle_from_index(int index) {
	if (index >= 0 && global_data->textBuffers.length > index)
		return global_data->textBuffers.start[index];
	return 0;
}

external ViewHandle view_handle_active()	 // does not include the commandline, may return null if no buffer is open
{
	return view_handle_from_index(global_data->activeTextBufferIndex);
}

external ViewHandle view_handle_cmdline() {
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
external BufferHandle buffer_handle_cmdline() {
	return((TextBuffer *)view_handle_cmdline())->backingBuffer;
}
external BufferHandle buffer_handle_from_view_handle(ViewHandle view_handle) {
	return ((TextBuffer *)view_handle)->backingBuffer;
}

external Location location_from_iterator(BufferHandle bufferHandle, TextIterator it) {
	BackingBuffer *bb = (BackingBuffer *)bufferHandle;
	int pos = getIteratorPos(bb->buffer, { it.current.block_index,it.current.sub_index });
	int line = getLine(bb, pos);
	int line_start = getLineStart(bb, line);
	return{ line, pos - line_start };
}

external Location location_from_cursor(ViewHandle viewHandle, int cursor_index) {
	TextBuffer *tb = (TextBuffer *)viewHandle;
	return getLocationFromCaret(tb, tb->ownedCarets_id.start[cursor_index]);

}

external void *function_info_ptr(void *handle, void *function) {
	BindingIdentifier bi = { 1, function };
	void *data = 0;
	CommonBuffer *cb = (CommonBuffer *)handle;
	if (cb->bindingMemory.lookup(bi, &data))
		return data;

	cb->bindingMemory.insert(bi, data);
	assert(cb->bindingMemory.lookup(bi, &data));
	return data; // @CLEANUP HASHTABLE_API....
}

external MemBlock memory_alloc(void *handle, size_t size) {
	CommonBuffer *cb = (CommonBuffer *)handle;
	return cb->allocator->alloc(size);
}

external void memory_free(void *handle, MemBlock *memblock) {
	CommonBuffer *cb = (CommonBuffer *)handle;
	cb->allocator->free(memblock);
}

external int byte_index_from_line(BufferHandle buffer_handle, int line) {
	return 0; //@fixme
}
external int line_from_byte_index(BufferHandle buffer_handle, int byte_index) {
	return 0; //@fixme
}


TextIterator _mk_it(MGB_Iterator mgbit, BufferHandle handle) {
	TextIterator it = {};
	it.current.block_index = mgbit.block_index;
	it.start.block_index = mgbit.block_index;

	it.current.sub_index = mgbit.sub_index;
	it.start.sub_index = mgbit.sub_index;
	it.buffer_handle = handle;
	return it;
}

external TextIterator text_iterator_start(BufferHandle buffer_handle) {
	MultiGapBuffer *mgb = ((BackingBuffer *)buffer_handle)->buffer;
	return _mk_it(getIterator(mgb), buffer_handle);
}

external TextIterator text_iterator_from_location(BufferHandle buffer_handle, Location loc) {
	BackingBuffer *backingBuffer = ((BackingBuffer *)buffer_handle);
	return _mk_it(iteratorFromLocation(backingBuffer, loc), buffer_handle);
}

external TextIterator text_iterator_from_cursor(ViewHandle view_handle, int cursor_index) {
	TextBuffer *tb = (TextBuffer *)view_handle;
	return _mk_it(getIteratorFromCaret(tb->backingBuffer->buffer, tb->ownedCarets_id.start[cursor_index]), tb->backingBuffer);
}

external char text_iterator_get_byte(TextIterator it, int dir) {
	MGB_Iterator _it = { it.current.block_index, it.current.sub_index };
	MultiGapBuffer *mgb = ((BackingBuffer *)it.buffer_handle)->buffer;
	if (dir > 0) {
		return *getCharacter(mgb, _it);
	} else {
		MoveIterator(mgb, &_it, -1);
		return *getCharacter(mgb, _it);
	}
}



#include "intrin.h"
#include "stdint.h"

external char32_t text_iterator_get_codepoint(TextIterator it, int dir) {
	assert(dir == 1 || dir == -1);
	//we better not be in the middle of a codepoint
	MGB_Iterator _it = { it.current.block_index, it.current.sub_index };
	MultiGapBuffer *mgb = ((BackingBuffer *)it.buffer_handle)->buffer;
	unsigned char bytes[8];
	int len = 0;
	unsigned char *start = 0;
	if (dir > 0) {
		bytes[len++] = *getCharacter(mgb, _it);
		int num_bytes = __lzcnt16((uint16_t)~bytes[0]) - 8;
		for (int i = 0; i < num_bytes - 1; i++) {
			if (!MoveIterator(mgb, &_it, 1))return 0;
			bytes[len++] = *getCharacter(mgb, _it);
		}

		start = bytes;
	} else {
		for (int i = 0; i < 8; i++) {
			if (!MoveIterator(mgb, &_it, -1))return 0;
			++len;
			bytes[sizeof(bytes) - len] = *getCharacter(mgb, _it);
			int num_lo = __lzcnt16((uint16_t)~bytes[sizeof(bytes) - len]) - 8;
			if (num_lo != 1) goto success; //is this correct?
		}
		assert(false && "tried to read utf-8 larger than 8 bytes long ...");
	success:
		start = &bytes[sizeof(bytes) - len];
	}


	char32_t result = 0;

	for (int i = 0; i < len; i++) {
		if (i == 0) {
			if (len == 1) {
				result = start[0] & 0x7f;
			} else {
				result = (start[0] << (len + 1)) >> (len + 1);
			}
		} else {
			result = (result << 6) | ((start[i*dir] << 2) >> 2);
		}
	}
	return result;
}


external char cursor_get_byte(ViewHandle view_handle, int cursor, int dir) {
	return text_iterator_get_byte(text_iterator_from_cursor(view_handle, cursor), dir);
}

external char32_t cursor_get_codepoint(ViewHandle view_handle, int cursor, int dir) {
	return text_iterator_get_codepoint(text_iterator_from_cursor(view_handle, cursor), dir);
}


external int text_iterator_move(TextIterator *it, int direction, API_MoveMode mode, bool wrap) {
	MultiGapBuffer *mgb = ((BackingBuffer *)it->buffer_handle)->buffer;
	switch (mode) {
	case move_mode_byte:
		return MoveIterator(mgb, (MGB_Iterator*)it, direction); //TODO return correct amount
		break;
	}
	assert(false);
	return 0;
}


// --- Markup
external void markup_remove(ViewHandle view_handle, void *function) {
	TextBuffer *tb = (TextBuffer *)view_handle;
	remove_rendering_changes(tb, function);
}

external void markup_highlight_color(ViewHandle view_handle, void *function, Location start_inclusive, Location end_exclusive, Color color) {
	TextBuffer *tb = (TextBuffer *)view_handle;
	change_rendering_highlight_color(tb, start_inclusive, end_exclusive, color, function);
}

external void markup_background_color(ViewHandle view_handle, void *function, Location start_inclusive, Location end_exclusive, Color color) {
	TextBuffer *tb = (TextBuffer *)view_handle;
	change_rendering_background_color(tb, start_inclusive, end_exclusive, color, function);
}

external void markup_text_color(ViewHandle view_handle, void *function, Location start_inclusive, Location end_exclusive, Color color) {
	TextBuffer *tb = (TextBuffer *)view_handle;
	change_rendering_color(tb, start_inclusive, end_exclusive, color, function);
}

external void markup_scale(ViewHandle view_handle, void *function, Location start_inclusive, Location end_exclusive, float scale) {
	TextBuffer *tb = (TextBuffer *)view_handle;
	change_rendering_scale(tb, start_inclusive, end_exclusive, scale, function);
}

external void markup_font(ViewHandle view_handle, void *function, Location start_inclusive, Location end_exclusive, char *font_name, int font_length) {
	TextBuffer *tb = (TextBuffer *)view_handle;
	String font_str = { font_name, font_length };
	Typeface::Font *font = getFont(font_str);
	change_rendering_font(tb, start_inclusive, end_exclusive, font, function);
}

RenderingState RenderingStateFromCharRenderingInfo(CharRenderingInfo info) {
	Color color;
	RenderingState rs = {};
	rs.background_color = colorFromInt(info.background_color);
	rs.highglight_color = colorFromInt(info.highlight_color);
	rs.text_color = colorFromInt(info.color);
	rs.scale = info.scale;
	//TODO FONT HANDLE??
	return rs;
}

CharRenderingInfo CharRenderingInfoFromRenderingState(RenderingState state) {
	CharRenderingInfo info = {};
	info.background_color = (int)state.background_color;
	info.color = (int)state.text_color;
	info.highlight_color = (int)state.highglight_color;
	info.scale = state.scale;
	return info;
}

external RenderingState markup_get_initial_rendering_state(void *view_handle) {
	TextBuffer *tb = (TextBuffer *)view_handle;
	RenderingState rs = {};
	return RenderingStateFromCharRenderingInfo(tb->initial_rendering_state);
}

external void markup_set_initial_rendering_state(void *view_handle, RenderingState state) {
	TextBuffer *tb = (TextBuffer *)view_handle;
	tb->initial_rendering_state = CharRenderingInfoFromRenderingState(state);
}


// --- view
external void *open_view(char *path, int path_length) {
	String path_string = { path, path_length }; //@hacky
	bool success;
	TextBuffer *buffer = openFileIntoNewBuffer(path_string, &success);
	if (success) {
		global_data->textBuffers.add(buffer);
		global_data->activeTextBufferIndex = global_data->textBuffers.length - 1;
		return &global_data->textBuffers[global_data->textBuffers.length - 1];
	}
	return 0;
}

external ViewHandle view_from_buffer_handle(BufferHandle buffer_handle) {
	BackingBuffer *backingBuffer = (BackingBuffer*)buffer_handle;
	TextBuffer *buffer = createTextBufferFromBackingBuffer(backingBuffer, String::make("hello"));
	global_data->textBuffers.add(buffer);
	return &global_data->textBuffers[global_data->textBuffers.length - 1];
}



external void view_clone(ViewHandle view_handle) {
	assert(false && "na man, no time to implement atm");
}

external void view_close(ViewHandle view_handle) {
	TextBuffer *tb = ((TextBuffer*)view_handle);
	int index;
	assert(textBuffer_index(tb, &index));
	freeTextBuffer(tb);
	global_data->textBuffers.remove(index);
	global_data->activeTextBufferIndex = 0;
	global_data->updateAllBuffers = true;
}

external void view_set_focused(ViewHandle view_handle) {
	TextBuffer *tb = ((TextBuffer*)view_handle);
	if (tb == global_data->commandLine) {
		global_data->isCommandlineActive = true;
	} else {
		int target_index;
		assert(textBuffer_index(tb, &target_index));
		assert(target_index >= 0 && target_index < global_data->textBuffers.length);
		global_data->activeTextBufferIndex = target_index;
	}
}

external void view_set_type(ViewHandle view_handle, int type) {
	TextBuffer *tb = ((TextBuffer*)view_handle);
	tb->bufferType = (BufferType)type;
}

external int view_get_type(ViewHandle view_handle) {
	TextBuffer *tb = ((TextBuffer*)view_handle);
	return (int)tb->bufferType;
}

external void *open_redirected_terminal(char *command, int command_length) {
	return platform_execute_command(command, command_length);
}


external bool menu_get_active(API_MenuItem *_out_MenuItem) {
	Menu menu = global_data->menu;
	if (!menu.active_item)return false;
	else {
		if (_out_MenuItem) *_out_MenuItem = ApiMenuItemFromMenuItem(global_data->menu.items.start[global_data->menu.active_item - 1]);
		return true;
	}
}
#include "History.h"

external void undo(ViewHandle handle) {
	TextBuffer *tb = (TextBuffer *)handle;
	undo(tb->backingBuffer);
}


external void redo(ViewHandle handle) {
	TextBuffer *tb = (TextBuffer *)handle;
	redo(tb->backingBuffer);
}

external void history_next_leaf(ViewHandle handle) {
	TextBuffer *tb = (TextBuffer *)handle;
	next_leaf(tb->backingBuffer);
}

external void history_insert_waypoint(ViewHandle handle) {
	TextBuffer *tb = (TextBuffer *)handle;
	_log_lazy_moves(&tb->backingBuffer->history);
	tb->backingBuffer->history.waypoints.add(tb->backingBuffer->history.state.location.prev_instruction_index);
}

external void cursors_remove_dups(ViewHandle handle) {
	TextBuffer *tb = (TextBuffer *)handle;
	removeOwnedEmptyCarets(tb);
}



int diffIndexMove(Layout *root, int active_index, int dir, LayoutType type) {
	int ack = 0;
	if (!dir)return 0;
	if (dir > 0) { // move right / down / forward
		LayoutLocator locator = locateLayout(root, active_index);
		if (!locator.parent)return 0;
		do {
			if (locator.child_index < locator.parent->number_of_children - 1 && locator.parent->type == type) {
				Layout *target_layout = locator.parent->children[locator.child_index + 1];
				ack += favourite_descendant(target_layout);
				locator.parent->favourite_child = locator.child_index + 1;
				return ack + 1;
			}
			for (int i = locator.child_index + 1; i < locator.parent->number_of_children; i++) {
				ack += number_of_leafs(locator.parent->children[i]);
			}
		} while (parentLayout(root, locator, &locator));
	} else { //move left / up / back
		LayoutLocator locator = locateLayout(root, active_index);
		if (!locator.parent)return 0;
		do {
			if (locator.child_index > 0 && locator.parent->type == type) {
				Layout *target_layout = locator.parent->children[locator.child_index - 1];
				ack += number_of_leafs(target_layout);
				ack = -ack;
				ack += favourite_descendant(target_layout);
				locator.parent->favourite_child = locator.child_index - 1;
				return ack;
			}
			for (int i = 0; i < locator.child_index; i++) {
				ack += number_of_leafs(locator.parent->children[i]);
			}
		} while (parentLayout(root, locator, &locator));
	}
	return 0;
}

external void move_layout(int dir, LayoutType type) {
	int new_index = diffIndexMove(global_data->layout, global_data->activeTextBufferIndex, dir, type);
	global_data->activeTextBufferIndex = clamp(global_data->activeTextBufferIndex + new_index, 0, global_data->textBuffers.length);
}

external void set_layout(Layout *layout) {
	global_data->layout = layout;
}


external int get_num_views() {
	return global_data->textBuffers.length;
}



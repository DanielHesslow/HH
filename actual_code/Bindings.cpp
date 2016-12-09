#include "api_2.h"
#include "ctype.h"
#include <set>
#include "windows.h"

internal void addDirFilesToMenu(std::string path);

#define USE_API
internal void showCommandLine()
{
	commandline_open();
}

internal void hideCommandLine()
{
	commandline_close();
}

internal void preFeedCommandLine(char *string, int length)
{
	commandline_open();
	commandline_clear();
	commandline_feed(string, length);
}

internal void preFeedOpenFile()
{
	char buffer[] = u8"openFile ";
	preFeedCommandLine(buffer, (sizeof(buffer) / sizeof(buffer[0]) - 1));
}

#define FOR_CURSORS(cursor,handle) for(int cursor=0; cursor<num_cursors(handle);cursor++)
void _delete(Mods mods, int direction)
{
	void *handle = view_handle_focused();
	bool deleted = false;
	if (mods & mod_control)
	{
		for (int cursor = 0; cursor<num_cursors(handle); cursor++)
		{
			if (selection_length(handle, cursor))
			{
				delete_selection(handle, cursor);
			}
			else
			{
				char byte;
				while ((byte = cursor_get_codepoint(handle, cursor, direction)) && isspace(byte) && byte != '\n')
				{
					cursor_remove_codepoint(handle, cursor, direction);
					deleted = true;
				}
				while ((byte = cursor_get_codepoint(handle, cursor, direction)) && !isspace(byte))
				{
					cursor_remove_codepoint(handle, cursor, direction);
					deleted = true;
				}
			}
		}
	}
	if (!deleted)
	{
		for (int cursor = 0; cursor<num_cursors(handle); cursor++)
			cursor_remove_codepoint(handle, cursor, direction);
	}
}

void _move(Mods mods, int direction)
{
	void *handle = view_handle_focused();
	bool moved = false;
	if (mods & mod_control)
	{
		for (int cursor = 0; cursor<num_cursors(handle); cursor++)
		{
			char byte;
			while ((byte = cursor_get_codepoint(handle, cursor, direction)) && isspace(byte) && byte != '\n')
			{
				if (!byte)break;
				cursor_move(handle, direction, cursor, mods & mod_shift, move_mode_codepoint);
				moved = true;
			}
			while ((byte = cursor_get_codepoint(handle, cursor, direction)) && !isspace(byte))
			{
				cursor_move(handle, direction, cursor, mods & mod_shift, move_mode_codepoint);
				moved = true;
			}
		}
	}
	if (!moved)
	{
		for (int cursor = 0; cursor<num_cursors(handle); cursor++)
			cursor_move(handle, direction, cursor, mods & mod_shift, move_mode_grapheme_cluster);
	}
}

void _moveRight(Mods mods)
{
	_move(mods, 1);
}

void _moveLeft(Mods mods)
{
	_move(mods, -1);
}

void _moveUp(Mods mods)
{
	void *handle = view_handle_focused();
	for (int cursor = 0; cursor<num_cursors(handle); cursor++)
		cursor_move(handle, -1, cursor, mods & mod_shift, move_mode_line);
}

void _moveDown(Mods mods)
{
	void *handle =view_handle_focused();
	for (int cursor = 0; cursor<num_cursors(handle); cursor++)
		cursor_move(handle, 1, cursor, mods & mod_shift, move_mode_line);
}

void _backSpace(Mods mods)
{
	_delete(mods, -1);
}


void _delete(Mods mods)
{
	_delete(mods, 1);
}

void charDownFileCommand(char *s, int s_len, void **ud)
{
	menu_clear();
	std::string string(s, s+s_len);
	addDirFilesToMenu(s);
}

#if 0
int diffIndexMove(Layout *root, int active_index, int dir, LayoutType type)
{
	int ack = 0;
	if (!dir)return 0;
	if (dir > 0)
	{ // move right / down / forward
		LayoutLocator locator = locateLayout(root, active_index);
		if (!locator.parent)return 0;
		do
		{
			if (locator.child_index < locator.parent->number_of_children - 1 && locator.parent->type == type)
			{
				Layout *target_layout = locator.parent->children[locator.child_index + 1];
				ack += favourite_descendant(target_layout);
				locator.parent->favourite_child = locator.child_index + 1;
				return ack + 1;
			}
			for (int i = locator.child_index + 1; i < locator.parent->number_of_children; i++)
			{
				ack += number_of_leafs(locator.parent->children[i]);
			}
		} while (parentLayout(root, locator, &locator));
	}
	else
	{ //move left / up / back
		LayoutLocator locator = locateLayout(root, active_index);
		if (!locator.parent)return 0;
		do
		{
			if (locator.child_index > 0 && locator.parent->type == type)
			{
				Layout *target_layout = locator.parent->children[locator.child_index - 1];
				ack += number_of_leafs(target_layout);
				ack = -ack;
				ack += favourite_descendant(target_layout);
				locator.parent->favourite_child = locator.child_index - 1;
				return ack;
			}
			for (int i = 0; i < locator.child_index; i++)
			{
				ack += number_of_leafs(locator.parent->children[i]);
			}
		} while (parentLayout(root, locator, &locator));
	}
	return 0;
}
#endif

#if 0

internal void CloseCommandLine(Data *data)
{
	clearBuffer(data->commandLine->backingBuffer->buffer);
	data->isCommandlineActive = false;
}

internal void preFeedCreateFile()
{
	char buffer[] = u8"createFile ";
	preFeedCommandLine(buffer, sizeof(buffer) / sizeof(buffer[0]));
}
#endif

enum Dir
{
	above,
	below,
};
#if 0
internal void insertCaret(TextBuffer *buffer, Dir dir)
{
	int pre_length = buffer->ownedCarets_id.length;
	for (int i = 0; i < pre_length; i++)
	{
		int caretId = buffer->ownedCarets_id.start[i];
		int pos = getCaretPos(buffer->backingBuffer->buffer, caretId);
		moveV_nc(buffer, no_select, i, dir == above);
		AddCaret(buffer, pos);
		buffer->cursorInfo.start[buffer->cursorInfo.length - 1].is_dirty = true;
	}
	removeOwnedEmptyCarets(buffer);
}
#endif



void insertCaretAbove()
{
	ViewHandle view_handle = view_handle_active();
	int _num_cursors = num_cursors(view_handle);
	for (int cursor = 0; cursor < _num_cursors; cursor++)
	{
		Location loc = location_from_cursor(view_handle, cursor);
		loc.line = max(loc.line - 1, 0);
		cursor_add(view_handle, loc);
	}
}

void insertCaretBelow()
{
	ViewHandle view_handle = view_handle_active();
	int _num_cursors = num_cursors(view_handle);
	for (int cursor = 0; cursor < _num_cursors; cursor++)
	{
		BufferHandle buffer_handle = buffer_handle_active();
		Location loc = location_from_cursor(view_handle, cursor);
		loc.line = min(loc.line + 1, num_lines(buffer_handle));
		cursor_add(view_handle, loc);
	}
}

void removeAllButOneCaret()
{
	ViewHandle view_handle = view_handle_active();
	int _num_cursors = num_cursors(view_handle);
	for (int cursor = 1; cursor < _num_cursors; cursor++)
	{
		cursor_remove(view_handle, 1);
	}
	delete_selection(view_handle, 0);
}


std::string directory(std::string path)
{
	int index_of_last_slash = path.rfind('/');
	if (index_of_last_slash == -1)return "";
	else return path.substr(0, index_of_last_slash);
}

internal void openFileCommand(char *start, int length, void **user_data)
{
	API_MenuItem item;
	bool has_active_menu = menu_get_active(&item);
	if (has_active_menu)
	{
		if (open_view(item.name, item.name_length))
		{
			commandline_close();
		}
		else
		{
			commandline_clear();
			commandline_feed("openFile ",strlen("openFile "));
			std::string path(start, start + length);
			std::string dir = directory(path);
			std::string item(item.name, item.name_length);
			std::string to_feed = dir + item + "/";
			commandline_feed((char *)to_feed.c_str(), to_feed.length());
			charDownFileCommand((char *)to_feed.c_str(),to_feed.length(),user_data);
		}
	}
	else
	{
		open_view(start, length);
		commandline_close();
	}
}

#if 0
void find_mark_down(char *text, int text_length)
{
	void *view_handle = view_handle_active()

	api.markup.remove(view_handle, &find_mark_down);
	if (text_length == 0) return;

	BufferHandle buffer_handle = api.handles.getBufferHandleFromViewHandle(view_handle);
	TextIterator it = api.text_iterator.make(buffer_handle);
	Location match_location;
	int i = 0;
	do {
		char b = api.text_iterator.get_byte(it, 1);
		if (b == text[i])
		{
			if (i == 0)
			{
				match_location = api.Location.from_iterator(buffer_handle, it);
			}

			if (++i == text_length)
			{
				TextIterator it_cpy = it;
				api.text_iterator.move(&it_cpy, 1, move_mode_codepoint, false);
				api.markup.highlight_color(view_handle, &find_mark_down, match_location, api.Location.from_iterator(buffer_handle, it_cpy), default_colorScheme.active_color);
				i = 0;
			}
		}
		else
		{
			i = 0;
		}
	} while (api.text_iterator.move(&it, 1, move_mode_byte, false));


}

void find(char *start, int length)
{
	markup_remove(buffer_handle_active(), find_mark_down);
}
#endif 


internal void addDirFilesToMenu(std::string path) 
{
	WIN32_FIND_DATA ffd;
	std::string search_path;
	size_t index_last_slash = path.rfind('\\');
	search_path ="./" + path + "*";

	HANDLE handle = FindFirstFileA(search_path.c_str(), &ffd);
	if (handle != INVALID_HANDLE_VALUE)
	{
		do
		{
			API_MenuItem newItem = {};
			newItem.name = ffd.cFileName;
			newItem.name_length = strlen(newItem.name);
			std::string p = ffd.cFileName;
			std::string extension = p.substr(p.rfind('.')+1,newItem.name_length);
			bool okExtension = extension == "txt"||extension == "c"|| extension == "cpp"|| extension == "bat" || extension == "h";
			
			if (okExtension || ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{
				menu_add(newItem);
			}
		} while (FindNextFileA(handle, &ffd) != 0);  //we trigger some breakpoint in here. what's t he problem?
		FindClose(handle);
	}
}



#if 0
void _cloneBuffer(Data *data)
{
	api.view.clone_view(view_handle_active());
}
#endif


internal void appendVerticalTab()
{
	ViewHandle handle = view_handle_active();
	for (int cursor = 0; cursor < num_cursors(handle); cursor++)
		append_codepoint(handle, cursor, -1, '\v');
}
#if 0

unsigned int hash_loc(Location loc)
{
	return silly_hash(loc.line) * 31 + silly_hash(loc.column);
}
unsigned int locator_equality(Location a, Location b)
{
	return a.line == b.line && a.column == b.column;
}
DEFINE_HashTable(Location, int, hash_loc, locator_equality);
DEFINE_HashTable(int, float, silly_hash, int_eq);
bool bool_eq(bool a, bool b) { return a == b; }
DEFINE_HashTable(int, bool, silly_hash, bool_eq);
#endif


std::set<int> big_titles;

internal bool is_big_title(void *buffer_handle, int line)
{

	TextIterator it = text_iterator_from_location(buffer_handle, { line,0 });
	int ack = 0;
	do {
		char b = text_iterator_get_byte(it,1);

		if (b == '\n') break;
		else if (b == '-') { if (++ack >= 3) return true; }
		else ack = 0;
	} while (text_iterator_move(&it, 1, move_mode_byte, false));
	return false;
}

internal void init_big_titles(void *buffer_handle, std::set<int> *big_titles)
{
	big_titles->clear();
	for (int i = 0; i < num_lines(buffer_handle); i++)
	{
		if (is_big_title(buffer_handle, i))
		{
			//insert(memo, i, true);
			big_titles->insert(i);
		}
	}
}


internal void mark_bigTitles(void *view_handle)
{
	bool changed = false;

	void *buffer_handle = buffer_handle_from_view_handle(view_handle);
	std::set<int> *big_titles;

	void **mem = function_info_ptr(buffer_handle, &mark_bigTitles);
	if (*mem)
	{
		big_titles = (std::set<int> *)*mem;
	}
	else
	{
		big_titles = (std::set<int> *)memory_alloc(buffer_handle, sizeof(std::set<int>));
		new (big_titles)std::set<int>();
		changed = true;
		init_big_titles(buffer_handle, big_titles);
		*mem = big_titles;
	}
#if 0
	BufferChange event;
	while (api.changes.next_change(buffer_handle, &mark_bigTitles, &event))
	{
		changed = true;
		api.changes.mark_all_read(buffer_handle, &mark_bigTitles);

		if (is_big_title(buffer_handle, event.location.line))
			big_titles->insert(event.location.line);
		else
			big_titles->erase(event.location.line);

		//lazy af way to handle linebreak lol
		if (is_big_title(buffer_handle, event.location.line - 1))
			big_titles->insert(event.location.line-1);
		else
			big_titles->erase(event.location.line-1);


		if (is_big_title(buffer_handle, event.location.line + 1))
			big_titles->insert(event.location.line + 1);
		else
			big_titles->erase(event.location.line + 1);
	}
#endif
	if (changed)
	{
		void *view_handle;
		ViewIterator vi = view_iterator_from_buffer_handle(buffer_handle);
		while (view_iterator_next(&vi, &view_handle))
		{
			float title_size = markup_get_initial_rendering_state(view_handle).scale*1.5f;
			markup_remove(view_handle, &mark_bigTitles);

			
			for (auto it = big_titles->begin(); it != big_titles->end(); ++it)
			{
				int line = *it;
				markup_scale(view_handle, &mark_bigTitles, { line, 0 }, { line + 1,0 }, title_size);
				markup_text_color(view_handle, &mark_bigTitles, { line, 0 }, { line + 1,0 }, default_colorScheme.active_color);
			}
		}
	}
}


#if 0
internal int elasticTab(TextBuffer *buffer, MGB_Iterator it, char32_t current_char, char32_t next_char, Typeface::Font *typeface, float scale)
{
#ifndef USE_API
	int tab_width = getCharacterWidth_std('\t', ' ', typeface, 1);

	//@BUG: we do not account for difference in tab width from different font scales.
	//maybe also memoize end of elastic tab as well as width of it?
	BindingIdentifier ident = { 0,&elasticTab };
	void **memoPtr;
	HashTable_Location_int *memo;
	if (!lookup(&buffer->backingBuffer->commonBuffer.bindingMemory, ident, &memoPtr))
	{
		memo = (HashTable_Location_int *)alloc_(sizeof(HashTable_Location_int), "ElasticTab hashtable struct"); //why are we leaking?
		*memo = DHDS_constructHT(Location, int, 50, buffer->allocator);
		insert(&buffer->backingBuffer->commonBuffer.bindingMemory, ident, memo);
	}
	else
	{
		memo = (HashTable_Location_int *)*memoPtr;
	}

	BufferChange event;
	if (next_history_change(buffer->backingBuffer, &elasticTab, &event))
	{
		// if we have one history_change we don't care about the rest, we don't want them next time through
		fast_forward_history_changes(buffer->backingBuffer, &elasticTab);
		clear(memo);
	}

	CharRenderingInfo rend;
	float our_w = calculateIteratorX(buffer, it, &rend);
	float max_w = our_w + rend.scale * tab_width;
	MultiGapBuffer *mgb = buffer->backingBuffer->buffer;
	MGB_Iterator itstart = it;
	int tab_num = 0;
	while (getPrev(mgb, &it))
	{
		char c = *getCharacter(mgb, it);
		if (isLineBreak(c)) { break; }
		if (c == '\v')
		{
			++tab_num;
		}
	}
	assert(tab_num == 0);
	Location loc;
	loc.line = getLineFromIterator(buffer, itstart);
	loc.column = tab_num;
	int *val;

	if (lookup(memo, loc, &val)) {
		return *val;
	}

	for (;;)
	{
		for (;;)
		{
			if (!getPrev(mgb, &it) || isLineBreak(*getCharacter(mgb, it)))break;
		};

		int ctab = 0;
		for (;;)
		{
			if (!(getNext(mgb, &it)) || isLineBreak(*getCharacter(mgb, it))) goto next;
			if (*getCharacter(mgb, it) == '\v')
			{
				if (ctab++ == tab_num)
				{
					// keep tmp on a seperate line to avoid undeffed behaviour, rend can't be modified and used in the same statment
					int tmp = calculateIteratorX(buffer, it, &rend);
					max_w = max(max_w, tmp + rend.scale*tab_width);
					break;
				}
			}
		}
		for (;;)
		{
			if (!getPrev(mgb, &it))goto next;
			if (isLineBreak(*getCharacter(mgb, it)))break;
		};
	}
next:
	it = itstart;
	while (getNext(mgb, &it) && !isLineBreak(*getCharacter(mgb, it)));
	for (;;)
	{
		int ctab = 0;
		for (;;)
		{
			if (!(getNext(mgb, &it)) || isLineBreak(*getCharacter(mgb, it))) goto end;
			if (*getCharacter(mgb, it) == '\v')
			{
				if (ctab++ == tab_num)
				{
					// keep tmp on a seperate line to avoid undeffed behaviour, rend can't be modified and used in the same statment
					int tmp = calculateIteratorX(buffer, it, &rend);
					max_w = max(max_w, tmp + rend.scale*tab_width);
					while (getNext(mgb, &it) && !isLineBreak(*getCharacter(mgb, it)));
					break;
				}
			}
		}
	}
end:
	int ret = max_w - our_w;
	insert(memo, loc, ret);
	return ret;
#else
#endif
}
#endif


#if 0
internal void setActive(Data *data, int index)
{
	if (data->textBuffers.length > index && index >= 0)
	{
		data->activeTextBufferIndex = index;
	}
}
#endif






































#define VK_LEFT 0x25
#define VK_RIGHT 0x27
#define VK_UP 0x26
#define VK_DOWN 0x28


#define VK_DELETE 0x2E
#define VK_BACK 0x08
#define VK_RETURN 0x0D
#define VK_ESCAPE 0x1B


void dir()
{
	char *cmd = "dir";
	void *buffer_handle = open_redirected_terminal(cmd,strlen(cmd));
	view_from_buffer_handle(buffer_handle);
}

#define external extern "C" __declspec(dllexport)

external ColorScheme get_colorScheme()
{
	default_colorScheme.active_color = hsl(0.7f,0.3f,0.3f);
	return default_colorScheme;
}

external void setBindingsLocal(void *view_handle)
{
	//api.callbacks.registerCallBack(callback_pre_render, view_handle, &mark_selection);
	//api.callbacks.registerCallBack(callback_pre_render, view_handle, &mark_bigTitles);

	bind_key_mods(view_handle, VK_LEFT, atMost, mod_control | mod_shift, _moveLeft);
	bind_key_mods(view_handle, VK_RIGHT, atMost, mod_control | mod_shift, _moveRight);
	bind_key_mods(view_handle, VK_BACK, atMost, mod_control, _backSpace);
	bind_key_mods(view_handle, VK_DELETE, atMost, mod_control, _delete);

	bind_key(view_handle, 'C', precisely, mod_control, []() {copy(view_handle_active()); });
	bind_key(view_handle, 'X', precisely, mod_control, []() {cut(view_handle_active()); });
	bind_key(view_handle, 'V', precisely, mod_control, []() {paste(view_handle_active()); });

	if (view_get_type(view_handle) == (int)buffer_mode_defualt)
	{
		bind_key(view_handle, 'D', precisely, mod_control, []() {dir(); });
		bind_key_mods(view_handle, VK_UP, atMost, mod_shift, _moveUp);
		bind_key_mods(view_handle, VK_DOWN, atMost, mod_shift, _moveDown);
		bind_key(view_handle, 'S', precisely, mod_control, []() {save(view_handle_active()); });
		bind_key(view_handle, 'Z', precisely, mod_control, []() {undo(view_handle_active()); });
		bind_key(view_handle, 'Z', precisely, mod_control | mod_shift, []() {redo(view_handle_active()); });
		bind_key(view_handle, 'W', precisely, mod_control, []() {view_close(view_handle_active()); });
		bind_key(view_handle, VK_UP, precisely, mod_control, insertCaretAbove);
		bind_key(view_handle, VK_DOWN, precisely, mod_control, insertCaretBelow);
		bind_key(view_handle, 'C', precisely, mod_alt, []() {view_clone(view_handle_active()); });
		//bind_key(view_handle, VK_PRIOR, precisely, mod_none, _moveUpPage);
		//bind_key(view_handle, VK_NEXT, precisely, mod_none, _moveDownPage);
		bind_key(view_handle, VK_ESCAPE, precisely, mod_none, removeAllButOneCaret);
		bind_key(view_handle, '\t', precisely, mod_control, appendVerticalTab);
	}

	else if (view_get_type(view_handle) == (int)buffer_mode_commandline)
	{
		bind_key(view_handle, VK_RETURN, atLeast, mod_none, commandline_execute_command);
		bind_key(view_handle, VK_DOWN, precisely, mod_none, []() {menu_move_active(1); });
		bind_key(view_handle, VK_UP, precisely, mod_none, []() {menu_move_active(-1); });
		bind_key(view_handle, VK_ESCAPE, precisely, mod_none, hideCommandLine);

		bind_command("openFile", openFileCommand, charDownFileCommand, 0);
		bind_command("o", openFileCommand, 0, 0);
		bind_command("createFile", [](char *start, int length, void **userdata) {open_view(start, length); }, 0, 0);
		bind_command("closeBuffer", [](char *start, int length, void **ud) {view_close(view_handle_active()); }, 0, 0);
		bind_command("c", [](char *start, int length, void **ud) {view_close(view_handle_active()); }, 0, 0);
		//api.callbacks.bindCommand("find", find, find_mark_down,0);
	}

	bind_key(view_handle, 'O', precisely, mod_control, preFeedOpenFile);
	//bind_key(view_handle, 'B', precisely, mod_control, win32_runBuild);
	//bind_key(view_handle, 'N', precisely, mod_control, preFeedCreateFile);
}


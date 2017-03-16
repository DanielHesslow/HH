#include "api.h"
#include "ctype.h"
#include <set>
#include "windows.h"

internal void addDirFilesToMenu(std::string path);

#define USE_API


internal void preFeedCommandLine(char *string) {
	commandline_open();
	commandline_clear();
	commandline_feed(string, strlen(string));
}

void _delete(Mods mods, int direction) {
	void *handle = view_handle_focused();
	bool deleted = false;
	if (mods & mod_control) {
		for (int cursor = 0; cursor < num_cursors(handle); cursor++) {
			if (selection_length(handle, cursor)) {
				delete_selection(handle, cursor);
			}
			else {
				char byte;
				while ((byte = cursor_get_codepoint(handle, cursor, direction)) && isspace(byte) && byte != '\n') {
					cursor_remove_codepoint(handle, cursor, direction);
					deleted = true;
				}
				while ((byte = cursor_get_codepoint(handle, cursor, direction)) && !isspace(byte)) {
					cursor_remove_codepoint(handle, cursor, direction);
					deleted = true;
				}
			}
		}
	}
	if (!deleted) {
		for (int cursor = 0; cursor < num_cursors(handle); cursor++)
			cursor_remove_codepoint(handle, cursor, direction);
	}
	cursors_remove_dups(handle);
}

void _move(Mods mods, int direction) {
	if (mods & mod_alt)return;
	void *handle = view_handle_focused();
	bool moved = false;
	if (mods & mod_control) {
		for (int cursor = 0; cursor < num_cursors(handle); cursor++) {
			char byte;
			while ((byte = cursor_get_codepoint(handle, cursor, direction)) && isspace(byte) && byte != '\n') {
				if (!byte)break;
				cursor_move(handle, direction, cursor, mods & mod_shift, move_mode_codepoint);
				moved = true;
			}
			while ((byte = cursor_get_codepoint(handle, cursor, direction)) && !isspace(byte)) {
				cursor_move(handle, direction, cursor, mods & mod_shift, move_mode_codepoint);
				moved = true;
			}
		}
	}
	if (!moved) {
		for (int cursor = 0; cursor < num_cursors(handle); cursor++)
			cursor_move(handle, direction, cursor, mods & mod_shift, move_mode_grapheme_cluster);
	}
	cursors_remove_dups(handle);
}

void _moveV(Mods mods, int dir) {
	void *handle = view_handle_focused();
	for (int cursor = 0; cursor < num_cursors(handle); cursor++)
		cursor_move(handle, dir, cursor, mods & mod_shift, move_mode_line);
	cursors_remove_dups(handle);
}


void charDownFileCommand(char *s, int s_len, void **ud) {
	menu_clear();
	std::string string(s, s + s_len);
	addDirFilesToMenu(s);
}

void insert_caretV(int dir) {
	ViewHandle view_handle = view_handle_active();
	int _num_cursors = num_cursors(view_handle);
	for (int cursor = 0; cursor < _num_cursors; cursor++) {
		Location loc = location_from_cursor(view_handle, cursor);
		loc.line = max(loc.line + dir, 0);
		cursor_add(view_handle, loc);
	}
	cursors_remove_dups(view_handle);
}

void removeAllButOneCaret() {
	ViewHandle view_handle = view_handle_active();
	int _num_cursors = num_cursors(view_handle);
	for (int cursor = 1; cursor < _num_cursors; cursor++) {
		cursor_remove(view_handle, 1);
	}
	delete_selection(view_handle, 0);
}


std::string directory(std::string path) {
	int index_of_last_slash = path.rfind('/');
	if (index_of_last_slash == -1)return "";
	else return path.substr(0, index_of_last_slash);
}


void num_views_change() {
#if 0
	Layout *leaf = (Layout *)0;
	static Layout *two_x = CREATE_LAYOUT(layout_type_x, 1, leaf, .5f, leaf);
	static Layout *three_x = CREATE_LAYOUT(layout_type_x, 1, leaf, .33f, leaf, .67f, leaf);

	// becuase we need have a unique memory location :/
	// this part of the api is contra-intuitive and this must in the very least be clearly described...
	static Layout *two_x_copy = CREATE_LAYOUT(layout_type_x, 1, leaf, .5f, leaf);
	static Layout *two_by_two = CREATE_LAYOUT(layout_type_y, 2, two_x, .5f, two_x_copy);

	switch (get_num_views()) {
	case 1: set_layout(leaf); break;
	case 2: set_layout(two_x); break;
	case 3: set_layout(three_x); break;
	case 4: set_layout(two_by_two); break;
	}
#endif
}

internal void openFileCommand(char *start, int length, void **user_data) {
	API_MenuItem item;
	bool has_active_menu = menu_get_active(&item);
	if (has_active_menu) {
		if (open_view(item.name, item.name_length)) {
			commandline_close();
		}
		else {
			commandline_clear();
			commandline_feed("openFile ", strlen("openFile "));
			std::string path(start, start + length);
			std::string dir = directory(path);
			std::string item(item.name, item.name_length);
			std::string to_feed = dir + item + "/";
			commandline_feed((char *)to_feed.c_str(), to_feed.length());
			charDownFileCommand((char *)to_feed.c_str(), to_feed.length(), user_data);
		}
	}
	else {
		open_view(start, length);
		commandline_close();
	}
	num_views_change();
}

#if 0
void find_mark_down(char *text, int text_length) {
	void *view_handle = view_handle_active()

		api.markup.remove(view_handle, &find_mark_down);
	if (text_length == 0) return;

	BufferHandle buffer_handle = api.handles.getBufferHandleFromViewHandle(view_handle);
	TextIterator it = api.text_iterator.make(buffer_handle);
	Location match_location;
	int i = 0;
	do {
		char b = api.text_iterator.get_byte(it, 1);
		if (b == text[i]) {
			if (i == 0) {
				match_location = api.Location.from_iterator(buffer_handle, it);
			}

			if (++i == text_length) {
				TextIterator it_cpy = it;
				api.text_iterator.move(&it_cpy, 1, move_mode_codepoint, false);
				api.markup.highlight_color(view_handle, &find_mark_down, match_location, api.Location.from_iterator(buffer_handle, it_cpy), default_colorScheme.active_color);
				i = 0;
			}
		}
		else {
			i = 0;
		}
	} while (api.text_iterator.move(&it, 1, move_mode_byte, false));


}

void find(char *start, int length) {
	markup_remove(buffer_handle_active(), find_mark_down);
}
#endif 


internal void addDirFilesToMenu(std::string path) {
	WIN32_FIND_DATA ffd;
	std::string search_path;
	size_t index_last_slash = path.rfind('\\');
	search_path = "./" + path + "*";

	HANDLE handle = FindFirstFileA(search_path.c_str(), &ffd);
	if (handle != INVALID_HANDLE_VALUE) {
		do {
			API_MenuItem newItem = {};
			newItem.name = ffd.cFileName;
			newItem.name_length = strlen(newItem.name);
			std::string p = ffd.cFileName;
			std::string extension = p.substr(p.rfind('.') + 1, newItem.name_length);
			bool okExtension = extension == "txt" || extension == "c" || extension == "cpp" || extension == "bat" || extension == "h";

			if (okExtension || ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
				menu_add(newItem);
			}
		} while (FindNextFileA(handle, &ffd) != 0);  //we trigger some breakpoint in here. what's t he problem?
		FindClose(handle);
	}
}




internal void appendVerticalTab() {
	ViewHandle handle = view_handle_active();
	for (int cursor = 0; cursor < num_cursors(handle); cursor++)
		append_byte(handle, cursor, -1, '\v');
}

std::set<int> big_titles;
internal bool is_big_title(void *buffer_handle, int line) {

	TextIterator it = text_iterator_from_location(buffer_handle, { line,0 });
	int ack = 0;
	do {
		char b = text_iterator_get_byte(it, 1);

		if (b == '\n') break;
		else if (b == '-') { if (++ack >= 3) return true; }
		else ack = 0;
	} while (text_iterator_move(&it, 1, move_mode_byte, false));
	return false;
}



internal void mark_bigTitles(void *view_handle) {
	void *buffer_handle = buffer_handle_from_view_handle(view_handle);
	{
		void *view_handle;
		ViewIterator vi = view_iterator_from_buffer_handle(buffer_handle);
		while (view_iterator_next(&vi, &view_handle)) {
			float title_size = markup_get_initial_rendering_state(view_handle).scale*1.5f;
			markup_remove(view_handle, &mark_bigTitles);


			for (int i = 0; i < num_lines(buffer_handle); i++) {
				int line = i;
				if (is_big_title(buffer_handle, line)) {
					markup_scale(view_handle, &mark_bigTitles, { line, 0 }, { line + 1,0 }, title_size);
					markup_text_color(view_handle, &mark_bigTitles, { line, 0 }, { line + 1,0 }, default_colorScheme.active_color);
				}
			}
		}
	}
}


#if 0
internal int elasticTab(TextBuffer *buffer, MGB_Iterator it, char32_t current_char, char32_t next_char, Typeface::Font *typeface, float scale) {
#ifndef USE_API
	int tab_width = getCharacterWidth_std('\t', ' ', typeface, 1);

	//@BUG: we do not account for difference in tab width from different font scales.
	//maybe also memoize end of elastic tab as well as width of it?
	BindingIdentifier ident = { 0,&elasticTab };
	void **memoPtr;
	HashTable_Location_int *memo;
	if (!lookup(&buffer->backingBuffer->commonBuffer.bindingMemory, ident, &memoPtr)) {
		memo = (HashTable_Location_int *)alloc_(sizeof(HashTable_Location_int), "ElasticTab hashtable struct"); //why are we leaking?
		*memo = DHDS_constructHT(Location, int, 50, buffer->allocator);
		insert(&buffer->backingBuffer->commonBuffer.bindingMemory, ident, memo);
	}
	else {
		memo = (HashTable_Location_int *)*memoPtr;
	}

	BufferChange event;
	if (next_history_change(buffer->backingBuffer, &elasticTab, &event)) {
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
	while (getPrev(mgb, &it)) {
		char c = *getCharacter(mgb, it);
		if (isLineBreak(c)) { break; }
		if (c == '\v') {
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

	for (;;) {
		for (;;) {
			if (!getPrev(mgb, &it) || isLineBreak(*getCharacter(mgb, it)))break;
		};

		int ctab = 0;
		for (;;) {
			if (!(getNext(mgb, &it)) || isLineBreak(*getCharacter(mgb, it))) goto next;
			if (*getCharacter(mgb, it) == '\v') {
				if (ctab++ == tab_num) {
					// keep tmp on a seperate line to avoid undeffed behaviour, rend can't be modified and used in the same statment
					int tmp = calculateIteratorX(buffer, it, &rend);
					max_w = max(max_w, tmp + rend.scale*tab_width);
					break;
				}
			}
		}
		for (;;) {
			if (!getPrev(mgb, &it))goto next;
			if (isLineBreak(*getCharacter(mgb, it)))break;
		};
	}
next:
	it = itstart;
	while (getNext(mgb, &it) && !isLineBreak(*getCharacter(mgb, it)));
	for (;;) {
		int ctab = 0;
		for (;;) {
			if (!(getNext(mgb, &it)) || isLineBreak(*getCharacter(mgb, it))) goto end;
			if (*getCharacter(mgb, it) == '\v') {
				if (ctab++ == tab_num) {
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


#define VK_LEFT 0x25
#define VK_RIGHT 0x27
#define VK_UP 0x26
#define VK_DOWN 0x28

#define VK_DELETE 0x2E
#define VK_BACK 0x08
#define VK_RETURN 0x0D
#define VK_ESCAPE 0x1B


void dir() {
	char *cmd = "dir";
	void *buffer_handle = open_redirected_terminal(cmd, strlen(cmd));
	view_from_buffer_handle(buffer_handle);
	num_views_change();
}

void build() {
	char *cmd = "cd ..\\HH\\actual_code && echo hello && bbuild";
	void *buffer_handle = open_redirected_terminal(cmd, strlen(cmd));
	view_from_buffer_handle(buffer_handle);
	num_views_change();
}


#define external extern "C" __declspec(dllexport)

external ColorScheme get_colorScheme() {
	default_colorScheme.active_color = hsl(0.7f, 0.3f, 0.3f);
	return default_colorScheme;
}

external void bindings(char VK_CODE, Mods mods, char *utf8, int utf8_len) {
	void *view = view_handle_focused();
	int buffer_type = view_get_type(view);

	mods = (Mods)(mods & 0b111); // strip out the info about left/right 

	//a merge for mods and VK so we can swith on both at the same time. 
	//(the define is not neccessary but it removes some noise in the code)
#define M(mods,vk) (mods)|((vk)<<8)
	int type = M(mods, VK_CODE);

	//this function handle its cases with mods etc internally, therefore we don't bother with putting them into the switch
	if ((VK_CODE == VK_LEFT) || (VK_CODE == VK_RIGHT)) _move(mods, VK_CODE == VK_RIGHT ? 1 : -1);

	switch (type) {
	case M(mod_control, 'C'): copy(view);  break;
	case M(mod_control, 'X'): cut(view); cursors_remove_dups(view); break;
	case M(mod_control, 'V'): paste(view); break;
	case M(mod_control, 'O'): preFeedCommandLine("openFile "); break;
	case M(mod_control, 'B'): build(); break;

	case M(mod_control, VK_BACK):
	case M(mod_none, VK_BACK):
		_delete(mods, -1); break;
	case M(mod_control, VK_DELETE):
	case M(mod_none, VK_DELETE):
		_delete(mods, 1); break;
	}


	if (buffer_type == buffer_mode_default) {
		switch (type) {
			// note move layout moves to the last thing we were at. We should also provide closest match thing. grids feels wierd.
		case M(mod_alt, VK_LEFT): move_layout(-1, layout_type_x);					break;
		case M(mod_alt, VK_RIGHT): move_layout(1, layout_type_x);					break;
		case M(mod_alt, VK_UP): move_layout(-1, layout_type_y);						break;
		case M(mod_alt, VK_DOWN): move_layout(1, layout_type_y);					break;
		case M(mod_control, 'D'): dir();					break;
		case M(mod_control, 'S'): save(view);				break;
		case M(mod_control, 'W'): view_close(view);	num_views_change();	break;
		case M(mod_alt, 'C'): view_clone(view);			break;
		case M(mod_control, 'L'): history_next_leaf(view);	break;
		case M(mod_control, 'Z'): history_insert_waypoint(view); undo(view); history_insert_waypoint(view);				break;
		case M(mod_control | mod_shift, 'Z'): redo(view);		break;
		case M(mod_none, VK_ESCAPE): removeAllButOneCaret();		break;

		}

		if (VK_CODE == VK_UP || VK_CODE == VK_DOWN) {
			int dir = VK_CODE == VK_UP ? -1 : 1;
			if (mods == mod_shift || mods == mod_none) _moveV(mods, dir);
			if (mods == mod_control) insert_caretV(dir);
		}
	}
	else if (buffer_type == buffer_mode_commandline) {
		switch (type) {
		case M(mod_none, VK_RETURN): commandline_execute_command(); break;
		case M(mod_none, VK_DOWN):   menu_move_active(1);   break;
		case M(mod_none, VK_UP):     menu_move_active(-1);  break;
		case M(mod_none, VK_ESCAPE): commandline_close();     break;
		}
	}

	//here we can intercept stuff If we like to. eg. change '.' to '$' if we were to code in something like php. 
	if (utf8 != 0) {
		for (int i = 0; i < utf8_len; i++)
			for (int j = 0; j < num_cursors(view_handle_focused()); j++)
				append_byte(view, j, -1, utf8[i]);
		if (utf8[0] == ' ' || utf8[0] == '\n')
			history_insert_waypoint(view);
	}
	mark_bigTitles(view);
#undef M
}

external void setBindingsLocal(void *view_handle) {
	if (view_get_type(view_handle) == (int)buffer_mode_commandline) {
		bind_command("openFile", openFileCommand, charDownFileCommand, 0);
		bind_command("createFile", [](char *start, int length, void **userdata) {open_view(start, length); num_views_change(); }, 0, 0);
	}
}


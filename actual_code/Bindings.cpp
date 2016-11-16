#include "api.h"
//#include <string>


//#include "DH_MacroAbuse.h"
//#include "windows.h"
//#define alloc_(a,x)malloc(a)
//#define free_(a)free(a)
//#include "strings.c"

//internal void addDirFilesToMenu(Data *data, DHSTR_String string, DHSTR_String  *name); //more platform, must be refactored to win32 
API api;
#define USE_API
internal void showCommandLine()
{
	api.commandLine.open();
}

internal void hideCommandLine()
{
	api.commandLine.close();
}

internal void preFeedCommandLine(char *string, int length)
{
	api.commandLine.open();
	api.commandLine.clear();
	api.commandLine.feed(string, length);
}

// ---- BACKEND API 

void memory_for_last_event() {}

internal void preFeedOpenFile()
{
	char buffer[] = u8"openFile ";
	preFeedCommandLine(buffer, (sizeof(buffer) / sizeof(buffer[0]) - 1));
}
#include "ctype.h"


void _delete(Mods mods, int direction)
{
	void *handle = api.handles.getFocusedViewHandle();
	bool deleted = false;
	if (mods & mod_control)
	{
		FOR_CURSORS(cursor, api, handle)
		{
			if (api.cursor.selection_length(handle, cursor))
			{
				api.cursor.delete_selection(handle, cursor);
			}
			else
			{
				char byte;
				while ((byte = api.cursor.get_codepoint(handle, cursor, direction)) && isspace(byte) && byte != '\n')
				{
					api.cursor.remove_codepoint(handle, cursor, direction);
					deleted = true;
				}
				while ((byte = api.cursor.get_codepoint(handle, cursor, direction)) && !isspace(byte))
				{
					api.cursor.remove_codepoint(handle, cursor, direction);
					deleted = true;
				}
			}
		}
	}
	if (!deleted)
	{
		FOR_CURSORS(cursor, api, handle)
			api.cursor.remove_codepoint(handle, cursor, direction);
	}
}

void _move(Mods mods, int direction)
{
	void *handle = api.handles.getFocusedViewHandle();
	bool moved = false;
	if (mods & mod_control)
	{
		FOR_CURSORS(cursor, api, handle)
		{
			char byte;
			while ((byte = api.cursor.get_codepoint(handle, cursor, direction)) && isspace(byte) && byte != '\n')
			{
				api.cursor.move(handle, direction, cursor, mods & mod_shift, move_mode_codepoint);
				moved = true;
			}
			while ((byte = api.cursor.get_codepoint(handle, cursor, direction)) && !isspace(byte))
			{
				api.cursor.move(handle, direction, cursor, mods & mod_shift, move_mode_codepoint);
				moved = true;
			}
		}
	}
	if (!moved)
	{
		FOR_CURSORS(cursor, api, handle)
			api.cursor.move(handle, direction, cursor, mods & mod_shift, move_mode_grapheme_cluster);
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
	void *handle = api.handles.getFocusedViewHandle();
	api.cursor.move(handle, -1, ALL_CURSORS, mods & mod_shift, move_mode_line);
}

void _moveDown(Mods mods)
{
	void *handle = api.handles.getFocusedViewHandle();
	api.cursor.move(handle, 1, ALL_CURSORS, mods & mod_shift, move_mode_line);
}

void _backSpace(Mods mods)
{
	_delete(mods, -1);
}


void _delete(Mods mods)
{
	_delete(mods, 1);
}

#if 0
internal int levDist(DHSTR_String string, DHSTR_String alternative)
{
	//callibrate and handle offsets (ie.) abcd should closer to bcd than it is to ape
	const int DIFF = 10;
	const int LEN = 1;

	int dist = 0;
	for (int i = 0; i < min(string.length, alternative.length); i++)
	{
		if (string.start[i] != alternative.start[i])
		{
			dist += DIFF;
		}
	}
	dist += max(string.length - alternative.length, 0)*LEN;
	return dist;
}
#endif

//bellow folllows a quick and dirty way of sorting which involves setting up a global variable
//its certainly not pretty but should work fine.
//static char *stringToCompareAgainst = (char *)0;
/*
internal int cmp(const void *va, const void *vb)
{
	CommandInfo *a = (CommandInfo*)va;
	CommandInfo *b = (CommandInfo*)vb;

	return levDist(stringToCompareAgainst, a->name) - levDist(stringToCompareAgainst, b->name);
}
*/
//---menus
#if 0
internal int cmpMenuItem(const void *a, const void *b)
{
	return ((MenuItem *)a)->ordering - ((MenuItem *)b)->ordering;
}

internal void sortMenu(Data *data)
{
	qsort(data->menu.start, data->menu.length, sizeof(MenuItem), cmpMenuItem);
}

internal void setOrderMenu(Data *data, DHSTR_String string)
{
	for (int i = 0; i < data->menu.length; i++)
	{
		data->menu.start[i].ordering = levDist(string, data->menu.start[i].name);
	}
}

internal void maxOrderSortedMenu(Data *data, int max)
{
	int newLen = -1;
	for (int i = 0; i<data->menu.length; i++)
	{
		if (newLen == -1)
		{
			if (data->menu.start[i].ordering>max)
			{
				newLen = i;
				free_(data->menu.start[i].name.start);
			}
		}
		else
		{
			free_(data->menu.start[i].name.start);
		}
	}
	if (newLen != -1)
	{
		data->menu.length = newLen;
	}
}
#endif
#if 0
//---string man
internal bool hasOkExtension(DHSTR_String file_name)
{
	bool hasDot;
	int index = DHSTR_index_of(file_name, '.', DHSTR_INDEX_OF_LAST, &hasDot);

	if (!hasDot) return false;
	DHSTR_String extension = DHSTR_subString(file_name, index, file_name.length);

	DHSTR_String extensions[] =
	{
		DHSTR_MAKE_STRING(".h"),
		DHSTR_MAKE_STRING(".c"),
		DHSTR_MAKE_STRING(".cpp"),
		DHSTR_MAKE_STRING(".hpp"),
		DHSTR_MAKE_STRING(".txt"),
		DHSTR_MAKE_STRING(".bat"),
		DHSTR_MAKE_STRING(".md"),
	};

	for (int i = 0; i < ARRAY_LENGTH(extensions); i++)
	{
		if (DHSTR_eq(extensions[i], extension, string_eq_length_matter)) return true;
	}
	return false;
}

#endif
#if 0
internal void charDownFileCommand(Data *data, DHSTR_String restOfTheBuffer)
{
#if 0
	freeMenuItems(data);
	DHSTR_String file;
	addDirFilesToMenu(data, restOfTheBuffer, &file);
	setOrderMenu(data, file);
	sortMenu(data);
	maxOrderSortedMenu(data, 10); //calibrate this
#endif
}
#endif

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
#if 0
internal void insertCaretAbove(TextBuffer *buffer)
{
	insertCaret(buffer, above);
}

internal void insertCaretBelow(TextBuffer *buffer)
{
	insertCaret(buffer, below);
}
#endif

internal void openFileCommand(char *start, int length, void **user_data)
{
	api.view.createFromFile(start, length);
	api.commandLine.close();
}


void find_mark_down(char *text, int text_length)
{
	void *view_handle = api.handles.getActiveViewHandle();

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
	api.markup.remove(api.handles.getActiveBufferHandle(), find_mark_down);
}

/*
void func()
{
	BufferHandle handle = api.handles.getActiveBufferHandle();
	//FORALLCURSORS(cursor_index)
	{
		TextIterator it = api.text_iterator.make_from_cursor(handle,cursor);
		char *string=0;
		bool last_was_letter=false;
		while (api.text_iterator.nextCodepoint(it, &string, false))
		{
			if (last_was_letter && !is_letter(string)){break;}
			string = (char *)0; //clear out the string
		}
	}
}
*/
#if 0

bool splitPath(DHSTR_String path, DHSTR_String *_out_directory, DHSTR_String *_out_file)
{
	bool hashSlash;
	int index = DHSTR_index_of(path, '/', DHSTR_INDEX_OF_LAST, &hashSlash);
	if (hashSlash)
	{
		if (_out_directory)*_out_directory = DHSTR_subString(path, 0, index);
		if (_out_file)*_out_file = index >= path.length - 1 ? DHSTR_MAKE_STRING("") : DHSTR_subString(path, index + 1, path.length - 1);
	}
	else
	{
		if (_out_directory)*_out_directory = path;
		if (_out_file)*_out_file = {};
	}
	return hashSlash;
}

internal void addDirFilesToMenu(Data *data, DHSTR_String path, DHSTR_String *_out_file) //more platform, must be refactored to win32 
{
	WIN32_FIND_DATAW ffd;
	DHSTR_String directory;
	bool hasDirectory = splitPath(path, &directory, _out_file);
	DHSTR_String searchPath;
	if (hasDirectory)
	{
		searchPath = DHSTR_MERGE_MULTI(alloca, DHSTR_MAKE_STRING("./"), directory, DHSTR_MAKE_STRING("/*"));
	}
	else
	{
		*_out_file = path;
		searchPath = DHSTR_MERGE_MULTI(alloca, DHSTR_MAKE_STRING("."), DHSTR_MAKE_STRING("/*"));
	}
	HANDLE handle = FindFirstFileW(DHSTR_WCHART_FROM_STRING(searchPath, alloca), &ffd);
	if (handle != INVALID_HANDLE_VALUE)
	{
		do
		{
			DH_Allocator macro_used_allocator = default_allocator;
			DHSTR_String p = DHSTR_STRING_FROM_WCHART(ALLOCATE, ffd.cFileName);
			MenuItem newItem = {};
			newItem.name = p;
			if (hasOkExtension(p))
			{
#if 0
				MenuData *menuData = (MenuData *)alloc_(sizeof(menuData), "menu data");
				menuData->isFile = true;
				newItem.data = menuData;
				Add(&data->menu, newItem);
#endif
			}
			else if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{
#if 0
				MenuData *menuData = (MenuData *)alloc_(sizeof(menuData), "menu data");
				menuData->isFile = false;
				newItem.data = menuData;
				Add(&data->menu, newItem);
#endif
			}
			else {
				free_(p.start);
			}
		} while (FindNextFileW(handle, &ffd) != 0);  //we trigger some breakpoint in here. what's the problem?
		FindClose(handle);
	}
}
#endif


#if 0
void _cloneBuffer(Data *data)
{
	api.view.clone_view(api.handles.getActiveViewHandle());
}
#endif

//#include "DH_DataStructures.h"

internal void appendVerticalTab()
{
	api.cursor.append_codepoint(api.handles.getActiveViewHandle(), ALL_CURSORS, -1, '\v');
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

#include <set>

std::set<int> big_titles;

internal bool is_big_title(void *buffer_handle, int line)
{

	TextIterator it = api.text_iterator.make_from_location(buffer_handle, { line,0 });
	int ack = 0;
	do {
		char b = api.text_iterator.get_byte(it,1);

		if (b == '\n') break;
		else if (b == '-') { if (++ack >= 3) return true; }
		else ack = 0;
	} while (api.text_iterator.move(&it, 1, move_mode_byte, false));
	return false;
}

internal void init_big_titles(void *buffer_handle, std::set<int> *big_titles)
{
	big_titles->clear();
	for (int i = 0; i < api.buffer.numLines(buffer_handle); i++)
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

	void *buffer_handle = api.handles.getBufferHandleFromViewHandle(view_handle);
	std::set<int> *big_titles;

	void **mem = api.memory.getFunctionInfo(buffer_handle, &mark_bigTitles);
	if (*mem)
	{
		big_titles = (std::set<int> *)*mem;
	}
	else
	{
		big_titles = (std::set<int> *)api.memory.allocateMemory(buffer_handle, sizeof(std::set<int>));
		new (big_titles)std::set<int>();
		changed = true;
		init_big_titles(buffer_handle, big_titles);
		*mem = big_titles;
	}

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

	if (changed)
	{
		void *view_handle;
		ViewIterator vi = api.buffer.view_iterator(buffer_handle);
		while (api.buffer.next(&vi, &view_handle))
		{
			float title_size = api.markup.get_initial_rendering_state(view_handle).scale*1.5f;
			api.markup.remove(view_handle, &mark_bigTitles);

			
			for (auto it = big_titles->begin(); it != big_titles->end(); ++it)
			{
				int line = *it;
				api.markup.scale(view_handle, &mark_bigTitles, { line, 0 }, { line + 1,0 }, title_size);
				api.markup.text_color(view_handle, &mark_bigTitles, { line, 0 }, { line + 1,0 }, default_colorScheme.active_color);
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
	void *buffer_handle = api.misc.openRedirectedCommandPrompt(cmd,strlen(cmd));
	api.view.createFromBufferHandle(buffer_handle);
}


void moveDownMenu()
{
	
}


//#include "windows.h"
extern "C"
{
	__declspec(dllexport) ColorScheme get_colorScheme()
	{
		default_colorScheme.active_color = hsl(0.7f,0.3f,0.3f);
		return default_colorScheme;
	}

	__declspec(dllexport) void setBindingsLocal(API _api, void *view_handle)
	{
		api = _api;
		//api.callbacks.registerCallBack(callback_pre_render, view_handle, &mark_selection);
		api.callbacks.registerCallBack(callback_pre_render, view_handle, &mark_bigTitles);

		api.callbacks.bindKey_mods(view_handle, VK_LEFT, atMost, mod_control | mod_shift, _moveLeft);
		api.callbacks.bindKey_mods(view_handle, VK_RIGHT, atMost, mod_control | mod_shift, _moveRight);
		api.callbacks.bindKey_mods(view_handle, VK_BACK, atMost, mod_control, _backSpace);
		api.callbacks.bindKey_mods(view_handle, VK_DELETE, atMost, mod_control, _delete);

		api.callbacks.bindKey(view_handle, 'C', precisely, mod_control, []() {api.clipboard.copy(api.handles.getActiveViewHandle()); });
		api.callbacks.bindKey(view_handle, 'X', precisely, mod_control, []() {api.clipboard.cut(api.handles.getActiveViewHandle()); });
		api.callbacks.bindKey(view_handle, 'V', precisely, mod_control, []() {api.clipboard.paste(api.handles.getActiveViewHandle()); });

		if (api.view.get_type(view_handle) == (int)buffer_mode_defualt)
		{
			api.callbacks.bindKey(view_handle, 'D', precisely, mod_control, []() {dir(); });
			api.callbacks.bindKey_mods(view_handle, VK_UP, atMost, mod_shift, _moveUp);
			api.callbacks.bindKey_mods(view_handle, VK_DOWN, atMost, mod_shift, _moveDown);
			api.callbacks.bindKey(view_handle, 'S', precisely, mod_control, []() {api.buffer.save(api.handles.getActiveViewHandle()); });
			//api.callbacks.bindKey(view_handle, 'Z', precisely, mod_control, undoMany);
			//api.callbacks.bindKey(view_handle, 'Z', precisely, mod_control | mod_shift, redoMany);
			api.callbacks.bindKey(view_handle, 'W', precisely, mod_control, []() {api.view.close(api.handles.getActiveViewHandle()); });
			//api.callbacks.bindKey(view_handle, VK_UP, precisely, mod_control, insertCaretAbove);
			//api.callbacks.bindKey(view_handle, VK_DOWN, precisely, mod_control, insertCaretBelow);
			api.callbacks.bindKey(view_handle, 'C', precisely, mod_alt, []() {api.view.clone_view(api.handles.getActiveViewHandle()); });
			//api.callbacks.bindKey(view_handle, VK_PRIOR, precisely, mod_none, _moveUpPage);
			//api.callbacks.bindKey(view_handle, VK_NEXT, precisely, mod_none, _moveDownPage);
			//api.callbacks.bindKey(view_handle, VK_ESCAPE, precisely, mod_none, removeAllButOneCaret);
			api.callbacks.bindKey(view_handle, '\t', precisely, mod_control, appendVerticalTab);
		}

		else if (api.view.get_type(view_handle) == (int)buffer_mode_commandline)
		{
			api.callbacks.bindKey(view_handle, VK_RETURN, atLeast, mod_none, api.commandLine.executeCommand);
			api.callbacks.bindKey(view_handle, VK_DOWN, precisely, mod_none, []() {api.misc.move_active_menu(1)});
			api.callbacks.bindKey(view_handle, VK_UP, precisely, mod_none, []() {api.misc.move_active_menu(-1)});
			api.callbacks.bindKey(view_handle, VK_ESCAPE, precisely, mod_none, hideCommandLine);

			api.callbacks.bindCommand("openFile", openFileCommand, 0, 0);
			api.callbacks.bindCommand("o", openFileCommand, 0, 0);
			api.callbacks.bindCommand("createFile", [](char *start, int length, void **userdata) {api.view.createFromFile(start, length); }, 0, 0);
			api.callbacks.bindCommand("closeBuffer", [](char *start, int length, void **ud) {api.view.close(api.handles.getActiveViewHandle()); }, 0, 0);
			api.callbacks.bindCommand("c", [](char *start, int length, void **ud) {api.view.close(api.handles.getActiveViewHandle()); }, 0, 0);
			//api.callbacks.bindCommand("find", find, find_mark_down,0);
		}

		api.callbacks.bindKey(view_handle, 'O', precisely, mod_control, preFeedOpenFile);
		//api.callbacks.bindKey(view_handle, 'B', precisely, mod_control, win32_runBuild);
		//api.callbacks.bindKey(view_handle, 'N', precisely, mod_control, preFeedCreateFile);
	}
}


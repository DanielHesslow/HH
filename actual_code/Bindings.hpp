#include "header.h"
internal void addDirFilesToMenu(Data *data, DHSTR_String string, DHSTR_String  *name); //more platform, must be refactored to win32 

	/* //how the api for the msging should work.
	while (Message msg = peek_msg(&textBuffer->backingBuffer, &elasticTab))
	{
	switch msg.type{
	case append:
	case remove:
	if (msg.line < currentLine) memo.length = 0;
	break;
	}
	}
	mark_all_msgs_read(&textBuffer->backingBuffer, &elasticTab);
	*/

internal void showCommandLine(Data *data)
{
	data->isCommandlineActive = true;
}

internal void hideCommandLine(Data *data)
{
	data->isCommandlineActive = false;
}

internal void feedCommandLine(Data *data, DHSTR_String string)
{
	for (int i = 0; i < string.length; i++)
	{
		appendCharacter(data->commandLine,string.start[i],0,true);
	}
}

internal void preFeedCommandLine(Data *data, char *string, int length)
{
	showCommandLine(data);
	clearBuffer(data->commandLine->backingBuffer->buffer);
	feedCommandLine(data, DHSTR_MAKE_STRING(string));
}

void memory_for_last_event() {}

internal void preFeedOpenFile(Data *data)
{
	char buffer[] = u8"openFile ";
	preFeedCommandLine(data, buffer, sizeof(buffer) / sizeof(buffer[0]));
}

internal void freeMenuItems(Data *data)
{
	for (int i = 0; i < data->menu.length; i++)
	{
		free_(data->menu.start[i].name.start);
		free_(data->menu.start[i].data);
	}
	data->menu.length = 0;
}

struct CommandInfo
{
	char *name;
	int hashedName;
	void(*func)(Data *data, DHSTR_String restOfTheBuffer);
	void(*charDown)(Data *data, DHSTR_String restOfTheBuffer);
};

//---textBuffer events

enum BufferEventType
{
	buffer_event_remove,
	buffer_event_append,
};
struct BufferEvent
{
	BufferEventType type;
	int line;
	int column;

};

struct MenuData
{
	bool isFile;
};
DEFINE_DynamicArray(CommandInfo)
DynamicArray_CommandInfo commands = constructDynamicArray_CommandInfo(100,"commandBindings");

internal CommandInfo findMatchingCommand(MultiGapBuffer *buffer, DynamicArray_CommandInfo commands, bool *success)
{
	CommandInfo allocationInfo = {};
	MGB_Iterator it = getIterator(buffer);
	
	int ignore;
	
	int hash = getNextTokenHash(buffer,&it,&ignore);
	for(int i = 0; i<commands.length;i++)
	{
		if(commands.start[i].hashedName == hash)//make sure the string is the same as well!
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
	CommandInfo command = findMatchingCommand(buffer,commands,&success);
	if (!success)
	{
		freeMenuItems(data);
		return;
	}
	
	MGB_Iterator rest = getIterator(buffer,DHSTR_strlen(command.name));

	int ignore;
	stripInitialWhite(buffer, &rest, &ignore);
	char *remainingString = getRestAsString(buffer,rest);
	if(command.charDown)
		command.charDown(data, DHSTR_MAKE_STRING(remainingString));
	free_(remainingString);
}

internal void executeCommand(Data *data)
{
	data->eatNext = true;

	MultiGapBuffer *buffer = data->commandLine->backingBuffer->buffer;

	bool success;
	CommandInfo command = findMatchingCommand(buffer,commands,&success);
	if (!success)
	{
		freeMenuItems(data);
		return;
	}
	
	MGB_Iterator rest = getIterator(buffer,DHSTR_strlen(command.name));

	int ignore;
	stripInitialWhite(buffer, &rest, &ignore);
	char *remainingString = getRestAsString(buffer,rest);
	if(command.func)
		command.func(data, DHSTR_MAKE_STRING(remainingString));

	free_(remainingString);
	data->updateAllBuffers = true;
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
	}else assert(false);
	return false;
}

global_variable DynamicArray_KeyBinding *bindings = (DynamicArray_KeyBinding *)0;

KeyBinding bindKeyBase(char VK_Code, ModMode modMode, Mods mods, void *func)
{
	KeyBinding binding = {};
	binding.VK_Code = VK_Code;
	binding.mods = mods;
	binding.funcP= func;
	binding.modMode = modMode;
	return binding;
}

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

internal void executeBindingFunction(KeyBinding binding, TextBuffer *buffer,Data *data, Mods currentMods)
{
	if (binding.funcType == Arg_TextBuffer_Mods)
	{
		binding.func_TextBuffer_Mods(buffer, currentMods);
	}
	else if(binding.funcType == Arg_TextBuffer)
	{
		binding.func_TextBuffer(buffer);
	}
	else if (binding.funcType == Arg_Data)
	{
		binding.func_Data(data);
	}
	else assert(false);
}

void _moveLeft(TextBuffer *textBuffer, Mods mods)
{
	if (mods & mod_control)
	{
		moveWhile(textBuffer, mods & mod_shift ? do_select:no_select, do_log, true, dir_left, &whileWord);
	}
	else
	{
		move(textBuffer, dir_left, do_log, mods & mod_shift ? do_select:no_select);
	}
	checkIdsExist(textBuffer);
}

void _moveRight(TextBuffer *textBuffer, Mods mods)
{
	if (mods & mod_control)
	{
		moveWhile(textBuffer, mods & mod_shift ? do_select : no_select, do_log, true, dir_right, &whileWord);
	}
	else
	{
		move(textBuffer, dir_right, do_log, mods & mod_shift?do_select:no_select);
	}
	checkIdsExist(textBuffer);
}

void _moveUp(TextBuffer *textBuffer, Mods mods)
{
	for (int i = 0; i < textBuffer->ownedCarets_id.length; i++)
	{
		moveV_nc(textBuffer, mods & mod_shift ? do_select: no_select, i,true);
	}
	removeOwnedEmptyCarets(textBuffer);
	checkIdsExist(textBuffer);
}

void _moveDown(TextBuffer *textBuffer, Mods mods)
{
	for (int i = 0; i < textBuffer->ownedCarets_id.length; i++)
	{
		moveV_nc(textBuffer, mods & mod_shift? do_select :no_select, i,false);
	}
	removeOwnedEmptyCarets(textBuffer);
	checkIdsExist(textBuffer);
}
 
void _backSpace(TextBuffer *textBuffer,Mods mods)
{
	if (validSelection(textBuffer))
	{
		removeSelection(textBuffer);
	}
	else
	{
		if (mods & mod_control)
			removeWhile(textBuffer, true, dir_left, &whileWord);
		else
			removeCharacter(textBuffer,true);
	}
}

void _delete(TextBuffer *textBuffer, Mods mods)
{
	if (validSelection(textBuffer))
	{
		removeSelection(textBuffer);
	}
	else
	{
		if (mods & mod_control)
			removeWhile(textBuffer, true, dir_right, &whileWord);
		else
			deleteCharacter(textBuffer, true);
	}
}

internal void toggle(TextBuffer *buffer, BufferType type)
{
	if (buffer->bufferType == regularBuffer)
	{
		buffer->bufferType = type;
	}
	else
	{
		buffer->bufferType = regularBuffer;
	}

}
/*
internal int levDist(char *string, char *alternative)
{
	//callibrate and handle offsets (ie.) abcd should closer to bcd than it is to ape
	const int DIFF = 10;
	const int LEN = 1;

	int dist = 0;
	while (*string && *alternative)
	{
		if (*string != *alternative)
		{
			dist += DIFF;
		}
		++string;
		++alternative;
	}
	dist += max(DHSTR_strlen(string) - DHSTR_strlen(alternative), 0)*LEN;
	return dist;
	}
*/
/*
*/
internal int levDist(DHSTR_String string, DHSTR_String alternative)
{
	//callibrate and handle offsets (ie.) abcd should closer to bcd than it is to ape
	const int DIFF = 10;
	const int LEN = 1;

	int dist = 0;
	for (int i = 0; i < min(string.length, alternative.length);i++)
	{
		if (string.start[i] != alternative.start[i])
		{
			dist+=DIFF;
		}
	}
	dist += max(string.length-alternative.length,0)*LEN;
	return dist;
}

//bellow folllows a quick and dirty way of sorting which involves setting up a global variable
//its certainly not pretty but should work fine.
global_variable char *stringToCompareAgainst = (char *)0;
/*
internal int cmp(const void *va, const void *vb)
{
	CommandInfo *a = (CommandInfo*)va;
	CommandInfo *b = (CommandInfo*)vb;

	return levDist(stringToCompareAgainst, a->name) - levDist(stringToCompareAgainst, b->name);
}
*/
//---menus
internal int cmpMenuItem(const void *a, const void *b)
{
	return ((MenuItem *)a)->ordering-((MenuItem *)b)->ordering;
}

internal void sortMenu(Data *data)
{
	qsort(data->menu.start, data->menu.length, sizeof(MenuItem), cmpMenuItem);
}

internal void setOrderMenu(Data *data, DHSTR_String string)
{
	for(int i = 0; i<data->menu.length;i++)
	{
		data->menu.start[i].ordering = levDist(string, data->menu.start[i].name);
	}
}

internal void maxOrderSortedMenu(Data *data, int max)
{	
	int newLen = -1;
	for(int i = 0; i<data->menu.length;i++)
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

//---string man
internal bool hasOkExtension(DHSTR_String file_name)
{
	bool hasDot;
	int index = DHSTR_index_of(file_name, '.', DHSTR_INDEX_OF_LAST, &hasDot);
	
	if (!hasDot) return false;
	DHSTR_String extension = DHSTR_subString(file_name, index, file_name.length);

	DHSTR_String extensions[]  =
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
		if (DHSTR_eq(extensions[i], extension,string_eq_length_matter)) return true;
	}
	return false;
}

internal void bindCommand(char *name, void(*func)(Data *data, DHSTR_String restOfTheBuffer))
{
	CommandInfo commandInfo = {};
	commandInfo.name= name;
	commandInfo.hashedName = hash(name);
	commandInfo.func = func;
	Add(&commands, commandInfo);
}

internal void bindCommand(char *name, void(*func)(Data *data, DHSTR_String restOfTheBuffer),void(*charDown)(Data *data, DHSTR_String restOfTheBuffer))
{
	CommandInfo commandInfo = {};
	commandInfo.name= name;
	commandInfo.hashedName = hash(name);
	commandInfo.func = func;
	commandInfo.charDown = charDown;
	Add(&commands, commandInfo);
}

#if 0
internal void compareCommand(Data *data)
{
	char *string = getRestAsString(&data->commandLine->backingBuffer->buffer, data->commandLine->backingBuffer->buffer.left);
	renderMenu(string, commands, data);
}
#endif 

//just a friendly little wrapper... I've got to fix this at some point...
internal void closeBufferCommand(Data *data, DHSTR_String restOfTheBuffer)
{//this needs to be delayed.
	closeTextBuffer(data);
}

internal void charDownFileCommand(Data *data,DHSTR_String restOfTheBuffer)
{
	freeMenuItems(data);
	DHSTR_String file;
	addDirFilesToMenu(data,restOfTheBuffer,&file);
	setOrderMenu(data, file);
	sortMenu(data);
	maxOrderSortedMenu(data,10); //calibrate this
}

internal void moveActiveBufferLeft(Data *data)
{
	--data->activeTextBufferIndex;
	data->activeTextBufferIndex = clamp(data->activeTextBufferIndex, 0, max(data->textBuffers.length - 1, 0));
	data->updateAllBuffers = true;
}

internal void moveActiveBufferRight(Data *data)
{
	++data->activeTextBufferIndex;
	data->activeTextBufferIndex = clamp(data->activeTextBufferIndex, 0, max(data->textBuffers.length - 1, 0));
	data->updateAllBuffers = true;
}

internal void moveDownMenu(Data *data)
{
	++data->activeMenuItem;
	data->activeMenuItem= clamp(data->activeMenuItem, 0, max(data->menu.length, 0));
}

internal void moveUpMenu(Data *data)
{
	--data->activeMenuItem;
	data->activeMenuItem = clamp(data->activeMenuItem, 0, max(data->menu.length, 0));
}

internal void CloseCommandLine(Data *data)
{
	clearBuffer(data->commandLine->backingBuffer->buffer);
	data->isCommandlineActive = false;
}

internal void preFeedCreateFile(Data *data)
{
	char buffer[] = u8"createFile ";
	preFeedCommandLine(data, buffer, sizeof(buffer) / sizeof(buffer[0]));
}
enum Dir
{
	above,
	below,
};

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

internal void insertCaretAbove(TextBuffer *buffer)
{
	insertCaret(buffer, above);
}

internal void insertCaretBelow(TextBuffer *buffer)
{
	insertCaret(buffer, below);
}

internal void removeAllButOneCaret(TextBuffer *buffer)
{
	for (int i = 1; i < buffer->ownedCarets_id.length; )
	{
		removeCaret(buffer, 1, true);
	}
}

internal void win32_runBuild(Data *data) //this is *highly* platform dependant. Should be moved to win32.cpp
{	//and refactor out the openfile into buffeer into its own win32 function.

	//check for leakages... This is just thrown together!
	SECURITY_ATTRIBUTES atributes = {};
	atributes.nLength = sizeof(atributes);
	atributes.bInheritHandle = true;
	atributes.lpSecurityDescriptor = 0; // makes sure this is OK!
    HANDLE readPipe;
    HANDLE writePipe;
    if(CreatePipe(&readPipe,&writePipe,&atributes,0))
    {
		STARTUPINFO si = {};
		si.cb = sizeof(si); //like why do we need to do this microsoft.. why? is this sturct variable size or what?
		si.hStdOutput = writePipe;										//appearently because they like to change the struct without it breaking shit
		si.hStdError = writePipe;										//all right i guess...
		si.dwFlags = STARTF_USESTDHANDLES;
		
		PROCESS_INFORMATION pi; // I like the naming consistency, why isn't this called PROCESSINFO
		char proc[] = "cmd.exe  /C W:\\HH\\CodeWorking\\build.bat";
		
		if(CreateProcess(0,proc,0,0,true,CREATE_UNICODE_ENVIRONMENT,0,0,&si,&pi))
		{
			//we should maybe not wait here, just lock this buffer?
			//this might take some time and should freeze us!
				
			//from msdn, do it:
			//Use caution when calling the wait functions and code that directly or indirectly creates windows.
			//If a thread creates any windows, it must process messages.Message broadcasts are sent to all windows in the system.
			//A thread that uses a wait function with no time - out interval may cause the system to become deadlocked.
			//Two examples of code that indirectly creates windows are DDE and the CoInitialize function.
			//Therefore, if you have a thread that creates windows, use MsgWaitForMultipleObjects or MsgWaitForMultipleObjectsEx, rather than WaitForSingleObject.
			
			WaitForSingleObject( pi.hProcess, INFINITE);
			CloseHandle( pi.hProcess );
		    CloseHandle( pi.hThread );	
		}
		else
		{
			//todo handle and log
			assert(false);
		}
    }
    else
    {
		//todo handle and log
    	assert(false);
    }
    
	char buffer[200000]; 
	DWORD len;
    ReadFile(readPipe,buffer,200000,&len,0);
	TextBuffer textBuffer = allocTextBuffer(20000, 2000, 2000, 2000);
	for (int i = 0; i < len; i++)
    {
		appendCharacter(&textBuffer, buffer[i], false);
    }
	initTextBuffer(&textBuffer);

	DH_Allocator macro_used_allocator = default_allocator; //used below. I try to avoid lambdas when possible
	textBuffer.fileName = DHSTR_CPY(DHSTR_MAKE_STRING("Build_Log"),ALLOCATE); //BLAHA MALLOC
 	Add(&data->textBuffers, textBuffer);
	//data->activeTextBufferIndex = data->textBuffers.length - 1;
	data->updateAllBuffers = true;
	CloseHandle(readPipe);
	CloseHandle(writePipe);
}

internal void openFileCommand(Data *data, DHSTR_String restOfTheBuffer)
{
	void *(*allocator)(int size) = [](int bytes) {return alloc_(bytes, "openFileCommand"); };

	DHSTR_String filePath = {};
	if (data->activeMenuItem)
	{
		MenuItem item = data->menu.start[data->activeMenuItem - 1];
		MenuData *md = (MenuData *)item.data;
		if (md->isFile)
		{
			bool hasSlash;
			int last_slash = DHSTR_index_of(restOfTheBuffer, '/', DHSTR_INDEX_OF_LAST, &hasSlash);
			if (hasSlash)
			{
				DHSTR_String dir = DHSTR_subString(restOfTheBuffer, 0, last_slash);
				filePath = DHSTR_MERGE_MULTI(allocator, dir, DHSTR_MAKE_STRING("/"), item.name);
				charDownFileCommand(data, filePath);
			}
			else
			{
				filePath = DHSTR_CPY(item.name,allocator);
			}
		}
		else
		{
			clearBuffer(data->commandLine->backingBuffer->buffer);
			preFeedOpenFile(data);
			
			bool hasSlash;
			int last_slash = DHSTR_index_of(restOfTheBuffer, '/', DHSTR_INDEX_OF_LAST, &hasSlash);
			if (hasSlash)
			{
				DHSTR_String dir = DHSTR_subString(restOfTheBuffer, 0, last_slash);
				feedCommandLine(data, dir);  //the one here is disturbing
				appendCharacter(data->commandLine, '/',false);
			}

			
			feedCommandLine(data, item.name);  //the one here is disturbing
			appendCharacter(data->commandLine, '/',false);			
			keyDown_CommandLine(data);
			return;
		}

	}
	else
	{
		filePath = DHSTR_CPY(restOfTheBuffer, allocator); //this wasn't copied before. surely it should be
	}

	bool success;
	TextBuffer textBuffer = openFileIntoNewBuffer(filePath, &success);
	if (success)
	{
		Add(&data->textBuffers, textBuffer);
		data->activeTextBufferIndex = data->textBuffers.length - 1;
		hideCommandLine(data);
	}
	else
	{
		free_(filePath.start);
	}
}

bool splitPath(DHSTR_String path, DHSTR_String *_out_directory, DHSTR_String *_out_file)
{
	bool hashSlash;
	int index = DHSTR_index_of(path, '/', DHSTR_INDEX_OF_LAST, &hashSlash);
	if (hashSlash)
	{
		if (_out_directory)*_out_directory = DHSTR_subString(path, 0, index);
		if (_out_file)*_out_file = index >= path.length-1 ? DHSTR_MAKE_STRING(""):DHSTR_subString(path, index+1, path.length-1);
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
	}else
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
			DHSTR_String p = DHSTR_STRING_FROM_WCHART(ALLOCATE,ffd.cFileName);
			MenuItem newItem = {};
			newItem.name = p;
			if (hasOkExtension(p))
			{
				MenuData *menuData = (MenuData *)alloc_(sizeof(menuData), "menu data");
				menuData->isFile = true;
				newItem.data = menuData;
				Add(&data->menu, newItem);
			}
			else if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			{
				MenuData *menuData = (MenuData *)alloc_(sizeof(menuData), "menu data");
				menuData->isFile = false;
				newItem.data = menuData;
				Add(&data->menu, newItem);
			}
			else {
				free_(p.start);
			}
		} while (FindNextFileW(handle, &ffd) != 0);  //we trigger some breakpoint in here. what's the problem?
		FindClose(handle);
	}
}
void _cloneBuffer(Data *data)
{
	TextBuffer *clonee = &data->textBuffers.start[data->activeTextBufferIndex];
	TextBuffer b = cloneTextBuffer(clonee);
	Add(&data->textBuffers, b);
}

internal void appendVerticalTab(TextBuffer *textBuffer)
{
	appendCharacter(textBuffer, u'\v');
}

struct Locator
{
	int line;
	int pos;
};

unsigned int hash_loc(Locator loc)
{
	return silly_hash(loc.line) * 31 + silly_hash(loc.pos);
}
unsigned int locator_equality(Locator a, Locator b)
{
	return a.line == b.line && a.pos == b.pos;
}
DEFINE_HashTable(Locator, int, hash_loc, locator_equality);


internal int elasticTab(TextBuffer *buffer, MGB_Iterator it, char32_t current_char, char32_t next_char, Typeface::Font *typeface, float scale)
{
	BindingIdentifier ident = { 0,&elasticTab };
	void **memoPtr;
	HashTable_Locator_int *memo;
	if (!lookup(&buffer->backingBuffer->bindingMemory, ident, &memoPtr))
	{
		memo = (HashTable_Locator_int *) alloc_(sizeof(HashTable_Locator_int), "ElasticTab hashtable struct");
		*memo = DHDS_constructHT(Locator,int,50,buffer->allocator); //@leak for now, should be bulk deallocated on backingbuffer closing
		insert(&buffer->backingBuffer->bindingMemory,ident,memo);
	}
	else
	{
		memo = (HashTable_Locator_int *)*memoPtr;
	}
	BufferChange event;
	bool changed = false;
	while (next_history_change(buffer->backingBuffer,&elasticTab,&event)) 
	{
		changed = true;
	}
	if (changed)clear(memo);

	float max_w = calculateIteratorX(buffer, it);
	MultiGapBuffer *mgb = buffer->backingBuffer->buffer;
	MGB_Iterator itstart = it;
	float our_w = max_w;
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
	Locator loc;
	loc.line = getLineFromIterator(buffer, itstart);
	loc.pos = tab_num;
	int *val;
	
	if (lookup(memo, loc, &val)){
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
			if (!(getNext(mgb, &it))|| isLineBreak(*getCharacter(mgb, it))) goto next;
			if (*getCharacter(mgb, it) == '\v')
			{
				if (ctab++ == tab_num)
				{
					max_w = max(max_w, calculateIteratorX(buffer, it));
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
					max_w = max(max_w, calculateIteratorX(buffer, it));
					while (getNext(mgb, &it) && !isLineBreak(*getCharacter(mgb, it)));
					break;
				}
			}
		}
	}
end:
	int ret = max_w - our_w + getCharacterWidth_std('\t', ' ', typeface, scale);
	insert(memo, loc, ret);
	return ret;
}

internal void setBindingsLocal(TextBuffer *textBuffer)
{
	bindKey(VK_LEFT, atMost, mod_control | mod_shift, _moveLeft);
	bindKey(VK_RIGHT, atMost, mod_control | mod_shift, _moveRight);
	bindKey(VK_BACK, atMost, mod_control, _backSpace);
	bindKey(VK_DELETE, atMost, mod_control, _delete);

	bindKey('C', precisely, mod_control, copy);
	bindKey('V', precisely, mod_control, paste);
	bindKey('X', precisely, mod_control, cut);

	if (textBuffer->bufferType == regularBuffer)
	{
		ContextCharWidthHook elasticTabHook;
		elasticTabHook.character = '\v';
		elasticTabHook.func= elasticTab;

		Add(&textBuffer->contextCharWidthHook, elasticTabHook);
		bindKey(VK_UP,        atMost,  mod_shift,   _moveUp);
		bindKey(VK_DOWN,      atMost,  mod_shift,   _moveDown);
		bindKey('S', precisely,  mod_control, saveFile);
		bindKey('Z', precisely,  mod_control, undoMany);
		bindKey('Z', precisely,  mod_control | mod_shift, redoMany);
		bindKey('W', precisely, mod_control, closeTextBuffer);
		bindKey('M', precisely, mod_control, [](TextBuffer *buffer) {toggle(buffer, HistoryBuffer); });
		//bindKey('P', precisely, mod_control, freeBufferedCharacters);
		bindKey(VK_UP, precisely, mod_control, insertCaretAbove);
		bindKey( VK_DOWN, precisely, mod_control, insertCaretBelow);
		bindKey('C', precisely, mod_alt, _cloneBuffer);
		bindKey( VK_ESCAPE, precisely, mod_none, removeAllButOneCaret);
		bindKey('\t', precisely, mod_control, appendVerticalTab);
	} 
	
	else if (textBuffer->bufferType == commandLineBuffer)
	{
		bindKey(VK_RETURN, atLeast, mod_none, executeCommand);
		bindKey(VK_DOWN, precisely, mod_none, moveDownMenu);
		bindKey(VK_UP, precisely, mod_none, moveUpMenu);
		bindKey(VK_ESCAPE, precisely, mod_none, hideCommandLine);

		bindCommand("openFile", openFileCommand, charDownFileCommand);
		bindCommand("o", openFileCommand, charDownFileCommand);
		bindCommand("createFile", createFile);
		bindCommand("closeBuffer", closeBufferCommand);
		bindCommand("c", closeBufferCommand); 
	}

	bindKey(VK_LEFT, precisely, mod_alt, moveActiveBufferLeft);
	bindKey(VK_RIGHT, precisely, mod_alt, moveActiveBufferRight);
	bindKey(VK_DOWN, precisely, mod_alt, showCommandLine);
	bindKey(VK_UP, precisely, mod_alt, hideCommandLine);
	bindKey('O', precisely, mod_control, preFeedOpenFile);
	bindKey('B',precisely, mod_control, win32_runBuild);
	bindKey('N', precisely, mod_control, preFeedCreateFile);
}

void _setLocalBindings(TextBuffer *textBuffer)
{
	assert(bindings == 0); // this is fine till we're using mutple threads, assert to make sure!
	bindings = &textBuffer->KeyBindings;
	setBindingsLocal(textBuffer);
	bindings = (DynamicArray_KeyBinding *) 0;
}
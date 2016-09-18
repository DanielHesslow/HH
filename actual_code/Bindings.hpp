#include "header.h"
internal void addDirFilesToMenu(Data *data, DHSTR_String string, DHSTR_String  *name); //more platform, must be refactored to win32 

#define USE_API

#ifdef USE_API
#include "api.h"
API api;
#endif

#ifndef USE_API
internal void showCommandLine(Data *data)
{
	data->isCommandlineActive = true;
}
#else
internal void showCommandLine()
{
	api.commandLine.open();
}
#endif 
#ifndef USE_API
#else
internal void hideCommandLine()
{
	api.commandLine.close();
}
#endif

#ifndef USE_API
internal void feedCommandLine(Data *data, DHSTR_String string)
{
	for (int i = 0; i < string.length; i++)
	{
		appendCharacter(data->commandLine,string.start[i],0,true);
	}
}
#else
internal void feedCommandLine(DHSTR_String string)
{
	api.commandLine.feed(string.start, string.length);
}
#endif
#ifndef USE_API
internal void preFeedCommandLine(Data *data, char *string, int length)
{
	showCommandLine(data);
	clearBuffer(data->commandLine->backingBuffer->buffer);
	feedCommandLine(data, DHSTR_MAKE_STRING(string));

}
#else
internal void preFeedCommandLine(char *string, int length)
{
	api.commandLine.open();
	api.commandLine.clear();
	api.commandLine.feed(string, length);
}
#endif
// ---- BACKEND API 

void memory_for_last_event() {}

#ifndef USE_API
internal void preFeedOpenFile(Data *data)
{
	char buffer[] = u8"openFile ";
	preFeedCommandLine(data, buffer, sizeof(buffer) / sizeof(buffer[0]));
}
#else
internal void preFeedOpenFile()
{
	char buffer[] = u8"openFile ";
	preFeedCommandLine(buffer, sizeof(buffer) / sizeof(buffer[0]));
}
#endif

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
	if (command.func)
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

// --- END BACKEND_API


#ifndef USE_API
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
#else
void _moveLeft(Mods mods) //probably just allow the user to request this?
{
	void *handle = api.handles.getActiveViewHandle();
	if (mods & mod_control) api.cursor.moveWhile(handle, -1, ALL_CURSORS, mods & mod_shift ? do_select : no_select, &whileWord);
	else api.cursor.move(handle, -1, ALL_CURSORS, mods & mod_shift ? do_select : no_select, move_mode_grapheme_cluster);
}
#endif

#ifndef USE_API
void _moveRight(TextBuffer *textBuffer, Mods mods)
{
	if (mods & mod_control)
	{
		moveWhile(textBuffer, mods & mod_shift ? do_select : no_select, do_log, true, dir_right, &whileWord);
	}
	else
	{
		move(textBuffer, dir_right, do_log, mods & mod_shift ? do_select : no_select);
	}
	checkIdsExist(textBuffer);
}
#else 
void _moveRight(Mods mods)
{
	void *handle = api.handles.getActiveViewHandle();
	if (mods & mod_control) api.cursor.moveWhile(handle, 1, ALL_CURSORS, mods & mod_shift ? do_select : no_select, &whileWord);
	else api.cursor.move(handle, 1, ALL_CURSORS, mods & mod_shift ? do_select : no_select, move_mode_grapheme_cluster);
}
#endif
#ifndef USE_API
void _moveUp(TextBuffer *textBuffer, Mods mods)
{
	for (int i = 0; i < textBuffer->ownedCarets_id.length; i++)
	{
		moveV_nc(textBuffer, mods & mod_shift ? do_select: no_select, i,true);
	}
	removeOwnedEmptyCarets(textBuffer);
	checkIdsExist(textBuffer);
}
#else
void _moveUp(Mods mods)
{
	void *handle = api.handles.getActiveViewHandle();
	api.cursor.move(handle, -1, ALL_CURSORS, mods & mod_shift ? do_select : no_select, move_mode_line);
}
#endif

#if 0
void _moveUpPage(TextBuffer *textBuffer, Mods mods)
{
	movePage(textBuffer, true);
}
void _moveDownPage(TextBuffer *textBuffer, Mods mods)
{
	movePage(textBuffer, false);
}
#endif
#ifndef USE_API
void _moveDown(TextBuffer *textBuffer, Mods mods)
{
	for (int i = 0; i < textBuffer->ownedCarets_id.length; i++)
	{
		moveV_nc(textBuffer, mods & mod_shift? do_select :no_select, i,false);
	}
	removeOwnedEmptyCarets(textBuffer);
	checkIdsExist(textBuffer);

}
#else
void _moveDown(Mods mods)
{
	void *handle = api.handles.getActiveViewHandle();
	api.cursor.move(handle, 1, ALL_CURSORS, mods & mod_shift ? do_select : no_select, move_mode_line);
}
#endif

#ifndef USE_API
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
#else
void _backSpace(Mods mods)
{
	void *handle = api.handles.getActiveViewHandle();
	int number_of_cursors = api.cursor.number_of_cursors(handle);
	for (int i = 0; i < number_of_cursors; i++)
	{
		if (api.cursor.selection_length(handle, i))
		{
			api.cursor.delete_selected(handle, i);
		}
		else
		{
			if (mods & mod_control)
				api.cursor.removeWhile(handle, -1, i, &whileWord);
			else
				api.cursor.remove_codepoint(handle, i, -1);

		}
	}
}
#endif

#ifndef USE_API
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
#else
void _delete(Mods mods)
{
	void *handle = api.handles.getActiveViewHandle();
	int number_of_cursors = api.cursor.number_of_cursors(handle);
	for (int i = 0; i < number_of_cursors; i++)
	{
		if (api.cursor.selection_length(handle, i))
		{
			api.cursor.delete_selected(handle, i);
		}
		else
		{
			if (mods & mod_control)
				api.cursor.removeWhile(handle, -1, i, &whileWord);
			else
				api.cursor.remove_codepoint(handle, i, 1);
		}
	}
}
#endif

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

internal void charDownFileCommand(Data *data, DHSTR_String restOfTheBuffer)
{
	freeMenuItems(data);
	DHSTR_String file;
	addDirFilesToMenu(data,restOfTheBuffer,&file);
	setOrderMenu(data, file);
	sortMenu(data);
	maxOrderSortedMenu(data,10); //calibrate this
}



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
			if (locator.child_index < locator.parent->number_of_children-1 && locator.parent->type == type)
			{
				Layout *target_layout = locator.parent->children[locator.child_index + 1];
				ack += favourite_descendant(target_layout);
				locator.parent->favourite_child = locator.child_index + 1;
				return ack+1;
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

#ifndef USE_API
internal int clamp_text_buffer_index(Data *data, int index)
{
	return(clamp(index, 0, min(number_of_leafs(data->layout), max(data->textBuffers.length - 1, 0))));
}

internal void moveActiveBufferLeft(Data *data)
{
	data->activeTextBufferIndex += diffIndexMove(data->layout, data->activeTextBufferIndex, -1, layout_type_x);
}

internal void moveActiveBufferRight(Data *data)
{
	data->activeTextBufferIndex += diffIndexMove(data->layout, data->activeTextBufferIndex, 1, layout_type_x);
}

internal void moveActiveBufferUp(Data *data)
{
	data->activeTextBufferIndex += diffIndexMove(data->layout, data->activeTextBufferIndex, -1, layout_type_y);
}

internal void moveActiveBufferDown(Data *data)
{
	data->activeTextBufferIndex += diffIndexMove(data->layout, data->activeTextBufferIndex, 1, layout_type_y);
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
#endif
internal void CloseCommandLine(Data *data)
{
	clearBuffer(data->commandLine->backingBuffer->buffer);
	data->isCommandlineActive = false;
}

#ifndef USE_API
internal void preFeedCreateFile(Data *data)
{
	char buffer[] = u8"createFile ";
	preFeedCommandLine(data, buffer, sizeof(buffer) / sizeof(buffer[0]));
}
#else
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

#ifndef USE_API
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

#if 1
		// null pointers below because it's a vararg and passing 0 instead of (void *) 0 only works if (void *) is one word 
		switch (data->textBuffers.length)
		{
		case 1:
			data->layout = 0;
			break;
		case 2:
			data->layout = CREATE_LAYOUT(layout_type_x, 2, nullptr, 0.5, nullptr);
			break;
		case 3:
			data->layout = CREATE_LAYOUT(layout_type_y, 1, nullptr, 0.5, nullptr, 0.75, nullptr);
			break;
		case 4:
			data->layout = CREATE_LAYOUT(layout_type_x, 5, nullptr, 0.25, CREATE_LAYOUT(layout_type_y,2,nullptr,0.33,nullptr,0.67,nullptr));
			break;
		}
#endif
	}
	else
	{
		free_(filePath.start);
	}
}
#else
internal void openFileCommand(char *start, int length)
{
	
}
#endif

#ifndef USE_API
void find_mark_down(Data *data, DHSTR_String restOfTheBuffer)
{
	TextBuffer *buffer = &data->textBuffers.start[data->activeTextBufferIndex];
	remove_rendering_changes(buffer, find_mark_down);

 	if (restOfTheBuffer.length == 0 )return;

	MultiGapBuffer *mgb = buffer->backingBuffer->buffer;
	MGB_Iterator it_cpy;
	MGB_Iterator it = getIterator(mgb);
	if (!pushValid(mgb,&it))return;

	Location match_location;
	int i = 0;
	do {
		char b = *getCharacter(mgb, it);
		if (b == restOfTheBuffer.start[i])
		{
			if (i == 0)
			{
				match_location = locationFromIterator(buffer, it);
			}

			if (++i == restOfTheBuffer.length)
			{
				it_cpy = it;
				getNext(mgb, &it_cpy);
				change_rendering_highlight_color(buffer, match_location, locationFromIterator(buffer, it_cpy), active_colorScheme.active_color, find_mark_down);
				i = 0;
			}
		}
		else
		{
			i = 0;
		}
	}
	while (getNext(mgb, &it));
}
#else
void find_mark_down(char *text, int text_length)
{
	void *view_handle = api.handles.getActiveViewHandle();

	api.markup.remove(view_handle, &find_mark_down);
	if (text_length == 0) return;

	BufferHandle buffer_handle = api.handles.getBufferHandleFromViewHandle(view_handle);
	TextIterator it = api.text_iterator.make(buffer_handle);
	Location match_location;
	int i = 0;
	char b;
	while (api.text_iterator.nextByte(&it, &b, false))
	{
		if (b == text[i])
		{
			if (i == 0)
			{
				match_location = api.Location.from_iterator(buffer_handle, it);
			}

			if (++i == text_length)
			{
				TextIterator it_cpy = it;
				api.text_iterator.nextCodepoint(&it_cpy, 0, false);
				api.markup.highlight_color(view_handle, &find_mark_down, match_location, api.Location.from_iterator(buffer_handle, it_cpy), active_colorScheme.active_color);
				i = 0;
			}
		}
		else
		{
			i = 0;
		}
	}
}
#endif

#ifndef USE_API
void find(Data *data, DHSTR_String restOfTheBuffer)
{
	TextBuffer *buffer = &data->textBuffers.start[data->activeTextBufferIndex];
	remove_rendering_changes(buffer, find_mark_down);
}
#else
void find(char *start, int length)
{
	api.markup.remove(api.handles.getActiveBufferHandle(), find_mark_down);
}
#endif

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
#ifndef USE_API
	TextBuffer *clonee = &data->textBuffers.start[data->activeTextBufferIndex];
	TextBuffer b = cloneTextBuffer(clonee);
	Add(&data->textBuffers, b);
#else
	api.view.clone_view(api.handles.getActiveViewHandle());
#endif
}

#ifndef USE_API
internal void appendVerticalTab(TextBuffer *textBuffer)
{
	appendCharacter(textBuffer, u'\v');
}
#else
internal void appendVerticalTab()
{
	api.cursor.append_codepoint(api.handles.getActiveViewHandle(), ALL_CURSORS, -1, '\v');
}
#endif


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


#ifndef USE_API
internal bool is_big_title(TextBuffer *textBuffer, int line)
{
	MultiGapBuffer *mgb = textBuffer->backingBuffer->buffer;
	MGB_Iterator it = iteratorFromLine(textBuffer, line);
	int ack = 0;
	do
	{
		char c = *getCharacter(mgb, it);
		if (isLineBreak(c))
		{
			break;
		}
		else if (c == '-')
		{
			if (++ack >= 3)
			{
				return  true;
			}
		}
		else
		{
			ack = 0;
		}
	} while (getNext(mgb, &it));
	return false;

}
#else
internal bool is_big_title(void *buffer_handle, int line)
{
	TextIterator it = api.text_iterator.make_from_location(buffer_handle, { line,0 });
	char b;
	int ack = 0;
	while (api.text_iterator.nextByte(&it, &b, false))
	{
		if (b == '\n') break;
		else if (b == '-') { if (++ack >= 3) return true; }
		else ack = 0;
	}
	return false;
}
#endif
#ifndef USE_API
internal void init_big_titles(TextBuffer *buffer, HashTable_int_float *memo)
{

	float title_size = buffer->initial_rendering_state.scale * 2;
	clear(memo);
	for (int i = 0; i < getLines(buffer->backingBuffer); i++)
	{
		if (is_big_title(buffer, i))
		{
			insert(memo, i, title_size);
		}
	}
}
#else
internal void init_big_titles(void *buffer_handle, HashTable_int_bool *memo)
{
	clear(memo);
	for (int i = 0; i < api.buffer.numLines(buffer_handle); i++)
	{
		if (is_big_title(buffer_handle, i))
		{
			insert(memo, i, true);
		}
	}
}
#endif

#ifndef USE_API
internal void mark_bigTitles(TextBuffer *buffer)
{
	bool changed = false;

	float title_size = buffer->initial_rendering_state.scale * 2;

	remove_rendering_changes(buffer, &mark_bigTitles);
	BindingIdentifier ident = { 0,&mark_bigTitles };
	
	void **memoPtr;
	HashTable_int_float *memo;
	if (!lookup(&buffer->backingBuffer->commonBuffer.bindingMemory, ident, &memoPtr))
	{
		memo = (HashTable_int_float *)alloc_(sizeof(HashTable_int_float), "bigTitles hashtable struct");
		*memo = DHDS_constructHT(int, float, 50, buffer->allocator);
		changed = true;
		init_big_titles(buffer, memo);
		insert(&buffer->backingBuffer->commonBuffer.bindingMemory, ident, memo);
	}
	else
	{
		memo = (HashTable_int_float *)*memoPtr;
	}

	BufferChange event;
	while (next_history_change(buffer->backingBuffer, &mark_bigTitles, &event))
	{
		changed = true;
		fast_forward_history_changes(buffer->backingBuffer, &mark_bigTitles);
		
		if (is_big_title(buffer,event.location.line))
			insert(memo, event.location.line, title_size);
		else
			remove(memo, event.location.line,0);

		//lazy af way to handle linebreak lol
		if (is_big_title(buffer, event.location.line-1))
			insert(memo, event.location.line-1, title_size);
		else
			remove(memo, event.location.line-1, 0);


		if (is_big_title(buffer, event.location.line+1))
			insert(memo, event.location.line+1, title_size);
		else
			remove(memo, event.location.line+1, 0);
	}

	if (changed)
	{
		remove_rendering_changes(buffer, &mark_bigTitles);

		for (int i = 0; i < memo->capacity; i++)
		{

			if (memo->buckets[i].state == bucket_state_occupied)
			{ 
				int line  = memo->buckets[i].key;
				change_rendering_scale(buffer, { line, 0 }, { line + 1,0 }, title_size, mark_bigTitles);
				change_rendering_color(buffer, { line, 0 }, { line + 1,0 }, active_colorScheme.active_color, mark_bigTitles);
			}
		}
	}
}
#else
internal void mark_bigTitles()
{
	bool changed = false;

	HashTable_int_bool *memo = (HashTable_int_bool *)api.memory.getFunctionInfo(api.handles.getActiveBufferHandle(), &mark_bigTitles);

	void *buffer_handle = api.handles.getActiveBufferHandle();
	DH_Allocator allocator;
	allocator.alloc = [](size_t bytes_to_alloc, void  *info, void *data) {return api.memory.allocateMemory(data, bytes_to_alloc); };
	allocator.free = [](void *mem, void *data) {api.memory.deallocateMemory(data, mem); };
	allocator.data = buffer_handle;

	void *mem = api.memory.getFunctionInfo(buffer_handle, &mark_bigTitles);
	if (!mem)
	{
		memo = (HashTable_int_bool *)api.memory.allocateMemory(buffer_handle, sizeof(HashTable_int_float));
		*memo = DHDS_constructHT(int, bool, 50, allocator);
		changed = true;
		init_big_titles(buffer_handle, memo);
	}
	BufferChange event;
	while (api.changes.next_change(buffer_handle, &mark_bigTitles, &event))
	{
		changed = true;
		api.changes.mark_all_read(buffer_handle, &mark_bigTitles);

		if (is_big_title(buffer_handle, event.location.line))
			insert(memo, event.location.line, true);
		else
			remove(memo, event.location.line, 0);

		//lazy af way to handle linebreak lol
		if (is_big_title(buffer_handle, event.location.line - 1))
			insert(memo, event.location.line - 1, true);
		else
			remove(memo, event.location.line - 1, 0);


		if (is_big_title(buffer_handle, event.location.line + 1))
			insert(memo, event.location.line + 1, true);
		else
			remove(memo, event.location.line + 1, 0);
	}

	if (changed)
	{
		void *view_handle;
		ViewIterator vi = api.buffer.view_iterator(api.handles.getActiveBufferHandle());
		ViewIterator vi_cpy = vi;
		while (api.buffer.next(&vi_cpy, &view_handle))
		{
			float title_size = api.markup.get_initial_rendering_state(view_handle).scale;
			api.markup.remove(view_handle, &mark_bigTitles);

			for (int i = 0; i < memo->capacity; i++)
			{

				if (memo->buckets[i].state == bucket_state_occupied)
				{
					int line = memo->buckets[i].key;
					api.markup.scale(view_handle, &mark_bigTitles, { line, 0 }, { line + 1,0 }, title_size);
					api.markup.text_color(view_handle, &mark_bigTitles, { line, 0 }, { line + 1,0 }, active_colorScheme.active_color);
				}
			}
		}
	}
}
#endif


internal int elasticTab(TextBuffer *buffer, MGB_Iterator it, char32_t current_char, char32_t next_char, Typeface::Font *typeface, float scale)
{
#ifndef USE_API
	int tab_width= getCharacterWidth_std('\t', ' ', typeface, 1);

	//@BUG: we do not account for difference in tab width from different font scales.
	//maybe also memoize end of elastic tab as well as width of it?
	BindingIdentifier ident = { 0,&elasticTab };
	void **memoPtr;
	HashTable_Location_int *memo;
	if (!lookup(&buffer->backingBuffer->commonBuffer.bindingMemory, ident, &memoPtr))
	{
		memo = (HashTable_Location_int *) alloc_(sizeof(HashTable_Location_int), "ElasticTab hashtable struct"); //why are we leaking?
		*memo = DHDS_constructHT(Location,int,50,buffer->allocator); 
		insert(&buffer->backingBuffer->commonBuffer.bindingMemory,ident,memo);
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
	float our_w = calculateIteratorX(buffer, it,&rend);
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
					// keep tmp on a seperate line to avoid undeffed behaviour, rend can't be modified and used in the same statment
					int tmp = calculateIteratorX(buffer, it,&rend);
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


internal void setActive(Data *data, int index)
{
	if (data->textBuffers.length > index && index >= 0)
	{
		data->activeTextBufferIndex = index;
	}
}


#ifndef USE_API
internal void setBindingsLocal(TextBuffer *textBuffer)
{
	Add(&textBuffer->renderingModifiers, &mark_selection);
	Add(&textBuffer->renderingModifiers, &mark_bigTitles);

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
		//bindKey('P', precisely, mod_control, freeBufferedCharacters);
		bindKey(VK_UP, precisely, mod_control, insertCaretAbove);
		bindKey( VK_DOWN, precisely, mod_control, insertCaretBelow);
		bindKey('C', precisely, mod_alt, _cloneBuffer);
		//bindKey(VK_PRIOR, precisely, mod_none, _moveUpPage);
		//bindKey(VK_NEXT, precisely, mod_none, _moveDownPage);
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
		bindCommand("find",  find, find_mark_down);
	}

	bindKey(VK_LEFT, precisely, mod_alt,  moveActiveBufferLeft);
	bindKey(VK_RIGHT, precisely, mod_alt, moveActiveBufferRight);
	bindKey(VK_UP, precisely, mod_alt,  moveActiveBufferUp);
	bindKey(VK_DOWN, precisely, mod_alt, moveActiveBufferDown);
	
	bindKey('1', precisely, mod_alt, [](Data *data) {setActive(data, 0);});
	bindKey('2', precisely, mod_alt, [](Data *data) {setActive(data, 1); });
	bindKey('3', precisely, mod_alt, [](Data *data) {setActive(data, 2); });
	bindKey('4', precisely, mod_alt, [](Data *data) {setActive(data, 3); });
	bindKey('5', precisely, mod_alt, [](Data *data) {setActive(data, 4); });
	bindKey('6', precisely, mod_alt, [](Data *data) {setActive(data, 5); });
	bindKey('7', precisely, mod_alt, [](Data *data) {setActive(data, 6); });
	bindKey('8', precisely, mod_alt, [](Data *data) {setActive(data, 6); });
	bindKey('9', precisely, mod_alt, [](Data *data) {setActive(data, 7); });
	bindKey('0', precisely, mod_alt, [](Data *data) {setActive(data, 9); });

	bindKey('O', precisely, mod_control, preFeedOpenFile);
	bindKey('B',precisely, mod_control, win32_runBuild);
	bindKey('N', precisely, mod_control, preFeedCreateFile);
}
#else 
internal void setBindingsLocal(void *view_handle)
{
	api.callbacks.registerCallBack(callback_pre_render, view_handle, &mark_selection);
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
#if 0
		ContextCharWidthHook elasticTabHook;
		elasticTabHook.character = '\v';
		elasticTabHook.func = elasticTab;

		Add(&textBuffer->contextCharWidthHook, elasticTabHook);
#endif
		api.callbacks.bindKey_mods(view_handle, VK_UP, atMost, mod_shift, _moveUp);
		api.callbacks.bindKey_mods(view_handle, VK_DOWN, atMost, mod_shift, _moveDown);
		api.callbacks.bindKey(view_handle, 'S', precisely, mod_control, []() {api.buffer.save(api.handles.getActiveBufferHandle());});
		//api.callbacks.bindKey(view_handle, 'Z', precisely, mod_control, undoMany);
		//api.callbacks.bindKey(view_handle, 'Z', precisely, mod_control | mod_shift, redoMany);
		api.callbacks.bindKey(view_handle, 'W', precisely, mod_control, [](){api.view.clone_view(api.handles.getActiveViewHandle());});
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
		//api.callbacks.bindKey(view_handle, VK_DOWN, precisely, mod_none, moveDownMenu);
		//api.callbacks.bindKey(view_handle, VK_UP, precisely, mod_none, moveUpMenu);
		api.callbacks.bindKey(view_handle, VK_ESCAPE, precisely, mod_none, hideCommandLine);

		api.callbacks.bindCommand(view_handle, "openFile", openFileCommand, 0,0);
		api.callbacks.bindCommand(view_handle, "o", openFileCommand, 0,0);
		api.callbacks.bindCommand(view_handle, "createFile", api.view.createFromFile ,0,0);
		api.callbacks.bindCommand(view_handle, "closeBuffer", [](char *start, int length) {api.view.close(api.handles.getActiveViewHandle()); }, 0, 0);
		api.callbacks.bindCommand(view_handle, "c", [](char *start, int length) {api.view.close(api.handles.getActiveViewHandle()); }, 0, 0);
		api.callbacks.bindCommand(view_handle, "find", find, find_mark_down,0);
	}

	api.callbacks.bindKey(view_handle, 'O', precisely, mod_control, preFeedOpenFile);
	//api.callbacks.bindKey(view_handle, 'B', precisely, mod_control, win32_runBuild);
	api.callbacks.bindKey(view_handle, 'N', precisely, mod_control, preFeedCreateFile);
}

#endif

void _setLocalBindings(TextBuffer *textBuffer)
{
	assert(bindings == 0); // this is fine till we're using mutple threads, assert to make sure!
	bindings = &textBuffer->KeyBindings;
	setBindingsLocal(textBuffer);
	bindings = (DynamicArray_KeyBinding *) 0;
}

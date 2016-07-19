
#include "DH_MacroAbuse.h"

#include "..\libs\utf8rewind-1.5.0\utf8rewind.h"
#include "..\libs\utf8rewind-1.5.0\utf8rewind.c"
#include "..\libs\utf8rewind-1.5.0\unicodedatabase.h"
#include "..\libs\utf8rewind-1.5.0\unicodedatabase.c"

#include "strings.c"


#include <stdint.h>
#include "platform.h"
inline uint64_t ciel_to_multiple_of(uint64_t i, int mult);

struct DH_Allocator
{
	void *(*alloc)(size_t bytes_to_alloc, void *allocation_info, void *data);
	void(*free)(void *mem_to_free, void *data);
	void *data;
};
void *platform_alloc_compat(size_t bytes_to_alloc, void *alloc_info, void *data) { return platform_alloc(bytes_to_alloc); }
void platform_free_compat(void *mem, void *data) { platform_free(mem); }

DH_Allocator platform_allocator = { platform_alloc_compat,platform_free_compat,0};

void *Allocate(DH_Allocator allocator, size_t bytes_to_alloc, void *allocation_info)
{
	return allocator.alloc(bytes_to_alloc, allocation_info, allocator.data);
}
void DeAllocate(DH_Allocator allocator, void *mem)
{
	allocator.free(mem, allocator.data);
}

#include "malloc.h" //for alloca, lets unlink to all this shit later on. I don't want it.
/*
void* __cdecl _alloca(_In_ size_t _Size);
#define alloca _alloca
*/

#include "arena_allocator.cpp"
#include "Allocation.h"
#include "DH_DataStructures.h"

#include "Allocation.cpp"
#include "header.h"
#include "MultiGapBuffer.cpp"
//#include "Parser.hpp"
#include "colorScheme.h"
//#include "lexer.cpp"
#include "ClipBoard.hpp"
#include "History.cpp"

internal int getLineFromIterator(TextBuffer *buffer, MGB_Iterator it);
internal float scaleFromLine(TextBuffer *buffer, int line);
internal int getLineStart(BackingBuffer *backingBuffer, int line);
internal void freeBufferedCharacters();

internal int calculateIteratorX(TextBuffer *textBuffer, MGB_Iterator it)
{
	
	//either us or move caret to X have some sort of problem (all or just high unicode stuff?)
	MultiGapBuffer *mgb = textBuffer->backingBuffer->buffer;
	float scale = scaleFromLine(textBuffer, getLineFromIterator(textBuffer, it));
	int initial_byte_pos = 0;
	if (!MoveIterator(mgb, &it, -1));
	for (;;)
	{
		if (isLineBreak(*getCharacter(mgb, it))) break;
		if (!MoveIterator(mgb, &it, -1))break;
		++initial_byte_pos;
	}
	if (!MoveIterator(mgb, &it, 1));

	char32_t codepoint_a, codepoint_b;
	int ack = getCodepoint(mgb, it, &codepoint_a);
	MoveIterator(mgb, &it, ack);
	int read;
	int x_ack = 0;
	while (ack <= initial_byte_pos)
	{
		read = getCodepoint(mgb, it, &codepoint_b);
		if (!read)break;
		ack += read;
		x_ack += getCharacterWidth(textBuffer, it, codepoint_a, codepoint_b, textBuffer->font, scale);
		MoveIterator(mgb, &it, read);
		codepoint_a = codepoint_b;
	}
	return x_ack;
}


internal int calculateCursorX(TextBuffer *textBuffer, int cursorId)
{
	MultiGapBuffer *mgb = textBuffer->backingBuffer->buffer;
	MGB_Iterator it = getIteratorFromCaret(mgb, cursorId);
	return calculateIteratorX(textBuffer, it);
}


internal void markPreferedCaretXDirty(TextBuffer *textBuffer, int caretIdIndex)
{
	textBuffer->cursorInfo.start[caretIdIndex].is_dirty = true;
}
internal void setPreferedX(TextBuffer *textBuffer, int caretIdIndex)
{
	if (textBuffer->cursorInfo.start[caretIdIndex].is_dirty)
	{
		textBuffer->cursorInfo.start[caretIdIndex].prefered_x = calculateCursorX(textBuffer, textBuffer->ownedCarets_id.start[caretIdIndex]);
		textBuffer->cursorInfo.start[caretIdIndex].is_dirty = false;
	}
}
internal int getPreferedCaretX(TextBuffer *textBuffer, int caretIdIndex)
{
	setPreferedX(textBuffer, caretIdIndex);
	return textBuffer->cursorInfo.start[caretIdIndex].prefered_x;
}

internal void gotoStartOfLine(TextBuffer *textBuffer, int line, select_ selection, int caretIdIndex)
{
	int caretId = textBuffer->ownedCarets_id.start[caretIdIndex];
	int currentChar = getCaretPos(textBuffer->backingBuffer->buffer, caretId);
	int lineChar = getLineStart(textBuffer->backingBuffer, line);
	
	for (int i = 0; i<currentChar - lineChar; i++)
	{
		move_nc(textBuffer, dir_left, caretIdIndex, do_log, selection,movemode_byte);
	}
	for (int i = 0; i<lineChar - currentChar; i++)
	{
		move_nc(textBuffer, dir_right, caretIdIndex, do_log, selection, movemode_byte);
	}
}

internal void moveCaretToX(TextBuffer *textBuffer, int targetX, select_ selection, int caretIdIndex)
{
	// this assumes that we're on the start of the line.
	
	int caretId = textBuffer->ownedCarets_id.start[caretIdIndex];
	float scale = scaleFromLine(textBuffer, getLineFromCaret(textBuffer, caretId));

	int x = 0;
	int dx = 0;

	MultiGapBuffer *mgb = textBuffer->backingBuffer->buffer;
	MGB_Iterator it = getIteratorFromCaret(mgb, caretId);
	char32_t codepoint_a,codepoint_b;
	int a_read_len = getCodepoint(mgb,it,&codepoint_a);
	int b_read_len;
	int ack = 0;
	MoveIterator(mgb, &it, a_read_len);
	while ((b_read_len = getCodepoint(mgb, it, &codepoint_b)))
	{
		MoveIterator(mgb, &it, b_read_len);
		dx = getCharacterWidth(textBuffer, it, codepoint_a, codepoint_b, textBuffer->font, scale);
		if (abs(x - targetX) < abs((x + dx) - targetX)||isLineBreak(codepoint_a)) break;
		ack += a_read_len;
		x += dx;
		codepoint_a = codepoint_b;
		a_read_len = b_read_len;
	}
	for (int i = 0; i < ack;i++)
		move_nc(textBuffer, dir_right, caretIdIndex, do_log, selection, movemode_byte);
}


// ---SELECTIONS---

void setNoSelection(TextBuffer *textBuffer, int caretIdIndex, log_ log)
{
	int caretId = textBuffer->ownedCarets_id.start[caretIdIndex];
	int selectionCaretId = textBuffer->ownedSelection_id.start[caretIdIndex];
	int length = getCaretPos(textBuffer->backingBuffer->buffer, caretId) - getCaretPos(textBuffer->backingBuffer->buffer, selectionCaretId);
	for (int i = 0; i < length; i++)
	{
		move_llnc(textBuffer, dir_right, caretIdIndex,log == do_log, move_caret_selection,movemode_byte);
	}
	for (int i = 0; i < -length; i++)
	{
		move_llnc(textBuffer, dir_left, caretIdIndex, log == do_log, move_caret_selection, movemode_byte);
	}
}

void setNoSelection(TextBuffer *textBuffer, log_ log)
{
	for (int i = 0; i < textBuffer->ownedCarets_id.length; i++)
	{
		setNoSelection(textBuffer, i,log);
	}
}

internal bool removeCharacter(TextBuffer *textBuffer, int caretIdIndex, bool log);
internal void removeSelection(TextBuffer *textBuffer, int caretIdIndex)
{
	int caretId = textBuffer->ownedCarets_id.start[caretIdIndex];
	int selectionCaretId = textBuffer->ownedSelection_id.start[caretIdIndex];

	int length = getCaretPos(textBuffer->backingBuffer->buffer, caretId) - getCaretPos(textBuffer->backingBuffer->buffer, selectionCaretId);

	for (int i = 0; i < length; i++)
		removeCharacter(textBuffer, caretIdIndex, true);

	for (int i = 0; i < -length; i++)
		deleteCharacter(textBuffer, caretIdIndex, true);
}

internal void removeSelection(TextBuffer *textBuffer)
{
	for (int i = 0; i < textBuffer->ownedCarets_id.length; i++)
	{
		removeSelection(textBuffer, i);
	}
	removeEmpty(textBuffer);
}


// ---LINE AND LOCATIONS---

internal int getNextColor(int *index)
{
	++*index;
	*index %= 3;
	return backgroundColors[*index++];
}

global_variable float scale = -1;
internal float scaleFromLine(TextBuffer *textBuffer, int line)
{
	if (scale == -1)
	{
		scale = stbtt_ScaleForPixelHeight(textBuffer->font->font_info, 14);
	}
	return scale;
}

internal float backgroundColorFromLine(TextBuffer *textBuffer, int line)
{
	return backgroundColors[line%ARRAY_LENGTH(backgroundColors)];
}
internal void redoLineSumTree(BackingBuffer *backingBuffer)
{
	DynamicArray_int *sum_tree = &backingBuffer->lineSumTree;
	MGB_Iterator it = getIterator(backingBuffer->buffer);
	int counter = 0;
	int line = 0;
	int read;
	do {
		char32_t codepoint;
		read = getCodepoint(backingBuffer->buffer, it, &codepoint);
		counter += read;
		if (isLineBreak(codepoint))
		{
			assert(counter != 0);
			binsumtree_set(sum_tree, line++, counter);
			counter = 0;
		}
	} while (MoveIterator(backingBuffer->buffer,&it,read));
}

internal void initLineSumTree(BackingBuffer *backingBuffer,DH_Allocator allocator)
{
	DynamicArray_int *sum_tree = &backingBuffer->lineSumTree;
	*sum_tree = DHDS_constructDA(int, 128,allocator);
	memset(sum_tree->start, 0, sizeof(int)*sum_tree->capacity);
}

internal int getLines(BackingBuffer *buffer)
{
	int len = binsumtree_get_length(&buffer->lineSumTree)+1;
	return len;
}
internal int getLine(TextBuffer *textBuffer, int pos)
{
	
	DynamicArray_int *sum_tree = &textBuffer->backingBuffer->lineSumTree;
	//we don't fucking work at the end. fix me.	
	//incremental updates
	HistoryEntry entry;
	while (next_history_change(textBuffer->backingBuffer, &getLine, &entry))
	{
		switch (entry.action)
		{
			case action_undelete:
			case action_add:
			{
				if (isLineBreak(entry.character))
				{
					int leef_index;
					if (!binsumtree_index_from_leef_index(sum_tree, entry.location.line, &leef_index))assert(false && "this shouldn't happen...");
					int len = sum_tree->start[leef_index];

					binsumtree_set(sum_tree, entry.location.line, entry.location.column +1); // we count the linebreak on the current line aswell. (assuming the linebreak is one byte long dude what about /r/n?)
					binsumtree_insert(sum_tree, entry.location.line+1, len-entry.location.column);
				}
				else
				{
					binsumtree_set_relative(sum_tree, entry.location.line, +1);
				}
			}break;
			
			case action_delete:
			{
				if (isLineBreak(entry.character))
				{
					int leef_index;
					if (!binsumtree_index_from_leef_index(sum_tree, entry.location.line, &leef_index))assert(false && "this shouldn't happen...");
					int len = sum_tree->start[leef_index + 1] - 1;
					binsumtree_set_relative(sum_tree, entry.location.line, len);
					binsumtree_remove(sum_tree, entry.location.line + 1);
				}
				else
				{
					binsumtree_set_relative(sum_tree, entry.location.line, -1);
				}
			}break;
			case action_remove:
			{
				if (isLineBreak(entry.character))
				{
					int leef_index;
					if (!binsumtree_index_from_leef_index(sum_tree, entry.location.line, &leef_index))assert(false && "this shouldn't happen...");
					int len = sum_tree->start[leef_index]-1;
					binsumtree_set_relative(sum_tree, entry.location.line-1, len);
					binsumtree_remove(sum_tree, entry.location.line);
				}
				else
				{
					binsumtree_set_relative(sum_tree, entry.location.line, -1);
				}
			}break;
		}
		
	}
	int res_index;
	if (!binsumtree_search(sum_tree, pos, &res_index))
	{
		res_index = binsumtree_get_length(sum_tree); //is this right???
	}
	return res_index;
}

internal int getLineStart(BackingBuffer *backingBuffer, int line)
{
	int start = binsumtree_getSumUntil(&backingBuffer->lineSumTree, line);
	return start;
}

internal Location getLocation(TextBuffer *buffer, int pos)
{
	Location loc;
	loc.line = getLine(buffer, pos);
	int start = getLineStart(buffer->backingBuffer, loc.line);
	loc.column = pos - start;
	return loc;
}

internal int getLineFromIterator(TextBuffer *buffer, MGB_Iterator it)
{
	int pos = getIteratorPos(buffer->backingBuffer->buffer, it);
	return getLine(buffer, pos);
}

internal int getLineFromCaret(TextBuffer *buffer, int caretId)
{
	int pos = getCaretPos(buffer->backingBuffer->buffer, caretId);
	return getLine(buffer, pos);
}

internal Location getLocationFromCaret(TextBuffer *buffer, int caretId)
{
	int pos = getCaretPos(buffer->backingBuffer->buffer, caretId);
	return getLocation(buffer, pos);
}

internal float getCurrentScale(TextBuffer *textBuffer, int caretIndexId)
{
	return scaleFromLine(textBuffer,getLineFromCaret(textBuffer, textBuffer->ownedCarets_id.start[caretIndexId]));
}

//---Character Functions---

internal bool isLineBreak(char32_t codepoint)
{
	// a vertical is not a linebreak btw.. (what does c say??)
	return codepoint == 0x000A || codepoint == 0x000C || codepoint == 0x000D|| codepoint == 0x0085 || codepoint == 0x2028 || codepoint == 0x2029;
}
internal bool isVerticalTab(char c)
{
	return c == 0x00B;
}

internal bool isSpace(char i)
{
	return i == ' ';
}

internal bool isWordDelimiter(char i)
{
	return !(iswalpha(i) || iswdigit(i));
}

internal bool isFuncDelimiter(char i)
{
	if (i == '_')return false;
	return isWordDelimiter(i);
}





internal bool validSelection(TextBuffer *buffer)
{
	for (int i = 0; i < buffer->ownedSelection_id.length; i++)
	{
		MultiGapBuffer *mg_buffer = buffer->backingBuffer->buffer;
		int caret_id = buffer->ownedCarets_id.start[i];
		int selection_id = buffer->ownedSelection_id.start[i];
		
		int carPos = getCaretPos(mg_buffer, caret_id);
		int selPos = getCaretPos(mg_buffer, selection_id);
		if (selPos != carPos) 
		{
			return true;
		}
	}
	return false;
}

internal void checkIdsExist(TextBuffer *textBuffer)
{
#ifdef DEBUG
	for (int i = 0; i < textBuffer->ownedCarets_id.length; i++)
	{
		assert(indexFromId(textBuffer->backingBuffer->buffer, textBuffer->ownedCarets_id.start[i])!=-1 && "id doens't exist...");
	}
#endif
}


void removeOwnedEmptyCarets(TextBuffer *textBuffer)
{
	removeEmpty(textBuffer);
	checkIdsExist(textBuffer);
}

// ---MODIFY BUFFER---
//global_variable int currentCaretIndex = 0;
internal void appendCharacter(TextBuffer *textBuffer, char character, int caretIdIndex, bool log) //Bool log, who the fuck am I?
{
	int caretId = textBuffer->ownedCarets_id.start[caretIdIndex];
	Location loc;
	if (log)
		loc = getLocationFromCaret(textBuffer, caretId);
	if (log)
		logAdded(&textBuffer->backingBuffer->history, character, caretIdIndex,loc);
	
	appendCharacter(textBuffer->backingBuffer->buffer, caretId, character);
	markPreferedCaretXDirty(textBuffer, caretIdIndex);
}


internal void appendCharacter(TextBuffer *textBuffer, char character)
{
	removeSelection(textBuffer);
	for (int i = 0; i < textBuffer->ownedCarets_id.length; i++)
	{
		appendCharacter(textBuffer, character, i, true);
	}
	setNoSelection(textBuffer,do_log);
}

internal bool removeCharacter(TextBuffer *textBuffer, int caretIdIndex, bool log)
{
	int caretId = textBuffer->ownedCarets_id.start[caretIdIndex];
	Location loc;
	if(log)
		loc= getLocationFromCaret(textBuffer, caretId);
	char toBeRemoved = *get(textBuffer->backingBuffer->buffer, caretId, dir_left);
	bool removed = removeCharacter(textBuffer->backingBuffer->buffer, caretId);
	markPreferedCaretXDirty(textBuffer, caretIdIndex);
	if (removed && log)
	{
		logRemoved(&textBuffer->backingBuffer->history, toBeRemoved, caretIdIndex,loc );
	}
	return removed;
}

internal void removeCharacter(TextBuffer *textBuffer, bool log)
{
	for (int i = 0; i < textBuffer->ownedCarets_id.length; i++)
	{
		removeCharacter(textBuffer, i, log);
	}
	setNoSelection(textBuffer, log ? do_log : no_log);
	removeOwnedEmptyCarets(textBuffer);
}

internal void unDeleteCharacter(TextBuffer *textBuffer,  char character, int caretIdIndex)
{
	//NOTE: this is not ment for user interaciton strictly for undoing 
	//previous deletes. As such we do not need to check that the following state is OK.
	invDelete(textBuffer->backingBuffer->buffer, textBuffer->ownedSelection_id.start[caretIdIndex], character);
}

internal void unDeleteCharacter(TextBuffer *textBuffer, char character)
{
	removeSelection(textBuffer);
	for (int i = 0; i < textBuffer->ownedCarets_id.length; i++)
	{
		unDeleteCharacter(textBuffer, i, character);
	}
	setNoSelection(textBuffer,no_log); //right log??
}

internal bool deleteCharacter(TextBuffer *textBuffer, int caretIdIndex, bool log)
{

	int caretId = textBuffer->ownedCarets_id.start[caretIdIndex];
	Location loc;
	if(log) loc = getLocationFromCaret(textBuffer, caretId);
	char toBeDeleted = *get(textBuffer->backingBuffer->buffer, caretId, dir_right);
	bool deleted = del(textBuffer->backingBuffer->buffer, caretId);
	if (deleted && log)
	{
		logDeleted(&textBuffer->backingBuffer->history, toBeDeleted, caretIdIndex, loc);
	}
	return deleted;
}

internal void deleteCharacter(TextBuffer *textBuffer, bool log)
{
	for (int i = 0; i < textBuffer->ownedCarets_id.length; i++)
	{
		deleteCharacter(textBuffer, i, log);
	}
	setNoSelection(textBuffer, log? do_log: no_log);
	removeOwnedEmptyCarets(textBuffer);
}


internal bool remove(TextBuffer *textbuffer, bool log, Direction direction, int caretIdIndex)
{
	if (direction == dir_left)
	{
		return removeCharacter(textbuffer, caretIdIndex, log);
	}
	if (direction == dir_right)
	{
		return deleteCharacter(textbuffer, caretIdIndex, log);
	}
	assert(false);
	return false;
}

// ---MOVE---

internal int bytes_of_codepoint(int cursorId, MultiGapBuffer *mgb, Direction dir)
{
	if (dir == dir_left)
	{
		//plus one is for the assymetrical seek stuff. Brilliant. 
		char *pos = get(mgb, cursorId, dir_left) + 1;
		char *ret = (char *)seeking_rewind(pos, 1000, -1);
		return pos - ret;
	}
	else // dir_right
	{
		char *pos = get(mgb, cursorId, dir_right);
		char *ret = (char *)seeking_forward(pos, 1000, 1);
		return  ret - pos;
	}

}


internal bool can_break_grapheme_cluster(char32_t codepoint_a, char32_t codepoint_b)
{
#define CR 0x000D //carage return
#define LF 0x000A //line feed
#define  ZWJ 0x200D //zero width joiner
#define ZWNJ 0x200C //zero width nonjoiner

	
	int general_category_a = PROPERTY_GET_GC(codepoint_a);
	int general_category_b = PROPERTY_GET_GC(codepoint_b);

	// !! GB1 !!
	if (codepoint_a == 0 || codepoint_b == 0)return true;
	// !! GB2 !!

	//GB3
	if (codepoint_a == CR && codepoint_b == LF)return false;
	//GB 4
	if ((general_category_a & UTF8_CATEGORY_CONTROL || codepoint_a == CR|| codepoint_a == LF)) return true;
	//GB 5
	if ((general_category_b & UTF8_CATEGORY_CONTROL || codepoint_b == CR || codepoint_b == LF)) return true;

	// !! GB6 !!
	// !! GB7 !!
	// !! GB8 !!
	//GB 9
	if ((general_category_b & (UTF8_CATEGORY_MARK_NON_SPACING | UTF8_CATEGORY_MARK_ENCLOSING)) || codepoint_b == ZWJ || codepoint_b == ZWNJ) return false;

	return true;
}
internal int bytes_of_grapheme_cluster(int cursorId, MultiGapBuffer *mgb, Direction dir)
{
	char32_t codepoint_a = 0;
	char32_t codepoint_b = 0;

	int ack=0;
	MGB_Iterator it = getIteratorFromCaret(mgb, cursorId);

	if (dir == dir_left)
	{
		if(!MoveIterator(mgb,&it,-bytes_of_codepoint(cursorId, mgb, dir)))return 0;
		ack += getCodepoint(mgb, it, &codepoint_a);
		for (;;)
		{
			if (!MoveIterator(mgb, &it, -bytes_of_codepoint(cursorId, mgb, dir)))return ack;
			int read = getCodepoint(mgb, it, &codepoint_b);
			if(can_break_grapheme_cluster(codepoint_b, codepoint_a))return ack;
			else {
				ack += read;
				codepoint_a = codepoint_b;
			}
		} 
	}
	else //dir_right
	{
		assert(dir == dir_right);
		ack += getCodepoint(mgb, it, &codepoint_a);
		for (;;)
		{
			if (!MoveIterator(mgb, &it, bytes_of_codepoint(cursorId, mgb, dir)))return ack;
			int read = getCodepoint(mgb, it, &codepoint_b);
			if (can_break_grapheme_cluster(codepoint_a, codepoint_b)) return ack;
			else {
				ack += read;
				codepoint_a = codepoint_b;
			}
		}
	}
}


//low level no cleanup basically a wrapper to the multigapbuffer
internal bool move_llnc_(TextBuffer *textBuffer, Direction dir, int caretId, bool log, MoveMode mode)
{
	Location loc;
	if(log)
		loc = getLocationFromCaret(textBuffer, caretId);
	bool success = false;
	MultiGapBuffer *mgb = textBuffer->backingBuffer->buffer;
	int move_len;
	if (mode == movemode_byte)
		move_len = 1;
	else if (mode == movemode_codepoint)
		move_len = bytes_of_codepoint(caretId, mgb, dir);
	else if (mode == movemode_grapheme_cluster)
		move_len = bytes_of_grapheme_cluster(caretId, mgb, dir);

	if (dir == dir_left)
	{
		for (int i = 0; i < move_len; i++)
			if (mgb_moveLeft(mgb, caretId))
			{
				if (log)
					logMoved(&textBuffer->backingBuffer->history, dir, false, caretId, loc);
				success = true;
			}
	}
	else //dir_right
	{
		for (int i = 0; i < move_len; i++)
			if (mgb_moveRight(mgb, caretId))
			{
				if (log)
					logMoved(&textBuffer->backingBuffer->history, dir, false, caretId, loc);
				success = true;
			}
	}
	return success;
}

internal bool move_llnc(TextBuffer *textBuffer, Direction dir, int caretIdIndex, bool log, MoveType type,MoveMode mode)
{
	bool success=false;
	if (type == move_caret_both || type == move_caret_insert)
	{
		success |= move_llnc_(textBuffer, dir, textBuffer->ownedCarets_id.start[caretIdIndex], log,mode);
	}

	if (type == move_caret_both || type == move_caret_selection)
	{
		success |= move_llnc_(textBuffer, dir, textBuffer->ownedSelection_id.start[caretIdIndex], log, mode);
	}
	return success;
}



internal bool move_nc(TextBuffer *textBuffer, Direction dir, int caretIdIndex, log_ log, select_ selection, MoveMode mode)
{
	bool success = move_llnc(textBuffer, dir, caretIdIndex, log == do_log, move_caret_insert,mode);
	if (selection == no_select)
	{
		setNoSelection(textBuffer, caretIdIndex,log);
	}
	return success;
}


internal bool move(TextBuffer *textBuffer, Direction dir, int caretIdIndex, log_ log, select_ selection)
{
	bool success = move_nc(textBuffer, dir, caretIdIndex, log, selection,movemode_grapheme_cluster);
	markPreferedCaretXDirty(textBuffer, caretIdIndex);
	removeOwnedEmptyCarets(textBuffer);
	return success;
}

internal void move(TextBuffer *textBuffer, Direction dir, log_ log, select_ selection)
{
	for (int i = 0; i < textBuffer->ownedCarets_id.length; i++)
	{
		move_nc(textBuffer,dir, i, log, selection,movemode_grapheme_cluster);
		markPreferedCaretXDirty(textBuffer, i);
	}
	removeOwnedEmptyCarets(textBuffer);
}


internal char charAtDirOfCaret(MultiGapBuffer *buffer, Direction dir,int caretId)
{
	return *get(buffer, caretId, dir);
}

internal bool whileWord(char character, int *state)
{
	if (*state == 0)
	{
		if (!isSpace(character))
		{
			++*state;
		}
	}
	else
	{
		if (isWordDelimiter(character)) 
		{
			return false;
		}
	}
	return true;
}

internal bool whileSameLine(char character, int *state)
{
	return !isLineBreak(character);
}

internal void moveWhile(TextBuffer *textBuffer, select_	selection, log_ log, bool setPrefX, Direction dir, bool (*func)(char, int *))
{
	for (int i = 0; i < textBuffer->ownedCarets_id.length; i++)
	{
		int caretId = textBuffer->ownedCarets_id.start[i];
		int state = 0;
		while (func(charAtDirOfCaret(textBuffer->backingBuffer->buffer, dir, caretId), &state))
		{
			if (!move(textBuffer,dir,i, log, selection))
			{
				break;
			}
		
		}
		if(setPrefX)
			markPreferedCaretXDirty(textBuffer, i);
		else
			setPreferedX(textBuffer, i);
	}
	removeOwnedEmptyCarets(textBuffer);
}

internal void removeWhile(TextBuffer *textBuffer, bool log, Direction dir, bool(*func)(char, int *))
{
	for (int i = 0; i < textBuffer->ownedCarets_id.length; i++)
	{
		int state = 0;
		int caretId = textBuffer->ownedCarets_id.start[i];
		while (func(charAtDirOfCaret(textBuffer->backingBuffer->buffer, dir, caretId), &state))
		{
			if (!remove(textBuffer, log, dir, i))
			{
				break;
			}
		}
		markPreferedCaretXDirty(textBuffer, i);
	}
	removeOwnedEmptyCarets(textBuffer);
}
internal void clamp(int *i, int low, int high)
{
	if (*i < low)
	{
		*i = low;
	}
	else if (*i > high)
	{
		*i = high;
	}
}
internal int clamp(int i, int low, int high)
{
	if (i < low)
	{
		return low;
	}
	if (i > high)
	{
		return high;
	}
	return i;
}



internal void moveV_nc(TextBuffer *textBuffer, select_ selection, int caretIdIndex, bool up)
{
	setPreferedX(textBuffer, caretIdIndex);
	int caretId = textBuffer->ownedCarets_id.start[caretIdIndex];
	int line = getLineFromCaret(textBuffer, caretId);
	int targetLine = line + (up ? -1 : 1);
	assert(line >= 0);
	if (targetLine >= 0 && targetLine < getLines(textBuffer->backingBuffer))
	{
		gotoStartOfLine(textBuffer, targetLine, selection, caretIdIndex);
		moveCaretToX(textBuffer, getPreferedCaretX(textBuffer,caretIdIndex), selection, caretIdIndex);
	}
}
internal void moveUp_nc(TextBuffer *textBuffer, select_ selection, int caretIdIndex) //handle selecitons
{
	moveV_nc(textBuffer, selection, caretIdIndex, true);
}

internal void moveDown_nc(TextBuffer *textBuffer, select_ selection, int caretIdIndex)
{
	moveV_nc(textBuffer, selection, caretIdIndex, false);
}

internal void insertTab(TextBuffer *textBuffer)
{
	for (int i = 0; i < textBuffer->ownedCarets_id.length; i++)
	{
		int caretId = textBuffer->ownedCarets_id.start[i];
		// tabs are always inserted in the begining of a line
		// the alternative doesn't make sence to me, 
		// at least not when programming..
		int line = getLineFromCaret(textBuffer, caretId);
		char currentChar = getCaretPos(textBuffer->backingBuffer->buffer, caretId);
		int lineChar = getLineStart(textBuffer->backingBuffer,line);
		int diff = currentChar - lineChar;
		for (int j = 0; j<diff; j++)
		{
			move_nc(textBuffer,dir_left,i,do_log,no_select,movemode_byte);
		}
		appendCharacter(textBuffer, '\t');
		for (int j = 0; j<diff; j++)
		{
			move_nc(textBuffer,dir_right,i,do_log,no_select,movemode_byte);
		}
	}
}

internal void removeTab(TextBuffer *textBuffer)
{
	for (int i = 0; i < textBuffer->ownedCarets_id.length; i++)
	{
		int caretId = textBuffer->ownedCarets_id.start[i];
		int line = getLineFromCaret(textBuffer, caretId);
		char currentChar = getCaretPos(textBuffer->backingBuffer->buffer, caretId);
		int lineChar = getLineStart(textBuffer->backingBuffer, line);
		int diff = currentChar - lineChar;
		for (int j = 0; j<diff; j++)
		{
			move_nc(textBuffer,dir_left,i,do_log,no_select,movemode_byte);
		}
		if (*get(textBuffer->backingBuffer->buffer, caretId,dir_right)== '\t')
		{
			deleteCharacter(textBuffer,i);
		}
		for (int j = 0; j<diff-1; j++)
		{
			move_nc(textBuffer,dir_right,i,do_log,no_select, movemode_byte);
		}
	}
} 

internal void gotoCharacter(TextBuffer *textBuffer, int target,int caretIdIndex)
{
	int current = getCaretPos(textBuffer->backingBuffer->buffer, textBuffer->ownedCarets_id.start[caretIdIndex]);;
	int diff = target - current;
	
	for (int i = 0; i <-diff; i++)
	{
		move_nc(textBuffer,dir_left,caretIdIndex,do_log,no_select,movemode_byte);
	}
	for (int i = 0; i < diff; i++)
	{
		move_nc(textBuffer,dir_right,caretIdIndex,do_log,no_select, movemode_byte);
	}
}

internal char *getCharacter(MultiGapBuffer *buffer, int target)
{
	return get(buffer,target);

}

internal char *getInitialCharacter(MultiGapBuffer *buffer)
{
	return buffer->start;
}

//win32 forwardDelc

// ---- stb TrueType stuff

internal void setUpTT(stbtt_fontinfo *allocationInfo, char *font_path)
{
	long size;
	FILE* fontFile = fopen(font_path, "rb");

	if (!fontFile)return;
	fseek(fontFile, 0, SEEK_END);
	size = ftell(fontFile);
	fseek(fontFile, 0, SEEK_SET);
	
	unsigned char* fontBuffer;
	fontBuffer = (unsigned char*)alloc_(size,"fontBuffer");
	fread(fontBuffer, size, 1, fontFile);
	fclose(fontFile);
	stbtt_InitFont(allocationInfo, fontBuffer, 0);
}

internal int getGlyph(Typeface::Font *font, char32_t codepoint)
{
	int *p_glyph_index;
	int glyph_index;
	if (lookup(&font->cachedGlyphs, codepoint, &p_glyph_index))
	{
		glyph_index = *p_glyph_index;
	}
	else
	{
		glyph_index = stbtt_FindGlyphIndex(font->font_info, codepoint);
		insert(&font->cachedGlyphs, codepoint, glyph_index);
	}
	return glyph_index;
}

internal int getCharacterWidth_std(char32_t currentChar, char32_t nextChar, Typeface::Font *font, float scale)
{
	if (currentChar == VK_TAB)
	{
		return getCharacterWidth_std(' ', ' ', font, scale) * 4;
	}
	int advanceWidth, leftSideBearing;
	int glyph_current = getGlyph(font,currentChar);
	int glyph_next = getGlyph(font,nextChar);

	stbtt_GetGlyphHMetrics(font->font_info, glyph_current, &advanceWidth, &leftSideBearing);
	int kerning = stbtt_GetGlyphKernAdvance(font->font_info, glyph_current, glyph_next);
	return (advanceWidth*scale + kerning*scale) * 3;
}
struct MGB_Iterator; //bacuse vs don't know how to highlight otherwise..


internal int getCharacterWidth(TextBuffer *buffer, MGB_Iterator it, char32_t current_char, char32_t next_char,  Typeface::Font *font, float scale)
{
	//currently about O(to fucking much). I'm just fucking lazy though. (I don't memoize anything)
	MultiGapBuffer *mgb= buffer->backingBuffer->buffer;

	for (int i = 0; i < buffer->contextCharWidthHook.length; i++)
	{
		if (buffer->contextCharWidthHook.start[i].character == current_char)
		{
			return buffer->contextCharWidthHook.start[i].func(buffer,it, current_char, next_char, font, scale);
		}
	}
	
	return getCharacterWidth_std(current_char, next_char, font, scale);
}


enum FileEncoding
{
	file_encoding_utf8,
	file_encoding_utf16,
	file_encoding_utf32,
	file_encoding_unknown,

};

FileEncoding recognize_file_encoding(void *start, size_t len, size_t *_out_length_as_utf8)
{
	int32_t error;
	utf8toutf16((const char*)start, len, 0, 0, &error);
	if (error == UTF8_ERR_NONE) { *_out_length_as_utf8 = len; return file_encoding_utf8; }
	
	*_out_length_as_utf8 = utf16toutf8((uint16_t *)start, len, 0, 0, &error);
	if (error == UTF8_ERR_NONE) { return file_encoding_utf16; }
	
	*_out_length_as_utf8 = utf32toutf8((uint32_t *)start, len, 0, 0, &error);
	if (error == UTF8_ERR_NONE) { return file_encoding_utf32; }
	return file_encoding_unknown;
}


bool openFile_mapping(TextBuffer *buffer, DHSTR_String string)
{
	bool success = false;
	HANDLE file_handle = CreateFileW(DHSTR_WCHART_FROM_STRING(string,alloca), GENERIC_READ, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	LARGE_INTEGER lisize = {};
	if (!file_handle)goto cleanup_file_handle;
	if (!GetFileSizeEx(file_handle, &lisize)) goto cleanup_file_handle;
	uint64_t file_size = (uint64_t)lisize.QuadPart;
	if (!file_size) { success = true; goto cleanup_file_handle; } // file mappings don't like zero lenght files. but we have allready 'opened' it is it's fine... 
	HANDLE mapping_handle = CreateFileMapping(file_handle, 0, PAGE_READONLY | SEC_COMMIT, 0, 0, 0);
	if (!mapping_handle)goto cleanup_mapping_handle;
	char *file_start = (char *)MapViewOfFile(mapping_handle, FILE_MAP_READ, 0, 0, 0);
	if (!file_start)goto cleanup_view;

	uint64_t buffer_size;
	MultiGapBuffer *mgb = buffer->backingBuffer->buffer;
	__try {
		size_t needed_size;
		FileEncoding encoding = recognize_file_encoding(file_start, file_size, &needed_size);

		growTo(buffer->backingBuffer->buffer, needed_size*2);

		char *buffer_write_start = mgb->start;
		size_t buffer_max_write = needed_size;

		//@fixme, cursor should be at start.
		switch (encoding)
		{
			int32_t errors;
			case file_encoding_unknown:
			case file_encoding_utf8:
			{
				memcpy(buffer_write_start, file_start, file_size);
			}break;
			case file_encoding_utf16:
			{
				utf16toutf8((const uint16_t *)file_start, file_size, buffer_write_start, buffer_max_write, &errors);
				assert(!errors);
			}break;
			case file_encoding_utf32:
			{
				utf32toutf8((const uint32_t *)file_start, file_size, buffer_write_start, buffer_max_write, &errors);
				assert(!errors);
			}break;
		}
	
		buffer->backingBuffer->buffer->blocks.start[0].length = needed_size;
		buffer->backingBuffer->buffer->blocks.start[0].start = 0;
		success = true;
	}
	__except (GetExceptionCode() == EXCEPTION_IN_PAGE_ERROR ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH)
	{
		// do shit all I guess??
		dprs("failed to read from file, page_error");
	}
cleanup_view:
	UnmapViewOfFile(file_start);
cleanup_mapping_handle:
	CloseHandle(mapping_handle);
cleanup_file_handle:
	CloseHandle(file_handle);
	return success;
}




internal bool openFile(TextBuffer *textBuffer, DHSTR_String fileName)
{
	//Failure Point
	return openFile_mapping(textBuffer, fileName);
	//platform dependant. REFACTOR / MOVE 
	//silly stupid slow method..
	//fix this at some point?-
	/*
	int c;
	FILE *file;
	file = _wfopen((const wchar_t *)fileName, L"r");

	if (file)
	{
		int len = 0;
		while ((c = getc(file)) != EOF)
		{
			appendCharacter(textBuffer, c,0,false);
			++len;
		}
		fclose(file);

		for (int i = 0; i < len; i++)
		{
			move_nc(textBuffer,dir_left,0, no_log, no_select);
		}

		textBuffer->fileName = fileName;
		return true;
	}
	return false;
	*/
}


void attatch_BackingBuffer(TextBuffer *textBuffer, BackingBuffer *backingBuffer)
{
	++backingBuffer->ref_count;
	textBuffer->backingBuffer = backingBuffer;
}


//obviously if you're going to change to which view the backingbuffer belongs attatch before you detatch, otherwise it'll be freed
void detach_BackingBuffer(TextBuffer *textBuffer)
{
	BackingBuffer *backingBuffer= textBuffer->backingBuffer;
	--backingBuffer->ref_count;
	if (!backingBuffer->ref_count)
	{
		arena_deallocate_all(backingBuffer->allocator);
	}
	textBuffer->backingBuffer = (BackingBuffer *)0;
}


internal BackingBuffer *allocBackingBuffer(int initialMultiGapBufferSize, int initialHistoryEntrySize)
{
	DH_Allocator allocator = arena_allocator(allocateArena(KB(64), platform_allocator, "BackingBuffer arena"));
	BackingBuffer *buffer = (BackingBuffer *)Allocate(allocator, sizeof(BackingBuffer), "textBuffer: backingBuffer");
	*buffer = {};
	buffer->allocator = allocator;
	MultiGapBuffer *stringBuffer = (MultiGapBuffer *)Allocate(allocator, sizeof(MultiGapBuffer), "multi gap buffer");
	*stringBuffer = createMultiGapBuffer(initialMultiGapBufferSize);
	History history = {};
	history.entries = constructDynamicArray_HistoryEntry(initialHistoryEntrySize, "history", allocator);
	history.branches = ORD_constructDynamicArray_HistoryBranch(initialHistoryEntrySize, "history branches", allocator);
	history.change_log = constructDynamicArray_HistoryChange(initialHistoryEntrySize, "history change log", allocator);
	history.events = ORD_constructDynamicArray_HistoryEventMarker(initialHistoryEntrySize, "history event markers", allocator);
	buffer->history = history;
	buffer->buffer = stringBuffer;
	buffer->bindingMemory = DHDS_constructHT(BindingIdentifier, PVOID, 20, allocator);
	buffer->binding_next_change = DHDS_constructHT(PVOID, HistoryChangeTracker, 20, allocator);
	return buffer;
}

DEFINE_HashTable(int, PVOID, silly_hash,int_eq);
global_variable HashTable_int_PVOID open_files = DHDS_constructHT(int, PVOID,128,default_allocator);
internal bool get_backingBuffer(char *file_name) 
{
	
}

Typeface::Font LoadFont(DHSTR_String path)
{
	Typeface::Font ret = {};
	ret.cachedBitmaps = DHDS_constructHT(ulli, CharBitmap, 256, default_allocator);
	ret.cachedGlyphs = DHDS_constructHT(char32_t, int, 256, default_allocator);
	ret.font_info = (stbtt_fontinfo *)Allocate(default_allocator, sizeof(stbtt_fontinfo), "fontinfo_info");
	ret.font_info->userdata = "stbtt internal allocation";
	setUpTT(ret.font_info,DHSTR_UTF8_FROM_STRING(path,alloca));
	stbtt_GetFontVMetrics(ret.font_info, &ret.ascent, &ret.descent, &ret.lineGap);
	ret.lineHeight = (ret.ascent - ret.descent + ret.lineGap);
	return ret;
}




#define DHEN_PAIR
#define DHEN_VALUES \
X(Regular,    hash_string_constexpr("regular"))\
X(Italic,     hash_string_constexpr("italic")) \
X(Bold,       hash_string_constexpr("bold")) \
X(Semi,       hash_string_constexpr("semi")) \
X(Demi,       hash_string_constexpr("demi")) \
X(Light,      hash_string_constexpr("light")) \
X(Condensed,  hash_string_constexpr("condensed")) \
X(Black,      hash_string_constexpr("black")) \


#define DHEN_PREFIX fontqual
#define DHEN_NAME FontQualifiers
#include "enums.h"
#undef DHEN_NAME 
#undef DHEN_PREFIX 
#undef DHEN_VALUES
#undef DHEN_PAIR
/*
FontQualifiers getQualifiers(char *name)
{
	int ret = 0;
	int ack;
	while (*name)
	{
		if (name == ' ')
		{
			for (int i = 0; i < ARRAY_LENGTH(FontQualifiers_value); i++)
			{
				if (fontQualifiers_value[i] == ack)
				{
					ret |= 
				}
			}
		}

	}
	return (FontQualifiers)ret;
}
*/

/*
FontQualifiers contins(char *s)
{
	while (*++s)
	{
	}
}
*/
Typeface LoadTypeface(int index)
{//fix me
	Typeface ret = {};
	AvailableTypeface avail = availableTypefaces.start[index];
	for (int i = 0; i < avail.member_len; i++)
	{
		if (DHSTR_eq(avail.members[i].name, DHSTR_MAKE_STRING("Regular"),string_eq_length_dont_care))
		{
			ret.Regular = LoadFont(avail.members[i].path);
			continue;
		}
		if (DHSTR_eq(avail.members[i].name, DHSTR_MAKE_STRING("Italic"), string_eq_length_dont_care))
		{
			ret.Italic = LoadFont(avail.members[i].path);
			continue;
		}
		if (DHSTR_eq(avail.members[i].name, DHSTR_MAKE_STRING("Bold"), string_eq_length_dont_care))
		{
			ret.Bold = LoadFont(avail.members[i].path);
			continue;
		}
		if (DHSTR_eq(avail.members[i].name, DHSTR_MAKE_STRING("Bold Italic"), string_eq_length_dont_care))
		{
			ret.BoldItalic = LoadFont(avail.members[i].path);
			continue;
		}
		if (DHSTR_eq(avail.members[i].name, DHSTR_MAKE_STRING("Demi Light"), string_eq_length_dont_care))
		{
			ret.DemiLight = LoadFont(avail.members[i].path);
			continue;
		}
		if (DHSTR_eq(avail.members[i].name, DHSTR_MAKE_STRING("Demi Bold"), string_eq_length_dont_care))
		{
			ret.Bold = LoadFont(avail.members[i].path);
			continue;
		}
		dprs(DHSTR_UTF8_FROM_STRING(avail.members[i].name,alloca));
	}
	ret.Regular = LoadFont(avail.members[10].path);

	assert(ret.Regular.font_info);
	if (!ret.BoldItalic.font_info)
	{
		if(ret.Italic.font_info)
			ret.BoldItalic = ret.Italic;
		else if(ret.Bold.font_info)
			ret.BoldItalic = ret.Bold;
		else
			ret.BoldItalic = ret.Regular;
	}
	
	if (!ret.DemiBold.font_info)
		ret.DemiBold = ret.Regular;
	if (!ret.DemiLight.font_info)
		ret.DemiLight = ret.Regular;

	if (!ret.Bold.font_info)
		ret.Bold = ret.DemiBold;
	if (!ret.Italic.font_info)
		ret.Italic = ret.DemiLight;

	assert(ret.Italic.font_info);
	assert(ret.Bold.font_info);

	return ret;
}
/*
struct MaybeTypeface
{
	bool exist;
	Typeface typeface;
};
DEFINE_DynamicArray(MaybeTypeface);
MaybeTypeface *loadedTypefaces = (MaybeTypeface *)0;


Typeface getTypeface(int index)
{
	if (!loadedTypefaces)
	{
		loadedTypefaces = (MaybeTypeface *)Allocate(platform_allocator, availableTypefaces.length*sizeof(MaybeTypeface),"loaded typeface");
	}
	if (!loadedTypefaces[index].exist)
	{
		loadedTypefaces[index].typeface = LoadTypeface(index);
		loadedTypefaces[index].exist = true;
	}
	return loadedTypefaces[index].typeface;
}
*/

struct MaybeFont
{
	bool exist;
	Typeface::Font font;
};

MaybeFont *loadedFonts = (MaybeFont *)0;
Typeface::Font *getFont(int index)
{
	if (!loadedFonts)
	{
		loadedFonts = (MaybeFont *)Allocate(platform_allocator, availableFonts.length*sizeof(MaybeFont), "loaded font");
	}
	if (!loadedFonts[index].exist)
	{
		loadedFonts[index].font= LoadFont(availableFonts.start[index].path);
		loadedFonts[index].exist = true;
	}
	return &loadedFonts[index].font;
}
Typeface::Font *getFont(DHSTR_String name)
{
	bool *rem = (bool *)alloca(availableFonts.length);
	memset(rem, true, availableFonts.length);
	int rem_len = availableFonts.length;
	for (int s = 0; s < name.length; s++)
	{
		for (int i = 0; i < availableFonts.length; i++)
		{
			if (!rem[i])continue;
			if (availableFonts.start[i].name.start[s] != name.start[s])
			{
				if (rem_len == 1)
				{
					dprs("could not load desired font, loaded:");
					dprs(availableFonts.start[i].name.start);
					return getFont(i);
				}
				else {
					rem[i] = false;
					--rem_len;
				}
			}
		}
	}
	for (int i = 0; i < availableFonts.length; i++)	{
	
		if (rem[i])
		{
			dprs("loaded font: ");
			dprs(DHSTR_UTF8_FROM_STRING(availableFonts.start[i].name,alloca));
			return getFont(i);
		}
	}
	assert(false && "cosmic rays, are strong in this one");
	return 0;
}

global_variable int running_textBuffer_id = 200;
internal TextBuffer allocTextBuffer(int initialMultiGapBufferSize, int initialLineInfoSize,int initialColorChangeSize, int initialHistoryEntrySize)
{
	//hold on are we not using the provided allocator???
	DH_Allocator allocator = arena_allocator(allocateArena(KB(64), platform_allocator, "textBuffer arena"));
	TextBuffer textBuffer = {};
	textBuffer.allocator = allocator;
	textBuffer.font = getFont(DHSTR_MAKE_STRING("Arial Unicode"));
	//textBuffer.font = getFont(DHSTR_MAKE_STRING("Segoe UI Symbol"));
	textBuffer.textBuffer_id = running_textBuffer_id++;
	textBuffer.bufferType = regularBuffer;
	textBuffer.KeyBindings = DHDS_constructDA(KeyBinding,20, allocator);
	textBuffer.ownedCarets_id = DHDS_constructDA(int,20, allocator);
	textBuffer.ownedSelection_id = DHDS_constructDA(int,20, allocator);
	textBuffer.cursorInfo = DHDS_constructDA(CursorInfo,20, allocator);
	textBuffer.contextCharWidthHook = DHDS_constructDA(ContextCharWidthHook,20, allocator);
	textBuffer.bindingMemory = DHDS_constructHT(BindingIdentifier, PVOID, 20, allocator);
	attatch_BackingBuffer(&textBuffer, allocBackingBuffer(initialMultiGapBufferSize, initialHistoryEntrySize));

	//that it already is a caret here is implicit. I'm not loving it.
	Add(&textBuffer.ownedCarets_id, textBuffer.backingBuffer->buffer->running_cursor_id-1);
	int caret = AddCaret(textBuffer.backingBuffer->buffer, textBuffer.textBuffer_id, 0);
	Add(&textBuffer.ownedSelection_id, caret);
	initLineSumTree(textBuffer.backingBuffer, allocator);
	return textBuffer;
}


void InsertCaret(TextBuffer *buffer, int pos, int index)  //for history reasons.
{
	//WHAT
	int id_sel = AddCaret(buffer->backingBuffer->buffer, buffer->textBuffer_id, pos);
	Insert(&buffer->ownedSelection_id, id_sel, index);
	int id = AddCaret(buffer->backingBuffer->buffer, buffer->textBuffer_id, pos);
	Insert(&buffer->ownedCarets_id, id, index);
	CursorInfo allocationInfo = {};
	Insert(&buffer->cursorInfo, allocationInfo, index);
}

void AddCaret(TextBuffer *buffer, int pos)
{ 
	int id_sel = AddCaret(buffer->backingBuffer->buffer, buffer->textBuffer_id, pos);
	Add(&buffer->ownedSelection_id, id_sel);
	int id = AddCaret(buffer->backingBuffer->buffer, buffer->textBuffer_id, pos);
	Add(&buffer->ownedCarets_id, id);
	logAddCaret(&buffer->backingBuffer->history, pos, pos, buffer->ownedCarets_id.length-1, getLocationFromCaret(buffer, id));
	CursorInfo allocationInfo = {};
	Add(&buffer->cursorInfo, allocationInfo);
}



internal void removeCaret(TextBuffer *buffer, int caretIdIndex, bool log)
{
	int pos_caret = getCaretPos(buffer->backingBuffer->buffer, buffer->ownedCarets_id.start[caretIdIndex]);
	int pos_selection = getCaretPos(buffer->backingBuffer->buffer, buffer->ownedSelection_id.start[caretIdIndex]);
	if (log)
		logRemoveCaret(&buffer->backingBuffer->history, pos_caret, pos_selection, caretIdIndex, getLocationFromCaret(buffer, buffer->ownedSelection_id.start[caretIdIndex]));

	removeCaret(buffer->backingBuffer->buffer, buffer->ownedCarets_id.start[caretIdIndex]);
	RemoveOrd(&buffer->ownedCarets_id, caretIdIndex);
	removeCaret(buffer->backingBuffer->buffer, buffer->ownedSelection_id.start[caretIdIndex]);
	RemoveOrd(&buffer->ownedSelection_id, caretIdIndex);
}

internal TextBuffer cloneTextBuffer(TextBuffer *buffer)
{
	TextBuffer ret = {};
	assert(false);
	return ret;
	/*
	stbtt_fontinfo *fontinfo = (stbtt_fontinfo *)alloc_(sizeof(stbtt_fontinfo), "fontinfo_info");
	*fontinfo = {};
	fontinfo->userdata = (void *)"stbtt_allocation";
	Typeface font = {};
	font.font_info = fontinfo;
	ret.ownedCarets_id = constructDynamicArray_int(20, "ownedCaretIds");
	ret.ownedSelection_id = constructDynamicArray_int(20, "ownedSelectionIds");
	ret.cursorInfo= constructDynamicArray_CursorInfo(20, "cursor info");
	 
	ret.KeyBindings = constructDynamicArray_KeyBinding(20, "keyBindings");
	ret.contextCharWidthHook = constructDynamicArray_ContextCharWidthHook(20, "contextCharWidthHook");
	ret.bufferType = buffer->bufferType;
	ret.backingBuffer = buffer->backingBuffer;
	ret.font= font;
	ret.fileName = strcpy(buffer->fileName);
	AddCaret(&ret, getCaretPos(buffer->backingBuffer->buffer, buffer->ownedCarets_id.start[0]));
	initTextBuffer(&ret);
	return ret;
	*/
	
}


internal void freeTextBuffer(TextBuffer buffer)
{
	detach_BackingBuffer(&buffer);
	arena_deallocate_all(buffer.allocator);
	/*
	freeMultiGapBuffer(buffer.backingBuffer->buffer);
	free_(buffer.font.font_info);
	*/
}

internal void closeTextBuffer(Data *data)
{
	int index = data->activeTextBufferIndex;
	TextBuffer textBuffer = data->textBuffers.start[index];
	freeTextBuffer(textBuffer);
	Remove(&data->textBuffers, index);
	data->activeTextBufferIndex = 0;
	data->updateAllBuffers = true;
}

internal TextBuffer openCommanLine()
{
	TextBuffer textBuffer = allocTextBuffer(2000, 2000, 200, 200);
	textBuffer.bufferType = commandLineBuffer;
	initTextBuffer(&textBuffer);
	ColorChange cc = {};
	cc.color = foregroundColor;
	cc.index = 0;
	return textBuffer;
}

// I don't really like this function
// return a pointer, null if failed?
// now we're returning garbage... oh here's a pointer to free_d mem
// do what you will... It's not very good at all..


internal TextBuffer openFileIntoNewBuffer(DHSTR_String fileName, bool *success)
{
	TextBuffer textBuffer = allocTextBuffer(2000, 2000, 2000, 20);

	initTextBuffer(&textBuffer);
	*success = openFile(&textBuffer, fileName);
	if(success)
	{
		textBuffer.fileName = fileName; //COPY ME??
		redoLineSumTree(textBuffer.backingBuffer);
	} else
	{
		freeTextBuffer(textBuffer);
	}
	return textBuffer;
}

internal void getFileWriteTime_PLATFORM(char *fileName)
{
	HANDLE hFile = CreateFileW((wchar_t *)fileName, GENERIC_READ, 0, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
	FILETIME *fileTime;
	GetFileTime(hFile, 0, 0, fileTime);
}



internal void saveFile_PLATFORM(MultiGapBuffer *buffer, DHSTR_String path)
{

	FILE *file = _wfopen(DHSTR_WCHART_FROM_STRING(path, alloca), L"w");

	if (file)
	{
		for (int i = 0; i < buffer->blocks.length; i++)
			if (buffer->blocks.length != 0)
				fwrite(start_of_block(buffer,i), sizeof(char), length_of_block(buffer,i), file);
	}

	fclose(file);
}


internal void saveFile(TextBuffer *textBuffer)
{
	if(textBuffer->fileName.start)
		saveFile_PLATFORM(textBuffer->backingBuffer->buffer, textBuffer->fileName);
}
internal void hideCommandLine(Data *data);

internal void createFile(Data *data, DHSTR_String name)
{
	TextBuffer textBuffer = allocTextBuffer(2000, 2000, 2000, 2000);
	textBuffer.fileName = name;
	initTextBuffer(&textBuffer);
	Add(&data->textBuffers, textBuffer);
	saveFile(&textBuffer);
	data->updateAllBuffers = true;
	hideCommandLine(data);
}

// ----	colors

internal int grayScaleToColor(uint8 org, int foregroundColor, int backgroundColor)
{
	float blend =  org    / (float)255;
	float blendInv = ((uint8) 255-org) / (float)255;

	uint8 r_a = foregroundColor >> 16 & 0x000000ff;
	uint8 g_a = foregroundColor >> 8  & 0x000000ff;
	uint8 b_a = foregroundColor		  & 0x000000ff;

	uint8 r_b = backgroundColor >> 16 & 0x000000ff;
	uint8 g_b = backgroundColor >> 8  & 0x000000ff;
	uint8 b_b = backgroundColor		  & 0x000000ff;

	uint8 r = (r_a * blend + r_b * blendInv);
	uint8 g = (g_a * blend + g_b * blendInv);
	uint8 b = (b_a * blend + b_b * blendInv);
	
	return r <<16 | g << 8  | b;
}

internal int grayScaleToColorFaster(uint8 alpha, int foregroundColor, int backgroundColor)
{
	//stack overflow copy pasta. 
	// it's slightly off, >>8 divs with 256, should div with 255 but that's allright
	// actually it's not, it's cleary visible and it looks like crap. 
	// or does it have additional bugs?

	//it is quite clever to calculate r & b at the same time 
	//SIMD this shit when I wan't more speed 

	unsigned int rb1 = ((0x100 - alpha) * (backgroundColor & 0xFF00FF)) >> 8;
	unsigned int rb2 = (alpha * (foregroundColor & 0xFF00FF)) >> 8;
	unsigned int g1 = ((0x100 - alpha) * (backgroundColor & 0x00FF00)) >> 8;
	unsigned int g2 = (alpha * (foregroundColor & 0x00FF00)) >> 8;
	return ((rb1 | rb2) & 0xFF00FF) + ((g1 | g2) & 0x00FF00);
}



// ---- Bitmaps

internal Bitmap subBitmap(Bitmap bitmap, int width, int height, int x, int y)
{
	Bitmap b = {};
	b.bytesPerPixel = bitmap.bytesPerPixel;
	b.height = min(height, bitmap.height - min(bitmap.height, y));
	b.width = min(width, bitmap.width- min(bitmap.width, x));
	b.stride = bitmap.stride;
	b.memory = ((char *)bitmap.memory) + min(y,bitmap.height)*bitmap.stride + min(x, bitmap.width) * bitmap.bytesPerPixel;
	return b;
}
internal Bitmap subBitmap(Bitmap bitmap, Rect rect)
{
	return subBitmap(bitmap, rect.width, rect.height, rect.x, rect.y);
}

internal Bitmap getVerticalBitmapNumber(Bitmap bitmap, int bitmaps, int index)
{
	int start = bitmap.width*((float)index / bitmaps);
	int end = bitmap.width * ((float)(index + 1) / bitmaps);
	return subBitmap(bitmap, end - start, bitmap.height, start, 0);
}

internal void fillBitmap(TextBuffer *textBuffer, Bitmap bitmap, int lineOffset,int y)
{
	Typeface::Font tmp_font_to_make_things_work = *textBuffer->font;
	if (y > 0)
	{
		float scale = scaleFromLine(textBuffer, lineOffset);
		renderRect(bitmap, 0, 0, bitmap.width, tmp_font_to_make_things_work.lineHeight*scale, backgroundColorFromLine(textBuffer,lineOffset));
	}
	while (y < bitmap.height && lineOffset<getLines(textBuffer->backingBuffer))
	{
		float scale = scaleFromLine(textBuffer, lineOffset);
		renderRect(bitmap, 0, y, bitmap.width, tmp_font_to_make_things_work.lineHeight*scale, backgroundColorFromLine(textBuffer,lineOffset));
		y += tmp_font_to_make_things_work.lineHeight * scaleFromLine(textBuffer, lineOffset++);
	}
	if (y < bitmap.height)
	{
		renderRect(bitmap, 0, y, bitmap.width, bitmap.height - y, backgroundColorFromLine(textBuffer,0));
	} 
}

internal void clearBitmap(Bitmap bitmap, int color)
{
	int size = bitmap.width*bitmap.height*bitmap.bytesPerPixel;
#if 0
	memset(bitmap.memory, color, size);

	//memset is 5x way faster than the below
	//but only works on bytes... //so our background can only be gray...
	//also it deos not handle stride
	//allocate a clear slate buffer 
	//and use memcpy?? 
	//or maybe use fill 4 bytes and then in memcopy to twice the size and continue.. maybe?

#else
	uint8 *row = (uint8 *)bitmap.memory;
	for (int y = 0; y < bitmap.height; ++y)
	{
		uint32 *pixel = (uint32 *)row;
		for (int x = 0; x <bitmap.width; ++x)
		{
			*pixel++ = color;
		}
		row += bitmap.stride;
	}
#endif
}


#define getRed(color)((color >> 16) & 0x000000ff)
#define getGreen(color)((color >> 8) & 0x000000ff)
#define getBlue(color)(color & 0x000000ff)
#define createColor(alpha, red,green,blue)(alpha << 24 | red << 16 | green << 8 | blue);
#include "emmintrin.h"
#include "smmintrin.h" //sse 4 for that one pack32us instruction... maybe instead we just remove it??		

__m128i SSE_ONES = _mm_set1_epi8(255);
__m128i SSE_255 = _mm_set1_epi16(255);
__m128i SSE_0 = _mm_set1_epi8(0);
__m128i SSE_000000ff = _mm_set1_epi32(0x000000ff);
__m128i SSE_0000ff00 = _mm_set1_epi32(0x0000ff00);
__m128i SSE_1 = _mm_set1_epi16(1);
__m128i SSE_8 = _mm_set1_epi16(8);

__m128i SSE_00ff = _mm_set1_epi16(0xff00);
__m128i SSE_ff00 = _mm_set1_epi16(0x00ff);

internal inline __m128i div255(__m128i x)
{

	//#define div255(x) ((x+1+((x+1)>>8))>>8)
  	x = _mm_add_epi16(x, SSE_1); // inc x
	__m128i x_shift = _mm_srli_epi16(x, 8);
	x = _mm_srli_epi16(_mm_add_epi16(x, x_shift), 8); // x= ((x >> 8) + x) >> 8

	return x;
}
internal inline __m128i andlow16(__m128i x)
{
	return _mm_and_si128(x, SSE_ff00);
}


internal void blitChar(CharBitmap from, Bitmap to, int x, int y,int color)
{
	int xOffset = x + from.xOff;
	int yOffset = y + from.yOff;

	uint8 *toRow   = (uint8 *)to.memory + xOffset / 3 * to.bytesPerPixel+ yOffset*to.stride;

    uint8_t *tmp[3] =
	{
		from.memory,
		from.memory + from.colorStride * from.height,
		from.memory + from.colorStride * from.height * 2,
	};
	int xo = xOffset % 3;
	if (xo < 0) xo = (1-xo)%3;
	xo = 2 - xo;  

	if (xo >= 1) tmp[0]++;
	if (xo >= 2) tmp[1]++;

	uint8_t *colors[3] =
	{
		tmp[0 + xo],
		tmp[(1 + xo) % 3],
		tmp[(2 + xo) % 3],
	};
	
	__m128i color_r = _mm_set1_epi16(getRed(color));
	__m128i color_g = _mm_set1_epi16(getGreen(color));
	__m128i color_b = _mm_set1_epi16(getBlue(color));

	for(int y = 0; y<from.height; y++)
	{
		if((y+yOffset) >= to.height||(y+yOffset)< 0)
		{
			toRow += to.stride;
		}
		else
		{
			int *toPixel  = (int *)toRow;

			bool ok_line = false;
			for(int x = 0; x<from.width; x+=8)
			{
				if (!((x + xOffset/3) >= to.width || (x + xOffset) < 0))
				{
					
					__m128i alpha_r_packed_8 = _mm_loadu_si128((__m128i *) &(colors[0][x + y*from.colorStride]));
					__m128i alpha_g_packed_8 = _mm_loadu_si128((__m128i *) &(colors[1][x + y*from.colorStride]));
					__m128i alpha_b_packed_8 = _mm_loadu_si128((__m128i *) &(colors[2][x + y*from.colorStride]));
					
					__m128i alpha_r = _mm_unpacklo_epi8(alpha_r_packed_8, SSE_0);
					__m128i alpha_g = _mm_unpacklo_epi8(alpha_g_packed_8, SSE_0);
					__m128i alpha_b = _mm_unpacklo_epi8(alpha_g_packed_8, SSE_0);
					
					__m128i max = _mm_set1_epi16(from.width-x);
					__m128i count = _mm_set_epi16(7,6,5,4,3,2,1,0);
					__m128i write_mask = _mm_cmplt_epi16(count, max);

					alpha_r = _mm_and_si128(write_mask, alpha_r);
					alpha_g = _mm_and_si128(write_mask, alpha_g);
					alpha_b = _mm_and_si128(write_mask, alpha_b);

					__m128i inv_alpha_r = _mm_sub_epi16(SSE_255, alpha_r);
					__m128i inv_alpha_g = _mm_sub_epi16(SSE_255, alpha_g);
					__m128i inv_alpha_b = _mm_sub_epi16(SSE_255, alpha_b);

					__m128i src_1 = _mm_loadu_si128((__m128i *) toPixel);
					__m128i src_2 = _mm_loadu_si128((__m128i *) (toPixel+4));
					
					__m128i src_b = _mm_packus_epi32(_mm_and_si128(src_1,SSE_000000ff), _mm_and_si128(src_2, SSE_000000ff));
					__m128i src_g = _mm_srli_epi16(_mm_packus_epi32(_mm_and_si128(src_1, SSE_0000ff00), _mm_and_si128(src_2, SSE_0000ff00)),8);
					__m128i src_r = _mm_packus_epi32(_mm_and_si128(_mm_srli_epi32(src_1,16), SSE_000000ff), _mm_and_si128(_mm_srli_epi32(src_2, 16), SSE_000000ff));
					
					//alpha blending
					__m128i res_r = div255(_mm_add_epi16(_mm_mullo_epi16(color_r, alpha_r), _mm_mullo_epi16(src_r, inv_alpha_r)));
					__m128i res_g = div255(_mm_add_epi16(_mm_mullo_epi16(color_g, alpha_g), _mm_mullo_epi16(src_g, inv_alpha_g)));
					__m128i res_b = div255(_mm_add_epi16(_mm_mullo_epi16(color_b, alpha_b), _mm_mullo_epi16(src_b, inv_alpha_b)));
					__m128i res_a = _mm_set1_epi16(0);

					// unpack
					// a a a a| g g g g| r r r r| b b b b| -> |argbargb|argbargb| 

					__m128i aglo = _mm_unpacklo_epi16(res_g, res_a);
					__m128i aghi = _mm_unpackhi_epi16(res_g, res_a);
					__m128i rblo = _mm_unpacklo_epi16(res_b, res_r);
					__m128i rbhi = _mm_unpackhi_epi16(res_b, res_r);


					_mm_storeu_si128((__m128i *)(toPixel + 0), _mm_packus_epi16(_mm_unpacklo_epi16(rblo, aglo), _mm_unpackhi_epi16(rblo, aglo)));
					_mm_storeu_si128((__m128i *)(toPixel + 4), _mm_packus_epi16(_mm_unpacklo_epi16(rbhi, aghi), _mm_unpackhi_epi16(rbhi, aghi)));
				}
				toPixel+=8;
			}
			toRow += to.stride;
		}
	}	
}

internal int swizzle(int i)
{
	uint8_t r = (i & 0x000000ff)>>0;
	uint8_t g = (i & 0x0000ff00)>>8;
	uint8_t b = (i & 0x00ff0000)>>16;
	uint8_t a = (i & 0xff000000)>>24;

	return a<<24|r<<16|g<<8|b<<0;
}

internal void blitBitmap(Bitmap from, Bitmap to, int xOffset, int yOffset)
{
	uint8 *fromRow = (uint8 *)from.memory;
	uint8 *toRow = (uint8 *)to.memory + xOffset*to.bytesPerPixel + yOffset*to.stride;

	for (int y = 0; y<from.height; y++)
	{
		if ((y + yOffset) >= to.height || (y + yOffset)< 0)
		{
			fromRow += from.stride;
			toRow += to.stride;
		}
		else
		{
			int *fromPixel =(int *) fromRow;
			int  *toPixel = (int *)toRow;

			for (int x = 0; x<from.width; x++)
			{

				if (!((x + xOffset) >= to.width || (x + xOffset) < 0))
				{
					int color = swizzle(*fromPixel);
					*toPixel = grayScaleToColor(color>>24, color, *toPixel);
				}
				++fromPixel;
				++toPixel;
			}
			fromRow += from.stride;
			toRow += to.stride;
		}
	}
}


internal int ciel_divide(int top, int bottom)
{
	return (top + bottom - 1) / bottom;
}

internal inline int ciel_to_multiple_of(int i, int mult)
{
	return ciel_divide(i,mult) * mult;
}

internal uint64_t ciel_divide(uint64_t  top, int bottom)
{
	return (top + bottom - 1) / bottom;
}

internal uint64_t ciel_to_multiple_of(uint64_t i, int mult)
{
	return ciel_divide(i, mult) * mult;
}

internal inline int floor_to_multiple_of(int i, int mult)
{
	return (i / mult)*mult;
}

long long unsigned int charBitmapIdentifier(float scale, int codepoint)
{
	long long int int_scale = ((long long int)(scale * (1 << 16))) << 32;
	return codepoint | int_scale;
}


CharBitmap loadBitmap(int codepoint, float scale, Typeface::Font *font)
{
	int xOff, yOff;

	int glyph_index = getGlyph(font, codepoint);
	stbtt_fontinfo *font_info = font->font_info;

	//stride's additional 2 comes from where exactey, hadn't we allready taken that into account?
	int width, height;
	void *charBitmapMem = stbtt_GetGlyphBitmap(font_info, scale * 3, scale, glyph_index, &width, &height, &xOff, &yOff);
	float die_off = 1.2; // [1.5,2]
	float intensity = 1;
	float subpix_bleed[] = { .5f * intensity / 3, .5f *die_off *intensity / 3 , .5f *intensity / (3 * die_off) };

	int bleed_extra_per_row = ciel_divide((ARRAY_LENGTH(subpix_bleed) - 1) * 2, 3);
	int color_stride = (ciel_divide(width + 2, 3) + bleed_extra_per_row) + 1;

	uint8_t *new_mem = (uint8_t *)alloc_(color_stride * 3 * height, "character bitmap");
	memset(new_mem, 0, color_stride * 3 * height);
	uint8_t *row_from = (uint8_t *)charBitmapMem;
	uint8_t *colors[3] =
	{
		new_mem,
		new_mem + color_stride * height,
		new_mem + color_stride * height * 2
	};

	for (int y = 0; y < height; y++)
	{
		uint8_t *pix_from = row_from;

		for (int x = 0; x < width; x++)
		{
			//colors[x % 3][x/3 + y*color_stride+1] += *pix_from * subpix_bleed[0];
			for (int i = 0; i < (ARRAY_LENGTH(subpix_bleed)); i++)
			{
				uint8 *r = &colors[(x + i) % 3][(x + i + 3) / 3 + y*color_stride];
				uint8 *l = &colors[(x + 3 - i) % 3][(x - i + 3) / 3 + y*color_stride];
				int v = *pix_from *subpix_bleed[i];

				*r = ((int)*r) + v > 255 ? 255 : *r + v; // saturate which means we can add more than one total for every pixel (which we want to do)
				*l = ((int)*l) + v > 255 ? 255 : *l + v;
			}
			++pix_from;
		}
		row_from += width;
	}

	free_(charBitmapMem);
	CharBitmap b = {};
	b.character = codepoint;
	b.scale = scale;
	b.yOff = yOff;
	b.xOff = xOff;
	b.height = height;
	b.width = color_stride - 1;
	b.colorStride = color_stride;
	b.memory = new_mem;

	insert(&font->cachedBitmaps, charBitmapIdentifier(scale, codepoint), b);
	return b;
}

internal CharBitmap getCharBitmap(int codepoint, float scale, Typeface::Font *font)
{
	bool found = false;
	long long unsigned int ident = charBitmapIdentifier(scale,codepoint);
	CharBitmap *lookedup;
	if(lookup(&font->cachedBitmaps, ident, &lookedup)) 
	{
		return *lookedup;
	}
	
	// if we try to load a ton of unicode stuff it's way better to not render every char than to choke.
	// at some point we might want to go multithreaded
	// however this is not an unicode editor, it an editor with unicode support.
	if (QueryFrameUsed() > .8f)
		return {};
	else
		return loadBitmap(codepoint, scale, font); 
}


struct CharRenderingInfo
{
	int color;
	int x;
	int y;
	float scale;
	Typeface::Font *typeface;
};


internal void renderCharacter(Bitmap bitmap, char32_t code_point, int x, int y, float scale, int color, Typeface::Font *Font)
{
	if (code_point == VK_TAB || isVerticalTab(code_point)) return;

	int xOff, yOff;
	CharBitmap charBitmap = getCharBitmap(code_point, scale, Font);
	Bitmap output = { bitmap.memory, bitmap.width, bitmap.height, bitmap.stride, bitmap.bytesPerPixel };
	blitChar(charBitmap, output, x, y, color);
}


internal void renderCharacter(Bitmap bitmap, char32_t code_point, CharRenderingInfo rendering)
{
	renderCharacter(bitmap, code_point, rendering.x, rendering.y, rendering.scale, rendering.color, rendering.typeface);
}

internal void renderCaret(Bitmap bitmap, int x, int y,float scale,Typeface::Font *typeface,int color)
{
	int width = getCharacterWidth_std('|', 'A', typeface, scale);
	
	renderCharacter(bitmap, '|', x - width / 2, y,scale, color, typeface); //use ascent / descent probably.
}


//		----	LOGIC

internal void renderText(Bitmap bitmap, char* string, float scale, int color, int x, int y, Typeface::Font *font)
{
	int remaining_len = DHSTR_strlen(string);
	char32_t last_codepoint = 0;
	while (*string)
	{
		char32_t codepoint;
		remaining_len -= codepoint_read(string, remaining_len, (uint32_t *)&codepoint);
		if(last_codepoint)
			x += getCharacterWidth_std(last_codepoint,codepoint, font, scale);
		renderCharacter(bitmap, codepoint, x, y, scale, color, font);
		last_codepoint = codepoint;
		++string;
	}
}

internal void renderText(Bitmap bitmap, DHSTR_String string, float scale, int color, int x, int y, Typeface::Font *font)
{
	char32_t last_codepoint = 0;

	for(int i = 0; i < string.length;)
	{
		char32_t codepoint;
		int read = codepoint_read(&string.start[i], string.length-i, (uint32_t *)&codepoint);
		if(last_codepoint)
			x+= getCharacterWidth_std(last_codepoint, codepoint, font, scale);
		renderCharacter(bitmap, codepoint, x, y, scale, color, font);
		last_codepoint = codepoint;
		i += read;
	}
}






internal void renderRect(Bitmap bitmap, int xOffset, int yOffset, int width, int height, int color)
{
	char *row = (char *)bitmap.memory + yOffset*bitmap.stride +xOffset*bitmap.bytesPerPixel;
	width = min(width, bitmap.width - min(xOffset,bitmap.width));
	for (int y = 0; y<height; y++)
	{
		if ((y + yOffset) >= bitmap.height)
		{
			return;
		}
		if ((y + yOffset)< 0)
		{
			row += bitmap.stride;
		}
		else
		{
			memset(row, color, width * bitmap.bytesPerPixel);
			/*
			int  *pixel = (int *)row;
			for (int x = 0; x<width; x++)
			{
				*pixel++ = color;
			}
			*/
			row += bitmap.stride;
		}
	}
	
	/*
	char *row = (char *)bitmap.memory+yOffset*bitmap.stride+xOffset*bitmap.bytesPerPixel;
	for (int y = 0; y<height; y++)
	{
		if ((y + yOffset) >= bitmap.height)
		{
			return;
		}
		
		if ((y + yOffset)< 0)
		{
			row += bitmap.stride;
		}
		else
		{
			int  *pixel = (int *)row;
			
			for (int x = 0; x<width; x++)
			{
				if (!((x + xOffset) >= bitmap.width || (x + xOffset) < 0))
				{
					*pixel = color;
				}
				++pixel;
			}
			row += bitmap.stride;
		}
	}
	*/
}


internal void renderRectColor(Bitmap bitmap, int xOffset, int yOffset, int width, int height, int color)
{
	char *row = (char *)bitmap.memory+yOffset*bitmap.stride+xOffset*bitmap.bytesPerPixel;
	for (int y = 0; y<height; y++)
	{
		if ((y + yOffset) >= bitmap.height)
		{
			return;
		}

		if ((y + yOffset)< 0)
		{
			row += bitmap.stride;
		}
		else
		{
			int  *pixel = (int *)row;

			for (int x = 0; x<width; x++)
			{
				if (!((x + xOffset) >= bitmap.width || (x + xOffset) < 0))
				{
					*pixel = color;
				}
				++pixel;
			}
			row += bitmap.stride;
		}
	}
}

internal void renderRectColor(Bitmap bitmap, Rect rect, int color)
{
	renderRectColor(bitmap, rect.x, rect.y, rect.width, rect.height, color);
}


internal void renderRect(Bitmap bitmap, Rect rect, int color)
{
	renderRect(bitmap, rect.x, rect.y, rect.width, rect.height, color);
}

internal MGB_Iterator getIteratorAtLine(TextBuffer *textBuffer, int line)
{
	int start = getLineStart(textBuffer->backingBuffer, line);
	return getIterator(textBuffer->backingBuffer->buffer, start);
}



#if 0
internal void renderText(TextBuffer *textBuffer, Bitmap bitmap, int startLine,int x,int y,bool drawCaret)
{
	MGB_Iterator it = getIteratorAtLine(textBuffer, startLine);
	
	int ccIndex = 0;
	int currentIndex = textBuffer->backingBuffer->lineInfo.start[startLine].start;
	//array out of bounds potential
	while (textBuffer->colorChanges.start[ccIndex].index <= currentIndex)
	{
		++ccIndex;
	}
	clamp(&--ccIndex,0,textBuffer->colorChanges.length-1);

	textBuffer->caretX = -10;
	int caretY = -10;
	
	
	int orgX = x;
	bool ended = false;
	//if we are emty we still need to render the caret.. therfore we set up some default values here!
	//we might end before we even start...

	//int currentColor = textBuffer->colorChanges.start[ccIndex].color;
	int currentColor = foregroundColor;
	clamp(&++ccIndex, 0, textBuffer->colorChanges.length - 1);

	int currentLine=startLine;

	float currentScale= textBuffer->backingBuffer->lineInfo.start[currentLine].scale;
	float caretScale=currentScale;
	
	if (it.sub_index == 0 && it.block_index>0 && ownsIndexOrOnSame(textBuffer->backingBuffer->buffer, &textBuffer->ownedCarets_id, it.block_index-1))
	{
		textBuffer->caretX = 0;
		caretY = y;
		if (drawCaret)
			renderCaret(bitmap, textBuffer->caretX + orgX, caretY, caretScale, textBuffer->fontInfo, caretColor);
	}
	bool selection = false;
	if(length(textBuffer->backingBuffer->buffer))
	while (!ended)
	{
		if (ccIndex >= 0 && ccIndex <= textBuffer->colorChanges.length - 1)
		{
			//assert(false);
		}
		/*
		if (textBuffer->colorChanges.start[ccIndex].index == currentIndex)
		{
			currentColor = textBuffer->colorChanges.start[ccIndex].color;
			clamp(&++ccIndex, 0, textBuffer->colorChanges.length - 1);
		}
		*/
		
		char currentChar = *getCharacter(textBuffer->backingBuffer->buffer, it);
		
		int caret=-1;
		ended = !getNext(textBuffer->backingBuffer->buffer, &it, &caret);
		
		++currentIndex;
		if (isLineBreak(currentChar))
		{

			currentLine++;
			currentScale = textBuffer->backingBuffer->lineInfo.start[currentLine].scale;
			if (y > bitmap.height) {
				break;
			}
			y += textBuffer->lineHeight*currentScale;
			x = orgX;
		}
		else
		{
			char nextChar = ended ? *getCharacter(textBuffer->backingBuffer->buffer, it) : 'A'; // some default to avoid derefing invalid memory
			int width = getCharacterWidth(currentChar, nextChar,textBuffer->fontInfo, currentScale);

			//this is rendering the selection 
			//the first is inclusive. 
			if (selection)
			{
				renderRect(bitmap, x, y - textBuffer->ascent*currentScale, width, textBuffer->lineHeight*currentScale, highlightColor);
			}
			renderCharacter(bitmap, currentChar, x, y, currentScale,currentColor,textBuffer->fontInfo);
			x += width;
		}
		bool onCaret = ownsIndexOrOnSame(textBuffer->backingBuffer->buffer, &textBuffer->ownedCarets_id, caret);
		bool onSelection = ownsIndexOrOnSame(textBuffer->backingBuffer->buffer, &textBuffer->ownedSelection_id, caret);
		if (caret != -1)
		{
			if (onSelection != onCaret)
			{
				selection = !selection;
			}
		}
		if (caret != -1 && onCaret)
		{
			textBuffer->caretX = x-orgX;
			caretY = y;
			caretScale = currentScale;

			if (drawCaret)
				renderCaret(bitmap, textBuffer->caretX+orgX, caretY, caretScale, textBuffer->fontInfo, onCaret ? caretColor : caretColorDark);
		}
	}
}
#endif

internal void bigTitles(TextBuffer *textBuffer, CharRenderingInfo *settings, int currentLine)
{
	settings->scale = scaleFromLine(textBuffer, currentLine);
}
typedef void(*LineChangeHook)(TextBuffer *textBuffer, CharRenderingInfo *settings, int currentLine);



internal void renderText(TextBuffer *textBuffer, Bitmap bitmap, int startLine, int x, int y, bool drawCaret, 
	LineChangeHook *lineChangeFuncs=0, int NumberOfLineChangeFuncs=0)
{
	MGB_Iterator it = getIteratorAtLine(textBuffer, startLine);
	MGB_Iterator last_written_it = getIteratorAtLine(textBuffer, startLine);

	textBuffer->caretX = -10;
	int orgX = x;
	bool ended = false;

	int currentLine = startLine;
	CharRenderingInfo rendering = {};
	rendering.x = x;
	rendering.y = y;
	rendering.scale = scaleFromLine(textBuffer, currentLine);
	rendering.color = foregroundColor;
	rendering.typeface = textBuffer->font;

	char32_t last_codepoint=0;

	
	MultiGapBuffer *mgb = textBuffer->backingBuffer->buffer;

	bool selection = false;
	if (length(textBuffer->backingBuffer->buffer))
	{
		while (!ended)
		{
			bool onCaret = HasCaretAtIterator(textBuffer->backingBuffer->buffer, &textBuffer->ownedCarets_id, it);
			bool onSelection = HasCaretAtIterator(textBuffer->backingBuffer->buffer, &textBuffer->ownedSelection_id, it);

			char32_t codepoint;
			MGB_Iterator prev_it = it;

			int read = getCodepoint(mgb, it, &codepoint);
			ended = !MoveIterator(mgb, &it, read);
			
			bool lineBreak= isLineBreak(codepoint);

			{
				int width = 0;
				if (last_codepoint != 0){
					width = getCharacterWidth(textBuffer,prev_it,last_codepoint, codepoint, rendering.typeface, rendering.scale);
				}
				if (selection)
				{//lol we're effectively done in the next iteration.. :/
					renderRect(bitmap, (rendering.x + 2) / 3, rendering.y - rendering.typeface->ascent*rendering.scale, (width + 2) / 3, rendering.typeface->lineHeight*rendering.scale, highlightColor);
				}
				rendering.x += width;

				if(!lineBreak)
					renderCharacter(bitmap, codepoint, rendering);
			}
			selection = onSelection ^ onCaret ^ selection;

			
			//oh yea! Think about this for a while...
			//hint: if none of select and caret are here don't toggle selection
			//		if both toggle selection twice (ie zero times)
			//		otherwise toggle. 

			if (onCaret)
			{
				if (drawCaret)
					renderCaret(bitmap, rendering.x, rendering.y, rendering.scale, rendering.typeface, onCaret ? caretColor : caretColorDark);
				textBuffer->caretX = (rendering.x - orgX);
			}


			if (lineBreak)
			{
				currentLine++;
				for (int i = 0; i < NumberOfLineChangeFuncs; i++)
				{
					lineChangeFuncs[i](textBuffer, &rendering, currentLine);
				}
				if (rendering.y > bitmap.height) { break; }
				rendering.y += rendering.typeface->lineHeight*rendering.scale;
				rendering.x = orgX;
				codepoint = 0;
			}

			last_codepoint = codepoint;
		}
	}
}


internal Bitmap margin(Bitmap bitmap, int left, int right, int top, int bottom)
{
	Rect r = {};
	r.height = bitmap.height - top - bottom;
	r.width = bitmap.width - left - right;
	r.x = left;
	r.y = top;
	return subBitmap(bitmap, r);
}
internal Bitmap drawBorder(Bitmap bitmap, int left, int right, int top, int bottom, int color)
{
	if (top > 0)
	{
		Rect rect = {};
		rect.height = min(bitmap.height,top);
		rect.width = bitmap.width;
		rect.x = 0;
		rect.y =  0;
		renderRectColor(bitmap, rect, color);
	}
	if (bottom > 0)
	{
		Rect rect = {};
		rect.height = min(bitmap.height, bottom);
		rect.width = bitmap.width;
		rect.x = 0;
		rect.y = max(0,bitmap.height-bottom);
		renderRectColor(bitmap, rect, color);
	}
	if (left>0)
	{
		Rect rect = {};
		rect.height = bitmap.height;
		rect.width = min(bitmap.width, left);
		rect.x = 0;
		rect.y = 0;
		renderRectColor(bitmap, rect, color);
	}
	if (right>0)
	{
		Rect rect = {};
		rect.height = bitmap.height;
		rect.width = min(bitmap.width, right);
		rect.x = max(0,bitmap.width-right);
		rect.y = 0;
		renderRectColor(bitmap, rect, color);
	}
	return margin(bitmap, left, right, top, bottom);
}

internal void renderCommandLine(Bitmap bitmap, Data data)
{
	TextBuffer *buffer = data.commandLine;
	int LH = buffer->font->lineHeight* scaleFromLine(buffer, 0);
	int padding = 2;
	int minWidth = 400;
	int width = bitmap.width / 2;
	width = width > minWidth? width : min(minWidth, bitmap.width - padding * 2);
	int height = LH*1.5;
	int x = (bitmap.width - width) /2;
	int y = 0+padding;
	Rect r = { x,y,width,height};
	renderRect(bitmap,r, backgroundColors[2]);

	int dec = buffer->font->descent* scaleFromLine(buffer, 0);

	if (data.menu.length > 0)
	{
		for (int i = 0; i < data.menu.length; i++)
		{
			y += height;
			Rect r = { x,y,width,height};
			Bitmap b = drawBorder(subBitmap(bitmap, r), 0, 0, 1, 0, 0xff999999);
			//crash??
			if (data.activeMenuItem-1== i)
			{
				clearBitmap(b, activeColor);
			}
			else
			{
				clearBitmap(b, backgroundColors[2]);
			}
			renderText(b,data.menu.start[i].name,scaleFromLine(buffer,0),foregroundColor,0,height-3,buffer->font); // the one here is a bit worrying... 
		}
	}
	renderText(buffer, subBitmap(bitmap, r), 0, 0, height + dec, true);
}


internal int calculateVisibleLines(TextBuffer *textBuffer, Bitmap bitmap, int line)
{
	Typeface::Font tmp_font_and_stuff = *textBuffer->font;
	int y = 0;
	int counter = 0;
	while (y < bitmap.height)
	{
		if (line + counter>getLines(textBuffer->backingBuffer))break;
		y += tmp_font_and_stuff.lineHeight*scaleFromLine(textBuffer, line + counter);
		++counter;
	}
	return counter - 1;
}

inline float abs(float f)
{
	return f >= 0 ? f : -f;
}
internal void updateText(TextBuffer *textBuffer, Bitmap bitmap, int x, int y,bool drawCaret)
{
	int visibleLines = calculateVisibleLines(textBuffer, bitmap, textBuffer->lastWindowLineOffset);
	int line = getLineFromCaret(textBuffer, textBuffer->ownedCarets_id.start[0]);
	float targetY = clamp(textBuffer->lastWindowLineOffset+0.5, line - visibleLines + 1, line);
	
	float diff = (targetY - textBuffer->lastWindowLineOffset);
	float dy = abs(diff) > 0.1 ? diff * 0.2 : diff;

	textBuffer->lastWindowLineOffset = textBuffer->lastWindowLineOffset + dy;

	int min = -textBuffer->caretX +40;
	int max = bitmap.width*3-textBuffer->caretX-x-x-40;  //magic, caret width....
	int targetX;

	if (bitmap.width > -min)
	{
		targetX = x+16; 
	}
	else
	{
		targetX = clamp(textBuffer->dx, min, max);
	}


	float diffx = (targetX - textBuffer->dx);
	int dx = abs(diffx) > 0.1 ? diffx * 0.2 : diffx;

	textBuffer->dx += dx;
	
	
	float firstScale = scaleFromLine(textBuffer, textBuffer->lastWindowLineOffset);
	
	//float offset = ((int)textBuffer->lastWindowLineOffset) - textBuffer->lastWindowLineOffset;
	float offset = 0;
	fillBitmap(textBuffer, bitmap, textBuffer->lastWindowLineOffset, y - textBuffer->font->descent * firstScale + (textBuffer->font->lineHeight*firstScale)*(offset));
	LineChangeHook lch[] = { bigTitles };
	renderText(textBuffer, margin(bitmap,x,x,y,y), textBuffer->lastWindowLineOffset, textBuffer->dx, y + (textBuffer->font->lineHeight*firstScale)*(1+offset), drawCaret, lch, ARRAY_LENGTH(lch));
}

internal void clearBuffer(MultiGapBuffer *buffer)
{
	for (int i = 0; i < buffer->blocks.length; i++)
	{
		buffer->blocks.start[i].length = 0;
	}
}


internal int getIndentLine(TextBuffer buffer, int line)
{
	MGB_Iterator it = getIteratorAtLine(&buffer, line);
	int c=0;
	do
	{
		if (*getCharacter(buffer.backingBuffer->buffer, it) != '\t')
			break;
		c++;
	} while (getNext(buffer.backingBuffer->buffer, &it));
	return c;
}


internal void movePage(TextBuffer *textBuffer, Bitmap bitmap, bool up)
{
#if 0
	fucking_fix_me_please

	//this is not a traditional movepage.
	// it moves the cursor one page.
	// not the screen.
	// this is not acceptable but neither is it a huge problem.
	// therefore we should fix it at another time. 
	int visibleLines = calculateVisibleLines(textBuffer, bitmap, textBuffer->lastWindowLineOffset)-1;
	if (up)
	{
		for (int i = 0; i < visibleLines; i++)
		{
			moveUp(textBuffer, false, currentCaretIndex);
		}
	}
	else
	{
		for (int i = 0; i < visibleLines; i++)
		{
			moveDown(textBuffer, false,currentCaretIndex);
		}
	}
#endif 
}

internal int getNextTokenHash(MultiGapBuffer *buffer, MGB_Iterator *it, int *counter)
{
	*counter = 0;
	int hash = 0;

	if (isFuncDelimiter(*getCharacter(buffer,*it)))
	{
		*counter = 1;
		hash = *getCharacter(buffer, *it);
		getNext(buffer, it);
		return hash;
	}

	do {
		++*counter;
		char character = (*getCharacter(buffer, *it));
		hash = hash * 101 + character;
	} while (getNext(buffer, it) && !isFuncDelimiter(*getCharacter(buffer, *it)));

	return hash;
}

internal bool stripInitialWhite(MultiGapBuffer *buffer, MGB_Iterator *it, int *counter)
{
	*counter = 0;
	while (isspace(*getCharacter(buffer,*it)))
	{
		if (!getNext(buffer, it))
		{
			return false;
		}
		++(*counter);
	}
	return true;
}

#include "bindings.hpp"

internal void initTextBuffer(TextBuffer *textBuffer)
{
	_setLocalBindings(textBuffer);
}

internal void renderScroll(TextBuffer *textBuffer,Bitmap bitmap)
{
	int height = 4;
	float progress = (float)getLineFromCaret(textBuffer,textBuffer->ownedCarets_id.start[0])/ ((float)getLines(textBuffer->backingBuffer)-1);
	int h = (progress * (bitmap.height-height));

	renderRect(bitmap, bitmap.width - 10, h, 10, height, foregroundColor);
}

bool colorIsDirty=true;


internal void renderInfoBar(TextBuffer *buffer, Bitmap bitmap, float scale, int x, int y,bool isActive)
{
	if(isActive)
		renderRectColor(bitmap, 0, 0, bitmap.width, bitmap.height, activeColor);
	else
		renderRectColor(bitmap, 0, 0, bitmap.width, bitmap.height, backgroundColors[0]);

	char string[400];
	//int pos = getLeftCaretIndex(&buffer->backingBuffer->buffer);
	Location loc = getLocationFromCaret(buffer, buffer->ownedCarets_id.start[0]);
	sprintf(string, "history_index: %d, loc:(%d:%d)\t mem:%dkb\t memBlocks:%d @%0.2f \t %s%s", buffer->backingBuffer->history.current_index, loc.line,loc.column, memoryConsumption/1024,blocks.length, memoryEffectivity, "font_name", "font_style");
	
	// bad, these allocations are completely unnessesary.. (and the only two we do every frame)
	DHSTR_String allocationInfo = DHSTR_MERGE_MULTI(alloca, DHSTR_MAKE_STRING(string), buffer->fileName);
	
	renderText(bitmap, DHSTR_UTF8_FROM_STRING(allocationInfo,alloca), scale, foregroundColor, x, y, buffer->font);
	//the filename looks freed to me... :/
	
	/*
	*/
}

#if 0
internal int getRenderedWidthNode(AST_EXPR expr)
{
	return 80;
}

internal int getRenderedHeightNode(AST_EXPR expr)
{
	return 50;
}

internal int getRenderedWidth(AST_EXPR expr)
{
	int PADDING = 0;
	int ack = 0; 
	for (int i = 0; i < expr.length; i++)
	{
		ack += getRenderedWidth(expr.children[i]);
	}
	return max(ack, getRenderedWidthNode(expr));
}



internal void renderAstNodeCentered(Bitmap bitmap, AST_EXPR expr, int offX, int offY, Typeface *typeface)
{
	int width = getRenderedWidthNode(expr)-1;
	int height = getRenderedHeightNode(expr)-1;
	Bitmap b = subBitmap(bitmap,width,height,offX-width/2,offY);
	b = drawBorder(b, 1, 1, 1, 1, backgroundColors[0]);
	
	if (expr.token.name.start)
	{
		renderText(b, expr.token.name.start, 0.012, 0xffffffff, 2, 15, &typeface->Italic);
	
		char buffer[20];

		sprintf(buffer, "%d %d | %d %d", expr.token.location.line, expr.token.location.column,expr.token.type.returnType.type_id, expr.token.type.returnType.modifiers.qualifier_BitField);
		renderText(b, buffer, 0.12, 0xffffffff, 2, 30, &typeface->Bold);
	}
	else
	{
		renderText(b, "*missing*", 0.012, 0xffffffff, 2, 15, &typeface->Regular);
	}
}

internal void renderAstExpr(Bitmap bitmap, AST_EXPR expr, int offX, int offY, Typeface *typeface)
{
	renderAstNodeCentered(bitmap, expr, offX, offY, typeface);
	int width = getRenderedWidth(expr);
	
	int currentX = offX - (width) / 2;
	for (int i = 0; i < expr.length; i++)
	{
		int c_width = getRenderedWidth(expr.children[i]);
		renderAstExpr(bitmap, expr.children[i], currentX+c_width/2, offY + getRenderedHeightNode(expr),typeface);
		currentX += c_width;
	}
}
#endif

typedef  void(*RequestString)(void *base_node, void *node, char* buffer, size_t buffer_len);
typedef bool(*RequestNodeChild)(void *base_node, void *node, int index, void **_out_child);
struct NodeInterface
{
	RequestString getName;
	RequestString getText;
	RequestNodeChild getChild;
};

internal int getRenderedWidthNode(void *node)
{
	return 80;
}

internal int getRenderedHeightNode(void *node)
{
	return 50;
}

internal int getRenderedWidth(void *node, NodeInterface nodeInterface, void *data)
{
	int PADDING = 0;
	int ack = 0;
	void *child;
	int index = 0;
	while (nodeInterface.getChild(data, node, index++, &child))
	{
		ack += getRenderedWidth(child,nodeInterface,data);
	}
	return max(ack, getRenderedWidthNode(node));
}


internal void renderNodeCentered(Bitmap bitmap, void *node, int offX, int offY, NodeInterface nodeInterface, Typeface::Font *font, void *data)
{
	int width = getRenderedWidthNode(node)-1;
	int height = getRenderedHeightNode(node)-1;
	Bitmap b = subBitmap(bitmap,width,height,offX-width/2,offY);
	b = drawBorder(b, 1, 1, 1, 1, activeColor);
	
	const int buffer_len = 40;
	char buffer[buffer_len];
	nodeInterface.getName(data, node, buffer, buffer_len);
	renderText(b, buffer, 0.012, foregroundColor, 2, 15, font );

	nodeInterface.getText(data, node, buffer, buffer_len);
	renderText(b, buffer, 0.008, foregroundColor, 2, 30, font);
}

internal void renderTree(Bitmap bitmap, void *node, int offX, int offY, Typeface::Font *typeface, NodeInterface nodeInterface, void *data_passed_to_interface_funcs)
{
	renderNodeCentered(bitmap, node, offX, offY, nodeInterface, typeface, data_passed_to_interface_funcs);
	int width = getRenderedWidth(node,nodeInterface, data_passed_to_interface_funcs);
	int currentX = offX - (width) / 2;
	void *child;
	int index = 0;
	while (nodeInterface.getChild(data_passed_to_interface_funcs,node, index++, &child))
	{
		int c_width = getRenderedWidth(child,nodeInterface, data_passed_to_interface_funcs);
		renderTree(bitmap, child, currentX + c_width / 2, offY + getRenderedHeightNode(node), typeface,nodeInterface,data_passed_to_interface_funcs);
		currentX += c_width;
	}
}



internal void renderScreen(TextBuffer *buffer, Bitmap bitmap, int x, int y, bool isActive, bool drawCaret)
{//@improve me... this api is not ok.
	if (!buffer->font->font_info)
	{
		assert(false && "font was not loaded correctly");
		return;
	}
	if (buffer->bufferType == regularBuffer)
	{
		//if(colorIsDirty)
		//parseForCC(buffer); 
		float infoScale = stbtt_ScaleForPixelHeight(buffer->font->font_info, 20);
		int infoSpace = buffer->font->lineHeight*infoScale;
	
		int padding = 0;

		Bitmap text = subBitmap(bitmap, bitmap.width-padding, bitmap.height - infoSpace, padding, 0);
		Bitmap allocationInfo = subBitmap(bitmap, bitmap.width, infoSpace, 0, bitmap.height - infoSpace);

		updateText(buffer, text, 3, 0,drawCaret);
		renderScroll(buffer,text);

		renderInfoBar(buffer, allocationInfo, infoScale,0, infoSpace-2,isActive);
	
	}
	else if (buffer->bufferType == ASTBuffer)
	{
#if 0
		clearBitmap(bitmap, backgroundColors[0]);
		Lexer lexer = createLexer(buffer->backingBuffer->buffer);
		bool eof = false;
		bool ignore = false;
		AST_EXPR expr = getNextExpr(&lexer, &eof);
		renderAstExpr(bitmap, expr, bitmap.width / 2, 0, &buffer->typeface);
		freeExpr(expr);
		freeLexer(lexer);
		free_(identifiers.start);
		identifiers.start = 0;
#endif
	}
	else if (buffer->bufferType == HistoryBuffer)
	{
		NodeInterface nodeInterface;
		clearBitmap(bitmap, backgroundColors[1]);

		//@cpp heavy... 
		nodeInterface.getText = [](void *phistory, void *node, char* buffer, size_t buffer_len) {
			HistoryEntry entry = *(HistoryEntry *)node;
			switch (entry.action)
			{
			case action_move:
				sprintf(buffer, "%s: %d", stringFromAction(entry.action),entry.direction); //@unsafe
				break;
			case action_add:
			case action_remove:
			case action_delete:
			case action_undelete:
				sprintf(buffer, "%s: %c", stringFromAction(entry.action),entry.character); //@unsafe
				break;
			case action_addCaret:
				sprintf(buffer, "%s", stringFromAction(entry.action)); //@unsafe
				break;
			case action_removeCaret:
				sprintf(buffer, "%s", stringFromAction(entry.action)); //@unsafe
				break;
			}
		};

		nodeInterface.getName = [](void *phistory, void *node, char* buffer, size_t buffer_len) {
			History *history = (History *)phistory;
			char *base_node = (char *)history->entries.start;
			int index = (((char *)node) - ((char *)base_node)) / sizeof(HistoryEntry);
			sprintf(buffer, "%d", index);//@unsafe
		};

		nodeInterface.getChild = [] (void *phistory, void *node, int index, void **_out_child)
		{
			History *history = (History *)phistory;
			char *base_node = (char *)history->entries.start;
			int ind = (((char *)node) - ((char *)base_node)) / sizeof(HistoryEntry);
			int c = 0;
			HistoryBranch branch;
			if (index == 0)
			{
				if (ind + 1 < history->entries.length)
				{
					*_out_child = &history->entries.start[ind + 1];
					auto it = make_iterator(&history->branches, ind+1);
					while (get_next(&history->branches, &it, ind+1, &branch))
					{
						if (branch.direction == branch_target) return false;
					}
					return true;
				}
				return false;
			}
			auto it = make_iterator(&history->branches, ind);
			while (get_next(&history->branches, &it,ind,&branch))
			{
				if (branch.direction == branch_start) ++c;
				if (c == index) 
				{
					*_out_child = &history->entries.start[branch.index]; 
					return true;
				}
			}
			return false;
		};

		renderTree(bitmap, &buffer->backingBuffer->history.entries.start[0], bitmap.width / 2, 0, buffer->font, nodeInterface, &buffer->backingBuffer->history);
	}
	else
	{
		assert(false && "buffertype is not handled by renderScreen");
	}
}

//clearly out of place 
internal char *getRestAsString(MultiGapBuffer *buffer, MGB_Iterator it)
{
	int counter=0;
	MGB_Iterator itCopy = it;
	do {//the stupidity is strong in this one
		counter++;
	} while (getNext(buffer, &itCopy));

	char *pointer = (char *)alloc_((counter+1)* sizeof(char),"getRestAsString");
	
	int i = 0; 
	do {
		pointer[i++]= *getCharacter(buffer,it);
	} while (getNext(buffer, &it));
	pointer[i++] = '\0';

	return pointer;
}


bool getActiveBuffer(Data *data, TextBuffer **activeBuffer_out)
{
	if (data->isCommandlineActive)
	{
		*activeBuffer_out = data->commandLine;
	}
	else
	{
		if (!(data->textBuffers.length > data->activeTextBufferIndex && data->activeTextBufferIndex >= 0)|| data->textBuffers.length==0)
		{
			return false;
		}
		else
		{
			*activeBuffer_out = &data->textBuffers.start[data->activeTextBufferIndex];
		}
	}
	return true;
}

//this part of sucks.. I need to fix this.
internal Rect centered(Bitmap big, Rect rect)
{
	Rect ret = {};
	ret.height = rect.height;
	ret.width= rect.width;
	ret.x = (big.width - rect.width) / 2;
	ret.y = (big.height- rect.height) / 2;
	return ret;
}


internal void renderBackground(Bitmap bitmap)
{ //some reving here 
	clearBitmap(bitmap, backgroundColors[0]);

#if 0
	int w,h;
	int comp;
	unsigned char *image = stbi_load("iconBorderless.png", &w, &h, &comp, STBI_rgb_alpha);
	if (image)
	{
		Bitmap from = {};
		from.bytesPerPixel = 4;
		from.height= h;
		from.width= w;
		from.memory= image;
		from.stride = w*from.bytesPerPixel;
		
		
		Rect rect = {};
		rect.height = h;
		rect.width = w;
		rect = centered(bitmap, rect);

		blitBitmap(from, bitmap, rect.x, rect.y);
	}
	free(image);
#endif
}

//support mutliple buffers to the same file.
internal void updateInput(Data *data, Input *input,Bitmap bitmap, uint64_t lastAction)
{
	TextBuffer *textBuffer;
	bool hasActiveBuffer = getActiveBuffer(data, &textBuffer);
	if (hasActiveBuffer)
	{
		textBuffer->lastAction = lastAction;
	}
	else
	{
		textBuffer = data->commandLine;
		data->isCommandlineActive = true;
	}

	if (input->inputType == input_character)
	{
		if (data->eatNext) 
		{
			data->eatNext = false;
			return;
		}
		if(data->isCommandlineActive)
		{
			if (!iswcntrl(input->character))
				appendCharacter(textBuffer,input->character);
			keyDown_CommandLine(data);
		}
		else
		{
			// sometimes windows fucks with us.
			// control m -> \r 
			// this handles that. but keeps alt-gr 
			// i wonder if this is acceptable for all layouts..
			// it works on a swedish one that's for sure...
			if (input->control && !input->alt)return;
#if 1
			if (input->character == '\t')
			{
				if (!input->shift)
				{
					insertTab(textBuffer);
				}
				else
				{
					removeTab(textBuffer);
				}
			}
			else 
#endif		
			if (isLineBreak(input->character)) //huston we have a problem. (crashes when we've got no buffer open)
			{// do we only append \r (^m) here?

				//int indent = getIndentLine(*textBuffer,currentCaretIndex);
				int indent = 0;
				//fucking_fix_me_please
				//appendCharacter(textBuffer, '\r\n');
				appendCharacter(textBuffer, '\n');

				for (int i = 0; i< indent; i++)
				{
					appendCharacter(textBuffer, '\t');
				}
			}
			else
			{
				if (!iswcntrl(input->character))
				{
					char buffer[4];
					int32_t errors;
					int read = utf16toutf8(&input->VK_Code, sizeof(char16_t), buffer, 4 * sizeof(char), &errors);
					if (!errors)
						for (int i = 0; i < read; i++) appendCharacter(textBuffer, buffer[i]);
					else
						assert(false && "could not convert keyboard input to utf8");
				}
			}

		}
	}
	else if (input->inputType == input_key)
	{
		Mods currentMods = mod_none;
		if (input->shift)  currentMods = currentMods | mod_shift;
		if (input->control)  currentMods = currentMods | mod_control;
		if (input->alt)  currentMods = currentMods | mod_alt;

		for (int i = 0; i<textBuffer->KeyBindings.length; i++)
		{
			KeyBinding bind = textBuffer->KeyBindings.start[i];
			if (bind.VK_Code != input->VK_Code) continue;

			if (matchMods(currentMods, bind.mods, bind.modMode))
			{
				Argument arg = {};
				arg.mods = currentMods;

				executeBindingFunction(bind, textBuffer, data, currentMods);
				return; //until the closeing function is delayed this is all we can do.
			}
		}
	}
	else
	{
#if 0
		for (int i = 0; i < input->mouseWheel / 120; i++)
		{
			moveUp(textBuffer, input->shift,currentCaretIndex);
		}
		for (int i = 0; i < -input->mouseWheel / 120; i++)
		{
			moveDown(textBuffer, input->shift,currentCaretIndex);
		}
#endif 
		//fucking_fix_me_please @fixme @todo
	}

}
global_variable float blinkTimePerS = 0.5;


internal Bitmap layout(Bitmap bitmap,int length, int index)
{
	const int PADDING = 1;
	Bitmap subBitmap = getVerticalBitmapNumber(bitmap, length, index);
	int leftPadding=PADDING;
	int rightPadding=PADDING;
	if (index == 0)
		leftPadding = 0;
	if(index == length-1)
		rightPadding= 0;
	subBitmap = drawBorder(subBitmap, leftPadding, rightPadding, 0, 0, activeColor);

	return subBitmap;
}

internal void renderFrame(Data data, Bitmap bitmap, uint64_t microsStart)
{
	local_persist bool wasCommandLineActive = false;
	if (data.updateAllBuffers || wasCommandLineActive||true)
	{
		for (int i = 0; i < data.textBuffers.length; i++)
		{
			Bitmap subBitmap = layout(bitmap, data.textBuffers.length, i);
			renderScreen(&data.textBuffers.start[i], subBitmap, 5, 0, i==data.activeTextBufferIndex,true);
		}
		if (data.textBuffers.length == 0)
		{
			renderBackground(bitmap);
		}
		if(data.isCommandlineActive)
			renderCommandLine(bitmap, data);
		
	}
	else
	{
		if (data.isCommandlineActive)
		{
			renderCommandLine(bitmap, data);
		}
		
		else if (data.textBuffers.length > data.activeTextBufferIndex && data.activeTextBufferIndex >= 0)
		{
			Bitmap subBitmap = layout(bitmap, data.textBuffers.length, data.activeTextBufferIndex);
			TextBuffer *textBuffer = (data.textBuffers.start + data.activeTextBufferIndex);
			bool drawCaret = ((microsStart - textBuffer->lastAction) / 1000 < 100) 
				|| ((((int)((microsStart-textBuffer->lastAction) * blinkTimePerS) / 1000000) % 2) == 0);
			
			renderScreen(textBuffer, subBitmap, 5, 0, true, drawCaret);
		}
		
	}	
	wasCommandLineActive= data.isCommandlineActive;
}




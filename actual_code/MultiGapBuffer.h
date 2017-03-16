#ifndef MULTIGAPBUFFER_H
#define MULTIGAPBUFFER_H

struct MGB_Iterator
{
	int block_index;
	int sub_index;
};

struct  BufferBlock
{
	int start;
	int length;
};

#define DA_TYPE BufferBlock
#define DA_NAME DA_BufferBlock
#include "DynamicArray.h"

struct CursorIdentifier
{
	int textBuffer_index;
	int id;
};

#define DA_TYPE CursorIdentifier
#define DA_NAME DA_CursorIdentifier
#include "DynamicArray.h"


struct MultiGapBuffer
{
	union {
		MemBlock block;
		struct {
			char *start;
			int capacity;
		};
	};
	//these two really are ordered *and* connected so maybe change how this works?
	DA_BufferBlock blocks;
	DA_CursorIdentifier cursor_ids;
	int running_cursor_id;
	DH_Allocator *allocator;
};

internal char *getCharacter(MultiGapBuffer *buffer, MGB_Iterator it);
internal int get(MultiGapBuffer *buffer, char *character);
//internal MultiGapBuffer createMultiGapBuffer(int size);
//internal void freeMultiGapBuffer(MultiGapBuffer *buffer);
internal int length(MultiGapBuffer *buffer);
internal int getIteratorPos(MultiGapBuffer *buffer, MGB_Iterator it);
internal int getCaretPos(MultiGapBuffer *buffer, int caretId);
internal MGB_Iterator getIterator(MultiGapBuffer *buffer, int pos);
internal MGB_Iterator getIterator(MultiGapBuffer *buffer);
internal bool getNext(MultiGapBuffer *buffer, MGB_Iterator *it);
internal bool getPrev(MultiGapBuffer *buffer, MGB_Iterator *it);
internal int indexFromId(MultiGapBuffer *buffer, int id);
internal MGB_Iterator getIteratorFromCaret(MultiGapBuffer *buffer, int id);
internal int AddCaret(MultiGapBuffer *buffer,int textBuffer_index, int i);
internal bool del(MultiGapBuffer *buffer, History *history, bool log, int caretId);
internal void removeCaret(MultiGapBuffer *buffer, int caretId);
internal void removeEmpty(TextBuffer *textBuffer);
internal bool mgb_moveLeft(MultiGapBuffer *buffer, History *history, bool log, int caretId);
internal bool mgb_moveRight(MultiGapBuffer *buffer, History *history, bool log, int caretId);
internal bool ownsIndex(MultiGapBuffer *buffer, DA_int *ids, int targetIndex);
internal bool HasCaretAtIterator(MultiGapBuffer*buffer, DA_int*ids, MGB_Iterator	 iterator);
internal bool isEmptyCaret(MultiGapBuffer *buffer, DA_int *ids, int index);
internal bool removeCharacter(MultiGapBuffer *buffer, History *history, bool log, int caretId);
internal void invDelete(MultiGapBuffer *buffer, int caretId, char character);
internal void appendCharacter(MultiGapBuffer *buffer, int caretId, char character);
bool pushValid(MultiGapBuffer *buffer, MGB_Iterator *it);
internal void grow(MultiGapBuffer *buffer);
#endif
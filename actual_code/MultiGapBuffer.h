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

DEFINE_DynamicArray(BufferBlock);

struct CursorIdentifier
{
	int textBuffer_index;
	int id;
};
DEFINE_DynamicArray(CursorIdentifier);


struct MultiGapBuffer
{
	char *start;
	DynamicArray_BufferBlock blocks;
	int length;
	int running_cursor_id;
	DynamicArray_CursorIdentifier cursor_ids;
};

internal char *getCharacter(MultiGapBuffer *buffer, MGB_Iterator it);
internal int get(MultiGapBuffer *buffer, char *character);
internal char *get(MultiGapBuffer *buffer, int pos);
internal char *get(MultiGapBuffer *buffer, int caretId, Direction dir);
internal MultiGapBuffer createMultiGapBuffer(int size);
internal void freeMultiGapBuffer(MultiGapBuffer *buffer);
internal int length(MultiGapBuffer *buffer);
internal int getIteratorPos(MultiGapBuffer *buffer, MGB_Iterator it);
internal int getCaretPos(MultiGapBuffer *buffer, int caretId);
internal MGB_Iterator getIterator(MultiGapBuffer *buffer, int pos);
internal MGB_Iterator getIterator(MultiGapBuffer *buffer);
internal bool getNext(MultiGapBuffer *buffer, MGB_Iterator *it, int *wasCaret = 0);
internal bool getPrev(MultiGapBuffer *buffer, MGB_Iterator *it);
internal int indexFromId(MultiGapBuffer *buffer, int id);
internal MGB_Iterator getIteratorFromCaret(MultiGapBuffer *buffer, int id);
internal void getSurroundingGap(MultiGapBuffer *buffer, int index, int *prevGap, int *nextGap);
internal int AddCaret(MultiGapBuffer *buffer,int textBuffer_index, int i);
internal bool del(MultiGapBuffer *buffer, int caretId);
internal int posFromId(MultiGapBuffer *buffer, int caretId);
internal void removeCaret(MultiGapBuffer *buffer, int caretId);
internal void removeEmpty(TextBuffer *textBuffer);
internal bool mgb_moveLeft(MultiGapBuffer *buffer, int caretId);
internal bool mgb_moveRight(MultiGapBuffer *buffer, int caretId);
internal bool ownsIndex(MultiGapBuffer *buffer, DynamicArray_int *ids, int targetIndex);
internal bool HasCaretAtIterator(MultiGapBuffer*buffer, DynamicArray_int *ids, MGB_Iterator	 iterator);
internal bool isEmptyCaret(MultiGapBuffer *buffer, DynamicArray_int *ids, int index);
internal bool removeCharacter(MultiGapBuffer *buffer, int caretId);
internal void invDelete(MultiGapBuffer *buffer, int caretId, char character);
internal void appendCharacter(MultiGapBuffer *buffer, int caretId, char character);
internal void grow(MultiGapBuffer *buffer);
#endif
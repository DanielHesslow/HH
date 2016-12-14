#include "MultiGapBuffer.h"


struct Gap
{
	BufferBlock *prev;
	BufferBlock *next;
	int length; // is implicit but why not.
};

Gap getGap(MultiGapBuffer *buffer, int index)
{
	assert(index >= 0 && index < buffer->blocks.length - 1);
	Gap gap = {};
	gap.prev = &buffer->blocks.start[index];
	gap.next = &buffer->blocks.start[index + 1];
	gap.length = gap.next->start - (gap.prev->start + gap.prev->length);
	return gap;
}

int indexFromId(MultiGapBuffer *buffer, int id)
{
	for (int i = 0; i < buffer->cursor_ids.length; i++)
	{
		if (buffer->cursor_ids.start[i].id == id)
			return i;
	}
	assert(false && "id doesn't exist!");
	return-1;
}

MGB_Iterator getIteratorFromCaret(MultiGapBuffer *buffer, int id)
{  
	int index = indexFromId(buffer, id);
	MGB_Iterator it;
	it.sub_index = 0;
	it.block_index = index + 1;
	pushValid(buffer, &it);
	return it;
}

void moveBlock(MultiGapBuffer *buffer, BufferBlock *block, int delta_pos)
{
	memmove(&buffer->start[block->start+delta_pos], &buffer->start[block->start], block->length * sizeof(char));
	block->start += delta_pos;
}

int get(MultiGapBuffer *buffer, char *character)
{
	int ack=0;
	for (int i = 0; i< buffer->blocks.length; i++)
	{
		if ((character > buffer->start + buffer->blocks.start[i].start)
			&& (character < buffer->start + buffer->blocks.start[i].start+buffer->blocks.start[i].length))
		{
			return ack + character - buffer->start + buffer->blocks.start[i].start;
		}
		ack += buffer->blocks.start[i].length;
	}
	assert(false&&"character is not in any block!");
	return -1;
}


char *end(MultiGapBuffer *buffer, BufferBlock block)
{
	return &buffer->start[block.start+block.length-1];
}

char *start(MultiGapBuffer *buffer, BufferBlock block)
{
	return &buffer->start[block.start];
}

MGB_Iterator getIterator(MultiGapBuffer *buffer, int pos)
{
	//I'm fucking in the head.
	//aka I'm probably not wokring on where the pos > blocks.start[1].start
	//maybe?
	BufferBlock b;
	int ack = 0;
	int i = 0;
	//the minus one here is making sure we're not going past the last block.
	//I'm not entierly sure about it. @bugs
	for (; i< buffer->blocks.length-1; i++)
	{
		b = buffer->blocks.start[i];
		ack += b.length;
		if (ack > pos)
		{
			ack -= b.length;
			break;
		}
	}

	MGB_Iterator it = {};
	it.block_index = i;
	it.sub_index = pos -ack;
	return it;
}


char *get(MultiGapBuffer *buffer, int pos)
{
	MGB_Iterator it = getIterator(buffer, pos);
	return &buffer->start[buffer->blocks.start[it.block_index].start + it.sub_index];
}
	
int getIteratorPos(MultiGapBuffer *buffer, MGB_Iterator it)
{
	int ack = 0;
	for (int i = 0; i < it.block_index; i++)
	{
		ack += buffer->blocks.start[i].length;
	}
	return ack+it.sub_index;
}

int getCaretPos(MultiGapBuffer *buffer, int caretId)
{
	int caretIndex = indexFromId(buffer, caretId);
	
	int ack = 0;
	for (int i = 0; i <= caretIndex; i++)
	{
		ack += buffer->blocks.start[i].length;
	}
	return ack;
}

MGB_Iterator getIterator(MultiGapBuffer *buffer)
{
	int block_index=0;
	for (int i = 0; i < buffer->blocks.length; i++)
	{
		if (buffer->blocks.start[i].length > 0)
		{
			block_index = i;
			break;
		}
	}

	return{block_index, 0};
}


int length_of_block(MultiGapBuffer *buffer, int index)
{
	return buffer->blocks.start[index].length;
}

char *start_of_block(MultiGapBuffer *buffer, int index)
{
	return &buffer->start[buffer->blocks.start[index].start];
}




// push the iterator to the right into a valid position.
// returns false if no valid position to the right exist.
bool pushRight(MultiGapBuffer *buffer, MGB_Iterator *it)
{
	while (it->sub_index >= length_of_block(buffer, it->block_index))
	{
		if (it->block_index == buffer->blocks.length - 1) return false;
		*it = { it->block_index + 1, it->sub_index - length_of_block(buffer, it->block_index) };
	}
	return true;
}

// push the iterator to the left into a valid position.
// returns false if no valid position to the left exist.
bool pushLeft(MultiGapBuffer *buffer, MGB_Iterator *it)
{
	while (it->sub_index < 0)  
	{
		if (it->block_index == 0)return false;
		*it = { it->block_index - 1, length_of_block(buffer, it->block_index-1)+it->sub_index};
	}
	return true;
}

// pushes an iterator into a valid postion, either to the right or to the left. Returns false if not possible.
bool pushValid(MultiGapBuffer *buffer, MGB_Iterator *it)
{
	bool l = pushLeft(buffer, it);
	bool r = pushRight(buffer, it);
	return r&&l;
}

bool getNext(MultiGapBuffer *buffer, MGB_Iterator *it)
{
	++it->sub_index;
	return pushValid(buffer, it);
}

bool getPrev(MultiGapBuffer *buffer, MGB_Iterator *it)
{
	--it->sub_index;
	return pushValid(buffer, it);
}

bool MoveIterator(MultiGapBuffer *buffer, MGB_Iterator *it, int direction)
{
	MGB_Iterator before = *it;
	it->sub_index += direction;
	if (pushValid(buffer, it))
	{
		return true;
	}
	else
	{
		//*it = before;
		return false;
	}
}

int getCodepoint(MultiGapBuffer *buffer, MGB_Iterator it, char32_t *code_point)
{
	pushRight(buffer, &it);
	int max_read = buffer->blocks.start[it.block_index].length - it.sub_index;
	return codepoint_read(getCharacter(buffer, it), max_read, (uint32_t *)code_point);
}

bool codepoint_next(MultiGapBuffer *buffer, MGB_Iterator *it, char32_t *codepoint)
{
	int read = getCodepoint(buffer, *it, codepoint);
	MoveIterator(buffer,it, read); 
	return read;
}

char invalid = 0;

char *getCharacter(MultiGapBuffer *buffer, MGB_Iterator it)
{
	BufferBlock block = buffer->blocks.start[it.block_index];
	if (!pushValid(buffer, &it)) return &invalid;
	char *ret= &buffer->start[block.start+it.sub_index];
	return ret;
}
	
char *get(MultiGapBuffer *buffer, int caretId, Direction dir)
{
	// this might get garbage on either side of the buffer
	// not doing checks for this *might* be beneficial. We'll see.
	
	//hrm investigate what the fuck we meant... That didn't seam to good now did it?
	int caretIndex = indexFromId(buffer,caretId);
	if (dir == dir_left)
	{
		return end(buffer, buffer->blocks.start[caretIndex]);
	}
	else
	{
		return start(buffer, buffer->blocks.start[caretIndex + 1]);
	}
}

MultiGapBuffer createMultiGapBuffer(int size,DH_Allocator allocator)
{
	DH_Allocator macro_used_allocator = allocator;
	MultiGapBuffer buffer = {};
	buffer.allocator = allocator;
	buffer.blocks = constructDynamicArray_BufferBlock(5,"multiGapBuffer blocks",allocator);
	buffer.start = (char *)ALLOC(allocator,size*sizeof(char));
	buffer.capacity = size;
	buffer.running_cursor_id = 994; // just some non-0 number to catch some bad bugs.
	BufferBlock first = {0,0};
	BufferBlock last = {size,0};
	Add(&buffer.blocks,first);
	Add(&buffer.blocks,last);
	buffer.cursor_ids = constructDynamicArray_CursorIdentifier(20,"mutliGapBuffer cursors",allocator);
	Add(&buffer.cursor_ids, { 0,buffer.running_cursor_id});
	return buffer;
}

int length(MultiGapBuffer *buffer)
{
	int ack=0;
	for(int i = 0; i< buffer->blocks.length;i++)
	{
		ack += buffer->blocks.start[i].length;
	}

	return ack;
}



void rebalance(MultiGapBuffer *buffer, int index, int excess)
{
	//pretty sure we've got issues.
	//does this even work if the target isn't an integer?
	if (index >= buffer->blocks.length - 1)
	{
		assert(!excess);
		return;
	}

	int target = (buffer->capacity - length(buffer))/(buffer->blocks.length-1);
	int current = getGap(buffer, index).length;
	
	int delta = target - current + excess;
	if (delta<0)
	{
		moveBlock(buffer, &buffer->blocks.start[index + 1], delta);
		rebalance(buffer, index + 1, 0);
	}
	else
	{
		rebalance(buffer, index + 1, delta);
		moveBlock(buffer, &buffer->blocks.start[index + 1], delta);
	}
}



void growTo(MultiGapBuffer *buffer, uint64_t size)
{
	int multiple = 2;
	while (buffer->capacity*multiple < size)multiple *= 2;
	char *new_mem = (char *)ALLOC(buffer->allocator,buffer->capacity * multiple * sizeof(char));
	buffer->capacity *= multiple;
	float gap = (buffer->capacity - length(buffer)) / ((float)buffer->blocks.length - 1);
	float ack = 0;

	for (int i = 0; i < buffer->blocks.length; i++)
	{
		BufferBlock block = buffer->blocks.start[i];
		memmove(&new_mem[(int)ack], &buffer->start[block.start], block.length*sizeof(char));
		buffer->blocks.start[i].start = (int)ack;
		ack += gap + buffer->blocks.start[i].length;
	}
	DeAllocate(buffer->allocator,buffer->start);
	buffer->start = new_mem;
}

void grow(MultiGapBuffer *buffer)
{
	growTo(buffer, buffer->capacity * 2 * sizeof(char));
}

void maybeGrow(MultiGapBuffer *buffer)
{//@not in action yet for obvious reasons.
	float effectivity = ((float)length(buffer) / (float)buffer->capacity);
	if (effectivity > 0.5)
	{
		grow(buffer);
	}
	else
	{
		rebalance(buffer, 0, 0);
	}
}

void order_mgb(MultiGapBuffer *buffer)
{
	int N = buffer->cursor_ids.length-1;
	//bubble sort should be fast enough here, we'll likely do 0 swaps. but you know.
	// PERFORMANCE..
	for (int n = 0; n < N; n++)
	{
		bool swap = false;
		for (int i = 0; (i + n) < N; i++)
		{
			Gap gap = getGap(buffer, i);
			if (!gap.next->length)
			{
				if (buffer->cursor_ids.start[i].id > buffer->cursor_ids.start[i + 1].id)
				{
					DHMA_SWAP(CursorIdentifier, buffer->cursor_ids.start[i], buffer->cursor_ids.start[i + 1]);
					swap = true;
				}
			}
		}
		if (!swap)break;
	}
}



int AddCaret_(MultiGapBuffer *buffer, int pos)
{ 
	// do no rebalancing, that's done on first insirt.
	// this speeds things up if multiple cursors are added at once.
	MGB_Iterator it = getIterator(buffer, pos);
	BufferBlock *old_block = &buffer->blocks.start[it.block_index];
	BufferBlock new_block;
	new_block.start = old_block->start + it.sub_index;
	new_block.length = old_block->length - it.sub_index;
	old_block->length = it.sub_index;
	Insert(&buffer->blocks, new_block, it.block_index + 1);
	return it.block_index + 1;
}

void AddCaretWithId(MultiGapBuffer *buffer, int textBuffer_index, int pos, int Id)
{
	for (int i = 0; i < buffer->cursor_ids.length; i++)
	{
		assert(buffer->cursor_ids.start[i].id != Id);
	}
	Insert(&buffer->cursor_ids, { textBuffer_index, Id}, AddCaret_(buffer, pos) - 1);
	order_mgb(buffer);
}

int AddCaret(MultiGapBuffer *buffer, int textBuffer_index, int pos)
{
	int index;
	if (buffer->running_cursor_id == 994) // the first index is implicit. which is not great but it does simplyfy things
	{
		index = 0;
	}
	else
	{
		index = AddCaret_(buffer, pos);
		Insert(&buffer->cursor_ids, { textBuffer_index, buffer->running_cursor_id }, index - 1);
	}
	order_mgb(buffer);
	return buffer->running_cursor_id++;
}



void appendCharacter(MultiGapBuffer *buffer, int caretId, char character)
{
	// for everybody complaining about how bad hungarian notation is.
	// this right here below is the propper apps-hungarian.
	// not the missunderstood one that microsoft is using today.
	// It's not bad. It's just a simple way to keep track of what's what. 
	// I'm guessing most people are using something similar. 

	int caretIndex = indexFromId(buffer,caretId);
	Gap gap= getGap(buffer,caretIndex);
	if (gap.length)
	{
		gap.prev->length++;
		*end(buffer, *gap.prev) = character;
	}
	else
	{
		maybeGrow(buffer);
		appendCharacter(buffer, caretId, character);
	}
}

void invDelete(MultiGapBuffer *buffer, int caretId, char character)
{
	int caretIndex = indexFromId(buffer, caretId);
	Gap gap = getGap(buffer,caretIndex);
	if (gap.length)
	{
		--gap.next->start;
		gap.next->length++;
		*start(buffer, *gap.next)= character;
	}
	else
	{
		maybeGrow(buffer);
		invDelete(buffer, caretId, character);
	}
}



bool removeCharacter(MultiGapBuffer *buffer, int caretId)
{
	for (;;)
	{
		int caretIndex = indexFromId(buffer, caretId);
		BufferBlock *prev = &buffer->blocks.start[caretIndex];
		if (prev->length > 0)
		{
			--prev->length;
			order_mgb(buffer);
			return true;
		}
		else
		{
			if (caretIndex == 0)
			{
				order_mgb(buffer);
				return false;
			}
			DHMA_SWAP(CursorIdentifier, buffer->cursor_ids.start[caretIndex], buffer->cursor_ids.start[caretIndex - 1]);
		}
	}
}

bool del(MultiGapBuffer *buffer, int caretId)
{
	for (;;)
	{
		int caretIndex = indexFromId(buffer, caretId);
		BufferBlock *next = &buffer->blocks.start[caretIndex+1];
		if (next->length > 0)
		{
			--next->length;
			++next->start;
			order_mgb(buffer);
			return true;
		}
		else
		{
			if (caretIndex >= buffer->blocks.length-2)
			{
				order_mgb(buffer);
				return false;
			}
			DHMA_SWAP(CursorIdentifier, buffer->cursor_ids.start[caretIndex], buffer->cursor_ids.start[caretIndex + 1]);
		}
	}
}

int posFromId(MultiGapBuffer *buffer, int caretId)
{
	char *tmp = get(buffer, caretId, dir_left);
	return get(buffer, tmp);
}



void removeCaret(MultiGapBuffer *buffer, int caretId)
{
	//ie mergeblocks
	int caretIndex = indexFromId(buffer, caretId);
	BufferBlock *prev = &buffer->blocks.start[caretIndex];
	BufferBlock *next = &buffer->blocks.start[caretIndex + 1];
	if (next->length != 0)
	{
		int new_nextPos = prev->start + prev->length;
		int delta = new_nextPos - next->start;
		if(caretIndex != buffer->blocks.length-2)
			moveBlock(buffer, next, delta);
		else
			moveBlock(buffer, prev, -delta);
		prev->length += next->length;
	}
	RemoveOrd(&buffer->blocks, caretIndex + 1);
	RemoveOrd(&buffer->cursor_ids, caretIndex);
}

bool ownsIndex(MultiGapBuffer *buffer, DynamicArray_int *ids, int targetIndex)
{
	for (int i = 0; i < ids->length; i++)
	{
		if (indexFromId(buffer, ids->start[i]) == targetIndex)
		{
			return true;
		}
	}
	return false;
}

bool HasCaretAtIterator(MultiGapBuffer*buffer, DynamicArray_int *ids, MGB_Iterator it) 
{
	if (it.sub_index != 0) return false;
	if (ownsIndex(buffer, ids, it.block_index-1)) return true;
	for (int i = it.block_index; i < buffer->blocks.length-1; i++)
	{
		if (buffer->blocks.start[i].length == 0)
		{
			if (ownsIndex(buffer, ids, i)) 
				return true;
		}
		else
		{
			break;
		}
	}
	
	for (int i = it.block_index - 2; i >= 0; i--)
	{
		if (buffer->blocks.start[i+1].length == 0)
		{
			if (ownsIndex(buffer, ids, i)) 
				return true;
		}
		else
		{
			break;
		}
	}
	return false;
}

bool isEmptyCaret(MultiGapBuffer *buffer, DynamicArray_int *ids, int index)
{//only forwards (just a helper for the function blow. should probably not be reused. needs testing.
	for (int i = 1;;i++)
	{
		if (buffer->blocks.start[index + i].length == 0)
		{
			//index + i != buffer->blocks.length - 1
			if (ownsIndex(buffer, ids, index + i))
			{
				return true;
			}
		}
		else
		{
			return false;
		}
	}
}
internal void removeCaret(TextBuffer *buffer, int caretIdIndex, bool log);
void removeEmpty(TextBuffer *textBuffer)
{
	MultiGapBuffer *buffer = textBuffer->backingBuffer->buffer;
	for (int i = textBuffer->ownedCarets_id.length-1; i >= 0; i--)
	{
		int index = indexFromId(buffer,textBuffer->ownedCarets_id.start[i]);
		assert(buffer->blocks.start[index + 1].length >= 0);
		if (isEmptyCaret(buffer,&textBuffer->ownedCarets_id,index))
		{
			removeCaret(textBuffer, i,true);
		}
	}
}

void CheckOverlapp(MultiGapBuffer *buffer)
{
#ifdef DEBUG
	for (int i = 0; i < buffer->blocks.length - 1; i++)
	{
		BufferBlock prev = buffer->blocks.start[i];
		BufferBlock next = buffer->blocks.start[i + 1];
		assert(next.start >= prev.start + prev.length && "The blocks are overlapping.");
	}
#endif
}

bool mgb_moveLeft(MultiGapBuffer *buffer, int caretId)
{
	int caretIndex = indexFromId(buffer, caretId);
	assert(caretIndex >= 0 && caretIndex <= buffer->blocks.length - 2);
	BufferBlock *p = &buffer->blocks.start[caretIndex];
	BufferBlock *n = &buffer->blocks.start[caretIndex + 1];

	if ((caretIndex == 0 && p->length == 0))
		return false;
	
	if (!p->length)
	{
		DHMA_SWAP(CursorIdentifier, buffer->cursor_ids.start[caretIndex], buffer->cursor_ids.start[caretIndex - 1]);
		CheckOverlapp(buffer);
		return mgb_moveLeft(buffer,caretId);
	}
	
	++n->length;
	--n->start;
	*start(buffer, *n) = *end(buffer, *p);
	--p->length;
	CheckOverlapp(buffer);
	order_mgb(buffer);
	return true;
}

bool mgb_moveRight(MultiGapBuffer *buffer, int caretId)
{
	int caretIndex = indexFromId(buffer, caretId);
	assert(caretIndex >= 0 && caretIndex <= buffer->blocks.length - 2);
	BufferBlock *p = &buffer->blocks.start[caretIndex];
	BufferBlock *n = &buffer->blocks.start[caretIndex + 1];

	if ((caretIndex == buffer->blocks.length - 2 && n->length == 0))
		return false;

	if (n->length == 0)
	{
		DHMA_SWAP(CursorIdentifier, buffer->cursor_ids.start[caretIndex], buffer->cursor_ids.start[caretIndex + 1]);
		CheckOverlapp(buffer);
		return mgb_moveRight(buffer, caretId);
	}

	++p->length;
	*end(buffer, *p) = *start(buffer, *n);
	++n->start;
	--n->length;
	order_mgb(buffer);
	return true;
}


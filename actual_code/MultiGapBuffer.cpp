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
	int caretIndex = indexFromId(buffer, caretId);
	MGB_Iterator it = { caretIndex+1,0 };
	if (dir == dir_left) MoveIterator(buffer, &it, -1);
	if (!pushValid(buffer, &it)) return &invalid;
	return getCharacter(buffer, it);
}

MultiGapBuffer createMultiGapBuffer(int size, DH_Allocator *allocator)
{
	MultiGapBuffer buffer = {};
	buffer.allocator = allocator;
	buffer.blocks = DA_BufferBlock::make(allocator);
	buffer.block = allocator->alloc(size*sizeof(char));
	buffer.capacity = size;
	buffer.running_cursor_id = 994; // just some non-0 number to catch some bad bugs.
	BufferBlock first = {0,0};
	BufferBlock last = {size,0};
	buffer.blocks.add(first);
	buffer.blocks.add(last);
	buffer.cursor_ids = DA_CursorIdentifier::make(allocator);
	buffer.cursor_ids.add({ 0,buffer.running_cursor_id });
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
	MemBlock new_block = buffer->allocator->alloc(buffer->capacity * multiple * sizeof(char));
	float gap = (new_block.cap- length(buffer)) / ((float)buffer->blocks.length - 1);
	float ack = 0;

	for (int i = 0; i < buffer->blocks.length; i++)
	{
		BufferBlock block = buffer->blocks.start[i];
		memmove(&((char *)new_block.mem)[(int)ack], &buffer->start[block.start], block.length*sizeof(char));
		buffer->blocks.start[i].start = (int)ack;
		ack += gap + buffer->blocks.start[i].length;
	}
	buffer->allocator->free(&buffer->block);
	buffer->block= new_block;
}

void grow(MultiGapBuffer *buffer)
{
	growTo(buffer, buffer->capacity * 2 * sizeof(char));
}

void maybeGrow(MultiGapBuffer *buffer)
{
	float load_factor = ((float)length(buffer) / (float)buffer->capacity);
	if (load_factor > 0.5)
	{
		grow(buffer);
	}
	else
	{
		rebalance(buffer, 0, 0);
	}
}
//function that mgb calls for internal moves, if it comes from history (log == false) we're doing something wrong!
inline void internal_move_index(MultiGapBuffer *mgb, History *history, int dir, int cursor_index, bool log) {
	assert(log);
	if(log)log_internal_move(history, dir, mgb->cursor_ids[cursor_index].id, mgb->cursor_ids[cursor_index].textBuffer_index);
	DHMA_SWAP(CursorIdentifier,mgb->cursor_ids[cursor_index], mgb->cursor_ids[cursor_index + dir]);
}


//function that history calls for internal moves
inline void internal_move(MultiGapBuffer *mgb, History *history, int dir, int cursor_id) {
	int cursor_index = indexFromId(mgb, cursor_id);
	DHMA_SWAP(CursorIdentifier, mgb->cursor_ids[cursor_index], mgb->cursor_ids[cursor_index + dir]);
}

//mgb calls for internal moves
int internal_push_index(MultiGapBuffer *mgb, History *history, int dir, int cursor_index, bool log) {
	if (dir == 1) {
		while (cursor_index < mgb->cursor_ids.length - 1) {
			if (getGap(mgb, cursor_index).next->length != 0) break;
			internal_move_index(mgb, history, 1, cursor_index, log);
			cursor_index += 1;
		}
	} else {
		while (cursor_index > 0) {
			if (getGap(mgb, cursor_index).prev->length != 0) break;
			internal_move_index(mgb, history, -1, cursor_index, log);
			cursor_index -= 1;
		}
	}
	return cursor_index;
}





int AddCaret_(MultiGapBuffer *buffer, int pos)
{ 
	// do no rebalancing, that's done on first insert.
	// this speeds things up if multiple cursors are added at once.
	MGB_Iterator it = getIterator(buffer, pos);
	BufferBlock *old_block = &buffer->blocks.start[it.block_index];
	BufferBlock new_block;
	new_block.start = old_block->start + it.sub_index;
	new_block.length = old_block->length - it.sub_index;
	old_block->length = it.sub_index;
	
	buffer->blocks.insert(new_block, it.block_index + 1);
	int caret_index = it.block_index;
	while (caret_index < buffer->blocks.length - 2 && getGap(buffer, caret_index).next->length == 0 )++caret_index; // we always do this at the end, we need internal consistency for history to work :)
	return caret_index;
}

void AddCaretWithId(MultiGapBuffer *buffer, int textBuffer_index, int pos, int Id)
{
	for (int i = 0; i < buffer->cursor_ids.length; i++)
	{
		assert(buffer->cursor_ids.start[i].id != Id);
	}
	buffer->cursor_ids.insert({ textBuffer_index, Id}, AddCaret_(buffer, pos));
}

int AddCaret(MultiGapBuffer *buffer, int textBuffer_index, int pos)
{
	int index;
	if (buffer->running_cursor_id == 994) // the first index is implicit. which is not great but it does simplyfy things ((slightly))
	{
		buffer->cursor_ids[0].textBuffer_index = textBuffer_index;
	}
	else
	{
		index = AddCaret_(buffer, pos);
		buffer->cursor_ids.insert({ textBuffer_index, buffer->running_cursor_id }, index);
	}
	return buffer->running_cursor_id++;
}

void appendCharacter(MultiGapBuffer *buffer, int caretId, char character)
{
	maybeGrow(buffer);
	int caretIndex = indexFromId(buffer, caretId);
	Gap gap= getGap(buffer,caretIndex);
	assert(gap.length);
	
	gap.prev->length++;
	*end(buffer, *gap.prev) = character;
}

void invDelete(MultiGapBuffer *buffer, int caretId, char character)
{
	maybeGrow(buffer);
	int caretIndex = indexFromId(buffer, caretId);
	Gap gap = getGap(buffer,caretIndex);
	assert(gap.length);
	
	--gap.next->start;
	gap.next->length++;
	*start(buffer, *gap.next)= character;
}

bool removeCharacter(MultiGapBuffer *buffer,History *history, bool log, int caretId)
{
	int caretIndex = indexFromId(buffer, caretId);

	caretIndex = internal_push_index(buffer, history, -1, caretIndex, log);
	BufferBlock *prev = getGap(buffer,caretIndex).prev;
	if (prev->length) {
		--prev->length;
		return true;
	} else {
		return false;
	}
}

bool del(MultiGapBuffer *buffer, History *history, bool log, int caretId) {
	int caretIndex = indexFromId(buffer, caretId);
	caretIndex = internal_push_index(buffer, history, 1, caretIndex, log);
	BufferBlock *next = getGap(buffer, caretIndex).next;
	if (next->length) {
		--next->length;
		++next->start;
		return true;
	} else {
		return false;
	}
}

void removeCaret(MultiGapBuffer *buffer, History *history, int caretId)
{
	//ie mergeblocks
	int caretIndex = indexFromId(buffer, caretId);
	caretIndex = internal_push_index(buffer, history, 1, caretIndex, true);    // we always do this at the end, we need internal consistency for history to work :)

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
	buffer->blocks.removeOrd(caretIndex + 1);
	buffer->cursor_ids.removeOrd(caretIndex);
}

bool ownsIndex(MultiGapBuffer *buffer, DA_int *ids, int targetIndex)
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

bool HasCaretAtIterator(MultiGapBuffer*buffer, DA_int *ids, MGB_Iterator it)
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

bool isEmptyCaret(MultiGapBuffer *buffer, DA_int *ids, int index)
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

bool mgb_moveLeft(MultiGapBuffer *buffer, History *history, bool log, int caretId)
{
	int caretIndex = indexFromId(buffer, caretId);
	assert(caretIndex >= 0 && caretIndex <= buffer->blocks.length - 2);
	BufferBlock *p = &buffer->blocks.start[caretIndex];
	BufferBlock *n = &buffer->blocks.start[caretIndex + 1];

	if ((caretIndex == 0 && p->length == 0))
		return false;
	
	if (!p->length)
	{
		internal_move_index(buffer,history,-1,caretIndex,log);
		CheckOverlapp(buffer);
		return mgb_moveLeft(buffer,history,log,caretId);
	}
	
	++n->length;
	--n->start;
	*start(buffer, *n) = *end(buffer, *p);
	--p->length;
	CheckOverlapp(buffer);
	return true;
}

bool mgb_moveRight(MultiGapBuffer *buffer, History *history, bool log, int caretId)
{
	int caretIndex = indexFromId(buffer, caretId);
	assert(caretIndex >= 0 && caretIndex <= buffer->blocks.length - 2);
	BufferBlock *p = &buffer->blocks.start[caretIndex];
	BufferBlock *n = &buffer->blocks.start[caretIndex + 1];

	if ((caretIndex == buffer->blocks.length - 2 && n->length == 0))
		return false;

	if (n->length == 0)
	{
		internal_move_index(buffer, history, 1, caretIndex,log);
		CheckOverlapp(buffer);
		return mgb_moveRight(buffer,history, log,caretId);
	}

	++p->length;
	*end(buffer, *p) = *start(buffer, *n);
	++n->start;
	--n->length;
	return true;
}


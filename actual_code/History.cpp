#include "History.h"
#include "DH_DataStructures.h"

int intFromDir(Direction dir)
{
	return dir == dir_left ? -1 : 1;
}

internal bool treatEQ(Action a, Action b)
{
	if ( (a == action_remove && b == action_delete) || (b == action_remove && a == action_delete))
	{
		return true;
	}
	return a == b;
}

internal void unDeleteCharacter(TextBuffer *textBuffer, char character, int caretIdIndex);
internal bool removeCharacter(TextBuffer *textBuffer, int caretIdIndex, bool log);
//internal void AddCaret(TextBuffer *buffer, int pos);
internal void removeCaret(TextBuffer *buffer, int caretIdIndex,bool log);
void InsertCaret(TextBuffer *buffer, int pos, int index);
void redoMany(TextBuffer *textBuffer);
bool get_prev_HistoryEntry_index(History *history, int *in_out_index)
{
	HistoryBranch elem;
	SparseArrayIterator_HistoryBranch it = make_iterator(&history->branches, *in_out_index);
	while (get_next(&history->branches, &it, *in_out_index, &elem))
	{
		if (elem.direction == branch_target)
		{
			*in_out_index = elem.index;
			return true;
		}
	}
	--*in_out_index;
	return *in_out_index >= 0;
}

void insert_event_marker(History *history, HistoryMarkerType type)
{
	HistoryEventMarker marker = {};
	marker.sparseArray_target_index = history->current_index;
	marker.type = type;
	Insert(&history->events, marker, ordered_insert_dont_care);
}

bool get_next_HistoryEntry_index(History *history, int *in_out_index) //in out is stupid though...
{
	HistoryBranch elem;
	SparseArrayIterator_HistoryBranch it = make_iterator(&history->branches, *in_out_index);
	while (get_next(&history->branches, &it, *in_out_index, &elem))
	{
		if (elem.direction == branch_target)
		{
			*in_out_index = elem.index;
			return true;
		}
	}
	bool success = *in_out_index < history->entries.length-1; // why the minus one though?
	if(success)
		++*in_out_index;
	return success;
}
internal int getLineLength(BackingBuffer *backingBuffer, int line);

Location move_right(TextBuffer *textBuffer, Location loc)
{
	++loc.column;
	if (loc.column > getLineLength(textBuffer->backingBuffer, loc.line))
	{
		loc.column = 0;
		++loc.line;
	}
	return loc;
}

void addChangeEvent(TextBuffer *textBuffer, HistoryEntry entry)
{
	Location loc = entry.location;
	BufferChangeAction action;
	DynamicArray_BufferChange *change_log = &textBuffer->backingBuffer->history.change_log;

	switch (entry.action)
	{
	case action_move:
		return;
	case action_delete:
		loc = move_right(textBuffer, loc);
	case action_remove:
		action = buffer_change_remove;
		break;
	case action_undelete:
		loc = move_right(textBuffer, loc);
	case action_add:
		action = buffer_change_add;
		break;
	}
	BufferChange ch = {};
	ch.action = action;
	ch.location = loc;
	ch.character = entry.character;
	Add(&textBuffer->backingBuffer->history.change_log, ch);
}

HistoryEntry invert_event(HistoryEntry prev_entry)
{
	switch (prev_entry.action)
	{
	case action_move:
		prev_entry.direction = -prev_entry.direction;
		break;
	case action_add:
		prev_entry.action = action_remove;
		break;
	case action_remove:
		prev_entry.action = action_add;
		break;
	case action_delete:
		prev_entry.action = action_undelete;
		break;
	case action_undelete:
		prev_entry.action = action_delete;
		break;
	case action_addCaret:
		prev_entry.action = action_removeCaret;
		break;
	case action_removeCaret:
		prev_entry.action = action_addCaret;
		break;
	default:
		assert(false && "action not found.");
	}
	return prev_entry;
}


void undo_history_event(TextBuffer *textBuffer, HistoryEntry prev_entry)
{
	addChangeEvent(textBuffer,invert_event(prev_entry));
	switch (prev_entry.action)
	{
	case action_move:
		for (int i = 0; i < -prev_entry.direction; i++)
			move_llnc(textBuffer, dir_right, prev_entry.caretIdIndex, false, prev_entry.selection ? move_caret_selection : move_caret_insert, movemode_byte);
		for (int i = 0; i < prev_entry.direction; i++)
			move_llnc(textBuffer, dir_left, prev_entry.caretIdIndex, false, prev_entry.selection ? move_caret_selection : move_caret_insert, movemode_byte);
		break;
	case action_add:
		removeCharacter(textBuffer, prev_entry.caretIdIndex, false);
		break;
	case action_remove:
		appendCharacter(textBuffer, prev_entry.character, prev_entry.caretIdIndex, false);
		break;
	case action_delete:
		unDeleteCharacter(textBuffer, prev_entry.character, prev_entry.caretIdIndex);
		break;
	case action_undelete:
		deleteCharacter(textBuffer, prev_entry.caretIdIndex, false);
		break;
	case action_addCaret:
		removeCaret(textBuffer, prev_entry.caretIdIndex, false);
		break;
	case action_removeCaret:
	{
		InsertCaret(textBuffer, prev_entry.pos_selection, prev_entry.caretIdIndex); 
		int diff = prev_entry.pos_caret - prev_entry.pos_selection;
		for (int i = 0; i < abs(diff); i++)
		{
			move(textBuffer, ((diff<0) ? dir_left : dir_right), prev_entry.caretIdIndex, no_log, do_select);
		}
	} break;
	default:
		assert(false && "action not found.");
		break;
	}
}



bool can_redo(History *history){
	return history->current_index < history->entries.length;
}
bool can_undo(History *history){
	return history->current_index > 0;
}


bool undo(TextBuffer *textBuffer)
{
	History *history = &textBuffer->backingBuffer->history;
	int prev_index = history->current_index;
	if (!get_prev_HistoryEntry_index(history, &prev_index)) return false;
	HistoryEntry prev = history->entries.start[prev_index];
	
	undo_history_event(textBuffer, prev);
	history->current_index = prev_index;
	return true;
}

bool redo(TextBuffer *textBuffer)
{
	History *history = &textBuffer->backingBuffer->history;
	if (can_redo(history))
	{
		int index = textBuffer->backingBuffer->history.current_index;
		HistoryEntry next = textBuffer->backingBuffer->history.entries.start[index]; 
		undo_history_event(textBuffer, invert_event(next));
		
		if (!get_next_HistoryEntry_index(history, &index))
			++textBuffer->backingBuffer->history.current_index;
		else
			textBuffer->backingBuffer->history.current_index = index; //this makes no sense right???
		return true;
	}
	return false;
}


void redoMany(TextBuffer *textBuffer)
{
	History *history = &textBuffer->backingBuffer->history;
	HistoryEventMarker elem;
	int depth=0;
	while (can_redo(history))
	{
		int index = history->current_index;
		redo(textBuffer);
		//can be sped up by moving it out and conditionally doing stuff, I won't bother now though.
		SparseArrayIterator_HistoryEventMarker it = make_iterator(&history->events,index);
		while (get_next(&history->events, &it, index, &elem))
		{
			switch (elem.type)
			{
			case history_mark_weak_end:
				if (depth == 0)
					return;
				break;
			case history_mark_start:
				++depth;
				break;
			case history_mark_end:
				--depth;
				if (depth == 0)
					return;
				break;
			}
		}
	}
	assert(depth == 0 && "unbalanced events...");
}

void undoMany(TextBuffer *textBuffer)
{
	History *history = &textBuffer->backingBuffer->history;
	HistoryEventMarker elem;
	int depth = 0;
	while (can_undo(history))
	{
		int index = history->current_index;
		//can be sped up by moving it out and conditionally doing stuff, I won't bother now though.
		SparseArrayIterator_HistoryEventMarker it = make_iterator(&history->events, index);
		while (get_next(&history->events, &it, index, &elem))
		{
			if(elem.type == history_mark_end)
				++depth;
		}
		undo(textBuffer);
		index = history->current_index;
		it = make_iterator(&history->events, index);
		while (get_next(&history->events, &it, index, &elem))
		{
			switch (elem.type)
			{
			case history_mark_weak_end:
				if (depth == 0)
					return;
				break;
			case history_mark_start:
				--depth;
				if (depth == 0)
					return;
				break;
			}
		}
	}
	assert(depth == 0 && "unbalanced events...");
}

//do changes for __curent__index___ to work!
void branch_from_current(History *history)
{
	HistoryBranch branch;
	branch.direction = branch_start;
	branch.sparseArray_target_index = history->current_index;
	branch.index = history->entries.length;
	Insert(&history->branches, branch, ordered_insert_first);

	branch.direction = branch_target;
	branch.sparseArray_target_index = history->entries.length;
	branch.index = history->current_index;
	Insert(&history->branches, branch, ordered_insert_first);
	history->current_index = history->entries.length;
}

void log_base(TextBuffer *textBuffer, HistoryEntry new_entry) // no maths lol
{
	History *history = &textBuffer->backingBuffer->history;
	if (!treatEQ(history->entries.start[history->current_index].action, new_entry.action))
	{
	//	insert_event_marker(history, history_mark_weak_end); // ayy weekend 
	}
	if (history->current_index != history->entries.length)
	{
		branch_from_current(history);
	}
	Add(&history->entries, new_entry);
	history->current_index = history->entries.length;
	addChangeEvent(textBuffer, new_entry);
}

void log_base(TextBuffer *textBuffer, Action action, char character, int caretIdIndex,Location location)
{
	History *history = &textBuffer->backingBuffer->history;
	HistoryEntry prev_entry = {};
	prev_entry.action = action;
	prev_entry.caretIdIndex = caretIdIndex;
	prev_entry.character = character;
	prev_entry.location = location;
	log_base(textBuffer, prev_entry);
}

void logRemoved(TextBuffer *textBuffer, char character, int caretIdIndex, Location location)
{
	log_base(textBuffer, action_remove, character, caretIdIndex, location);
}

void logDeleted(TextBuffer *textBuffer, char character, int caretIdIndex, Location location)
{
	log_base(textBuffer, action_delete, character, caretIdIndex, location);
}

void logAdded(TextBuffer *textBuffer, char character, int caretIdIndex, Location location)
{
	log_base(textBuffer, action_add, character, caretIdIndex, location);
}

void logUndelete(TextBuffer *textBuffer, char character, int caretIdIndex, Location location)
{
	log_base(textBuffer, action_undelete, character, caretIdIndex, location);
}

void logMoved(TextBuffer *textBuffer, Direction direction, bool selection, int caretIdIndex, Location location)
{
	History *history = &textBuffer->backingBuffer->history;
	int prev_index=history->current_index;
	for (int i = 0; i < textBuffer->backingBuffer->binding_next_change.capacity;i++)
	{
		textBuffer->backingBuffer->binding_next_change.buckets[i].value.move_change = true;
	}
	while (get_prev_HistoryEntry_index(history, &prev_index))
	{
		HistoryEntry *prev = &history->entries.start[prev_index];
		if (prev->action == action_move)
		{
			if (prev->caretIdIndex == caretIdIndex && prev->selection == selection) //reuse the old one
			{
				prev->direction += intFromDir(direction);
				return;
			}
		}
		else
		{
			break;
		}
	}

	{
		HistoryEntry prev_entry = {};
		prev_entry.action = action_move;
		prev_entry.caretIdIndex = caretIdIndex;
		prev_entry.direction = intFromDir(direction);
		prev_entry.selection = selection;
		prev_entry.location = location;
		log_base(textBuffer, prev_entry);
	}
}

void logAddCaret(TextBuffer *textBuffer, int pos_caret, int pos_selection, int caretIdIndex, Location location)
{
	//do branch
	HistoryEntry prev_entry = {};
	prev_entry.action = action_addCaret;
	prev_entry.caretIdIndex = caretIdIndex;
	prev_entry.pos_caret = pos_caret;
	prev_entry.pos_selection = pos_selection;
	prev_entry.location = location;
	log_base(textBuffer, prev_entry);
}

void logRemoveCaret(TextBuffer *textBuffer, int pos_caret, int pos_selection, int caretIdIndex,Location location)
{
	//do branch
	HistoryEntry prev_entry = {};
	prev_entry.action = action_removeCaret;
	prev_entry.caretIdIndex = caretIdIndex;
	prev_entry.pos_caret = pos_caret;
	prev_entry.pos_selection = pos_selection;
	prev_entry.location = location;
	log_base(textBuffer, prev_entry);
}

void fast_forward_history_changes(BackingBuffer *buffer, void *function)
{
	HistoryChangeTracker *change_tracker;
	if (!lookup(&buffer->binding_next_change, function, &change_tracker))
	{
		change_tracker = insert(&buffer->binding_next_change, function, {});
	}
	change_tracker->next_index = buffer->history.change_log.length;
	change_tracker->move_change = false;
}

bool has_move_changed(BackingBuffer *buffer, void *function)
{
	HistoryChangeTracker *change_tracker;
	if (!lookup(&buffer->binding_next_change, function, &change_tracker))
	{
		HistoryChangeTracker ch = {};
		ch.next_index = 0;
		change_tracker = insert(&buffer->binding_next_change, function, ch);
	}
	bool ret = change_tracker->move_change;
	change_tracker->move_change = false;
	return ret;
}

bool next_history_change(BackingBuffer *buffer, void *function, BufferChange *_out_change)
{ 
	HistoryChangeTracker *change_tracker;
	if (!lookup(&buffer->binding_next_change, function, &change_tracker)) 
	{
		HistoryChangeTracker ch = {};
		change_tracker = insert(&buffer->binding_next_change, function, ch);
	}
	DynamicArray_HistoryEntry entries = buffer->history.entries;

	// since move events are allowed to be changed not just appended like the rest we need to hande that case as well.
	// ie. A move event can change from 1 move to the left to 5 moves to the left ( or to the right for that matter)
	// without appending additional events
	// api users does not have to deal with that shit, it's just memory optimization. 

	if (change_tracker->next_index < buffer->history.change_log.length)
	{
		BufferChange reference = buffer->history.change_log.start[change_tracker->next_index];

		++change_tracker->next_index;
		*_out_change = reference;
		return true;
	}
	
	_out_change = (BufferChange *)0; // if somebody illegally tries to deref this having this be null is the best course of action
	return false;
}

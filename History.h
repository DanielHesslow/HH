#ifndef HISTORY_H
#define HISTORY_H


#define DHEN_NAME Action
#define DHEN_PREFIX action
#define DHEN_VALUES X(move) X(add) X(remove) X(delete) X(undelete) X(addCaret) X(removeCaret)
#include "enums.h"
#undef DHEN_NAME
#undef DHEN_PREFIX
#undef DHEN_VALUES




struct HistoryEntry
{
	Action action;
	int caretIdIndex;
	bool selection; 
	union {
		struct 
		{
			int pos_caret;
			int pos_selection;
		};
		int direction;
		char16_t character;
	};
	Location location;
};

DEFINE_DynamicArray(HistoryEntry)

enum BranchDirection
{
	branch_start,
	branch_target,
};

struct HistoryBranch
{
	BranchDirection direction; //a bit redundant since we can only come from smaller indices but whatever
	int index;
	intrusive_sparse_array_body;
};

enum HistoryMarkerType
{
	history_mark_weak_end, //ends undo/redo if on toplevel
	history_mark_end, //ends an event
	history_mark_start, //starts an event
};

struct HistoryEventMarker
{
	HistoryMarkerType type;
	intrusive_sparse_array_body;
};


DEFINE_Complete_SparseArray(HistoryBranch);
DEFINE_Complete_SparseArray(HistoryEventMarker);

DEFINE_DynamicArray(BufferChange);

struct History
{
	DynamicArray_HistoryEntry entries;    //history
	DynamicArray_BufferChange change_log; //highlevel changes of the buffer, what we can listen to.
	int current_index;
	ORD_DynamicArray_HistoryBranch branches; // as a sparse array
	ORD_DynamicArray_HistoryEventMarker events; // as a sparse array
};


internal int  intFromDir(Direction dir);
internal bool treatEQ(Action a, Action b);
internal bool undo(TextBuffer *textBuffer);
internal bool redo(TextBuffer *textBuffer);
internal void undoMany(TextBuffer *textBuffer);
internal void logRemoved(TextBuffer *textBuffer, char character, int caretIdIndex,Location location);
internal void logDeleted(TextBuffer *textBuffer, char character, int caretIdIndex, Location location);
internal void logAdded(TextBuffer *textBuffer, char character, int caretIdIndex, Location location);
internal void logMoved(TextBuffer *textBuffer, Direction direction, bool selection, int caretIdIndex, Location location);
internal void logAddCaret(TextBuffer *textBuffer, int pos_caret, int pos_selection, int caretIdIndex, Location location);
internal void logRemoveCaret(TextBuffer *textBuffer, int pos_caret, int pos_selection, int caretIdIndex, Location location);
internal void insert_event_marker(History *history, HistoryMarkerType type);

#endif
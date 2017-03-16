#include "History.h"

// RATIONALE / DESIGN DECISSIONS
// Why the fuck is this history handeling 800 lines long?
// like what were you thinking, this is trivial, right?

// Nah, man, not totally trivial anyway. 
// First off we're never overwriting history, so we've got a DAG, (directed asyclic graph).
// Second, we need to not waste a bunch of memory, it's totally valid to write code for 10 hours straight, a lot of stuff can happen in that time
// Most of the complexity of this code is because of optimizations on memory consumptions
// eg, DAGs ususually have a transision matrix but the normal case for us is one branch forward and no branch otherwise
// sometimes we have no branch forward, sometimes we have branches elsewhere
// therefor the forward branch is implicit except if some other branch ends there.

// Saving all information for a certain action is not ok, usually we manipulate in the same view and the same cursors multiple times
// if we'd save that info we get _at least_ 8 bytes per action which is *a lot*,
// (currently the case of a character insert is only 1 byte data + 1 byte opcode, factor of five better)
// so we can't do that, instead we have a state which we can manipulate, usually we might go back and forth between multiple different states
// so therefor we have what I call registers which we save down,

// furthermore some instructions like move takes no data and some like set_cursor takes a bunch (like 8 or something)
// so we have to have a variable data length, since we need to go back and forth we could either do the unicode thing and have leading bits
// in each byte which indicates how many more bytes there are in the current instruction etc.
// or we can have them be separate steams. I choose the latter. There might obviously be a better solution which I havn't considered. 

// note all manipulations are relative to previous values, so we can undo/redo them.
// further complicating stuff moves are lazy because we usually move multiple cursors left and right and "patching" them after they're added seemd too complicated

// NOTE TODO
// NOT YET WORKING with mutliple views
// we need to make sure that we can only undo stuff that we've made in one view. undoing stuff in other views seems counter intuitive to me
// certainly when/if we get networked connections from other people. 

// NOTE PREF
// we currently store branches in unsorted dynamic arrays, this makes for slow lookup times
// same goes for undo_waypoints
// keeping two dynamic arrays for the branches one sorted in from and one to and then doing binary searches seams quite a bit faster to me 
// two hashtables might also work but that'd be slow for iterating over branches in order which we need for next_leaf (but that is super rare anyway)
// however I don't see this beeing a problem anytime soon, undoing/redoing is rare, however the need to maybe branch to end on each log
// might indicate that a hashtable for at least the to-branches might be a good idea. 
// we'll get to that if it's needed.

// Oh and we cursor additions/removes should probably be lazy as well. 

// Another alternative to this is to employ full on compression, that would for sure mean we get a lower memory foot print and the local complexity goes down
// However I don't know about the perf. Might actually be better. Might be better to keep this and do it post. IDK.
// Currently we're where we need to be. ish.


		
bool at_start(History *history, HistoryLocation location);
bool at_end(History *history, HistoryLocation location);
bool move_backward(History *history, HistoryLocation *location);
bool move_forward(History *history, HistoryLocation *location, int branch_index);

struct DataAddRemoveCursor
{
	int pos;
	bool is_selection;
};

struct InstructionData
{
	union
	{
		char add_remove;
		int  move;
		HCursorInfo set_cursor;
		DataAddRemoveCursor add_remove_cursor;
	};
};

int data_length_from_opcode(HistoryOpCode op_code)
{
	switch (op_code)
	{
	case op_add:
	case op_remove:
		return sizeof(char);
	case op_move:
		return sizeof(int);
	case op_internal_move:
		return 0; // no data bitch, unary is king.
	case op_remove_cursor:
	case op_add_cursor:
		return sizeof(DataAddRemoveCursor);
	case op_set_cursor:
		return sizeof(HCursorInfo);
	}
	assert(false && "unhandled history_op_code");
	return 0;
}

HistoryLocation last_inserted_instruction(History *history)
{
	HistoryLocation end;
	end.prev_data_index = history->data.length;
	end.prev_instruction_index = history->instructions.length;
	move_backward(history, &end);
	return end;
}

void set_branch(History *history, HistoryLocation from, int index)
{
	for (int i = 0; i < history->branches.length; i++)
	{
		if (history->branches.start[i].from == from)
		{
			history->branches.start[i].last_used = (i == (index-1));
		}
	}
}

void branch_current_to_end(History *history)
{
	HistoryLocation end;
	end.prev_data_index = history->data.length;
	end.prev_instruction_index = history->instructions.length;

	Branch branch;
	branch.from = history->state.location;
	branch.to = end;
	branch.last_used = true;
	if (branch.from != branch.to) //check seems unneccsary no?
	{
		set_branch(history,branch.from,0);
		history->branches.add(branch);
		history->state.location = branch.to;
	}
}

Instruction *instruction_from_location(History *history, HistoryLocation loc, void **data)
{
	if (at_start(history,loc))
	{
		if (data) *data = 0;
		return 0;
	}
	else
	{
		if (data) *data = &history->data.start[loc.prev_data_index];
		return &history->instructions.start[loc.prev_instruction_index];
	}
}

void append_instruction_raw(History *history, Instruction instruction, void *data)
{
	HistoryLocation *location = &history->state.location;

	if (history->state.location.prev_instruction_index != history->instructions.length - 1)
	{
		branch_current_to_end(history);
	}
	else
	{
		Instruction *instr = instruction_from_location(history,history->state.location, 0);
		if (!instr) location->prev_data_index += 1;
		else location->prev_data_index += data_length_from_opcode(instr->op_code);
		++location->prev_instruction_index;
	}
	history->instructions.add(instruction);
	int data_len = data_length_from_opcode(instruction.op_code);
	for (int i = 0; i < data_len; i++)
		history->data.add(((uint8_t *)data)[i]);

}

void append_instruction(History *history, HistoryOpCode op_code, int cursor_id, int view_id, int direction, void *data)
{
	Instruction instruction = {};
	for (int i = 0; i < 8; i++)
	{
		if (history->state.cursor_reg[i].cursor_id == cursor_id)
		{
			assert(history->state.cursor_reg[i].cursor_id == cursor_id);
			instruction.cursor_reg_index = i;
			goto cursor_set;
		}
	}

	{	//insert caret information into caret 'register'
		history->state.last_cursor_reg = (history->state.last_cursor_reg + 1) & 7; //this can proably be done in a better way.
		HCursorInfo old_info = history->state.cursor_reg[history->state.last_cursor_reg];
		HCursorInfo relative = {};
		relative.cursor_id = cursor_id - old_info.cursor_id;
		relative.view_id =  view_id - old_info.view_id;
		HCursorInfo absolute= {};
		absolute.cursor_id = cursor_id;
		absolute.view_id = view_id;

		history->state.cursor_reg[history->state.last_cursor_reg] = absolute;

		instruction.cursor_reg_index = history->state.last_cursor_reg;
		instruction.op_code = op_set_cursor;
		append_instruction_raw(history, instruction, &relative);
	}

cursor_set:
	instruction.op_code = op_code;
	instruction.direction = direction > 0;
	append_instruction_raw(history, instruction, data);
}


void _log_lazy_moves(History *history)
{
	for (int i = 0; i < history->waiting_moves.length; i++)
	{
		WaitingMove move = history->waiting_moves.start[i];
		int dir = move.direction > 0 ? 1 : -1;
		int magnitude = move.direction*dir;
		append_instruction(history, op_move,move.cursor_id,move.view_id,move.direction, &magnitude);
	};
	history->waiting_moves.length = 0;
}

inline bool checked_add_s32(int32_t a, int32_t b, int32_t *_out_res)
{
	int64_t res64 = (int64_t)a + (int64_t)b;
	int32_t res32 = (int32_t)res64;
	*_out_res = res32;
	return res32 == res64;
}

// --- header declared functions
void log_move(History *history, int direction, int cursor_id, int view_id)
{
	for (int i = 0; i < history->waiting_moves.length; i++)
	{
		if (history->waiting_moves.start[i].cursor_id == cursor_id)
		{
			WaitingMove *move = &history->waiting_moves.start[i];
			int res;
			if (checked_add_s32(move->direction, direction, &res))
			{
				move->direction = res;
				return;
			}
			else
			{
				_log_lazy_moves(history);
			}
		}
	}
	WaitingMove move = {};
	move.cursor_id = cursor_id;
	move.direction = direction;
	move.view_id= view_id;
	history->waiting_moves.add(move);
}


void log_internal_move(History *history, int8_t direction, int cursor_id, int view_id) 	{
	_log_lazy_moves(history);
	append_instruction(history, op_internal_move, cursor_id, view_id, direction, NULL);

}

void log_add(History *history, int8_t direction, char byte, int cursor_id, int view_id)
{ 
	_log_lazy_moves(history);
	append_instruction(history, op_add, cursor_id, view_id, direction,&byte);
}

void log_remove(History *history, int direction, char byte, int cursor_id, int view_id)
{
	_log_lazy_moves(history);
	append_instruction(history, op_remove,cursor_id,view_id,direction, &byte);
}

void log_add_cursor(History *history, int pos, bool is_selection, int cursor_id, int view_id)
{
	_log_lazy_moves(history);
	DataAddRemoveCursor data= {};
	data.pos = pos;
	data.is_selection = is_selection;
	append_instruction(history, op_add_cursor,cursor_id,view_id,0,&data);
}

void log_remove_cursor(History *history, int pos, bool is_selection,int cursor_id, int view_id)
{
	_log_lazy_moves(history);
	DataAddRemoveCursor data = {};
	data.pos = pos;
	data.is_selection = is_selection;
	append_instruction(history, op_remove_cursor, cursor_id, view_id,0, &data);
}

int data_length_at_position(History*history, int instruction_index)
{
	if (instruction_index == -1)return 1;
	else return data_length_from_opcode(history->instructions.start[instruction_index].op_code);
}

int num_branches(History *history, HistoryLocation location)
{
	if (at_end(history,location))return 0;
	int ack = 0;
	//branches from current
	for (int i = 0; i < history->branches.length; i++)
	{
		if (history->branches.start[i].from == location)
		{
			++ack;
		}
	}

	//if there's one ahead
	location.prev_data_index += data_length_at_position(history, location.prev_instruction_index);
	++location.prev_instruction_index;
	for (int i = 0; i < history->branches.length; i++)
	{
		if (history->branches.start[i].to == location)
		{
			return ack;
		}
	}
	return ack + 1;
}

bool at_end(History *history, HistoryLocation location)
{
	return ((history->instructions.length - 1) <= location.prev_instruction_index);
}

bool at_start(History *history, HistoryLocation location)
{
	return location.prev_instruction_index <= -1;
}

bool can_redo(History *history, HistoryLocation location)
{
	return num_branches(history, location) > 0;
}

bool can_undo(History *history, HistoryLocation location)
{
	return !at_start(history, location);
}



void move_forward_ignore_branches(History *history, HistoryLocation *location)
{
	location->prev_data_index += data_length_at_position(history, location->prev_instruction_index);
	++location->prev_instruction_index;
}

bool move_forward(History *history, HistoryLocation *location, int branch_index)
{ //state = -1 -1 = 'before buffer'
	if (at_end(history,*location))
	{
		return false;
	}

	if (branch_index == -1)
	{
		for (int i = 0; i< history->branches.length; i++)
		{
			if (history->branches.start[i].from == *location)
			{
				if (history->branches.start[i].last_used)
				{
					*location = history->branches.start[i].to;
					return true;
				}
			}
		}

		HistoryLocation old = *location;
		location->prev_data_index += data_length_at_position(history, location->prev_instruction_index);
		++location->prev_instruction_index;

		for (int i = 0; i < history->branches.length; i++)
		{
			if (history->branches.start[i].to == *location)
			{
				*location = old;
				return false;
			}
		}
		return true;
	}
	else
	{
		if (!branch_index)
		{
			HistoryLocation old = *location;
			location->prev_data_index += data_length_at_position(history, location->prev_instruction_index);
			++location->prev_instruction_index;
			bool success = true;
			for (int i = 0; i < history->branches.length; i++)
			{
				if (history->branches.start[i].to == *location)
				{
					*location = old;
					success = false;
				}
			}
			if (success)
			{
				set_branch(history,old, 0);
				return true;
			}
		}
		else
		{
			--branch_index;
		}

		for (int i = 0; i < history->branches.length; i++)
		{
			if (history->branches.start[i].from == *location)
			{
				if (!(branch_index--))
				{
					set_branch(history,*location,i+1);
					*location = history->branches.start[i].to;
					return true;
				}
			}
		}
	}
	assert(false&& "redo bottom out fail");
	return false;
}

void move_backward_ignore_branches(History *history, HistoryLocation *location)
{
	--location->prev_instruction_index;
	location->prev_data_index -= data_length_at_position(history, location->prev_instruction_index);
}

bool move_backward(History *history, HistoryLocation *location)
{ //state = -1 -1 = 'before buffer'
	if (at_start(history, *location))return false;
	if (!location->prev_instruction_index)
	{
		*location = { -1,-1 };
	}
	else
	{
		for (int i = 0; i < history->branches.length; i++) // slow af. update me when we have bimaps or atleast hashmaps with multipe values
		{
			if (history->branches.start[i].to == *location)
			{
				*location  = history->branches.start[i].from;
				return true;
			}
		}
		move_backward_ignore_branches(history, location);
	}
	return true;
}
struct ExpandedInstruction
{
	union
	{
		HCursorInfo *cursor_info;
	};
	int cursor_id;

	int direction;
	TextBuffer *textBuffer;
	int cursor_id_index;
	bool is_selection;
	HistoryOpCode op_code;
	union
	{
		char add_remove;
		int  move;
		HCursorInfo set_cursor;
		DataAddRemoveCursor add_remove_cursor;
	}data;
};

bool expand_instruction_at(BackingBuffer *backingBuffer, HistoryLocation location, ExpandedInstruction *_out_instr)
{
	ExpandedInstruction ret = {};
	History *history = &backingBuffer->history;
	void *data;
	Instruction *instr = instruction_from_location(history, location, &data);
	if (!instr)return false;
	ret.cursor_info = &history->state.cursor_reg[instr->cursor_reg_index];
	ret.cursor_id = ret.cursor_info->cursor_id;

	switch (instr->op_code)
	{
	case op_add:
	case op_remove:
		ret.data.add_remove = *(char *)data;
		break;
	case op_move:
		ret.data.move= *(int*)data;
		break;
	case op_remove_cursor:
	case op_add_cursor:
		ret.data.add_remove_cursor= *(DataAddRemoveCursor *)data;
		break;
	case op_set_cursor:
		ret.data.set_cursor = *(HCursorInfo *)data;
		break;
	}
	for (int i = 0; i <backingBuffer->textBuffers.length; i++)
	{
		if (backingBuffer->textBuffers.start[i]->textBuffer_id == ret.cursor_info->view_id) ret.textBuffer = backingBuffer->textBuffers.start[i];
	}
	ret.cursor_id_index = -1;
	ret.op_code = instr->op_code;
	ret.direction = instr->direction;
	if (!ret.textBuffer)
	{
		*_out_instr = ret;
		return true;
	}

	for (int i = 0; i < ret.textBuffer->ownedCarets_id.length; i++)
	{
		if (ret.textBuffer->ownedCarets_id.start[i] == ret.cursor_id)
		{
			ret.cursor_id_index= i;
			ret.is_selection = false;
		}
		
	}
	for (int i = 0; i < ret.textBuffer->ownedSelection_id.length; i++)
	{
		if (ret.textBuffer->ownedSelection_id.start[i] == ret.cursor_id)
		{
			ret.cursor_id_index = i;
			ret.is_selection = true;
		}
	}
	*_out_instr = ret;
	return true;
}

bool undo_(BackingBuffer *backingBuffer)
{
	History *history = &backingBuffer->history;
	MultiGapBuffer *mgb = backingBuffer->buffer;
	_log_lazy_moves(history);
	ExpandedInstruction instr;

	bool success = expand_instruction_at(backingBuffer, history->state.location, &instr);
	if (!success) return false;
	switch (instr.op_code)
	{
	case op_add:
		assert(instr.cursor_id_index != -1 && "textBuffer doens't own cursor that history said it did");
		if (instr.direction > 0)
			deleteCharacter(instr.textBuffer, instr.cursor_id_index, false);
		else
			removeCharacter(instr.textBuffer, instr.cursor_id_index, false);
		break;
	case op_remove:
		assert(instr.cursor_id_index != -1 && "textBuffer doens't own cursor that history said it did");
		if (instr.direction > 0)
			unDeleteCharacter(instr.textBuffer, instr.data.add_remove, instr.cursor_id_index,false);
		else
			appendCharacter(instr.textBuffer, instr.data.add_remove, instr.cursor_id_index, false);
		break;
	case op_set_cursor:
	{
		instr.cursor_info->cursor_id -= instr.data.set_cursor.cursor_id;
		instr.cursor_info->view_id -= instr.data.set_cursor.view_id;
	} break;
	case op_move:
	{
		Direction dir = instr.direction > 0 ? dir_left: dir_right;
		for (int i = 0; i < instr.data.move; i++)
			move_llnc_(instr.textBuffer,dir, instr.cursor_id, false, movemode_byte);
		break;
	}
	case op_internal_move:
	{
		int dir = instr.direction > 0 ? -1 : 1;
		internal_move(mgb,history, dir, instr.cursor_id);
	}break;
	case op_remove_cursor:
	{	
		DataAddRemoveCursor data = instr.data.add_remove_cursor;
		AddCaretWithId(mgb, instr.cursor_info->view_id, data.pos, instr.cursor_id);
		auto DA = data.is_selection ? &instr.textBuffer->ownedSelection_id : &instr.textBuffer->ownedCarets_id;
		DA->add(instr.cursor_id);
	}break;
	case op_add_cursor:
	{

		DataAddRemoveCursor data = instr.data.add_remove_cursor;
		removeCaret(mgb, instr.cursor_id);
		auto DA = data.is_selection ? &instr.textBuffer->ownedSelection_id : &instr.textBuffer->ownedCarets_id;
		DA->removeOrd(instr.cursor_id_index);

	}break;
	}
	return move_backward(history, &history->state.location); //we're not right,are we?
}


bool redo_(BackingBuffer *backingBuffer, int index)
{
	History *history = &backingBuffer->history;
	MultiGapBuffer *mgb = backingBuffer->buffer;
	HistoryLocation loc = history->state.location;
	if (!move_forward(history, &loc, index))return false;

	_log_lazy_moves(history);
	ExpandedInstruction instr;
	bool success = expand_instruction_at(backingBuffer, loc, &instr);
	if (!success) return false;

	switch (instr.op_code)
	{
	case op_remove:
		assert(instr.cursor_id_index != -1 && "textBuffer doens't own cursor that history said it did");
		if (instr.direction > 0)
			deleteCharacter(instr.textBuffer, instr.cursor_id_index, false);
		else
			removeCharacter(instr.textBuffer, instr.cursor_id_index, false);
		break;
	case op_add:
		assert(instr.cursor_id_index != -1 && "textBuffer doens't own cursor that history said it did");
		if (instr.direction > 0)
			unDeleteCharacter(instr.textBuffer, instr.data.add_remove, instr.cursor_id_index,false);
		else
			appendCharacter(instr.textBuffer, instr.data.add_remove, instr.cursor_id_index, false);
		break;
	case op_set_cursor:
	{
		instr.cursor_info->cursor_id += instr.data.set_cursor.cursor_id;
		instr.cursor_info->view_id   += instr.data.set_cursor.view_id;
	} break;
	case op_move:
	{
		Direction dir = instr.direction > 0 ? dir_right : dir_left;
		for (int i = 0; i < instr.data.move; i++)
			move_llnc_(instr.textBuffer, dir, instr.cursor_id, false, movemode_byte);
		break;
	}
	case op_internal_move:
	{
		int dir = instr.direction > 0 ? 1 : -1;
		internal_move(mgb,history, dir,instr.cursor_id);
		break;
	}
	case op_add_cursor:
	{
		DataAddRemoveCursor data = instr.data.add_remove_cursor;
		AddCaretWithId(mgb, instr.cursor_info->view_id, data.pos, instr.cursor_id);
		auto DA = data.is_selection ? &instr.textBuffer->ownedSelection_id : &instr.textBuffer->ownedCarets_id;
		DA->add(instr.cursor_id);
	}break;
	case op_remove_cursor:
	{
		assert(instr.cursor_id_index != -1);
		DataAddRemoveCursor data = instr.data.add_remove_cursor;
		removeCaret(mgb, instr.cursor_id);
		auto DA = data.is_selection ? &instr.textBuffer->ownedSelection_id : &instr.textBuffer->ownedCarets_id;
		DA->removeOrd(instr.cursor_id_index);

	}break;
	}
	history->state.location = loc;
	return true;
}





struct HistoryIterator
{
	bool add_remove;
	bool move;
	bool cursor_add_remove;
};

// note branchindexing doesn't work right.
// we continue on with the same branch index which will break stuff. aint good.
bool waypoint_at(History *history, int prev_instruction_index)
{
	for (int i = 0; i < history->waypoints.length; i++)
	{
		if (history->waypoints.start[i] == prev_instruction_index) return true;
	}
	return false;
}

void redo(BackingBuffer *backingBuffer)
{
	History *history = &backingBuffer->history;
	HistoryIterator it = {};
	do{
		if (!can_redo(history, history->state.location))break;
		//if (end_iteration(&instruction, &it))break;
		redo_(backingBuffer, -1);
		ExpandedInstruction instruction;
		if (!expand_instruction_at(backingBuffer, history->state.location, &instruction))return;
		if (instruction.textBuffer)
			markPreferedCaretXDirty(instruction.textBuffer, instruction.cursor_id_index);
	} while (!waypoint_at(history, history->state.location.prev_instruction_index));
}

void undo(BackingBuffer *backingBuffer)
{
	History *history = &backingBuffer->history;
	HistoryIterator it = {};
	do{
		if (!can_undo(history, history->state.location))break;
		ExpandedInstruction instruction;
		if(!expand_instruction_at(backingBuffer, history->state.location, &instruction))return;
		undo_(backingBuffer);
		if (instruction.textBuffer)
			markPreferedCaretXDirty(instruction.textBuffer, instruction.cursor_id_index);
	}while (!waypoint_at(history, history->state.location.prev_instruction_index));
}

bool has_branch_from_instruction(History *history, int prev_instruction_index)
{
	for (int i = 0; i < history->branches.length; i++)
	{
		if (prev_instruction_index == history->branches.start[i].from.prev_instruction_index)
		{
			return true;
		}
	}
	return false;
}

bool has_branch_to_instruction(History *history, int prev_instruction_index)
{
	for (int i = 0; i < history->branches.length; i++)
	{
		if (prev_instruction_index == history->branches.start[i].to.prev_instruction_index)
		{
			return true;
		}
	}
	return false;
}


bool is_leaf(History *history, HistoryLocation location)
{
	if (has_branch_from_instruction(history, location.prev_instruction_index)) return false;
	if (has_branch_to_instruction(history, location.prev_instruction_index+1)) return true;
	return false;
}

// obtains the location of the next leaf
HistoryLocation next_leaf(History *history, HistoryLocation current)
{
	//NOTE PERFORMANCE, branches in unsorted array lol
	if (current.prev_instruction_index == history->instructions.length - 1)
	{
		current = { -2, -2 };
	}
	HistoryLocation min;
	min.prev_instruction_index = history->instructions.length;
	min.prev_data_index = -1;
	for (;;)
	{
		bool has_min = false;
		for (int i = 0; i < history->branches.length; i++)
		{
			if (history->branches.start[i].to.prev_instruction_index > (current.prev_instruction_index +1)
				&& history->branches.start[i].to.prev_instruction_index < min.prev_instruction_index)
			{
				min = history->branches.start[i].to;
				has_min = true;
			}
		}
		if (!has_min)break;
		HistoryLocation attempt = min;
		move_backward_ignore_branches(history, &min);
		if (is_leaf(history, min))
		{
			return min;
		}
		else
		{
			current = attempt;
		}
	}
	return last_inserted_instruction(history);
}


#include"L:\MemStack.h"
//undos/redos to set the buffer to the next leafs state.
void next_leaf(BackingBuffer *backingBuffer)
{
	//NOTE PERFORMANCE, branches in unsorted array lol
	_log_lazy_moves(&backingBuffer->history);

	History *history = &backingBuffer->history;
	HistoryLocation current = history->state.location;
	HistoryLocation target = next_leaf(history, current);
	HistoryLocation target_cpy = target;
	int *arr = (int *)MemStack_GetTop();
	int len = 0;
	
	while (current != target)
	{
		if (target.prev_instruction_index > current.prev_instruction_index)
		{
			if (has_branch_to_instruction(history, target.prev_instruction_index))
			{
				arr[len++] = target.prev_instruction_index;
			}
			move_backward(history, &target);
		}
		else
		{
			assert(undo_(backingBuffer));
			current = history->state.location;
		}
	}
	target = target_cpy;
	int index = len - 1;
	while (history->state.location != target)
	{
		int branch = 0;
		int num_branches = 1;
		for (int i = 0; i < history->branches.length; i++)
		{
			if (history->branches.start[i].from == history->state.location)
			{
				if (index >= 0 && arr[index] == history->branches.start[i].to.prev_instruction_index)
				{
					branch = num_branches;
					--index;
				}
				++num_branches;
			}
		}
		assert(redo_(backingBuffer,branch));
	}
}












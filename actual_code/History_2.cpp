#include "History_2.h"

bool at_start(History_2 *history, HistoryLocation location);
bool at_end(History_2 *history, HistoryLocation location);
bool move_backward(History_2 *history, HistoryLocation *location);
bool move_forward(History_2 *history, HistoryLocation *location, int branch_index);

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
	case op_remove_cursor:
	case op_add_cursor:
		return sizeof(DataAddRemoveCursor);
	case op_set_cursor:
		return sizeof(HCursorInfo);
	}
	assert(false && "unhandled history_op_code");
	return 0;
}

void branch_current_to_end(History_2 *history)
{
	HistoryLocation end;
	end.prev_data_index = history->data.length;
	end.prev_instruction_index = history->instructions.length;

	Branch branch;
	branch.from = history->state.location;
	branch.to = end;
	if (branch.from != branch.to) //check seems unneccsary no?
	{
		Add(&history->branches, branch);
		history->state.location = branch.to;
	}
}

Instruction *instruction_from_location(History_2 *history, HistoryLocation loc, void **data)
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

void append_instruction_raw(History_2 *history, Instruction instruction, void *data)
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
	Add(&history->instructions, instruction);
	int data_len = data_length_from_opcode(instruction.op_code);
	for (int i = 0; i < data_len; i++)
		Add(&history->data, ((uint8_t *)data)[i]);

}

void append_instruction(History_2 *history, HistoryOpCode op_code, int cursor_id, int view_id, int direction, void *data)
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
		HCursorInfo new_info = {};
		new_info.cursor_id = cursor_id - old_info.cursor_id;
		new_info.view_id =  view_id - old_info.view_id;
		history->state.cursor_reg[history->state.last_cursor_reg] = new_info;

		instruction.cursor_reg_index = history->state.last_cursor_reg;
		instruction.op_code = op_set_cursor;
		append_instruction_raw(history, instruction, &new_info);
	}

cursor_set:
	instruction.op_code = op_code;
	instruction.direction = direction > 0;
	append_instruction_raw(history, instruction, data);
}


void _log_cached_moves(History_2 *history)
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

inline bool checked_add(int32_t a, int32_t b, int32_t *_out_res)
{
	int64_t res64 = (int64_t)a + (int64_t)b;
	int32_t res32 = (int32_t)res64;
	*_out_res = res32;
	return res32 == res64;
}

// --- header declared functions
void log_move(History_2 *history, int direction, int cursor_id, int view_id)
{
	for (int i = 0; i < history->waiting_moves.length; i++)
	{
		if (history->waiting_moves.start[i].cursor_id == cursor_id)
		{
			WaitingMove *move = &history->waiting_moves.start[i];
			int res;
			if (checked_add(move->direction, direction, &res))
			{
				move->direction = res;
				return;
			}
			else
			{
				_log_cached_moves(history);
			}
		}
	}
	WaitingMove move = {};
	move.cursor_id = cursor_id;
	move.direction = direction;
	move.view_id= view_id;
	Add(&history->waiting_moves, move);
}
void log_add(History_2 *history, int8_t direction, char byte, int cursor_id, int view_id)
{ 
	_log_cached_moves(history);
	append_instruction(history, op_add, cursor_id, view_id, direction,&byte);
}
void log_remove(History_2 *history, int direction, char byte, int cursor_id, int view_id)
{
	_log_cached_moves(history);
	append_instruction(history, op_remove,cursor_id,view_id,direction, &byte);
}

void log_add_cursor(History_2 *history, int pos, bool is_selection, int cursor_id, int view_id)
{
	_log_cached_moves(history);
	DataAddRemoveCursor data= {};
	data.pos = pos;
	data.is_selection = is_selection;
	append_instruction(history, op_add_cursor,cursor_id,view_id,0,&data);
}

void log_remove_cursor(History_2 *history, int pos, bool is_selection,int cursor_id, int view_id)
{
	_log_cached_moves(history);
	DataAddRemoveCursor data = {};
	data.pos = pos;
	data.is_selection = is_selection;
	append_instruction(history, op_remove_cursor, cursor_id, view_id,0, &data);
}

int data_length_at_position(History_2*history, int instruction_index)
{
	if (instruction_index == -1)return 1;
	else return data_length_from_opcode(history->instructions.start[instruction_index].op_code);
}

int num_branches(History_2 *history, HistoryLocation location)
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

bool at_end(History_2 *history, HistoryLocation location)
{
	return ((history->instructions.length - 1) <= location.prev_instruction_index);
}

bool at_start(History_2 *history, HistoryLocation location)
{
	return location.prev_instruction_index <= -1;
}

bool can_redo(History_2 *history, HistoryLocation location)
{
	return num_branches(history, location) > 0;
}

bool can_undo(History_2 *history, HistoryLocation location)
{
	return !at_start(history, location);
}




bool move_forward(History_2 *history, HistoryLocation *location, int branch_index)
{ //state = -1 -1 = 'before buffer'
	if (at_end(history,*location))
	{
		return false;
	}

	if (!branch_index)
	{
		HistoryLocation old = *location;
		location->prev_data_index += data_length_at_position(history, location->prev_instruction_index);
		++location->prev_instruction_index;
		
		for (int i = 0; i < history->branches.length; i++)
		{
			if (history->branches.start[i].to == *location)
			{
				*location = old;
				goto next;
			}
		}
		return true;
	}
	else
	{
		--branch_index;
	}

	next:
	for (int i = 0; i < history->branches.length; i++)
	{
		if (history->branches.start[i].from == *location)
		{
			if (!(branch_index--))
			{
				*location = history->branches.start[i].to;
				return true;
			}
		}
	}
	return false;
}


bool move_backward(History_2 *history, HistoryLocation *location)
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
		--location->prev_instruction_index;
		location->prev_data_index -= data_length_at_position(history, location->prev_instruction_index);
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
	History_2 *history = &backingBuffer->history_2;
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

void undo_(BackingBuffer *backingBuffer)
{
	History_2 *history = &backingBuffer->history_2;
	MultiGapBuffer *mgb = backingBuffer->buffer;
	_log_cached_moves(history);
	ExpandedInstruction instr;
	bool success = expand_instruction_at(backingBuffer, history->state.location, &instr);
	if (!success) return;
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
	case op_remove_cursor:
	{	
		DataAddRemoveCursor data = instr.data.add_remove_cursor;
		AddCaretWithId(mgb, instr.cursor_info->view_id, data.pos, instr.cursor_id);
		auto DA = data.is_selection ? &instr.textBuffer->ownedSelection_id : &instr.textBuffer->ownedCarets_id;
		Add(DA, instr.cursor_id);
	}break;
	case op_add_cursor:
	{

		DataAddRemoveCursor data = instr.data.add_remove_cursor;
		removeCaret(mgb, instr.cursor_id);
		auto DA = data.is_selection ? &instr.textBuffer->ownedSelection_id : &instr.textBuffer->ownedCarets_id;
		RemoveOrd(DA, instr.cursor_id_index);

	}break;
	}
	move_backward(history, &history->state.location); //we're not right,are we?
}


void redo_(BackingBuffer *backingBuffer, int branch_index)
{
	History_2 *history = &backingBuffer->history_2;
	MultiGapBuffer *mgb = backingBuffer->buffer;
	HistoryLocation loc = history->state.location;
	if (!move_forward(history, &loc, branch_index))return;

	_log_cached_moves(history);
	ExpandedInstruction instr;
	bool success = expand_instruction_at(backingBuffer, loc, &instr);
	if (!success) return;
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
		Direction dir = instr.direction < 0 ? dir_left : dir_right;
		for (int i = 0; i < instr.data.move; i++)
			move_llnc_(instr.textBuffer, dir, instr.cursor_id, false, movemode_byte);
		break;
	}
	case op_add_cursor:
	{
		DataAddRemoveCursor data = instr.data.add_remove_cursor;
		AddCaretWithId(mgb, instr.cursor_info->view_id, data.pos, instr.cursor_id);
		auto DA = data.is_selection ? &instr.textBuffer->ownedSelection_id : &instr.textBuffer->ownedCarets_id;
		Add(DA, instr.cursor_id);
	}break;
	case op_remove_cursor:
	{
		assert(instr.cursor_id_index != -1);
		DataAddRemoveCursor data = instr.data.add_remove_cursor;
		removeCaret(mgb, instr.cursor_id);
		auto DA = data.is_selection ? &instr.textBuffer->ownedSelection_id : &instr.textBuffer->ownedCarets_id;
		RemoveOrd(DA, instr.cursor_id_index);

	}break;
	}
	history->state.location = loc;
}





struct HistoryIterator
{
	bool add_remove;
	bool move;
	bool cursor_add_remove;
};

bool end_iteration(ExpandedInstruction *instruction, HistoryIterator *it)
{
	if (!instruction->is_selection)
	{
		if (instruction->op_code == op_add || instruction->op_code == op_remove)
		{
			it->add_remove = true;
		}
		if (instruction->op_code == op_move)
		{
			if (it->add_remove)
			{
				return true;
			}
			it->move = true;
		}
		if (instruction->op_code == op_add_cursor || instruction->op_code == op_remove_cursor)
		{
			it->cursor_add_remove = true;
		}
	}
	return false;
}

// note branchindexing doesn't work right.
// we continue on with the same branch index which will break stuff. aint good.
// must come up with a better way to solve this. I sorta need coroutines or something 
void redo(BackingBuffer *backingBuffer, int branch_index)
{
	History_2 *history = &backingBuffer->history_2;
	HistoryIterator it = {};
	while (can_redo(history, history->state.location))
	{
		ExpandedInstruction instruction;
		HistoryLocation loc = history->state.location;
		move_forward(history, &loc, branch_index);
		expand_instruction_at(backingBuffer, loc, &instruction);
		if (end_iteration(&instruction, &it))break;
		redo_(backingBuffer, branch_index);
		if(instruction.textBuffer)
			markPreferedCaretXDirty(instruction.textBuffer, instruction.cursor_id_index);
	}
}

void undo(BackingBuffer *backingBuffer)
{
	History_2 *history = &backingBuffer->history_2;
	HistoryIterator it = {};
	while (can_undo(history, history->state.location))
	{
		ExpandedInstruction instruction;
		expand_instruction_at(backingBuffer, history->state.location, &instruction);
		if (end_iteration(&instruction, &it))break;
		undo_(backingBuffer);
		if (instruction.textBuffer)
			markPreferedCaretXDirty(instruction.textBuffer, instruction.cursor_id_index);
	}
}





#ifndef History_2_H
#define History_2_H
#include "header.h"

Data *global_data;

enum HistoryOpCode : uint8_t
{
	op_add,
	op_remove,
	op_move,
	op_add_cursor,
	op_remove_cursor,
	op_set_cursor,
	op_set_view,
};

struct Instruction
{
	HistoryOpCode op_code : 3;
	uint8_t cursor_reg_index : 4;
	bool direction : 1;
};


struct HistoryLocation
{
	int prev_data_index;
	int prev_instruction_index;
	bool (operator ==) (HistoryLocation &other)
	{
		return other.prev_data_index == prev_data_index &&
			other.prev_instruction_index== prev_instruction_index;
	}
	bool(operator !=) (HistoryLocation &other)
	{
		return !(other == *this);
	}
};

struct HCursorInfo
{
	int cursor_id;
	int view_id;
};

struct HistoryState
{
	HistoryLocation location;
	HCursorInfo cursor_reg[8];
	int last_cursor_reg;
};

struct Branch
{
	HistoryLocation from;
	HistoryLocation to;

	bool(operator ==) (Branch &other)
	{
		return other.from == from&&
			other.to == to;
	}
};

DEFINE_DynamicArray(Instruction);
DEFINE_DynamicArray(Branch);
DEFINE_DynamicArray(uint8_t);

struct WaitingMove
{
	int cursor_id;
	int view_id;
	int direction;
};
DEFINE_DynamicArray(WaitingMove);


struct History_2
{
	DynamicArray_Branch branches;
	DynamicArray_Instruction instructions;
	DynamicArray_uint8_t data;
	HistoryState state;
	int next_branch;
	DynamicArray_WaitingMove waiting_moves;
};

History_2 alloc_History(DH_Allocator allocator)
{
	History_2 ret = {};
	ret.branches = DHDS_constructDA(Branch,50,allocator);
	ret.instructions = DHDS_constructDA(Instruction, 50, allocator);
	ret.data = DHDS_constructDA(uint8_t, 50, allocator);
	ret.waiting_moves = DHDS_constructDA(WaitingMove, 20, allocator);
	ret.state.location = { -1,-1 };
	return ret;
}

enum HistoryDirection
{
	history_dir_left,
	history_dir_right,
};

void log_move(History_2 *history, int8_t direction, int cursor_id, int view_id);
void log_add(History_2 *history, int8_t direction, char byte, int cursor_id, int view_id);
void log_remove(History_2 *history, int direction, char byte, int cursor_id, int view_id);
void log_add_cursor(History_2 *history, int pos, bool is_selection, int cursor_id, int view_id);
void log_remove_cursor(History_2 *history, int pos, bool is_selection, int cursor_id, int view_id);
struct BackingBuffer;
void undo(BackingBuffer *backingBuffer);
void redo(BackingBuffer *backingBuffer, int branch_index);
#endif
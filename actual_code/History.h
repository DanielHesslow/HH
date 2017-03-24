#ifndef History_H
#define History_H
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
	op_internal_move,
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
	// this can be implemented instead by referencing a mgb_cursor_index which gets it down to ~two bytes.
	// however replaying changes and getting locations will be a bitch. so I reverted. (we need to simulate internal moves that happens in mgb etc.)
	// which will basically mean if we change something it will break
	// we don't even have this problem yet.

	// the problem is when we thrash our 'regs' but really, the usual case is not that, so trading a bit of memory for that seams fair enough.
	// I guess we could implement some do_for_all operation but idk. another problem for another day. I guess.


	int cursor_id;
	int view_id;

	Location location;
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
		return other.from == from && other.to == to;
	}
	bool last_used;
};


#define DA_NAME DA_Instruction
#define DA_TYPE Instruction
#include "DynamicArray.h"
#define DA_NAME DA_Branch
#define DA_TYPE Branch
#include "DynamicArray.h"
#define DA_NAME DA_u8
#define DA_TYPE uint8_t
#include "DynamicArray.h"

struct WaitingMove
{
	int cursor_id;
	int view_id;
	int direction;
};
#define DA_NAME DA_WaitingMove
#define DA_TYPE WaitingMove
#include "DynamicArray.h"



struct History
{
	DA_int waypoints;
	DA_Branch branches;
	DA_Instruction instructions;
	DA_u8 data;
	HistoryState state;
	int next_branch;
	DA_WaitingMove waiting_moves;
};

History alloc_History(DH_Allocator *allocator)
{
	History ret = {};
	ret.branches = DA_Branch::make(allocator);
	ret.instructions = DA_Instruction::make(allocator);
	ret.data = DA_u8::make(allocator);
	ret.waiting_moves = DA_WaitingMove::make(allocator);
	ret.waypoints = DA_int::make(allocator);
	ret.state.location = { -1,-1 };
	return ret;
}

enum HistoryDirection
{
	history_dir_left,
	history_dir_right,
};
struct MultiGapBuffer;	
void log_move(History *history, MultiGapBuffer *mgb, int direction, int cursor_id, int view_id);
void log_add(History *history, MultiGapBuffer *mgb, int8_t direction, char byte, int cursor_id, int view_id);
void log_remove(History *history, MultiGapBuffer *mgb, int direction, char byte, int cursor_id, int view_id);
void log_add_cursor(History *history, MultiGapBuffer *mgb, int pos, bool is_selection, int cursor_id, int view_id);
void log_remove_cursor(History *history, MultiGapBuffer *mgb, int pos, bool is_selection, int cursor_id, int view_id);
struct BackingBuffer;
void undo(BackingBuffer *backingBuffer);
void redo(BackingBuffer *backingBuffer);
void log_internal_move(History *history, MultiGapBuffer *mgb, int8_t direction, int cursor_id, int view_id);

#endif
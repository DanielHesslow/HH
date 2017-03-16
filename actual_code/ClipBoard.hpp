

struct ClipboardItem {
	String string;
	bool onSeperateLine;
};

#define DA_TYPE ClipboardItem
#define DA_NAME DA_ClipboardItem
#include "DynamicArray.h"
struct InternalClipboard {
	DA_ClipboardItem clips;
};


internal String string_from_clipboard(InternalClipboard *board) {
	if (board->clips.length == 0) return String::make("");

	char *ack = MemStack_GetTop();
	int len = 0;
	for (int i = 0; i < board->clips.length; i++) {
		String str = board->clips[i].string;
		memcpy(&ack[len], str.start, str.length);
		len += str.length;
		ack[len++] = '\n';
	}
	return String::make(ack, len, general_allocator);
}



// serialization / deserialization things
// it's not really clipBoard specific but thats what I need it for atm.
// consider using memcpy insead of pop a bunch of times 
// if this ever gets somewhat slow.


#define DA_TYPE char
#define DA_NAME DA_char
#include "DynamicArray.h"



void *reserveByte(DA_char *buffer, size_t bytes) {
	for (int i = 0; i < bytes; i++) {
		buffer->add(0);
	}
	return &buffer->start[buffer->length - bytes];
}

void *getBytes(char **buffer, size_t bytes) {
	void *ret = *buffer;
	*buffer += bytes;
	return ret;
}

#define pop(buffer,type) ((type *)getBytes(buffer, sizeof(type)))
#define push(buffer,type) ((type *)reserveByte(buffer, sizeof(type)))

void serialize(DA_char *buffer, ClipboardItem item) {
	assert(item.string.length < INT16_MAX);
	*push(buffer, int16_t) = item.string.length;
	*push(buffer, bool) = item.onSeperateLine;

	char *p = item.string;
	while (*p) {
		*push(buffer, char) = *p++;
	}
}

void serialize(DA_char *buffer, InternalClipboard board) {
	assert(board.clips.length < INT16_MAX);
	*push(buffer, int16_t) = board.clips.length;

	for (int i = 0; i < board.clips.length; i++) {
		serialize(buffer, board.clips.start[i]);
	}
}

ClipboardItem deSerialize_ClipboardItem(char **buffer) {
	ClipboardItem ret = {};

	int16_t length = *pop(buffer, int16_t);;
	ret.onSeperateLine = *pop(buffer, bool);
	ret.string = { (char *)getBytes(buffer,length), length };
	ret.string = ret.string.copy(general_allocator);
	return ret;
}

InternalClipboard deSerialize_InternalClipboard(char **buffer) {
	InternalClipboard ret = {};
	int16_t length = *pop(buffer, int16_t);
	ret.clips = DA_ClipboardItem::make(general_allocator);
	ret.clips.reserve(length);
	for (int i = 0; i < length; i++) {
		ret.clips.add(deSerialize_ClipboardItem(buffer));
	}
	return ret;
}
internal String getLineAsString(TextBuffer *buffer, int caretIdIndex) {
	MultiGapBuffer *mg_buffer = buffer->backingBuffer->buffer;
	MGB_Iterator it = iteratorFromLine(buffer->backingBuffer, getLineFromCaret(buffer->backingBuffer, buffer->ownedCarets_id.start[caretIdIndex]));
	DA_char dynamicString = DA_char::make(general_allocator);
	for (;;) {
		char currentCharacter = *getCharacter(mg_buffer, it);
		dynamicString.add(currentCharacter);
		bool success = getNext(mg_buffer, &it);
		if (!success || isLineBreak(currentCharacter)) {
			break;
		}
	}
	String str = { dynamicString.start,dynamicString.length - 1 }; // to strip the linebreak. 
	str = str.copy(general_allocator); // note this is neccessary to be able to free the string since we loose the capacity from the da
	dynamicString.destroy();
	return str;
}


internal String getSelectionOrRowAsString(TextBuffer *buffer, int caretIdIndex, bool *isRow) {
	MultiGapBuffer *mg_buffer = buffer->backingBuffer->buffer;
	int selPos = getCaretPos(mg_buffer, buffer->ownedSelection_id.start[caretIdIndex]);
	int carPos = getCaretPos(mg_buffer, buffer->ownedCarets_id.start[caretIdIndex]);
	MGB_Iterator it = getIterator(mg_buffer, min(selPos, carPos));
	int length = abs(selPos - carPos);

	if (isRow)*isRow = length == 0;

	if (length == 0) {
		return getLineAsString(buffer, caretIdIndex);
	} else {
		char *ret = (char*)general_allocator->alloc(sizeof(char) * (length + 1)).mem; // @CLEANUP ALLOCATION
		for (int i = 0; i < length; i++) {
			ret[i] = *getCharacter(mg_buffer, it);
			getNext(mg_buffer, &it);
		}
		ret[length] = 0;

		return{ ret,length };
	}
}


internal void win32_StoreInClipboard(InternalClipboard *clipboard);
internal void copy(TextBuffer *textBuffer) {
	InternalClipboard board = {};
	board.clips = DA_ClipboardItem::make(general_allocator);
	board.clips.reserve(textBuffer->ownedCarets_id.length);
	for (int i = 0; i < textBuffer->ownedCarets_id.length; i++) {
		bool isRow;
		ClipboardItem item;
		item.string = getSelectionOrRowAsString(textBuffer, i, &isRow);
		item.onSeperateLine = isRow;
		board.clips.add(item);
	}
	win32_StoreInClipboard(&board);
	for (int i = 0; i < textBuffer->ownedCarets_id.length; i++) {
		board.clips.start[i].string.destroy(general_allocator);
	}
	board.clips.destroy();
}

internal InternalClipboard win32_LoadFromClipboard();

internal void paste(TextBuffer *textBuffer) {
	//insert_event_marker(&textBuffer->backingBuffer->history, history_mark_start); 

	InternalClipboard board = win32_LoadFromClipboard();
	if (board.clips.start) {
		for (int i = board.clips.length - 1; i >= 0; i--) {
			ClipboardItem item = board.clips.start[i];
			if (item.onSeperateLine) {
				gotoStartOfLine(textBuffer, getLineFromCaret(textBuffer->backingBuffer, textBuffer->ownedCarets_id.start[i]), no_select, i);
			}

			for (int j = 0; j < item.string.length; j++) {
				appendCharacter(textBuffer, item.string[j], i % textBuffer->ownedCarets_id.length, true);
			}

			if (i / textBuffer->ownedCarets_id.length > 0 || item.onSeperateLine) {
				appendCharacter(textBuffer, '\n', i % textBuffer->ownedCarets_id.length, true);
			}
			board.clips.start[i].string.destroy(general_allocator);
		}
	}

	setNoSelection(textBuffer, do_log);
	//insert_event_marker(&textBuffer->backingBuffer->history, history_mark_end); 
}

internal void cut(TextBuffer *textBuffer) {
	copy(textBuffer);
	if (!validSelection(textBuffer)) {
		//this fucks us up
		for (int i = 0; i < textBuffer->ownedCarets_id.length; i++) {
			char c;
			while ((c = *get(textBuffer->backingBuffer->buffer, textBuffer->ownedCarets_id[i], dir_left)) && c != '\n') {
				if (!removeCharacter(textBuffer, i, true))break;
			}
			while ((c = *get(textBuffer->backingBuffer->buffer, textBuffer->ownedCarets_id[i], dir_right)) && c != '\n') {
				if (!deleteCharacter(textBuffer, i, true))break;
			}
			deleteCharacter(textBuffer, i, true);
		}
	} else {
		removeSelection(textBuffer);
	}
}


#undef push
#undef pop
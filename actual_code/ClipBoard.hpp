

struct ClipboardItem
{
	char16_t *string;
	bool onSeperateLine;
};

DEFINE_DynamicArray(ClipboardItem);
struct InternalClipboard
{
	DynamicArray_ClipboardItem clips;
};


internal char16_t *getAsUnicode(InternalClipboard *board)
{
	if (board->clips.length == 0) return 0;
	char16_t *ack = strcpy(board->clips.start[0].string);
	for (int i = 1; i < board->clips.length;i++)
	{
		{
			char16_t *old = ack;
			ack = mergeStrings(old, board->clips.start[i].string);
			free_(old);
		}
	
		{
			char16_t *old = ack;
			ack = mergeStrings(ack, (char16_t *)u"\n"); //slow af
			free_(old);
		}
	}

	return ack;
}

internal char *getAsAscii(InternalClipboard *board)
{
	if (board->clips.length == 0) return 0;
	char16_t *asUnicode= getAsUnicode(board);
	
	char *asAscii = createcharFromchar16_t(asUnicode);
	free_(asUnicode);
	return asAscii;
}


// serialization / deserialization things
// it's not really clipBoard specific but thats what I need it for atm.
// consider using memcpy insead of pop a bunch of times 
// if this ever gets somewhat slow.

DEFINE_DynamicArray(char);
DEFINE_DynamicArray(char16_t);

void *reserveByte(DynamicArray_char *buffer, size_t bytes)
{
	void *start = &buffer->start[buffer->length];
	for (int i = 0; i < bytes; i++)
	{
		Add(buffer, 0);
	}
	return start;
}

void *getBytes(char **buffer, size_t bytes)
{
	void *ret = *buffer;
	*buffer += bytes;
	return ret;
}

#define pop(buffer,type) ((type *)getBytes(buffer, sizeof(type)))
#define push(buffer,type) ((type *)reserveByte(buffer, sizeof(type)))

void serialize(DynamicArray_char *buffer, ClipboardItem item)
{
	*push(buffer, int16_t) = DHSTR_strlen(item.string);
	*push(buffer, bool) = item.onSeperateLine;

	char16_t *p = item.string;
	while (*p)
	{
		*push(buffer, char16_t) =*p++;
	}
}

void serialize(DynamicArray_char *buffer, InternalClipboard board)
{
	*push(buffer, int16_t) = board.clips.length;

	for (int i = 0; i < board.clips.length;i++)
	{
		serialize(buffer, board.clips.start[i]);
	}
}

ClipboardItem deSerialize_ClipboardItem(char **buffer)
{
	ClipboardItem ret = {};

	int16_t length = *pop(buffer, int16_t);;
	ret.onSeperateLine = *pop(buffer, bool);
	ret.string= ((char16_t *)alloc_(sizeof(char16_t)*length+1,"desierialized clipboard string"));
	for (int i = 0; i < length; i++)
	{
		ret.string[i] = *pop(buffer, char16_t);
	}
	ret.string[length] = 0;
	return ret;
}

InternalClipboard deSerialize_InternalClipboard(char **buffer)
{
	InternalClipboard ret = {};
	int16_t length = *pop(buffer, int16_t);
	ret.clips = constructDynamicArray_ClipboardItem(length*2, "desierializing some clipboard.");
	ret.clips.length = length;
	for (int i = 0; i < length; i++)
	{
		ret.clips.start[i] = deSerialize_ClipboardItem(buffer);
	}
	return ret;
}
internal char16_t *getLineAsString(TextBuffer *buffer, int caretIdIndex)
{
	MultiGapBuffer *mg_buffer = buffer->backingBuffer->buffer;
	MGB_Iterator it = iteratorFromLine(buffer->backingBuffer, getLineFromCaret(buffer->backingBuffer, buffer->ownedCarets_id.start[caretIdIndex]));
	DynamicArray_char16_t dynamicString = constructDynamicArray_char16_t(50, "getLineAsString");
	for (;;)
	{
		char16_t currentCharacter = *getCharacter(mg_buffer, it);
		Add(&dynamicString, currentCharacter);
		bool success = getNext(mg_buffer, &it);
		if (!success || isLineBreak(currentCharacter))
		{
			dynamicString.start[dynamicString.length - 1] = 0;
			break;
		}
	}
	return dynamicString.start;
}


internal char16_t *getSelectionOrRowAsString(TextBuffer *buffer, int caretIdIndex, bool *isRow)
{
	MultiGapBuffer *mg_buffer = buffer->backingBuffer->buffer;
	int selPos = getCaretPos(mg_buffer, buffer->ownedSelection_id.start[caretIdIndex]);
	int carPos = getCaretPos(mg_buffer, buffer->ownedCarets_id.start[caretIdIndex]);
	MGB_Iterator it = getIterator(mg_buffer, min(selPos, carPos));
	int length = abs(selPos - carPos);

	if (isRow)*isRow = length == 0;

	if (length == 0)
	{
		return getLineAsString(buffer, caretIdIndex);
	}
	else
	{
		char16_t *ret = (char16_t *)alloc_(sizeof(char16_t) * (length + 1), "selection as string, tmp");
		for (int i = 0; i < length; i++)
		{
			ret[i] = *getCharacter(mg_buffer, it);
			getNext(mg_buffer, &it);
		}
		ret[length] = 0;

		return ret;
	}
}


internal void win32_StoreInClipboard(InternalClipboard *clipboard);
internal void copy(TextBuffer *textBuffer)
{
	InternalClipboard board = {};
	board.clips = constructDynamicArray_ClipboardItem(textBuffer->ownedCarets_id.length, "clipbuffer clips");
	board.clips.length = textBuffer->ownedCarets_id.length;
	for (int i = 0; i < textBuffer->ownedCarets_id.length; i++)
	{
		bool isRow;
		board.clips.start[i].string = getSelectionOrRowAsString(textBuffer,i,&isRow);
		board.clips.start[i].onSeperateLine = isRow;
	}
	win32_StoreInClipboard(&board);
	for (int i = 0; i < textBuffer->ownedCarets_id.length; i++)
	{
		free_(board.clips.start[i].string);
	}
	free_(board.clips.start);
}

internal InternalClipboard win32_LoadFromClipboard();

internal void paste(TextBuffer *textBuffer)
{
	//insert_event_marker(&textBuffer->backingBuffer->history, history_mark_start); 

	InternalClipboard board = win32_LoadFromClipboard();
	if (board.clips.start)
	{
		for (int i = board.clips.length-1; i >= 0; i--)
		{
			ClipboardItem item = board.clips.start[i];
			if (item.onSeperateLine)
			{
				gotoStartOfLine(textBuffer, getLineFromCaret(textBuffer->backingBuffer, textBuffer->ownedCarets_id.start[i]), no_select, i);
			}

			int length = DHSTR_strlen(item.string);
			
			for (int j = 0; j < length; j++)
			{
				appendCharacter(textBuffer, item.string[j], i % textBuffer->ownedCarets_id.length,true);
			}
			if (i / textBuffer->ownedCarets_id.length>0||item.onSeperateLine)
			{
				appendCharacter(textBuffer, '\n', i % textBuffer->ownedCarets_id.length,true);
			}
			free_(board.clips.start[i].string);
		}
	}

	setNoSelection(textBuffer, do_log);
	//insert_event_marker(&textBuffer->backingBuffer->history, history_mark_end); 
}

internal void cut(TextBuffer *textBuffer)
{
	//copy is normalizeing as well...
	copy(textBuffer);
	if (!validSelection(textBuffer))
	{
		moveWhile(textBuffer, no_select, do_log, true, dir_left, whileSameLine);
		moveWhile(textBuffer, do_select,  do_log, true, dir_right, whileSameLine);
		move(textBuffer, dir_right, do_log, do_select);
	}
	removeSelection(textBuffer); //remove row.
}
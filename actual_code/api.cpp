// BACKEND API LAYER


#include "api.h"
#include "header.h"
Data *global_data;


ViewIterator view_iterator_from_buffer_handle(BufferHandle buffer_handle)
{
	ViewIterator ret;
	ret.buffer_handle = buffer_handle;
	ret.next = 0;
	return ret;
}

bool next_view(ViewIterator *iterator, ViewHandle *view_handle)
{
	// slow, what ever.
	int ack = 0;
	for (int i = 0; i < global_data->textBuffers.length; i++)
	{
		if (&global_data->textBuffers.start[i].backingBuffer == iterator->buffer_handle)
		{
			if (ack++ == iterator->next)
			{
				++iterator->next;
				*view_handle = &global_data->textBuffers.start[i];
				return true;
			}
		}
	}
	return false;
}

bool save_view(ViewHandle handle)
{
	saveFile((TextBuffer *)handle);
	return true; // heh
}
int lines_from_buffer_handle(BufferHandle handle)
{
	return getLines((BackingBuffer *)handle);
}

bool next_change(BufferHandle buffer_handle, void *function, BufferChange *bufferChange)
{
	return next_history_change((BackingBuffer *)buffer_handle, function, bufferChange);
}

void mark_all_changes_read(BufferHandle buffer_handle, void *function)
{
	fast_forward_history_changes((BackingBuffer *)buffer_handle, function);
}

bool hash_moved_since_last(BufferHandle buffer_handle, void *function)
{
	return has_move_changed((BackingBuffer *)buffer_handle, function);
}



void clipboard_copy(ViewHandle view_handle)
{
	copy((TextBuffer *)view_handle);
}
void clipboard_paste(ViewHandle view_handle)
{
	paste((TextBuffer *)view_handle);
}
void clipboard_cut(ViewHandle view_handle)
{
	cut((TextBuffer *)view_handle);
}

ViewHandle commandline_open()
{
	global_data->isCommandlineActive = true;
}

void commandline_close()
{
	global_data->isCommandlineActive = false;
}

void textBuffer_clear(TextBuffer *buffer)
{
	MultiGapBuffer *mgb = buffer->backingBuffer->buffer;
	for (int i = 0; i < mgb->blocks.length; i++)
	{
		mgb->blocks.start[i].length = 0;
	}
}

void commandline_clear()
{
	textBuffer_clear(global_data->commandLine);
}

void commandline_feed(char *string, int string_length)
{
	for (int i = 0; i < string_length; i++)
	{
		appendCharacter(global_data->commandLine, string[i], 0, true);
	}
}

void commandline_executeCommand()
{

}


API getAPI()
{
	API api = {};
	api.buffer.view_iterator = view_iterator_from_buffer_handle;
	api.buffer.next          = next_view;
	api.buffer.save          = save_view;
	api.buffer.numLines      = lines_from_buffer_handle;

	api.changes.has_moved     = hash_moved_since_last;
	api.changes.mark_all_read = mark_all_changes_read;
	api.changes.next_change   = next_change;
	
	api.clipboard.copy  = clipboard_copy;
	api.clipboard.cut   = clipboard_cut;
	api.clipboard.paste = clipboard_paste;

	api.commandLine.clear          = commandline_clear;
	api.commandLine.close          = commandline_close;
	api.commandLine.executeCommand = commandline_execute_command;
	api.commandLine.feed           = commandline_feed;
	api.commandLine.open           = commandline_open;

	return api;
}






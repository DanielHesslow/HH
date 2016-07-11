#include "header.h"
#include "Parser.hpp"
#include "colorScheme.h"

 treatEQ(Action a , Action b)
{ 
	if (a == action_remove && b == action_delete || b == action_remove && a == action_delete)
	{
		return true;
	}
	return a == b;
}


int intFromDir(Direction dir)
{
	return dir == dir_left ? -1 : 1;
}


void undo(TextBuffer *textBuffer)
{
	//HistoryEntry prev = *(textBuffer->history.entries + textBuffer->history.length-1);
	HistoryEntry prev = textBuffer->history.entries.start[textBuffer->history.entries.length-1];
   
	if(prev.action == action_move)
	{
		if (prev.action == action_move)
		{
			for (int i = 0; i < -prev.direction; i++)
			{
				moveRight(textBuffer, false);

			}
			for (int i = 0; i < prev.direction; i++)
			{
				moveLeft(textBuffer, false);
			}
		}
	}
	else if(prev.action == action_add)
	{
		removeCharacter(textBuffer,false);
	}
	else if(prev.action == action_remove)
	{
		appendCharacter(textBuffer,prev.character,false);
	}
	else if (prev.action == action_delete)
	{
		unDeleteCharacter(textBuffer, prev.character);
	}
	else
	{
		assert(false);
	}
	textBuffer->history.availableRedos++;
	textBuffer->history.entries.length--;
}


void redo(TextBuffer *textBuffer)
{
	if(textBuffer->history.availableRedos>0)
	{
		HistoryEntry next = textBuffer->history.entries.start[textBuffer->history.entries.length ];

		if(next.action == action_move)
		{
			for (int i = 0; i < -next.direction; i++)
			{
				moveLeft(textBuffer, false);

			}
			for (int i = 0; i < next.direction; i++)
			{
				moveRight(textBuffer, false);
			}
		}
		else if(next.action == action_remove)
		{
			removeCharacter(textBuffer, false);
		}
		else if(next.action == action_add)
		{
			appendCharacter(textBuffer,next.character, false);
		}
		else if (next.action == action_delete)
		{
			deleteCharacter(textBuffer, false);
		}
		else
		{
			assert(false);
		}
		
		textBuffer->history.availableRedos--;
		textBuffer->history.entries.length++;
	}
}

void redoMany(TextBuffer *textBuffer)
{
	HistoryEntry next = textBuffer->history.entries.start[textBuffer->history.entries.length];
	Action lastAction = next.action;

	while(textBuffer->history.availableRedos>0)
	{
		HistoryEntry workingEntry = textBuffer->history.entries.start[textBuffer->history.entries.length];

		redo(textBuffer);
		if(!treatEQ(workingEntry.action, lastAction)){
			break;
		}
		if ((lastAction == action_add || lastAction == action_remove || lastAction == action_delete) && isLineBreak(workingEntry.character))
		{
			break;
		}
	}
}

void undoMany(TextBuffer *textBuffer)
{

	HistoryEntry prev = textBuffer->history.entries.start[textBuffer->history.entries.length-1];
	Action lastAction = prev.action;

	while(textBuffer->history.entries.length>0)
	{
		undo(textBuffer);
		HistoryEntry workingEntry = textBuffer->history.entries.start[textBuffer->history.entries.length-1];
		if (!treatEQ(workingEntry.action, lastAction)) {
			break;
		}
		if ((lastAction == action_add || lastAction == action_remove||lastAction==action_delete) && isLineBreak(workingEntry.character))
		{
			break;
		}
	}
}

asså }r inte det här super najs eller någor


void logRemoved(History *history, char16_t character)
{
	history->availableRedos = 0;
	Add(&history->entries, { action_remove,character });
}

void logDeleted(History *history, char16_t character)
{
	history->availableRedos = 0;
	Add(&history->entries, { action_delete, character });
}


void logAdded(History *history, char16_t character)
{
	history->availableRedos = 0;
	Add(&history->entries, { action_add, character });
}

void logMoved(History *history, Direction direction)
{
	history->availableRedos = 0;
	HistoryEntry *lastEntry = &history->entries.start[history->entries.length - 1];
	if (lastEntry->action == action_move)
	{
		lastEntry->direction += intFromDir(direction);
		if (lastEntry->direction == 0)
		{
			//--history->length; // a zero length move is not of much value
		}
	}
	else
	{
		HistoryEntry entry = {};
		entry.action = action_move;
		entry.direction = intFromDir(direction);
		Add(&history->entries, entry);			// is this right? possibly intFromDir is truncated to a uint16...

	}

}










//	----	GAPBUFFER





inline internal float getCurrentScale(TextBuffer *textBuffer)
{
	return textBuffer->lineInfo.start[textBuffer->caretLine].scale;
}





// ---- Gap buffer manipulation

internal bool isCharacter()
{
	return false;
}
internal int leftSize(GapBuffer *buffer)
{
	return buffer->left-buffer->leftEdge;
}
internal int rightSize(GapBuffer *buffer)
{
	return buffer->rightEdge-buffer->right;
}

internal bool isLineBreak(char16_t i)
{
	return i == 0x000A || i == 0x000B || i == 0x000C || i == 0x000D|| i == 0x0085 || i == 0x2028 || i == 0x2029;
}
internal bool isSpace(int i)
{
	return i == ' ';
}
internal bool isWordDelimiter(int i)
{
	return !(isalpha(i) || isdigit(i)) || isLineBreak(i);
	//return i == ' '|| i=='\t' || isLineBreak(i);
}
internal bool isFuncDelimiter(int i)
{
	if (i == '_')return false;
	return isWordDelimiter(i);
}

internal bool getNextCharacter(GapBuffer *buffer, char16_t **character)
{

	if (*character== buffer->left) {
		*character = buffer->right;
	}
	else
	{
		++*character;
	}

	if (*character >= buffer->rightEdge)
	{
		return false;
	}
	return true;
}

internal bool validSelection(Selection s)
{
	return s.length != 0 && s.start;
}

internal bool getPreviousCharacter(GapBuffer *buffer, char16_t **character)
{
	if(*character == buffer->right)
	{
		*character = buffer->left;
	}
	else
	{
		--*character;
	}
	if(*character<=buffer->leftEdge)
	{
		//this case isnt working correctly... why? maybe it is though... Havn't had problems in a while heh... Did I fix you?
		return false;
	}
	return true;
}

internal int length(GapBuffer buffer)
{
	return buffer.leftEdge - buffer.left + buffer.right - buffer.rightEdge;
}

internal void appendCharacter(TextBuffer *textBuffer, char16_t character, bool log)
{
	if (textBuffer->buffer.left + 1 == textBuffer->buffer.right)
	{
		return; //buffer is full make it bigger.. you know at some point.. When we wan't to.
	}
	if (log)
		logAdded(&textBuffer->history, character);
	if (isLineBreak(character))
	{
		textBuffer->caretLine++;
	}
	*(++textBuffer->buffer.left) = character;
}

internal void removeCharacter(TextBuffer *textBuffer, bool log)
{
	
	if (textBuffer->buffer.left != textBuffer->buffer.leftEdge)
	{
		if (log)
			logRemoved(&textBuffer->history, *textBuffer->buffer.left);

		if (isLineBreak(*textBuffer->buffer.left))textBuffer->caretLine--;
		--textBuffer->buffer.left;
	}
}
internal void unDeleteCharacter(TextBuffer *textBuffer, char16_t character)
{
	//NOTE: this is not ment for user interaciton strictly for undoing 
	//previous deletes. As such we do not need to check that the following state is OK.
	*(--textBuffer->buffer.right) = character;
}


internal void deleteCharacter(TextBuffer *textBuffer, bool log)
{
	if (!(textBuffer->buffer.right == textBuffer->buffer.rightEdge))
	{
		if (log)
			logDeleted(&textBuffer->history, *textBuffer->buffer.right);

		++textBuffer->buffer.right;
	}
}


global_variable int lastMoveCaretX;

internal bool updateLastMove=true;
internal void moveLeft(TextBuffer *textBuffer,bool log,bool selection)
{
	updateLastMove = true;
	if (textBuffer->buffer.left != textBuffer->buffer.leftEdge)
	{
		if (log)
			logMoved(&textBuffer->history, dir_left);

		if (isLineBreak(*(textBuffer->buffer.left)))
		{
			textBuffer->caretLine--;
		}
		*(--textBuffer->buffer.right) = *(textBuffer->buffer.left);
		textBuffer->buffer.left--;

		if (selection)
		{
			if (!textBuffer->selection.start || textBuffer->selection.length == 0)
			{
				textBuffer->selection.start = textBuffer->buffer.right+1;
			}
			moveSelection(textBuffer, dir_left);
		}
		else
		{
			textBuffer->selection.length = 0;
		}
	}
}



internal void moveRight(TextBuffer *textBuffer, bool log, bool selection)
{

	updateLastMove = true;
	if (textBuffer->buffer.right != textBuffer->buffer.rightEdge)
	{
		if (log)
			logMoved(&textBuffer->history, dir_right);

		if (isLineBreak(*(textBuffer->buffer.right)))
		{
			textBuffer->caretLine++;
		}
		*(++textBuffer->buffer.left) = *(textBuffer->buffer.right);
		textBuffer->buffer.right++;

		if (selection)
		{

			if (!validSelection(textBuffer->selection))
			{
				textBuffer->selection.start = textBuffer->buffer.left;
			}
			moveSelection(textBuffer, dir_right);
		}
		else
		{
			textBuffer->selection.length = 0;
		}

	}
}

internal void moveRightWord(TextBuffer *textBuffer,bool selection)
{
	moveRight(textBuffer,true,selection);
	do{
		moveRight(textBuffer,true,selection);
	} while (!isWordDelimiter(*textBuffer->buffer.left)&& textBuffer->buffer.right != textBuffer->buffer.rightEdge);
}

internal void moveLeftWord(TextBuffer *textBuffer,bool selection)
{
	moveLeft(textBuffer,true,selection);
	do{
		moveLeft(textBuffer,true,selection);
	} while (!isWordDelimiter(*textBuffer->buffer.left) && textBuffer->buffer.left != textBuffer->buffer.leftEdge);
}

internal void removeWord(TextBuffer *textBuffer)
{
	removeCharacter(textBuffer);
	do {
		removeCharacter(textBuffer);
	} while (!isWordDelimiter(*(textBuffer->buffer.left)) && textBuffer->buffer.left != textBuffer->buffer.leftEdge);
}

internal int getLine(GapBuffer *buffer, int *lineNumbers,int lines)
{
	int caretPos = leftSize(buffer);
	int counter=0;
	while(*lineNumbers<caretPos)
	{
		++counter;
		++lineNumbers;
		if(counter ==lines)return lines;
	}
	return counter-1;
}

internal void moveCursorToX(TextBuffer *textBuffer, int targetX, bool selection)
{

	float scale = textBuffer->lineInfo.start[textBuffer->caretLine].scale;

	int x = 0;
	int dx =0;
	char16_t *character = textBuffer->buffer.leftEdge;
	
	bool ended;
	int previousX;
	while (x<targetX) //dx is now previous right? no good
	{
		dx = getCharacterWidth(*textBuffer->buffer.left,*textBuffer->buffer.right, textBuffer->fontInfo, scale);
		
		if (abs(x - targetX) < abs((x+dx) - targetX))
		{
			break;
		}
		x += dx;
		moveRight(textBuffer,true,selection);
		
		if (isLineBreak(*textBuffer->buffer.left))
		{
			moveLeft(textBuffer, true, selection);
			break;
		}
		previousX = x;
	}
}

internal void gotoStartOfLine(TextBuffer *textBuffer,int line,bool selection)
{
	int currentChar = leftSize(&textBuffer->buffer);
	int lineChar = textBuffer->lineInfo.start[line].start;
	for (int i = 0; i<currentChar - lineChar; i++)
	{
		moveLeft(textBuffer,true,selection);
	}
	for (int i = 0; i<lineChar - currentChar; i++)
	{
		moveRight(textBuffer,true,selection);
	}
}
internal void moveUp(TextBuffer *textBuffer, bool selection) //handle selecitons
{
	if (textBuffer->caretLine > 0)
	{
		gotoStartOfLine(textBuffer, textBuffer->caretLine - 1,selection);
		moveCursorToX(textBuffer, lastMoveCaretX,selection);
		updateLastMove = false;
	}
}	
internal void moveDown(TextBuffer *textBuffer, bool selection)
{
	if (textBuffer->caretLine < textBuffer->lineInfo.length-1)
	{
		gotoStartOfLine(textBuffer, textBuffer->caretLine + 1, selection);
		moveCursorToX(textBuffer, lastMoveCaretX, selection);
		updateLastMove = false;
	}
	
}	

internal void insertTab(TextBuffer *textBuffer)
{
	// tabs are always inserted in the begining of a line
	// the alternative doesn't make sence to me, 
	// at least not when programming..
	
	char16_t currentChar = leftSize(&textBuffer->buffer);
	int lineChar = textBuffer->lineInfo.start[textBuffer->caretLine].start;
	int diff = currentChar - lineChar;
	for (int i = 0; i<diff; i++)
	{
		moveLeft(textBuffer);
	}
	appendCharacter(textBuffer, '\t');
	for (int i = 0; i<diff; i++)
	{
		moveRight(textBuffer);
	}
}
internal void removeTab(TextBuffer *textBuffer)
{
	char16_t currentChar = leftSize(&textBuffer->buffer);
	int lineChar = textBuffer->lineInfo.start[textBuffer->caretLine].start;
	int diff = currentChar - lineChar;
	for (int i = 0; i<diff; i++)
	{
		moveLeft(textBuffer);
	}
	if (*textBuffer->buffer.right == '\t')
	{
		deleteCharacter(textBuffer);
	}
	for (int i = 0; i<diff-1; i++)
	{
		moveRight(textBuffer);
	}
} 

internal Selection getRowSelection(TextBuffer *textBuffer)
{
	// for now we don't have selections...
	// so we need should select the row.	
	gotoStartOfLine(textBuffer,textBuffer->caretLine,false);
	int length = textBuffer->lineInfo.start[textBuffer->caretLine+1].start- textBuffer->lineInfo.start[textBuffer->caretLine].start;
	return { textBuffer->buffer.right, length};
}

internal void gotoCharacter(TextBuffer *textBuffer,char16_t *character)
{	
	int diffLeft = textBuffer->buffer.left - character;
	int diffRight = character - textBuffer->buffer.right;

	if (character== textBuffer->buffer.left)
	{
		return;
	}
	else if (diffLeft > 0)
	{
		for (int i = 0; i < diffLeft; i++)
		{
			moveLeft(textBuffer);
		}
	}
	else if (diffRight >= 0)
	{
		for (int i = 0; i < diffRight+1; i++)
		{
			moveRight(textBuffer);
		}
	}
	else
	{
		assert(false);	//there is no char between the left and the right. that is empty space...
	}
}




//win32 forwardDelc
internal void copy(TextBuffer *textBuffer);
internal void paste(TextBuffer *buffer);

// ---- stb TrueType stuff



internal void setUpTT(stbtt_fontinfo *fontInfo)
{
	long size;
	unsigned char* fontBuffer;

	FILE* fontFile = fopen("font.ttf", "rb");
	if (!fontFile)return;
	fseek(fontFile, 0, SEEK_END);
	size = ftell(fontFile);
	fseek(fontFile, 0, SEEK_SET);

	fontBuffer = (unsigned char*)malloc(size);
	fread(fontBuffer, size, 1, fontFile);
	fclose(fontFile);

	stbtt_InitFont(fontInfo, fontBuffer, 0);
}

internal int getCharacterWidth(char16_t currentChar, char16_t nextChar, stbtt_fontinfo *fontInfo, float scale)
{
	if (currentChar == VK_TAB)
	{
		return getCharacterWidth(' ', ' ',fontInfo,scale) * 4;
	}
	int advanceWidth, leftSideBearing;
	stbtt_GetCodepointHMetrics(fontInfo, currentChar, &advanceWidth, &leftSideBearing);
	int kerning = stbtt_GetCodepointKernAdvance(fontInfo, currentChar, nextChar);
	return advanceWidth*scale + kerning*scale;
}

// ---- IO

global_variable char* fileName = "notes.txt";
internal void openFile(TextBuffer *textBuffer)
{
	//silly stupid slow method..
	//fix this at some point?
	int c;
	FILE *file;
	//file = fopen("texteditor.hpp", "r");
	file = fopen("fileName", "r");
	if (file)
	{
		while ((c = getc(file)) != EOF)
		{
			appendCharacter(textBuffer, c, false);
		}
		fclose(file);
	}
	for (int i = 0; i < leftSize(&textBuffer->buffer) + rightSize(&textBuffer->buffer); i++)
	{
		moveLeft(textBuffer,false);
	}
}


internal void saveFile(GapBuffer *buffer)
{
	FILE *file;
	file = fopen("fileName", "w");
	//file = fopen("thingsToDo.txt", "w");
	if (file)
	{
		char16_t *c;
		c = buffer->leftEdge;
		while (getNextCharacter(buffer, &c))
		{
			if (fputc(*c, file) == EOF)
			{
				break;
			}
		}
	}
}

// ----	colors


internal Color overlayColor(Color a, Color b)
{
	Color ret;
	float aa = (float)a.a/(float)255;
	float ba = (float)b.a/(float)255;
	
	ret.r = (a.r*aa + b.r*ba);
	ret.g = (a.g*aa + b.g*ba);
	ret.b = (a.b*aa + b.b*ba);
	ret.a = (a.a + b.a);
	return ret;
}

internal int grayScaleToColor(char org, int foregroundColor,int backgroundColor)
{
	Color a,b;
	a.whole= foregroundColor;
	b.whole= backgroundColor;
	a.a = org;
	b.a = 255-org;
	return overlayColor(a,b).whole;
}



// ---- Bitmaps

internal void fillBitmap(TextBuffer *textBuffer, int lineOffset,int y)
{
	if (y > 0)
	{
		float scale = textBuffer->lineInfo.start[lineOffset].scale;
		renderRect(&textBuffer->bitmap, 0, 0, textBuffer->bitmap.width, textBuffer->lineHeight*scale, textBuffer->lineInfo.start[lineOffset].color);
	}
	while (y < textBuffer->bitmap.height && lineOffset<textBuffer->lineInfo.length)
	{
		float scale = textBuffer->lineInfo.start[lineOffset].scale;
		renderRect(&textBuffer->bitmap, 0, y, textBuffer->bitmap.width, textBuffer->lineHeight*scale, textBuffer->lineInfo.start[lineOffset].color);
		y += textBuffer->lineHeight * textBuffer->lineInfo.start[lineOffset++].scale;
	}
	if (y < textBuffer->bitmap.height)
	{
		renderRect(&textBuffer->bitmap, 0, y, textBuffer->bitmap.width, textBuffer->bitmap.height - y, textBuffer->lineInfo.start[0].color);
	}
}

internal void clearBitmap(Bitmap bitmap, int color)
{
	int size = bitmap.width*bitmap.height*4;
	memset(bitmap.memory, color, size);

	//memset is 5x way faster than the below
	//but only works on bytes... //so our background can only be gray...
	
	//allocate a clear slate buffer 
	//and use memcpy?? 
	//or maybe use fill 4 bytes and then in memcopy to twice the size and continue.. maybe?

	/*
	uint8 *row = (uint8 *)bitmap.memory;
	for (int y = 0; y < bitmap.height; ++y)
	{
		uint32 *pixel = (uint32 *)row;
		for (int x = 0; x <bitmap.width; ++x)
		{
			*pixel++ = color;
		}
		row += bitmap.pitch;
	}

	*/
}



// ---- font renderning




//clean me up please!
internal void blitChar(Bitmap from, Bitmap to, int xOffset, int yOffset,int color)
{
	uint8 *fromRow = (uint8 *)from.memory;
	uint8 *toRow   = (uint8 *)to.memory + xOffset*4+ yOffset*to.pitch;

	for(int y = 0; y<from.height; y++)
	{
		if((y+yOffset) >= to.height||(y+yOffset)< 0)
		{
			fromRow += from.pitch;
			toRow += to.pitch;
			
		}
		else
		{
			uint8 *fromPixel  = fromRow;
			int  *toPixel  = (int *)toRow;

			for(int x = 0; x<from.width; x++)
			{
				
				if (!((x + xOffset) >= to.width || (x + xOffset) < 0))
				{
					*toPixel = grayScaleToColor(*fromPixel, color, *toPixel);
				}
				++fromPixel;
				++toPixel;
			}
			fromRow += from.pitch;
			toRow += to.pitch;
		}
	}	
}

DynamicArray_CharBitmap bufferedCharacters;

internal Bitmap getCharBitmap(char16_t character, float scale,stbtt_fontinfo *fontInfo, int *xOff, int *yOff)
{
	for (int i = 0; i < bufferedCharacters.length; i++)
	{
		CharBitmap current = bufferedCharacters.start[i];
		if (current.character == character && current.scale == scale)
		{
			*xOff = current.xOff;
			*yOff = current.yOff;

			return current.bitmap;
		}
	}
	
	int width, height;
	void *charBitmapMem = stbtt_GetCodepointBitmap(fontInfo, scale, scale, (int)character, &width, &height, xOff, yOff);
	Bitmap charBitmap = { charBitmapMem, width, height, width, 1};
	CharBitmap b = {};
	b.bitmap = charBitmap;
	b.character = character;
	b.scale = scale;
	b.yOff = *yOff;
	b.xOff = *xOff;

	Add(&bufferedCharacters, b);
	return charBitmap;
}

internal void renderCharacter(Bitmap bitmap, char16_t character, int x, int y, float scale,int color, stbtt_fontinfo *fontInfo)
{
	if (character == VK_TAB)return;
	//int width, height, xOff, yOff; 
	//void *charBitmapMem = stbtt_GetCodepointBitmap(fontInfo, scale, scale, (int)character, &width, &height, &xOff, &yOff);
	//Bitmap charBitmap = { charBitmapMem,width,height,width,1 };
	int xOff, yOff;
	Bitmap charBitmap = getCharBitmap(character, scale, fontInfo,&xOff,&yOff);
	Bitmap output = { bitmap.memory,bitmap.width,bitmap.height,bitmap.pitch,1 };
	blitChar(charBitmap, output, x+xOff, y+yOff, color);
	
	//free(charBitmapMem);
}


internal void renderCaret(Bitmap bitmap, int x, int y,float scale,stbtt_fontinfo *fontInfo)
{
	int width = getCharacterWidth('|', 'A', fontInfo, scale);
	renderCharacter(bitmap, '|', x - width / 2, y,scale, caretColor,fontInfo); //use ascent / descent probably.
}


//		----	LOGIC

internal int getNextColor(int *index)
{
	++*index;
	*index %= 3;
	return backgroundColors[*index++];
}
internal void updateLineInfo(TextBuffer *textBuffer)
{

	int index = 0;
	textBuffer->lines = 0;
	textBuffer->lineInfo.length = 0;
	

	char16_t* character = textBuffer->buffer.leftEdge;
	int previousCounter = 0;
	int counter = 0;
	bool title=false;

	int dashCounter = 0;
	
	float scale = 0.006;
	int color = getNextColor(&index);

	do {
		
		if (isLineBreak(*character))
		{

			
			if (title)
			{
				scale *=2; 
				color = getNextColor(&index);
				title = false;
			}

			LineInfo elem = { previousCounter, scale, color };
			Add(&textBuffer->lineInfo, elem);
			dashCounter = 0;
			
			previousCounter = counter;
		}
		if(*character == '-')dashCounter++;
		else dashCounter = 0;
		if (dashCounter >= 3) title = true;

		++counter;
	} while (getNextCharacter(&textBuffer->buffer, &character));

	LineInfo elem = { previousCounter, scale, color };
	Add(&textBuffer->lineInfo, elem);

	
}

internal bool nextWordFit(GapBuffer *buffer, char16_t *character, int x, int bitmapWidth,stbtt_fontinfo *fontInfo,float scale)
{
	//fixme please I'm slow af
	bool ended = false;
	bool ended2 = false;
	while (!ended2)
	{
		ended2=ended;
		int currentChar = *character;
		ended = !getNextCharacter(buffer, &character);
		char16_t nextChar = *character;
		if (ended)
		{
			nextChar = ' ';
		}

		x += getCharacterWidth(currentChar, nextChar,fontInfo,scale);
		
		if (x >= bitmapWidth)
		{
			return false;
		}
		if (isSpace(currentChar))
		{
			return true;
		}
	}
	return true;
}

internal void updateHMetricts(TextBuffer *textBuffer)
{
	stbtt_GetFontVMetrics(textBuffer->fontInfo, &textBuffer->ascent, &textBuffer->descent, &textBuffer->lineGap);
	textBuffer->lineHeight = (textBuffer->ascent - textBuffer->descent + textBuffer->lineGap);
}

/*
internal void renderText(Bitmap *bitmap, GapBuffer *buffer, int x, int y,float scale,int color)
{
	char16_t *character = buffer->leftEdge;
	while (getNextCharacter(buffer, &character))
	{
		int width = getCharacterWidth(*character, ' '); //ignore kerning for now...
		renderCharacter(*bitmap, *character, x, y, scale, color);
		x += width;
	}
}
*/

internal void renderRect(Bitmap *bitmap, int xOffset, int yOffset, int width, int height, int color)
{
	//downscale failure :(
	char *row = (char *)bitmap->memory+yOffset*bitmap->pitch+xOffset*4;
	for (int y = 0; y<height; y++)
	{
		if ((y + yOffset) >= bitmap->height || (y + yOffset)< 0)
		{
			row += bitmap->pitch;
		}
		else
		{
			int  *pixel = (int *)row;
			
			for (int x = 0; x<width; x++)
			{
				if (!((x + xOffset) >= bitmap->width || (x + xOffset) < 0))
				{
					*pixel = color;
				}
				++pixel;
			}
			row += bitmap->pitch;
		}
	}
}

internal char16_t *getCharacterAtLine(TextBuffer *textBuffer, int line)
{
	int start = textBuffer->lineInfo.start[line].start;
	char16_t * character = textBuffer->buffer.leftEdge;
	for (int i = 0; i < start; i++)
	{
		getNextCharacter(&textBuffer->buffer, &character);
	}
	return character;
}



internal void renderText(TextBuffer *textBuffer,int startLine,int x,int y)
{

	Selection selection = normalizedSelection(textBuffer,textBuffer->selection);
	char16_t *character = getCharacterAtLine(textBuffer, startLine);
	int ccIndex = 0;

	while (textBuffer->colorChanges.start[ccIndex].p <= character)
	{
		++ccIndex;
	}
	ccIndex--;

	int orgX = x;
	bool ended = false;
	//if we are emty we still need to render the caret.. therfore we set up some default values here!
	textBuffer->caretX = x;
	int caretY = y;
	//we might end before we even start...
	ended = !getNextCharacter(&textBuffer->buffer, &character); 
	int currentColor = textBuffer->colorChanges.start[ccIndex++].color;
	
	int remainingSelection = 0;
	
	int currentLine=startLine;

	float currentScale= textBuffer->lineInfo.start[currentLine].scale;
	float caretScale=currentScale;
	while (!ended)
	{

		if (character == selection.start)
		{
			remainingSelection = selection.length;
		}
		if (textBuffer->colorChanges.start[ccIndex].p == character)
		{
			currentColor = textBuffer->colorChanges.start[ccIndex++].color;
		}
		char16_t currentChar = *character;
		ended = !getNextCharacter(&textBuffer->buffer, &character);
		if (isLineBreak(currentChar))
		{
			--remainingSelection;
			currentLine++;
			currentScale = textBuffer->lineInfo.start[currentLine].scale;
			y += textBuffer->lineHeight*currentScale;
			x = orgX;
			if (y > textBuffer->bitmap.height) {
				break;
			}
		}
		else
		{
			char16_t nextChar = ended ? *character : 'A'; // some default to avoid derefing invalid memory
			/*	//this does not work as it should.. it should either be cleaned up or removed.. Not sure what is better
				//probably prefere having wrapping but I should think about it.
			if (!nextWordFit(buffer, character, x, bitmap.width))
			{
				y += lineHeight;
				x = orgX;
			}
			*/
			int width = getCharacterWidth(currentChar, nextChar,textBuffer->fontInfo, currentScale);
			if (remainingSelection > 0)
			{
				renderRect(&textBuffer->bitmap, x, y - textBuffer->ascent*currentScale, width, textBuffer->lineHeight*currentScale, highlightColor);
				--remainingSelection;
			}
			renderCharacter(textBuffer->bitmap, currentChar, x, y, currentScale,currentColor,textBuffer->fontInfo);
			x += width;
		}
		if (character == textBuffer->buffer.right)
		{
			textBuffer->caretX = x;
			caretY = y;
			caretScale = currentScale;
		}
	}

	renderCaret(textBuffer->bitmap, textBuffer->caretX, caretY, caretScale,textBuffer->fontInfo);
}


internal int clamp(int i,int low, int high)
{
	if (i < low)
	{
		return low;
	}
	if (i > high)
	{
		return high;
	}
	return i;
}


/*
internal void renderCommandLine(Bitmap *bitmap,GapBuffer *buffer,float scale)
{
	int width = 200;
	int height = 20;
	int x = (bitmap->width - width) / 2;
	int y = 20;
	
	renderRect(bitmap,x,y,width,height,0xffffffff);
	renderText(bitmap, buffer, x+10, y + getLineHeight(), scale, 0xff000000);
}
*/
internal int calculateVisibleLines(TextBuffer *textBuffer, int line)
{
	int y = 0;
	int counter = 0;
	while (y < textBuffer->bitmap.height)
	{
		y += textBuffer->lineHeight*textBuffer->lineInfo.start[line+counter].scale;
		++counter;
	}
	return counter - 1;
}
internal void updateText(TextBuffer *textBuffer,int x, int y)
{
	//note: the additional one is for lazy cieling.. 


	//1 + ((textBuffer->bitmap.height - textBuffer->lineHeight) / textBuffer->lineHeight);
	local_persist int lineOffsetForRender = 0;
	int visibleLines = calculateVisibleLines(textBuffer, lineOffsetForRender);

	lineOffsetForRender = clamp(lineOffsetForRender, textBuffer->caretLine - visibleLines + 1, textBuffer->caretLine); // this plus one on the other hand is wierd
	//clearBitmap(textBuffer->bitmap, background);
	float firstScale = textBuffer->lineInfo.start[lineOffsetForRender].scale;
	fillBitmap(textBuffer, lineOffsetForRender, y-textBuffer->descent*firstScale);
	renderText(textBuffer, lineOffsetForRender,x,y+textBuffer->lineHeight*firstScale);
}

internal void clearBuffer(GapBuffer *buffer)
{
	buffer->left = buffer->leftEdge;
	buffer->right = buffer->rightEdge;
}

internal Selection SelectionOrRow(TextBuffer *textBuffer)
{
	if (textBuffer->selection.start)
	{
		return textBuffer->selection;
	}
	else
	{
		return getRowSelection(textBuffer);
	}
}
internal Selection normalizedSelection(TextBuffer *textBuffer, Selection s)
{
	if (s.length < 0)
	{
		s.length = -s.length;
		for (int i = 0; i < s.length; i++)
		{
			getPreviousCharacter(&textBuffer->buffer, &s.start);
		}
		return s;
	}
	else
	{
		return s;
	}
}
internal Selection normalizedSelectionOrRow(TextBuffer *textBuffer)
{
	Selection ret = textBuffer->selection;
	if (!validSelection(textBuffer->selection))
	{
		return getRowSelection(textBuffer);
	}
	return normalizedSelection(textBuffer, textBuffer->selection);
}

internal void removeSelection(TextBuffer *textBuffer,Selection s)
{
	//fix me up, this is ugly
	gotoCharacter(textBuffer, s.start);
	removeCharacter(textBuffer);
	for(int i = 0; i<s.length-1;i++)
	{
		deleteCharacter(textBuffer);
	}
	textBuffer->selection.length = 0;

}


internal void cut(TextBuffer *textBuffer)
{
	//copy is normalizeing as well...
	// double work isn't very nice...

	copy(textBuffer);
	Selection s = normalizedSelectionOrRow(textBuffer);
	removeSelection(textBuffer, s);

} 

internal int getIndentLine(GapBuffer *buffer)
{
	char16_t *character = buffer->left;
	int counter=0;
	do
	{
		if(isLineBreak(*character)) return counter;
		if(*character == '\t')++counter;
	}while(getPreviousCharacter(buffer,&character));
	return counter;
}



void moveSelection(TextBuffer *textBuffer, Direction dir)
{
	textBuffer->selection.length += intFromDir(dir);
}


void processInputToBuffer(TextBuffer *textBuffer, Input *input,bool *recalcColor)
{

	if (input->isChar)
	{
		*recalcColor = true;
		if (input->character == '\t')
		{
			if (!input->shift)
			{
				insertTab(textBuffer);	
			}
			else
			{
				removeTab(textBuffer);
			}
		}
		else if(isLineBreak(input->character))
		{
			int indent = getIndentLine(&textBuffer->buffer);
			appendCharacter(textBuffer, input->character);

			for(int i = 0; i< indent;i++)
			{
				appendCharacter(textBuffer, '\t');
			}
		}
		else
		{
			appendCharacter(textBuffer, input->character);
		}
	}
	else
	{
		//this needs to be fixed up.
		//this is basically duped code form the platform layer... 
		//remove it in the platform.
		local_persist bool oldShiftDown=false;
		switch (input->keys)
		{
		case key_up:
			moveUp(textBuffer,input->shift);
			break;

		case key_copy:
			copy(text
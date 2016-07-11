#include "header.h"
#include "colorScheme.h"



global_variable identifier idents[] =
{
	{ ast_type, hash( u"bool"  )},
	{ ast_type, hash( u"int"   )},
	{ ast_type, hash( u"float" )},
	{ ast_type, hash( u"char"  )},
	{ ast_type, hash( u"enum"  )},
	{ ast_type, hash( u"union" )},
	{ ast_type, hash( u"struct")},
	{ ast_type, hash( u"union" )},
	{ ast_type, hash( u"void"  )},
	{ ast_type, hash( u"true"  )},
	{ ast_type, hash( u"false" )},
	{ ast_type, hash(u"short") },
	{ ast_type, hash(u"long")  },
	{ ast_type, hash(u"double") },
	
	{ ast_keyword, hash(u"auto") },
	{ ast_keyword, hash(u"break") },
	{ ast_keyword, hash(u"case") },
	{ ast_keyword, hash(u"const") },
	{ ast_keyword, hash(u"default") },
	{ ast_keyword, hash(u"do") },
	{ ast_keyword, hash(u"else") },
	{ ast_keyword, hash(u"extern") },
	{ ast_keyword, hash(u"for") },
	{ ast_keyword, hash(u"goto") },
	{ ast_keyword, hash(u"if") },
	{ ast_keyword, hash(u"register") },
	{ ast_keyword, hash(u"return") },
	{ ast_keyword, hash(u"signed") },
	{ ast_keyword, hash(u"sizeof") },
	{ ast_keyword, hash(u"static") },
	{ ast_keyword, hash(u"switch") },
	{ ast_keyword, hash(u"typedef") },
	{ ast_keyword, hash(u"unsigned") },
	{ ast_keyword, hash(u"volatile") },
	{ ast_keyword, hash(u"while") },
	{ ast_keyword, hash(u"define") },
	{ ast_keyword, hash(u"include") },
};
DynamicArray_identifier knownTokens = DynamicArrayFromFixedSize(idents, sizeof(idents) / sizeof(idents[0]),"parser:identifiers");

internal int orderedInsert(DynamicArray_identifier *arr, identifier id)
{
	unsigned int upper = arr->length - 1;
	unsigned int lower = 0;

	for (;;)
	{
		unsigned int middle = (upper + lower) << 1;
		if (middle == upper || middle == lower)		//I'm not sure about this.. let's check it out later.
		{
			Insert(arr, id, middle);		
		}
		int middleHash = arr->start[middle].hashedName;
		if (id.hashedName > middleHash)
		{
			lower = middle;
		}
		else if (id.hashedName < middleHash)
		{
			upper = middle;
		}
		else
		{
			return middle;
		}
	}
}

internal bool stripInitialWhite(MultiGapBuffer *buffer, char **character, int *counter)
{
	*counter = 0;
	while (isspace(**character))
	{
		if (!getNextCharacter(buffer, character))
		{
			return false;
		}
		++(*counter);
	}
	return true;
}

internal int getNextTokenHash(MultiGapBuffer *buffer, char **character, int *counter)
{
	*counter = 0;
	int hash = 0;

	if (isFuncDelimiter (**character))
	{
		*counter=1;
		hash = **character;
		getNextCharacter(buffer, character);
		return hash;
	}
	
	do {
		++*counter;
		hash = hash * 101 + (**character);
	} while (getNextCharacter(buffer, character)&&!isFuncDelimiter(**character));
	
	//} while (getNextCharacter(buffer, character) && (!isFuncDelimiter(**character) || (isspace(**character) && hash == 0)));

	return hash;
}

internal int tokenMatch(int token)
{
	// linear search... could probably cut off about 20 % of runtime here...
	// thats quite alot.
	// we could also save abit of the cycles when it differs.
	// also maybe not when working in comment?
	// also let's not reparse everything shall we!
	
	for (int i = 0; i<knownTokens.length; i++)
	{
		if (knownTokens.start[i].hashedName == token)	
		{
			return i;
		}
	}
	return -1;
}

internal bool isLineComment(MultiGapBuffer *buffer, char *character)
{
	if (*character == '/')
	{
		if (getNextCharacter(buffer, &character))
		{
			if (*character == '/')
			{
				return true;
			}
		}
	}
	return false;
}

internal bool isStartBlockComment(MultiGapBuffer *buffer, char *character)
{
	if (*character == '/')
	{
		if (getNextCharacter(buffer, &character))
		{
			if (*character == '*')
			{
				return true;
			}
		}
	}
	return false;
}

internal bool isEndBlockComment(MultiGapBuffer *buffer, char *character)
{
	if (*character == '*')
	{
		if (getNextCharacter(buffer, &character))
		{
			if (*character == '/')
			{
				return true;
			}
		}
	}
	return false;
}

internal int continueToEOL(MultiGapBuffer *buffer, char **character)
{
	int counter = 0;
	while (!isLineBreak(**character))
	{
		if (!getNextCharacter(buffer, character)) {
			return counter;
		}
		++counter;
	}
	return counter;
}

internal int continueToEndOfBlockComment(MultiGapBuffer *buffer, char **character)
{
	int counter = 0;
	while (!isEndBlockComment(buffer, *character))
	{
		if (!getNextCharacter(buffer, character)) {
			return counter;
		}
		counter++;
	}
	return counter;
}

internal int continueToCharacterAfter(MultiGapBuffer *buffer, char **character, char target)
{
	int counter = 0;
	do{//continue to
		if (!getNextCharacter(buffer, character)) {
			return counter;
		}
		++counter;
	} while (**character != target);
	
	getNextCharacter(buffer, character);	//after
	return counter+1;
}
internal void advanceWord(MultiGapBuffer *buffer, char **character)
{
	while (getNextCharacter(buffer, character) && !isFuncDelimiter(**character));
}

//actually we do not need to construct an ast... I think...
#if 0
internal void parseForSymbols(TextBuffer *textBuffer)
{

	knownTokens.length = sizeof(idents) / sizeof(idents[0]);

	char *character = textBuffer->buffer.leftEdge;
	int hashedWord=0;
	int lastHashedWord = -1;
	int counter = 0;
	while (character != textBuffer->buffer.rightEdge && hashedWord != -1)
	{
		int c;
		if (!stripInitialWhite(&textBuffer->buffer, &character, &c))
		{
			return;
		}
		counter += c;

		bool firstAlpha = isalpha(*character);
		hashedWord = getNextTokenHash(&textBuffer->buffer, &character,&c);
		counter += c;
		
		if (hashedWord == -1){break;}

		if (hashedWord == hash(u"union") || hashedWord == hash(u"struct") || hashedWord == hash(u"enum"))
		{
			hashedWord = getNextTokenHash(&textBuffer->buffer, &character,&c);
			counter += c;

			if (hashedWord != u'{') // annonomous structs etc. doens't work..
			{
				identifier ident = {};
				ident.hashedName = hashedWord;
				ident.type = ast_struct;
				Add(&knownTokens,ident);
			}
		}

		if (*character == '(' && firstAlpha)
		{
			AST_Type type = knownTokens.start[tokenMatch(lastHashedWord)].type;
			if (type == ast_type)
			{
				identifier ident = {};
				ident.hashedName = hashedWord;
				ident.type = ast_function;
				Add(&knownTokens, ident);
			}
		}
		lastHashedWord = hashedWord;
	}
}
#endif 
#if 0
internal void parseForCC(TextBuffer *textBuffer)
{
	textBuffer->colorChanges.length = 0;
	parseForSymbols(textBuffer);
	char *character = textBuffer->buffer.leftEdge;
	getNextCharacter(&textBuffer->buffer,&character);
	
	int color = foregroundColor;
	int oldColor = 0x12583902;
	int lastHash=-1;
	int counter = 0;
	while(character!=textBuffer->buffer.rightEdge)
	{
		int c;

		if (!stripInitialWhite(&textBuffer->buffer, &character,&c))
		{
			break;
		}
		//is everything my fault?? are whitespaces not counted properly??
		counter += c;

		char *start = character;
		int start_index = counter;

		int hash = getNextTokenHash(&textBuffer->buffer, &character,&c);
		counter += c;
		
		int offset=0;
		
		if (hash == '/')
		{
		
			if (*character == '*')
			{
				color = commentColor;
				counter += continueToEndOfBlockComment(&textBuffer->buffer, &character);
			}
			if (*character == '/')
			{
				color = commentColor;
				counter += continueToEOL(&textBuffer->buffer, &character);
			}
		}
		else if (hash == '\'' || hash== '\"')
		{
			color = literalColor;
			counter += continueToCharacterAfter(&textBuffer->buffer, &character, hash);
		}
		else if (isdigit(*start))
		{
			color = literalColor;
		}
		else
		{
			int index = tokenMatch(hash);
			if(index!=-1)
			{
				if (knownTokens.start[index].type == ast_type)
				{
					color = typeColor;
				}
				else if (knownTokens.start[index].type == ast_keyword)
				{
					color = keywordColor;
				}
				else if (knownTokens.start[index].type == ast_struct)
				{
					color = structColor;
				}
				else if (knownTokens.start[index].type == ast_function)
				{
					color = funcColor;
				}
			} else 
			{
				// this is a very bad test for a function... 
				// first of we don't strip whitespace, second off we don't know if this is an identifier.
				/*
				 if (*(character)=='(')
				{
					color = funcColor;
				}
				else
				*/
				color = foregroundColor;
			}
		}
		
		if (color != oldColor)
		{

			ColorChange elem = {};
			elem.color = color;
			elem.index = start_index;
			
			Add(&textBuffer->colorChanges, elem);
			oldColor = color;
		}

		lastHash = hash;
	}
}
#endif
struct Lexer_old 
{
	MultiGapBuffer *buffer;
	char *character;
	int counter;
};


enum asdkjbas
{
	token_keyword,
	token_type,
};

struct Token_old
{
	asdkjbas type;
	int hash;
	int position;
	int length;
	
	union {
		char *string;
		int index;
	};
};

DEFINE_DynamicArray(Token_old)

internal Lexer_old createLexer_old(MultiGapBuffer *buffer, int start = 0)
{
	Lexer_old ret = {};
	ret.buffer = buffer;
	MGB_Iterator it = getIterator(buffer);
	ret.iterator = it;
	ret.counter = start;
	return ret;
}


/*
DynamicArray_Token tokens = constructDynamicArray_Token(10);
internal void lex(MultiGapBuffer *buffer)
{
	Lexer lexer = createLexer(buffer);
	Token t;
	while (getNextToken(&lexer,&t))
	{
		Add(&tokens, t);
	}
}
*/

internal bool getNextToken(Lexer_old *lexer, Token_old *ret)
{
	int counter;
	if (!stripInitialWhite(lexer->buffer, &lexer->character, &counter)) return false; 
	
	lexer->counter += counter;
	*ret = {};
	ret->position = counter;

	int hash = 0;
	do
	{
		++lexer->counter;
		hash = 101 * hash + *lexer->character;
	} while (getNextCharacter(lexer->buffer, &lexer->character) && !isFuncDelimiter(*lexer->character));

	ret->length = lexer->counter - ret->position;
	ret->hash = hash;
	ret->string = (char *)alloc_(ret->length + 1 * sizeof(char),"nextToken"); // '\0' -> + 1
	memcpy(ret->string, (lexer->character - ret->length), ret->length * sizeof(char));
	ret->string[ret->length] = 0;

	return true;
}



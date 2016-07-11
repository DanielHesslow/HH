
#define DEFINE_Stack(type)\
type Pop(DynamicArray_##type *arr)\
{\
	return arr->start[--arr->length];\
}\
\
type Peek(DynamicArray_##type *arr)\
{\
	return arr->start[arr->length - 1];\
}\
\
void Push(DynamicArray_##type *arr, type elem)\
{\
	Add(arr, elem);\
}\






//not working yoo.
//just cpy replace..

//shimming 

char16_t *getNextCharacter_MultiGapBuffer(LBuffer buffer, void *iterator,bool *success)
{
	MultiGapBuffer *multiGapBuffer = (MultiGapBuffer *) buffer.data;
	
	*success = getNext(multiGapBuffer,(MGB_Iterator *)iterator);
	
	return getCharacter(multiGapBuffer, *(MGB_Iterator *)iterator);
} 

void *getFirst_MultiGapBuffer(LBuffer buffer)
{
	MultiGapBuffer *multiGapBuffer = (MultiGapBuffer *) buffer.data;
	MGB_Iterator *it = (MGB_Iterator *)alloc_(sizeof(MGB_Iterator),"lexer MGB_Iterator");
	*it = getIterator(multiGapBuffer);

	return it;
} 

LBuffer get_LBuffer(MultiGapBuffer *multiGapBuffer)
{
	LBuffer ret = {};
	ret.getNextCharacter = getNextCharacter_MultiGapBuffer;
	ret.getFirst = getFirst_MultiGapBuffer;
	ret.data = multiGapBuffer;
	return ret;
}

LBuffer get_LBuffer_Define(MultiGapBuffer *multiGapBuffer, void *iterator)
{
	LBuffer ret = {};
	ret.getNextCharacter = getNextCharacter_MultiGapBuffer;
	ret.getFirst = getFirst_MultiGapBuffer;
	ret.data = multiGapBuffer;
	return ret;	
}

DEFINE_DynamicArray(char16_t);

enum TYPE_TYPE
{
	unknown,
	type_specifier,
	type_declaration,
	type_modifier,
	function,
	variable,
};


// you know that sometimes you're just angry at your code
// and you have to cheer yourself up.
// this is one of those times. 
// it's staying!
enum type_qualifier 
{
	constness = 1 << 1,
	volatileness = 1 << 2,
	restrictness = 1 << 3,
	unsignedness = 1 << 4,	//wait why both unsigned and signed? surely it's either singed or unsigned.... Nope - C-standard
	signedness = 1 << 5,	// (char can be either or neither..)	

};

struct type_modifiers
{
	type_qualifier qualifier_BitField;
	signed char size; // -1 = short, 0 = int,  1 = long, 2 = long long 
};

struct Type
{
	int type_id;
	type_modifiers modifiers;
	Type *pointee_type;
};


struct TypeSignature
{
	TYPE_TYPE typeType;
	Type returnType;

	Type *inValues;
	size_t inValueLength;
};


struct String
{
	size_t length;
	char16_t *start;
};

struct Token
{
	TypeSignature type;
	String name;
	int hash;
	Location location;
};
DEFINE_DynamicArray(Token);


Token createToken(const char16_t *name)
{
	Token t = {};
	t.name.start = strcpy((char16_t *)name);
	t.name.length = DHSTR_strlen((char16_t *)name);
	t.hash = hash(name);
	return t;
}



struct LexerState
{
	void *iterator;
	LBuffer buffer;
	Location location;
};

DEFINE_DynamicArray(LexerState);
DEFINE_Stack(LexerState);

struct Lexer
{
	DynamicArray_LexerState stateStack;
	void *iterator;
	LBuffer buffer;
	DynamicArray_char16_t tempStorage;	
	Token lastToken;
	bool hasBufferedToken;
	Location location;
};

void pushLexerState(Lexer *lexer, LBuffer buffer, Location loc)
{
	LexerState state = {};
	state.buffer = lexer->buffer;
	state.iterator = lexer->iterator;
	state.location = lexer->location;

	lexer->buffer = buffer;
	lexer->iterator = buffer.getFirst(buffer);
	lexer->location = loc;

	Push(&lexer->stateStack, state);
}

void popLexerState(Lexer *lexer)
{
	LexerState state = Pop(&lexer->stateStack);
	lexer->buffer = state.buffer;
	lexer->iterator = state.iterator;
	lexer->location = state.location;
}

Lexer createLexer(MultiGapBuffer *MultiGapBuffer)
{
	Lexer ret = {};
	ret.stateStack =  constructDynamicArray_LexerState(100, "lexerStateStack");
	ret.buffer = get_LBuffer(MultiGapBuffer);
	ret.tempStorage = constructDynamicArray_char16_t(100, "lexer_input");
	ret.location.line= 1;
	pushLexerState(&ret, ret.buffer, ret.location);
	return ret;
}

void freeLexer(Lexer lexer)
{
	free_(lexer.stateStack.start);
	free_(lexer.tempStorage.start);
}


String StringFromDAC(DynamicArray_char16_t dac)
{
	String s = {};
	s.length = dac.length;
	size_t size = sizeof(char16_t)*(s.length);
	s.start = (char16_t *)alloc_(size+1,"string from DynamicArray_char16_t");
	memcpy((void *)s.start,(void *) dac.start,size);
	s.start[s.length] = 0;
	return s;
}

static const char16_t *multiCharSymbolTokens[] 
{
	u"//",
	u"/*",
	u"*/",
	u"++",
	u"--",
	u"==",
	u"!=",
	u">=",
	u"<=",
	u"&&",
	u"||",

	u">>",
	u"<<",
	u"+=",
	u"-=",
	u"*=",
	u"/=",
	u"%=",
	u"/=",
	u"<<=",
	u">>=",

	u"&=",
	u"^=",
	u"|=",
	u"->"
};

//todo handle escape characters

char16_t getNextChar(Lexer *lexer, bool *eof)
{
	bool success;
	int preblocks = blocks.length;
	char16_t ret =  *lexer->buffer.getNextCharacter(lexer->buffer, lexer->iterator, &success);
	*eof = !success;	
	if (*eof)
	{
		if (lexer->stateStack.length > 0)
		{
			popLexerState(lexer);
			*eof = false;
		}
	}

	if (ret == '\n')
	{
		lexer->location.line++;
		lexer->location.column = 0;
	}
	else
	{
		++lexer->location.column;
	}
	return ret;
}
char16_t readNextIntoTemp(Lexer *lexer, bool *eof)
{
	char16_t next = getNextChar(lexer, eof);
	Add(&lexer->tempStorage, next);
	return next;
}

void rollbackLexer(Lexer *lexer, Lexer lexer_image)
{
	lexer->iterator = lexer_image.iterator;
	lexer->tempStorage.length = lexer_image.tempStorage.length;
	lexer->location = lexer_image.location;
}

int readSymbolIntoTemp(Lexer *lexer)
{
	Lexer before = *lexer;
	int symbols = ARRAY_LENGTH(multiCharSymbolTokens);
	for (int i = 0; i<symbols; i++)
	{
		for (int j = 0;; j++)
		{
			char16_t referenceChar = multiCharSymbolTokens[i][j];
			if (referenceChar)
			{
				if (j == lexer->tempStorage.length)
				{
					bool eof;
					readNextIntoTemp(lexer,&eof);
					if ( eof || !isFuncDelimiter(lexer->tempStorage.start[j]))
					{
						*lexer = before;
						return -1;
					}
				}
				if (lexer->tempStorage.start[j] != referenceChar)break; //check next symbol
			}
			else // null terminator.
			{
				return i;
			}
		}
	}
	
	rollbackLexer(lexer, before);
	return -1;
}


void stripWhite(Lexer *lexer)
{
	for (;;)
	{
		Lexer prev = *lexer;
		bool eof;
		char16_t character = getNextChar(lexer, &eof);
		if (eof|| !isspace(character))
		{
			rollbackLexer(lexer,prev);
			return;
		}
	}
}

void stripUntilEOL(Lexer *lexer)
{
	for (;;)
	{
		Lexer prev = *lexer;
		bool eof;
		char16_t character = getNextChar(lexer, &eof);
		if (eof || isLineBreak(character))
		{
			rollbackLexer(lexer, prev);
			return;
		}
	}
}

void stripUntilSymbol(Lexer *lexer)
{
	for (;;)
	{
		Lexer prev = *lexer;
		bool eof;
		char16_t character = getNextChar(lexer, &eof);
		if (eof || ispunct(character))
		{
			rollbackLexer(lexer, prev);
			return;
		}
	}
}

void readUntil(Lexer *lexer, bool(*isDelimiter)(char16_t))
{
	for (;;)
	{
		Lexer prev = *lexer;
		bool eof;
		char16_t character = readNextIntoTemp(lexer, &eof);
		if (eof || isDelimiter(character))
		{
			rollbackLexer(lexer, prev);
			return;
		}
	}
}

void readIdentifierIntoTemp(Lexer *lexer)
{
	readUntil(lexer, isFuncDelimiter);
}

inline bool numDelimiter(char16_t character) { return (isFuncDelimiter(character) && character != '.'); }
void readNumberIntoTemp(Lexer *lexer)
{
	readUntil(lexer, numDelimiter);
}
//below will fuck up eof?
inline bool stringLiteralDelimiter(char16_t character) { return character == '"'; }
void readStringLiteralIntoTemp(Lexer *lexer)
{
	readUntil(lexer, stringLiteralDelimiter);
	bool eof;
	readNextIntoTemp(lexer, &eof);
}

inline bool charLiteralDelimiter(char16_t character) { return character == '\''; }
void readCharLiteralIntoTemp(Lexer *lexer)
{
	readUntil(lexer, charLiteralDelimiter);
	bool eof;
	readNextIntoTemp(lexer, &eof);
}

Location lexIntoStorage(Lexer *lexer, bool *eof)
{
	stripWhite(lexer);
	Location loc = lexer->location;
	char16_t character = readNextIntoTemp(lexer, eof);
	if (!*eof)
	{
		if (isdigit(character))
		{
			readNumberIntoTemp(lexer);
		}
		else if (isalpha(character))
		{
			readIdentifierIntoTemp(lexer);
		}
		else
		{
			if (character == '"')
			{
				readStringLiteralIntoTemp(lexer);
			}
			else if (character == '\'')
			{
				readCharLiteralIntoTemp(lexer);
			}
			else
			{
				int index = readSymbolIntoTemp(lexer);
				if (index == 0) //line comment
				{
					stripUntilEOL(lexer);
					lexer->tempStorage.length = 0;
					loc = lexIntoStorage(lexer, eof);
				}
				else if (index == 1) //block comment
				{	
					for (;;)
					{
						lexer->tempStorage.length = 0;
						stripUntilSymbol(lexer);
						int index = readSymbolIntoTemp(lexer);
						if (index == -1)
						{
							readNextIntoTemp(lexer, eof);
						}
						if (index == 2||*eof) break;
					}

					lexer->tempStorage.length = 0;
					loc=lexIntoStorage(lexer, eof);
				}
			}
		}
	}
	return loc;
}

Token lex(Lexer *lexer, bool *eof)
{
	if (lexer->hasBufferedToken)
	{
		return lexer->lastToken;
	}
	else
	{
		Token t = {};

		if (!lexer->iterator)
		{
			lexer->iterator = lexer->buffer.getFirst(lexer->buffer);
		}
		*eof = false;

		t.location = lexIntoStorage(lexer, eof);

		t.name = StringFromDAC(lexer->tempStorage);
		t.hash = lexer->tempStorage.start[0];
		for (int i = 1; i < lexer->tempStorage.length; i++)
		{
			t.hash = t.hash * 101 + lexer->tempStorage.start[i];
		}
		lexer->tempStorage.length = 0;
		lexer->lastToken = t;
		return t;
	}
}









struct TypeMap
{
	Type type;
	int hash;
};
DEFINE_DynamicArray(TypeMap);



//--- PreProcessor

bool match(Token t,const char *name)
{
	if (t.hash == hash(name))
	{
		for (int i = 0; i < t.name.length; i++)
		{
			if (t.name.start[i] == name[i])
			{
				return true;
			}
		}
	}
	return false;
}


Token preprocess(Lexer *lexer, bool *eof, DynamicArray_TypeMap *typemap)
{
	Token t = lex(lexer, eof);
	if (match(t, "#"))
	{
		free_((void *)t.name.start);
		t = lex(lexer, eof);
		
		return t;
	}
	else
	{
		return t;
	}
}




// --- TYPE-MARKER



int initialTypes[]{
	hash(u"void"),
	hash(u"bool"),
	hash(u"int"),
	hash(u"float"),
	hash(u"char"),
	hash(u"double"),
};



//implement a generic macro binary search!

//this should work right??
int binarySearch(DynamicArray_TypeMap arr, int elem, bool *hasElem)
{
	int lowerBound = 0;
	int upperBound = arr.length - 1;

	for (;;)
	{
		int center = (lowerBound + upperBound) / 2;

		int centerElem = arr.start[center].hash;
		
		if (elem == centerElem)
		{
			*hasElem = true;
			return center; //found it yoo
		}
		
		if (center == lowerBound) // int div rounds down (but we still check one iteration to early...
		{
			if (elem == arr.start[upperBound].hash)
			{
				*hasElem = true;
				return upperBound;
			}
			else
			{
				*hasElem = false;
				return upperBound; 
			}
		}
		
		if (elem>centerElem)
		{
			lowerBound = center;
		}
		else
		{
			upperBound = center;
		}
	}
}


static DynamicArray_TypeMap identifiers = {};

int orderTypeMap(const void *a, const void *b)
{
	return ((TypeMap *)a)->hash - ((TypeMap *)b)->hash;
}
//might be reduced to one binarySearch 
//returns 0 on fail
Type getType(int hash, bool *hasType) 
{
	Type type = identifiers.start[binarySearch(identifiers, hash,hasType)].type;
	return type;
}


//ie typedef a b
//addTypdef(hash(a),hash(b);



void addType_(int hash,Type type)
{
	bool hasHash;
	int index = binarySearch(identifiers, hash, &hasHash);
	assert(!hasHash && "unhahdled Hash-clash");
	TypeMap map = {};
	map.hash = hash;
	map.type= type;

	Insert(&identifiers, { type,hash }, index);
}

void addTypedef(int fromHash, int newHash, type_modifiers newModifiers) //so this seams unnessasary..
{
	bool hasType;
	Type t = getType(fromHash,&hasType);
	t.modifiers = newModifiers;

	if (hasType)
	{
		addType_(newHash, t);
	}
	else
	{
		assert(false && "typedeffing to unknown type");
	}
}

void addType(int hash)
{
	Type t = {};
	t.type_id = identifiers.length;
	addType_(hash,t);
}

// don't relly on hashes beeing different.
// this is not production quality.... But well it's not like the rest of the codebase is...




// i'm not sure how typemodifiers / typequalifiers should be handled.
// they should probably be assigned to their own type_id, 
// like unsigned int and int should have different type_id's
// otherwise typedefs won't work...(as implemented now)
// however, I'd sort of like to have them be their own nodes.
// What we'll do is have them visible in the declaration but then calculate their type_id.  

// also pointers shuld probably be merged into the type here aswell..
// I'm just not really sure how to do it right.

// also how do the pointers work?
// do they also have thier own type_id? probably not
// but they need to be able to be type_modified as well.
// do they have refence to a type?

#if 0	//here's the what we have to do.

//this is why I've put this off for a while...

unsigned int == int unsigned

const int == int const

int * != int

const int * != int *const

long long int != long int
unsigned unsigned int == unsigned int;

//not allowed
signed unsigned int 
long short int


// also
long == long int
short == short int

unsigned == unsigned int 
signed == signed int


// for _obvious_  reasons:
unsigned char != char != signed char

// yea this totaly makes sense.

// I mean COME THE FUCK ON!
//this fact is not supported atm.
#endif 

//--- Parser
struct  AST_EXPR
{
	AST_EXPR *children;
	int length;
	Token token;
};


DEFINE_DynamicArray(AST_EXPR);
int type_qualifiers[] // type_modifiers.
{
	hash(u"const"),
	hash(u"volatile"),
	hash(u"restrict"),
	hash(u"signed"),
	hash(u"unsigned"),
	hash(u"long"),
	hash(u"short"),
};



int getTypeQualifierIndex(int hash)
{
	int len = ARRAY_LENGTH(type_qualifiers);
	for (int i = 0; i < len; i++)
	{
		if (type_qualifiers[i] == hash)
		{
			return i;
		}
	}
	return -1;
}

void applyModifier(Type *type, int QIndex)
{
	switch (QIndex)
	{
		case 0:
			type->modifiers.qualifier_BitField = (type_qualifier) (((int)type->modifiers.qualifier_BitField) | constness);
			break;
		case 1:
			type->modifiers.qualifier_BitField = (type_qualifier)(((int)type->modifiers.qualifier_BitField) | volatileness);
			break;
		case 2:
			type->modifiers.qualifier_BitField = (type_qualifier)(((int)type->modifiers.qualifier_BitField) | restrictness);
			break;
		case 3:
			type->modifiers.qualifier_BitField = (type_qualifier)(((int)type->modifiers.qualifier_BitField) | signedness);
			break;
		case 4:
			type->modifiers.qualifier_BitField = (type_qualifier)(((int)type->modifiers.qualifier_BitField) | unsignedness);
			break;
		case 5:
			type->modifiers.size = (type->modifiers.size + 1) % 2;
			break;
		case 6:
			type->modifiers.size = -1;
			break;
		default:
		{
			assert(QIndex != 7 && "pointers are not handled in applyModifier");
			assert(false && "unknown modifier_type");
		}
	}
}

// some sort of an resume_and_continue on error?
// do propper peaking yoo.

AST_EXPR createExpr(Token t, AST_EXPR *children, int length)
{
	AST_EXPR ret = {};
	ret.token = t;
	ret.children = (AST_EXPR *)alloc_(length*sizeof(AST_EXPR), "ast_expr_children");
	for (int i = 0; i < length; i++)
	{
		ret.children[i] = children[i]; // memcpy if needed
	}
	ret.length = length;
	return ret;
}


void freeType(Type type)
{
	if (type.pointee_type)
	{
		freeType(*type.pointee_type);
		free_(type.pointee_type);
	}
}
void freeExpr(AST_EXPR expr)
{
	for (int i = 0; i < expr.length; i++)
	{
		freeExpr(expr.children[i]);
	}

	//type-shit
	freeType(expr.token.type.returnType);
	for (int i = 0; i < expr.token.type.inValueLength; i++)
	{
		freeType(expr.token.type.inValues[i]);
	}

	free_(expr.token.name.start);
	if (expr.children)
		free_(expr.children);
}


DEFINE_Stack(AST_EXPR);

AST_EXPR parse_Struct(Lexer *lexer, bool *eof, Token t);
AST_EXPR parse_Enum(Lexer *lexer, bool *eof, Token t);


void reportError(Lexer *lexer, const char *error)
{
	OutputDebugString(error);
	OutputDebugString(" | parser\n");
}

AST_EXPR getNextExpr(Lexer *lexer, bool *eof, const char16_t *exprDelimiter = u";", int *exitDelimiter = 0);

//typedefs are handled here but the rest of of the preprocessor (defines ifdefs and so on) is handled the preprocess.
AST_EXPR markTypes_complex(Lexer *lexer, bool *eof)
{
	// freeing us is a bitt messed up.
	// bug!

	if (!identifiers.start)
	{
		identifiers = constructDynamicArray_TypeMap(100, "typemap");
		for (int i = 0; i < ARRAY_LENGTH(initialTypes); i++)
		{
			Type type = {};
			type.type_id = i + 1;
			TypeMap map = {};
			map.hash = initialTypes[i];
			map.type = type;
			Add(&identifiers, map);
		}
		qsort(identifiers.start, identifiers.length, sizeof(TypeMap), orderTypeMap);
		type_modifiers mods = {};
		mods.qualifier_BitField = (type_qualifier)10;
	}

	DynamicArray_AST_EXPR stack = constructDynamicArray_AST_EXPR(10, "type_marker_stack");

	// whenever we get a struct/union/enum we know that what follows 
	// is a type. it might be previously defined or be defined inline.
	// does that mean that structs/enums/unions allready should be a tree?
	

	//bad and slow fucking code.
	// that's allright though.
	//now we just want it to work... 
	// or you know almost work would be progress as well ;)

	Location loc = lexer->location;
	for (;;)
	{
		Lexer lexer_image = *lexer;
		Token t = preprocess(lexer, eof, &identifiers);
		bool hasType;
		Type id = getType(t.hash, &hasType);
		
		AST_EXPR expr;
		expr.token = t;
		expr.length = 0;
		
		if (hasType)
		{
			Push(&stack, expr);
		}
		else
		{
			int Qindex = getTypeQualifierIndex(t.hash);
			if (Qindex != -1 || (match(t, "*") && stack.length > 0))
			{
				Push(&stack, expr);
			}
			else
			{
				if (match(t, "struct")|| match(t, "union"))
				{
					Push(&stack,parse_Struct(lexer, eof, t)); 
				}
				else if (match(t, "enum"))
				{
					Push(&stack, parse_Enum(lexer, eof, t)); 
				}
				else if (stack.length == 0)
				{
					free_(stack.start);
					return expr;
				}
				else
				{
					//lexer->hasBufferedToken = true; //apearently this doesn't work.... gotta fix this. this is unnecessary work!
					free_(t.name.start);
					rollbackLexer(lexer, lexer_image);
					break;
				}
			}
		}
	}
	
	// this pointer thingis is a bit unclear.
	// there is probably a much better way to write this.
	// also why the fuck would we need to store them on our stack in the first place?

	Type retType = {};
	Type *type = &retType;
	
	AST_EXPR retExpr = {};
	AST_EXPR *partial_expr = &retExpr;

	DynamicArray_AST_EXPR type_parts = constructDynamicArray_AST_EXPR(10, "type_children");
	
	int len = stack.length; // do not inline into for, Pop reduces the length. 
	for (int i = 0; i < len; i++)
	{
		AST_EXPR expr = Pop(&stack);
		Token t = expr.token;
		
		int QIndex = getTypeQualifierIndex(t.hash);
		if (QIndex != -1)
		{
			expr.token.type.typeType = type_modifier;
			applyModifier(type, QIndex);
		}
		else
		{
			if (match(t,"*"))
			{
				free_(t.name.start);
				Type *type_ = (Type *)alloc_(sizeof(Type), "pointee type");
				*type_ = {};
				type->pointee_type = type_;
				type = type_;
				expr.token.type.typeType = type_modifier;

				AST_EXPR _tmp = {};
				Insert(&type_parts, _tmp,0); //probably invert the order here?
				
				*partial_expr = createExpr(createToken(u"Pointer"), type_parts.start, type_parts.length);
				partial_expr = &partial_expr->children[0];
				
				type_parts.length = 0;
				continue;
			}
			if (match(t, "struct") || match(t, "union")||match(t,"enum"))
			{
				//do nothing... this is temp. will merge the loops soon enough.
			}
			else
			{
				expr.token.type.typeType = type_specifier;
				bool hasType;
				Type id = getType(t.hash,&hasType);
				assert(hasType&& "tokens on the type markers stack should either be modifiers/qualifiers or types.");
				type->type_id= id.type_id;
				if (((int)id.modifiers.qualifier_BitField)||id.modifiers.size ||id.pointee_type)
				{
					type->modifiers.size |= id.modifiers.size;
					type->modifiers.qualifier_BitField = (type_qualifier)(((int)type->modifiers.qualifier_BitField) |((int) id.modifiers.qualifier_BitField));
				}
			}
		}
		Add(&type_parts, expr);
	}
	
	*partial_expr = createExpr(createToken(u"Type"), type_parts.start, type_parts.length);

	free_(type_parts.start);

	//unsigned / signed / long / long long / short are all ints.
	if (retType.type_id == 0 && (retType.modifiers.size || (int)retType.modifiers.qualifier_BitField &(signedness|unsignedness)))
	{
		retType.type_id = 3; //int 
	}
	TypeSignature sig = {};
	sig.returnType = retType;
	sig.typeType = type_specifier;
	retExpr.token.type= sig;
	free_(stack.start);
	return retExpr;
}

AST_EXPR markTypes(Lexer *lexer, bool *eof)
{
	if (!identifiers.start)
	{
		identifiers = constructDynamicArray_TypeMap(100, "typemap");
		for (int i = 0; i < ARRAY_LENGTH(initialTypes); i++)
		{
			Type type = {};
			type.type_id = i + 1;
			TypeMap map = {};
			map.hash = initialTypes[i];
			map.type = type;
			Add(&identifiers, map);
		}
		qsort(identifiers.start, identifiers.length, sizeof(TypeMap), orderTypeMap);
		type_modifiers mods = {};
		mods.qualifier_BitField = (type_qualifier)10;
	}

	DynamicArray_int mods = constructDynamicArray_int(10, "type_marker_mods");
	AST_EXPR type_expr = {};
	Location loc = lexer->location;
	bool isPointer = false;

	for (int i =0;;i++)
	{
		Lexer lexer_image = *lexer;
		Token t = preprocess(lexer, eof, &identifiers);
		bool hasType;
		Type id = getType(t.hash, &hasType);

		AST_EXPR expr = {};
		expr.token = t;
		expr.length = 0;

		if (hasType)
		{
			if (type_expr.token.name.start)
				reportError(lexer, "multiple types may not follow each other");
			else
			{
				type_expr = expr;
				type_expr.token.type.returnType = id;
			}
		}
		else
		{
			int Qindex = getTypeQualifierIndex(t.hash);
			if (Qindex != -1)
			{
				free_(t.name.start);
				Add(&mods,Qindex);
			}
			else if (match(t,"*"))
			{
				if (i == 0)
				{
					type_expr = createExpr(createToken(u"Pointer"), (AST_EXPR *)0, 0);
					isPointer = true;
				}
				else
				{
					free_(t.name.start);
					rollbackLexer(lexer, lexer_image);
					break;
				}
			}
			else
			{
				if (match(t, "struct") || match(t, "union"))
				{
					if (type_expr.token.name.start)
						reportError(lexer, "multiple types may not follow each other");
					type_expr = parse_Struct(lexer, eof, t);
				}
				else if (match(t, "enum"))
				{
					if (type_expr.token.name.start)
						reportError(lexer, "multiple types may not follow each other");
					type_expr = parse_Enum(lexer, eof, t);
				}//not anything type related
				else if (i == 0)
				{
					free_(mods.start);
					return expr;
				}
				else
				{
					free_(t.name.start);
					rollbackLexer(lexer, lexer_image);
					break;
				}
			}
		}
	}
	
	for (int i = 0; i < mods.length; i++)
	{
		applyModifier(&type_expr.token.type.returnType, mods.start[i]);
	}
	Type t = type_expr.token.type.returnType; //just saving keystrokes..
	if (t.type_id == 0 && (t.modifiers.size || (int)t.modifiers.qualifier_BitField &(signedness | unsignedness)))
	{
		type_expr.token.type.returnType.type_id = 3; //int 
	}

	free_(mods.start);

	type_expr.token.type.returnType = t;
	type_expr.token.type.typeType = type_specifier;
	
	return type_expr;
}


enum Assoc
{
	assoc_left_to_right,
	assoc_right_to_left,
};

enum OpType
{
	nonary,	//I'm a funny guy, amiright guys!
	post_unary,
	pre_unary,
	binary,
	ternary,
};

struct Operator
{
	const char16_t *name;
	int hash;
	int presidence;
	OpType opType;
	Assoc assoc;
};

Operator ops[]
{
	{u"++", hash(u"++"),1,post_unary,		assoc_left_to_right},
	{u"--", hash(u"--"),1,post_unary,		assoc_left_to_right},
	{u"(",  hash(u"("),1,binary, 	assoc_left_to_right}, //function call...  not really binary 
	{u"[",  hash(u"["),1,binary, 	assoc_left_to_right}, //array index....	 not really binary
	{u".",   hash(u"."), 1,binary, 	assoc_left_to_right}, 
	{u"->",  hash(u"->"),1,binary, 	assoc_left_to_right}, 
	
	{u"+",hash(u"+"),2,pre_unary,		assoc_right_to_left},	
	{u"-",hash(u"-"),2,pre_unary,		assoc_right_to_left}, 
	{u"!",hash(u"!"),2,pre_unary,		assoc_right_to_left}, 
	{u"~",hash(u"~"),2,pre_unary,		assoc_right_to_left}, 
	{u"++",hash(u"++"),2,pre_unary,		assoc_right_to_left}, 
	{u"--",hash(u"--"),2,pre_unary,		assoc_right_to_left}, 
	{u"*",hash(u"*"),2,pre_unary,		assoc_right_to_left}, 
	{u"&",hash(u"&"),2,pre_unary,		assoc_right_to_left}, 
	{u"sizeof",hash(u"sizeof"),2,pre_unary,	assoc_right_to_left}, //really this is an operator...
	{u"alignof",hash(u"alignof"),2,pre_unary,	assoc_right_to_left}, //really this is an operator...

	{u"*",hash(u"*"),3,binary, 	assoc_left_to_right}, 
	{u"/",hash(u"/"),3,binary, 	assoc_left_to_right},
	{u"%",hash(u"%"),3,binary, 	assoc_left_to_right}, 
	
	{u"+",hash(u"+"),4,binary, 	assoc_left_to_right}, 
	{u"-",hash(u"-"),4,binary, 	assoc_left_to_right},

	{u"<<",hash(u"<<"),5,binary, 	assoc_left_to_right}, 
	{u">>",hash(u">>"),5,binary, 	assoc_left_to_right},

	{u"<=",hash(u"<="),6,binary, 	assoc_left_to_right}, 
	{u">=",hash(u">="),6,binary, 	assoc_left_to_right},
	{u">",hash(u">"),6,binary, 	assoc_left_to_right},
	{u"<",hash(u"<"),6,binary, 	assoc_left_to_right},

	{u"==",hash(u"=="),7,binary, 	assoc_left_to_right},
	{u"!=",hash(u"!="),7,binary, 	assoc_left_to_right},

	{u"&",hash(u"&"),8,binary, 	assoc_left_to_right},
	
	{u"^",hash(u"^"),9,binary, 	assoc_left_to_right},

	{u"|",hash(u"|"),10,binary, 	assoc_left_to_right},
	
	{u"&&",hash(u"&&"),11,binary, 	assoc_left_to_right},

	{u"||",hash(u"||"),12,binary, 	assoc_left_to_right},

	{u"?",hash(u"?"),13,ternary, 	assoc_right_to_left},
	
	{u"=",hash(u"="),14,binary, 	assoc_right_to_left},
	{u"+=",hash(u"+="),14,binary, 	assoc_right_to_left},
	{u"-=",hash(u"-="),14,binary, 	assoc_right_to_left},
	{u"*=",hash(u"*="),14,binary, 	assoc_right_to_left},
	{u"/=",hash(u"/="),14,binary, 	assoc_right_to_left},
	{u"%=",hash(u"%="),14,binary, 	assoc_right_to_left},
	{u">>=",hash(u">>="),14,binary, 	assoc_right_to_left},
	{u"<<=",hash(u"<<="),14,binary, 	assoc_right_to_left},
	{u"&=",hash(u"&="),14,binary, 	assoc_right_to_left},
	{u"^=",hash(u"^="),14,binary, 	assoc_right_to_left},
	{u"|=",hash(u"|="),14,binary, 	assoc_right_to_left},

	{ u",",hash(u","),15, binary, 	assoc_left_to_right },
	
	{u";",hash(u";"),16, post_unary, 	assoc_right_to_left }, //this is basically what it is.. sort of... isch
	{u")",hash(u")"),16, nonary, 	assoc_right_to_left }, 
	{ u"(",hash(u"("),16, pre_unary, 	assoc_right_to_left }, 

	{ u"{",hash(u"{"),16, pre_unary, 	assoc_right_to_left }, 
	{ u"}",hash(u"}"),16, nonary, 	assoc_right_to_left }, 
};

//make binary_search
int matchOp(int hash, OpType *type, int len)
{
	int length = ARRAY_LENGTH(type);
	for(int i = 0; i<ARRAY_LENGTH(ops);i++)
	{
		if(ops[i].hash == hash)
		{
			for(int j = 0; j<length;j++)
			{
				if(type[j]==ops[i].opType)
				{
					return i;
				}
			}
		}
	}
	return -1;
}

int matchOp(int hash, int previousArgs)
{
	if(previousArgs == 0)
	{
		OpType arr[] = {pre_unary,nonary};
		return matchOp(hash,arr, ARRAY_LENGTH(arr));	
	}
	else if(previousArgs == 1)
	{
		OpType arr[] = {post_unary,binary,ternary};
		return matchOp(hash,arr,ARRAY_LENGTH(arr));	
	}
	else if (previousArgs == 2)
	{
		OpType arr[] = {ternary };
		return matchOp(hash, arr, ARRAY_LENGTH(arr));
	}
	else
	{
		assert(false && "no operator with that many arguments exist!");
	}
	return -1;
}



DEFINE_DynamicArray(Operator);

DEFINE_Stack(Operator);

struct OpInfo
{
	Operator op;
	Token token;
};

struct keyword
{
	const char16_t *name;
	int hash;

	bool simpleType;
	//since msvc doesn't support c99 
	//placing this in a union just makes shit a mess when list initilizing
	//so you know, I don't.. (who cares anyway)

	int children; //simple keywords just pull a couple of children.
	
	union {
		AST_EXPR(*getExpr)(Lexer *lexer, bool *eof,Token t);
	};
};


void eatToken(Lexer *lexer, bool *eof, const char *expectedToken, const char *error)
{
	Token t = preprocess(lexer,eof,&identifiers);
	if (!match(t, expectedToken))
	{
		reportError(lexer, error);
	}
	free_(t.name.start);
}

void eatTokenIf(Lexer *lexer, bool *eof, const char *expectedToken)
{
	Lexer lexer_image = *lexer;
	Token t = preprocess(lexer, eof,&identifiers);
	if (!match(t, expectedToken))
	{
		rollbackLexer(lexer, lexer_image);
	}
	free_(t.name.start);
}

AST_EXPR parse_For(Lexer *lexer, bool *eof, Token t)
{
	eatToken(lexer, eof, "(", "expected initial paren after for");
	int out;
	AST_EXPR init = getNextExpr(lexer, eof, u";)",&out);
	if (out != 0)reportError(lexer, "expected semicolon after init in for");
	AST_EXPR check = getNextExpr(lexer, eof, u";)", &out);
	if (out != 0)reportError(lexer, "expected semicolon after check in for");
	AST_EXPR iteration = getNextExpr(lexer, eof, u";)", &out);
	if (out != 1)reportError(lexer, "expecting ending paren after iteration part of for");
	AST_EXPR block = getNextExpr(lexer, eof);
	AST_EXPR children[] = { init,check,iteration,block};
	return createExpr(t,children,4);
}

AST_EXPR parseArgList(Lexer *lexer, bool *eof)
{//without initial paren
	DynamicArray_AST_EXPR stack = constructDynamicArray_AST_EXPR(2, "argument list stack");
	for (;;)
	{
		// this is bad 
		// we actually want
		// getNextExpr(lexer,eof,',)');
		int exit;
		AST_EXPR next = getNextExpr(lexer, eof, u",)", &exit);
		
		if (exit == 1||*eof)
		{
			Push(&stack, next);
			if (next.token.name.start)
				reportError(lexer, "last argument in a function may not be followed with a comma");

			free_(stack.start);
			return createExpr(createToken(u"arglist"), stack.start, stack.length);
		}
		Push(&stack, next);
	}

	eatToken(lexer, eof, ")", "expected ending paren before for block");
}



AST_EXPR parse_Function(Lexer *lexer, bool *eof, AST_EXPR id)
{
	if (id.children)
	{
		reportError(lexer, "function name is not an identifier");
	}
	
	AST_EXPR Args = parseArgList(lexer,eof);
	AST_EXPR block = getNextExpr(lexer, eof);
	AST_EXPR children[] = { id,Args,block };
	return createExpr(createToken(u"function"), children, 3);
}


AST_EXPR parse_If(Lexer *lexer, bool *eof, Token t)
{
	eatToken(lexer, eof, "(", "expected initial paren after if");
	AST_EXPR check = getNextExpr(lexer,eof,u")");
	AST_EXPR block = getNextExpr(lexer, eof);
	AST_EXPR children[] = { check,block };
	return createExpr(t, children, 2);
}
AST_EXPR parse_Else(Lexer *lexer, bool *eof, Token t)
{
	AST_EXPR block = getNextExpr(lexer, eof);
	AST_EXPR children[] = { block };
	return createExpr(t, children, 1);
}
AST_EXPR parse_Switch(Lexer *lexer, bool *eof, Token t)
{
	eatToken(lexer, eof, "(", "expected initial paren after if");
	AST_EXPR check = getNextExpr(lexer, eof, u")");

	AST_EXPR block = getNextExpr(lexer, eof);

	AST_EXPR children[] = { block,check};
	return createExpr(t, children, 2);
}

AST_EXPR parse_While(Lexer *lexer, bool *eof, Token t)
{
	eatToken(lexer, eof, "(", "expected initial paren after while");
	AST_EXPR check = getNextExpr(lexer, eof,u")");
	eatToken(lexer, eof, ")", "expected ending paren bofore while block");
	AST_EXPR block = getNextExpr(lexer, eof);
	AST_EXPR children[] = { check,block };
	return createExpr(t, children, 2);
}

AST_EXPR parse_Do(Lexer *lexer, bool *eof, Token t)
{
	AST_EXPR block = getNextExpr(lexer, eof);
	eatToken(lexer, eof, "while", "do block should be followed by a while");
	AST_EXPR check = getNextExpr(lexer, eof);
	AST_EXPR children[] = { block,check};
	return createExpr(t, children, 2);
}

AST_EXPR parse_Default(Lexer *lexer, bool *eof, Token t)
{
	eatToken(lexer, eof, ":", "default should be followed by a colon");
	return createExpr(t, (AST_EXPR *)0, 0);
}

AST_EXPR parse_Case(Lexer *lexer, bool *eof, Token t)
{
	//this isn't really a good parsing but I have no idea what is.
#if 0
	switch (var)
	{
	case 1:
		doA();
	case 2:
		doB();
		break;
	}
#endif
	//the above is absolutly valid code.
	//where case 2 is a subpart of case 1;
	// maybe case1-doA()-doB();
	//		 case2-doB();
	// is the rigth way 
	// but then doB();
	// looks like it appears at two places in the code.
	// if you change the name it will change in two places which might be counter-intuitive..
	// so for now we just do this shit
	// everything is children of the switch case and this just checks the colon.
	
	Token number = preprocess(lexer, eof,&identifiers);

	AST_EXPR condition= {};
	condition.token = number;

	eatToken(lexer, eof, ":", "case should be followed by a colon");
	
	AST_EXPR children[] = { condition};
	return createExpr(t, children, 1);
}



AST_EXPR parse_Struct(Lexer *lexer, bool *eof, Token t)
{

	Lexer lexer_image = *lexer;

	Token name = preprocess(lexer, eof,&identifiers);
	AST_EXPR name_expr;
	bool annon = match(name, "{");
	if (annon)
	{
		rollbackLexer(lexer, lexer_image);
		free_(name.name.start);
		name_expr = createExpr(createToken(u"annon"), 0, 0);
	}
	else
	{
		name_expr = createExpr(name, 0, 0);
	}
	
	lexer_image = *lexer;
	Token next = preprocess(lexer, eof, &identifiers);
	if (match(next, "{"))
	{
		free_(next.name.start);
		DynamicArray_AST_EXPR exprs = constructDynamicArray_AST_EXPR(100, "enum parsing");
		for (;;)
		{
			int exit;
			AST_EXPR enum_item = getNextExpr(lexer, eof, u"};", &exit);
			if (*eof || exit == 0)
				break;
			else if (enum_item.token.name.start)
				Add(&exprs, enum_item);
		}
		AST_EXPR items = createExpr(createToken(u"items"), exprs.start, exprs.length);
		free_(exprs.start);
		AST_EXPR children[] = { name_expr,items };
		return createExpr(t, children, 2);
	}
	else
	{
		rollbackLexer(lexer, lexer_image);
		free_(next.name.start);
		AST_EXPR children[] = { name_expr};
		return createExpr(t, children, 1);
	}
}

AST_EXPR parse_Enum(Lexer *lexer, bool *eof, Token t)
{

	Lexer lexer_image = *lexer;

	Token name = preprocess(lexer, eof, &identifiers);
	AST_EXPR name_expr;
	bool annon = match(name, "{");
	if (annon)
	{
		rollbackLexer(lexer, lexer_image);
		free_(name.name.start);
		name_expr = createExpr(createToken(u"annon"), 0, 0);
	}
	else
	{
		name_expr = createExpr(name, 0, 0);
	}

	lexer_image = *lexer;
	Token next = preprocess(lexer, eof, &identifiers);
	if (match(next, "{"))
	{
		free_(next.name.start);
		DynamicArray_AST_EXPR exprs = constructDynamicArray_AST_EXPR(100, "enum parsing");
		
		for (;;)
		{
			int exit;
			AST_EXPR enum_item = getNextExpr(lexer, eof, u",};", &exit);
			if (*eof || exit == 1)
				break;
			else if (exit == 2)
			{
				reportError(lexer, "enum items are separarated with commas not semicolons");
				Add(&exprs, enum_item);
			}
			else if(enum_item.token.name.start)
				Add(&exprs, enum_item);
		}
		AST_EXPR items = createExpr(createToken(u"items"), exprs.start, exprs.length);
		free_(exprs.start);
		AST_EXPR children[] = { name_expr,items };
		return createExpr(t, children, 2);
	}
	else
	{
		rollbackLexer(lexer, lexer_image);
		free_(next.name.start);
		AST_EXPR children[] = { name_expr };
		return createExpr(t, children, 1);
	}
}



keyword keywords[] =
{
	{ u"if",hash("if"),false,0,parse_If},
	{ u"else",hash("else"),false,0,parse_Else },
	{ u"do", hash("do"), false, 0,parse_Do},
	{ u"while", hash("while"),false,0, parse_While},
	{ u"for", hash("for"), false,0,parse_For},
//	{ u"struct", hash("struct"), false,0,parse_Struct},
//	{ u"union", hash("union"), false, 0,parse_Struct },
//	{ u"enum", hash("enum"), false,0,parse_Struct },
	{ u"case", hash("case"), false, 0,parse_Case },
	{ u"default", hash("default"), 0,false, parse_Default },
	{ u"switch", hash("switch"), 0,false, parse_Switch},


//not sure you're allowed to do this, I mean it is a union so you know..
//msvc doesn't complain. We'll se when we've ported this mess ;)

	{ u"break", hash("break"), true,0},
	{ u"continue", hash("continue"), true,0},
	{ u"return", hash("return"), true,1},

	{ u"static", hash("static"), true, 1 },
	{ u"extern", hash("extern"), true, 1 },
	{ u"sizeof", hash("sizeof"), true, 1 },
	{ u"register", hash("register"), true, 1 },
};

DEFINE_DynamicArray(OpInfo);
DEFINE_Stack(OpInfo);

void stackToAst(DynamicArray_AST_EXPR *stack, DynamicArray_OpInfo *ops, Operator currentOp)
{
	for (;;)
	{
		if (ops->length <= 0)
		{
			return;
		}

		OpInfo opInfo = Pop(ops);
		Operator op = opInfo.op;

		if (currentOp.presidence < op.presidence || (currentOp.presidence == op.presidence && currentOp.assoc == assoc_right_to_left))
		{//a poor mans conditional pop
			Push(ops, opInfo);
			return;
		}

		if (op.opType == pre_unary || op.opType == post_unary)
		{
			AST_EXPR child = {};

			if (stack->length > 0)
				 child = Pop(stack);

			AST_EXPR children[] = { child };
			Push(stack, createExpr(opInfo.token, children, 1));
		}
		else if (op.opType == binary)
		{
			AST_EXPR a = {};
			AST_EXPR b = {};

			if (stack->length > 0)	
				a = Pop(stack); 
			
			if (stack->length > 0) //if this fails. there is no way to know the correct order, allways goes to the right
				b = Pop(stack);
		
			AST_EXPR children[] = { b, a };
		
			Push(stack, createExpr(opInfo.token, children, 2));
		}
		else
		{
			//handle me! (ternary)
		}
	}
}


int findKeywordIndex(Token t)
{
	for (int i = 0; i < ARRAY_LENGTH(keywords);i++)
	{
		if (t.hash == keywords[i].hash)
		{
			for (int j = 0; j < t.name.length; j++)
			{
				if (t.name.start[j] != keywords[i].name[j])
				{
					return -1;
				}
			}
			return i;
		}
	}
	return -1;
}




//alloca is in malloc.h -.-
AST_EXPR getNextExpr(Lexer *lexer, bool *eof, const char16_t *exprDelimiter, int *exitDelimiter)
{
	DynamicArray_OpInfo op_stack = constructDynamicArray_OpInfo(10, "operator stack");
	DynamicArray_AST_EXPR ret_stack = constructDynamicArray_AST_EXPR(10, "token stack");
	AST_EXPR *placeForNext = 0;
	bool lastWasIdentifier = false;
	for (;;)
	{
		AST_EXPR _expr = markTypes_complex(lexer, eof);
		if (_expr.token.type.typeType != unknown)
		{//assumed to some kind of a type
			AST_EXPR ignore = {};
			AST_EXPR children[] = { _expr, ignore };
			Token t = createToken(u"var decl");
			t.location = _expr.token.location;
			//t.type = _expr.token.type;   since every expression is freeing their pointer types, this needs to be cloned if should assign it here aswell.
			AST_EXPR expression = createExpr(t, children, 2);
			placeForNext = &expression.children[1];
			Push(&ret_stack, expression);
			continue;
		}
		Token t = _expr.token;
	
		if (*eof)
		{
			free_(t.name.start);
			break;
		}

		//int opIndex = matchOp(t.hash, ret_stack.length == 0 ? 0 : 1);
		int opIndex = matchOp(t.hash, lastWasIdentifier ? 1 : 0);
		if (opIndex == -1)
		{
			opIndex = matchOp(t.hash, lastWasIdentifier == 0 ? 1 : 0);
		}
		lastWasIdentifier = false;

		if (opIndex != -1)
		{
			//free_(t.name.start);
			Operator op = ops[opIndex];
			Operator prev = Peek(&op_stack).op;
			
			stackToAst(&ret_stack, &op_stack,op);
			for (int i = 0; i < strlen((char *)exprDelimiter); i++)
			{
				if (t.name.length == 1 && t.name.start[0]==exprDelimiter[i])
				{
					free_(t.name.start);
					if (exitDelimiter)
						*exitDelimiter = i;
					goto function_exit;
				}
			}
			

			if (op.opType == nonary)
			{
				Push(&ret_stack,createExpr(t, 0, 0));
				break;
			}

			if (op.hash == '(')
			{
				free_(t.name.start);
				if (op.opType == pre_unary)
				{
					Push(&ret_stack, getNextExpr(lexer, eof,u")"));
				}
				else
				{
					Push(&ret_stack,parse_Function(lexer,eof,Pop(&ret_stack)));
					break;
				}
			}
			else if (op.hash == '{')
			{
				free_(t.name.start);
				DynamicArray_AST_EXPR children = constructDynamicArray_AST_EXPR(10, "block TempStack");
				for (;;)
				{
					AST_EXPR child = getNextExpr(lexer, eof, u";");
					if (child.token.name.start[0] == '}'||*eof)
					{
						free_(child.token.name.start);
						break;
					}
					Push(&children, child);
				}
				t.name.start = strcpy((char16_t *)u"{}");
				Push(&ret_stack,createExpr(t,children.start, children.length));
				free_(children.start);
				break;
			}
			else
			{
				OpInfo opInfo = {};
				opInfo.token= t;
				opInfo.op = op;
				Push(&op_stack, opInfo);
			}
		}
		else
		{
			int keywordIndex = findKeywordIndex(t);
			if (keywordIndex!=-1)
			{
				keyword keyword = keywords[keywordIndex];
				if (keyword.simpleType)
				{
					if (keyword.children == 0)
					{
						AST_EXPR expr = {};
						expr.token = t;
						Push(&ret_stack, expr);
					}
					else
					{
						AST_EXPR child = {};
						AST_EXPR children[] = {child};
						AST_EXPR expr = createExpr(t, children, 1);
						placeForNext = &expr.children[0];
						Push(&ret_stack, expr);
						continue;
					}
				}
				else
				{
					AST_EXPR expr = keyword.getExpr(lexer, eof,t);
					Push(&ret_stack, expr);
					break;
				}
			}
			else
			{
				lastWasIdentifier = true;
				AST_EXPR id = {};
				id.token = t;
				Push(&ret_stack, id);
			}
		}
		if (placeForNext)
		{
			assert(ret_stack.length> 0);
			*placeForNext = Pop(&ret_stack);
			placeForNext = 0;
		}
	}

	
	// make it work then make it pretty
	// a goto here works. I'll clean it up later
	// @cleanup

function_exit:

	Operator op = {};
	op.presidence = 100;
	
	//stackToAst(&ret_stack, &op_stack,op);
	if (ret_stack.length > 1)
	{
		reportError(lexer, "missing semicolon ???");
	}
	stackToAst(&ret_stack, &op_stack,op);
	
	for (int i = 1; i < ret_stack.length; i++)
	{
		freeExpr(ret_stack.start[i]);
	}
	for (int i = 0; i < op_stack.length; i++)
	{
		free_(op_stack.start[i].token.name.start);
	}
	AST_EXPR ret = ret_stack.start[0];
	free_(op_stack.start);
	free_(ret_stack.start);

	if (ret_stack.length == 0)
	{
		AST_EXPR expr = {};
		return expr;
	}
	else
	{
		
		return ret;
	}
}
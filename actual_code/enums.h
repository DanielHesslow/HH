#define CONCAT1(x, y) x##y
#define CONCAT2(x, y) CONCAT1(x, y)

#define CONCAT1_(x, y) x##_##y
#define CONCAT2_(x, y) CONCAT1_(x, y)




#ifndef DHEN_NAME
void blaha()
{
	DHEN_NAME__MUST_BE_DEFINED = 2;
}
#elif defined DHEN_VALUES
#if 1
	#ifdef DHEN_PREFIX
		#define DHEN_VALUE(name) CONCAT2_(DHEN_PREFIX,name)
	#else
		#define DHEN_VALUE(name) name
	#endif

#ifdef DHEN_PAIR 
	#define X(a,w) DHEN_VALUE(a)=w,
#else
	#define X(a) DHEN_VALUE(a),
#endif
	enum DHEN_NAME
		{
			DHEN_VALUES
		};
#undef X


#ifdef DHEN_PAIR
	#define X(a, w) case DHEN_VALUE(a):  return #a;
#else
	#define X(a) case DHEN_VALUE(a):  return #a;
#endif
		char *(CONCAT2(stringFrom, DHEN_NAME))(DHEN_NAME enum_)
		{
			switch (enum_)
			{
				DHEN_VALUES
			}
			assert(false && "invalid enum");
			return 0;
		}
	#undef X
#ifdef DHEN_PAIR
#define X(a,w) DHEN_VALUE(a),
#else
#define X(a) DHEN_VALUE(a),
#endif
	DHEN_NAME CONCAT2_(DHEN_NAME, values)[] = { DHEN_VALUES };
#undef X

#ifdef DHEN_PAIR
#define X(a,w) #a,
#else
#define X(a) #a,
#endif
	char *CONCAT2_(DHEN_NAME, strings)[] = { DHEN_VALUES };
#undef X

	#undef DHEN_VALUE
		
#endif
#else
void blaha()
{
	DHEN_VALUES__MUST_BE_DEFINED = 2;
}
#endif




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




/*
*/

/*
#define MAP_X_PREFIX_0(prefix)         
#define MAP_X_PREFIX_1(prefix,a) X(prefix,a)
#define MAP_X_PREFIX_2(prefix,a,b) X(prefix,a) X(prefix,b)
#define MAP_X_PREFIX_3(prefix,a,b,c) X(prefix,a) X(prefix,b) X(prefix,c)
#define MAP_X_PREFIX_4(prefix,a,b,c,d) X(prefix,a) X(prefix,b) X(prefix,c) X(prefix,d)
#define MAP_X_PREFIX_5(prefix,a,b,c,d,e) X(prefix,a) X(prefix,b) X(prefix,c) X(prefix,d) X(prefix,e)
#define MAP_X_PREFIX_6(prefix,a,b,c,d,e,f) X(prefix,a) X(prefix,b) X(prefix,c) X(prefix,d) X(prefix,e) X(prefix,f)
#define MAP_X_PREFIX_7(prefix,a,b,c,d,e,f,g) X(prefix,a) X(prefix,b) X(prefix,c) X(prefix,d) X(prefix,e) X(prefix,f) X(prefix,g)
#define MAP_X_PREFIX_8(prefix,a,b,c,d,e,f,g,h) X(prefix,a) X(prefix,b) X(prefix,c) X(prefix,d) X(prefix,e) X(prefix,f) X(prefix,g) X(prefix,h)
#define MAP_X_PREFIX_9(prefix,a,b,c,d,e,f,g,h,i) X(prefix,a) X(prefix,b) X(prefix,c) X(prefix,d) X(prefix,e) X(prefix,f) X(prefix,g) X(prefix,h) X(prefix,i)
#define MAP_X_PREFIX_10(prefix,a,b,c,d,e,f,g,h,i,j) X(prefix,a) X(prefix,b) X(prefix,c) X(prefix,d) X(prefix,e) X(prefix,f) X(prefix,g) X(prefix,h) X(prefix,i) X(prefix,j)
#define MAP_X_PREFIX_11(prefix,a,b,c,d,e,f,g,h,i,j,k) X(prefix,a) X(prefix,b) X(prefix,c) X(prefix,d) X(prefix,e) X(prefix,f) X(prefix,g) X(prefix,h) X(prefix,i) X(prefix,j) X(prefix,k)
#define MAP_X_PREFIX_12(prefix,a,b,c,d,e,f,g,h,i,j,k,l) X(prefix,a) X(prefix,b) X(prefix,c) X(prefix,d) X(prefix,e) X(prefix,f) X(prefix,g) X(prefix,h) X(prefix,i) X(prefix,j) X(prefix,k) X(prefix,l)
#define MAP_X_PREFIX_13(prefix,a,b,c,d,e,f,g,h,i,j,k,l,m) X(prefix,a) X(prefix,b) X(prefix,c) X(prefix,d) X(prefix,e) X(prefix,f) X(prefix,g) X(prefix,h) X(prefix,i) X(prefix,j) X(prefix,k) X(prefix,l) X(prefix,m)
#define MAP_X_PREFIX_14(prefix,a,b,c,d,e,f,g,h,i,j,k,l,m,n) X(prefix,a) X(prefix,b) X(prefix,c) X(prefix,d) X(prefix,e) X(prefix,f) X(prefix,g) X(prefix,h) X(prefix,i) X(prefix,j) X(prefix,k) X(prefix,l) X(prefix,m) X(prefix,n)
#define MAP_X_PREFIX_15(prefix,a,b,c,d,e,f,g,h,i,j,k,l,m,n,o) X(prefix,a) X(prefix,b) X(prefix,c) X(prefix,d) X(prefix,e) X(prefix,f) X(prefix,g) X(prefix,h) X(prefix,i) X(prefix,j) X(prefix,k) X(prefix,l) X(prefix,m) X(prefix,n) X(prefix,o)
#define MAP_X_PREFIX_16(prefix,a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p) X(prefix,a) X(prefix,b) X(prefix,c) X(prefix,d) X(prefix,e) X(prefix,f) X(prefix,g) X(prefix,h) X(prefix,i) X(prefix,j) X(prefix,k) X(prefix,l) X(prefix,m) X(prefix,n) X(prefix,o) X(prefix,p)
#define MAP_X_PREFIX_17(prefix,a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q) X(prefix,a) X(prefix,b) X(prefix,c) X(prefix,d) X(prefix,e) X(prefix,f) X(prefix,g) X(prefix,h) X(prefix,i) X(prefix,j) X(prefix,k) X(prefix,l) X(prefix,m) X(prefix,n) X(prefix,o) X(prefix,p) X(prefix,q)
#define MAP_X_PREFIX_18(prefix,a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r) X(prefix,a) X(prefix,b) X(prefix,c) X(prefix,d) X(prefix,e) X(prefix,f) X(prefix,g) X(prefix,h) X(prefix,i) X(prefix,j) X(prefix,k) X(prefix,l) X(prefix,m) X(prefix,n) X(prefix,o) X(prefix,p) X(prefix,q) X(prefix,r)
#define MAP_X_PREFIX_19(prefix,a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s) X(prefix,a) X(prefix,b) X(prefix,c) X(prefix,d) X(prefix,e) X(prefix,f) X(prefix,g) X(prefix,h) X(prefix,i) X(prefix,j) X(prefix,k) X(prefix,l) X(prefix,m) X(prefix,n) X(prefix,o) X(prefix,p) X(prefix,q) X(prefix,r) X(prefix,s)
#define MAP_X_PREFIX_20(prefix,a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t) X(prefix,a) X(prefix,b) X(prefix,c) X(prefix,d) X(prefix,e) X(prefix,f) X(prefix,g) X(prefix,h) X(prefix,i) X(prefix,j) X(prefix,k) X(prefix,l) X(prefix,m) X(prefix,n) X(prefix,o) X(prefix,p) X(prefix,q) X(prefix,r) X(prefix,s) X(prefix,t)
#define MAP_X_PREFIX_21(prefix,a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t,u) X(prefix,a) X(prefix,b) X(prefix,c) X(prefix,d) X(prefix,e) X(prefix,f) X(prefix,g) X(prefix,h) X(prefix,i) X(prefix,j) X(prefix,k) X(prefix,l) X(prefix,m) X(prefix,n) X(prefix,o) X(prefix,p) X(prefix,q) X(prefix,r) X(prefix,s) X(prefix,t) X(prefix,u)
#define MAP_X_PREFIX_22(prefix,a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t,u,v) X(prefix,a) X(prefix,b) X(prefix,c) X(prefix,d) X(prefix,e) X(prefix,f) X(prefix,g) X(prefix,h) X(prefix,i) X(prefix,j) X(prefix,k) X(prefix,l) X(prefix,m) X(prefix,n) X(prefix,o) X(prefix,p) X(prefix,q) X(prefix,r) X(prefix,s) X(prefix,t) X(prefix,u) X(prefix,v)
#define MAP_X_PREFIX_23(prefix,a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t,u,v,w) X(prefix,a) X(prefix,b) X(prefix,c) X(prefix,d) X(prefix,e) X(prefix,f) X(prefix,g) X(prefix,h) X(prefix,i) X(prefix,j) X(prefix,k) X(prefix,l) X(prefix,m) X(prefix,n) X(prefix,o) X(prefix,p) X(prefix,q) X(prefix,r) X(prefix,s) X(prefix,t) X(prefix,u) X(prefix,v) X(prefix,w)
#define MAP_X_PREFIX_24(prefix,a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p,q,r,s,t,u,v,w,x) X(prefix,a) X(prefix,b) X(prefix,c) X(prefix,d) X(prefix,e) X(prefix,f) X(prefix,g) X(prefix,h) X(prefix,i) X(prefix,j) X(prefix,k) X(prefix,l) X(prefix,m) X(prefix,n) X(prefix,o) X(prefix,p) X(prefix,q) X(prefix,r) X(prefix,s) X(prefix,t) X(prefix,u) X(prefix,v) X(prefix,w) X(prefix,x)
#define MAKE_ENUM_BASE(name,prefix,...)enum name{ DHMA_INTERNAL_EXPAND(CONCAT2(MAP_X_PREFIX_, DHMA_NUMBER_OF_ARGS(__VA_ARGS__)) (prefix,__VA_ARGS__))};

#define ENUM_VALUE_TOKEN(val) val,
#define MAKE_ENUM(name, prefix, ...)



enum_test:
	MAKE_ENUM_BASE(enum_test, enum_test_prefix, a, b, c, d, e, f);


	*/


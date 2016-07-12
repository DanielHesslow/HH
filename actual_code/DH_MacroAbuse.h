#ifndef DH_MACRO_ABUSE
#define DH_MACRO_ABUSE

#define ARRAY_LENGTH(array) ((sizeof((array)))/(sizeof((array)[0])))

#define DHMA_SWAP(type, a, b)do{type tmp = a; a = b; b = tmp;}while(0)


#define DHMA_STRING(s) #s
#define DHMA_STRING_(value) DHMA_STRING(value)
#define DHMA_LOCATION (DHMA_STRING_(__LINE__) ", "__FILE__)
#define DHMA_LOCATION_ (__LINE__,  __FILE__)

#define ALLOCATE(bytes_to_alloc)Allocate(macro_used_allocator,bytes_to_alloc,DHMA_LOCATION)



// --- DEBUG STUFF---

#undef assert
#ifdef DEBUG 
#define assert(cond) do { if (!(cond)) {MessageBox(NULL, #cond,  "assert failed", MB_OK | MB_ICONEXCLAMATION);  __debugbreak();} } while(0)
#else
// @test me tomorrow
#define assert(cond) do { if (!(cond)) {MessageBox(NULL, DHMA_STRING(cond) "\n" DHMA_STRING_(__LINE__) ", " __FILE__ ,  "assert failed"  , MB_OK | MB_ICONEXCLAMATION); ExitProcess(-1);} } while(0)
#endif

char OutputDebugString_buffer[1000];
#define dpr(integer)\
sprintf(OutputDebugString_buffer, DHMA_STRING(integer) ": %d (" DHMA_STRING_(__LINE__) ", " __FILE__ ")\n", integer);\
OutputDebugString(OutputDebugString_buffer);

#define dprs(s)\
sprintf(OutputDebugString_buffer, ":\"%s\" (" DHMA_STRING_(__LINE__) ", " __FILE__ ")\n", s);\
OutputDebugString(OutputDebugString_buffer);





//--- NUMBER_OF_ARGS()
#ifdef _MSC_VER // Microsoft compilers
#define DHMA_NUMBER_OF_ARGS(...)  (DHMA_INTERNAL_EXPAND_ARGS_PRIVATE(DHMA_REMOVE_FIRST_ARG(__VA_ARGS__)))
#define DHMA_REMOVE_FIRST_ARG(...) unused, __VA_ARGS__
#define DHMA_INTERNAL_EXPAND(x) x
#define DHMA_INTERNAL_EXPAND_ARGS_PRIVATE(...) DHMA_INTERNAL_EXPAND(DHMA_INTERNAL_GET_ARG_COUNT_PRIVATE(__VA_ARGS__, 69, 68, 67, 66, 65, 64, 63, 62, 61, 60, 59, 58, 57, 56, 55, 54, 53, 52, 51, 50, 49, 48, 47, 46, 45, 44, 43, 42, 41, 40, 39, 38, 37, 36, 35, 34, 33, 32, 31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0))
#define DHMA_INTERNAL_GET_ARG_COUNT_PRIVATE(_1_, _2_, _3_, _4_, _5_, _6_, _7_, _8_, _9_, _10_, _11_, _12_, _13_, _14_, _15_, _16_, _17_, _18_, _19_, _20_, _21_, _22_, _23_, _24_, _25_, _26_, _27_, _28_, _29_, _30_, _31_, _32_, _33_, _34_, _35_, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _70, count, ...) count

#else // Non-Microsoft compilers
#   define DHMA_NUMBER_OF_ARGS(...) DHMA_INTERNAL_GET_ARG_COUNT_PRIVATE(0, ## __VA_ARGS__, 70, 69, 68, 67, 66, 65, 64, 63, 62, 61, 60, 59, 58, 57, 56, 55, 54, 53, 52, 51, 50, 49, 48, 47, 46, 45, 44, 43, 42, 41, 40, 39, 38, 37, 36, 35, 34, 33, 32, 31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19, 18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0)
#   define DHMA_INTERNAL_GET_ARG_COUNT_PRIVATE(_0, _1_, _2_, _3_, _4_, _5_, _6_, _7_, _8_, _9_, _10_, _11_, _12_, _13_, _14_, _15_, _16_, _17_, _18_, _19_, _20_, _21_, _22_, _23_, _24_, _25_, _26_, _27_, _28_, _29_, _30_, _31_, _32_, _33_, _34_, _35_, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50, _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, _61, _62, _63, _64, _65, _66, _67, _68, _69, _70, count, ...) count
#endif



#endif
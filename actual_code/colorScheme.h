#if(!defined ColorScheme)
#define ColorScheme
#if 1
//dark scheme
uint32 backgroundColors[] = { 0x00272727, 0x00232323, 0x00191919 };

//int background = 0xff262721; (sublime color isch)
int highlightColor = 0x00333333;
//int caretColor = 0xff999933;
//int caretColorBright = 0xff999933;
int caretColorDark  = 0xffcccc66;

int foregroundColor = 0xffaaaaaa;

int keywordColor = 0xff7777ff;
int commentColor = 0xff77aa77;
int pointerColor = 0xffdddd77; //same as multiply ... :(

int arrowColor   = pointerColor;
int funcColor    = 0xffcc4444;
int typeColor    = keywordColor;//0xff666666;
int literalColor = 0xffaaaa00;
int structColor = 0xff33cc99;
int activeColor = 0xff559933;

int caretColor     = 0xff00ffff;
#else
//light theme
uint32 backgroundColors[] = { 0x00f5f5f5, 0x00ffffff, 0x00fafafa };

int caretColorDark = 0xffcccc66;

int literalColor = 0xffaaaa00;
int structColor = 0xff33cc99;
int activeColor = 0xffffbb66;

int highlightColor = 0x00111111;
int caretColor     = 0xff666600;

int foregroundColor = 0xff000000;
int keyWordColor = 0xff000044;
int commentColor = 0xff004400;
int pointerColor = 0xffd555500;
int arrowColor   = pointerColor;
int funcColor    = 0xffcc1111;
int typeColor    = keyWordColor;

#endif
#endif 
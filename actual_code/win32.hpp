#ifndef WIN32H
#define WIN32H
float QueryFrameUsed();
#include "texteditor.hpp"

struct win32WindowDimensions
{
	int width;
	int height;	
};

struct win32OffScreenBuffer{
	BITMAPINFO 	info;
	Bitmap		bitmap;
};

#endif
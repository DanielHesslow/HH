#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cstdio>
#include "Allocation.h"

//technically KiB, MiB, Gib but I don't fucking care
#define KB(value)(1024*value)
#define MB(value)(1024*KB(value))
#define GB(value)(1024*MB(value))

#define STBTT_STATIC
#define STB_TRUETYPE_IMPLEMENTATION 
#define STBTT_malloc(x,u) alloc_(x,u)
#define STBTT_free(x,u) free_(x)
#define STBTT_strlen(x) strlen(x);

#include "..\\libs\\stb_truetype.h" 
#define STB_IMAGE_IMPLEMENTATION 
#include "..\\libs\\stb_image.h" 

#include "win32.hpp"

global_variable bool running;
global_variable win32OffScreenBuffer bitmap;

//deeeeeebuuuuuggg

global_variable char *addresses[]{(char *) 0x500000, (char *)0x600000, (char *)0x700000, (char *)0x800000, (char *)0x900000 };
global_variable int ind = 0;

//#define fixed_mem
void *platform_alloc(size_t bytes_to_alloc)
{
	assert(bytes_to_alloc >= KB(4));
#ifdef fixed_mem
	return VirtualAlloc(addresses[ind++], bytes_to_alloc, MEM_RESERVE |MEM_COMMIT, PAGE_READWRITE);
#else 
	return VirtualAlloc(NULL, bytes_to_alloc, MEM_COMMIT, PAGE_READWRITE);
#endif
}

void platform_free(void *mem)
{
	VirtualFree(mem, 0, MEM_RELEASE);
}

win32WindowDimensions getWindowDimensions(HWND window)
{
	win32WindowDimensions ret;
	RECT clientRect;
	GetClientRect(window, &clientRect);
	ret.width = clientRect.right-clientRect.left;
	ret.height = clientRect.bottom-clientRect.top;
	return(ret);
}

internal void win32ResizeDIBSection(win32OffScreenBuffer *bitmap, int width, int height)
{
	//having this as a separate allocation is fine for now
	//It is not called very often anyway...
	if(bitmap->bitmap.memory)
	{
		VirtualFree(bitmap->bitmap.memory, 0, MEM_RELEASE);
	}

	bitmap->bitmap.width = width;
	bitmap->bitmap.height = height;

	bitmap->info.bmiHeader.biSize = sizeof(bitmap->info.bmiHeader);
	bitmap->info.bmiHeader.biWidth = width;
	bitmap->info.bmiHeader.biHeight = -height;
	bitmap->info.bmiHeader.biPlanes = 1;
	bitmap->info.bmiHeader.biBitCount = 32;
	bitmap->info.bmiHeader.biCompression = BI_RGB;

	bitmap->bitmap.bytesPerPixel = 4;
	bitmap->bitmap.stride = bitmap->bitmap.width*bitmap->bitmap.bytesPerPixel;
	int bitmapMemorySize = bitmap->bitmap.width*bitmap->bitmap.height*bitmap->bitmap.bytesPerPixel;
	bitmap->bitmap.memory= VirtualAlloc(0, bitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);
}

internal void win32UpdateWindow(HDC deviceContext, int windowWidth, int windowHeight, win32OffScreenBuffer bitmap)
{
	// so this is fucking slow. I'd rather it wasn't but idk. Don't know how BitBlt works and if it would work?
	SetDIBitsToDevice(deviceContext,
		0, 0, windowWidth, windowHeight,
		0, 0, 0, bitmap.bitmap.height,
		bitmap.bitmap.memory,
		&bitmap.info,
		DIB_RGB_COLORS);
}




global_variable bool redraw = true;
global_variable bool redrawAll = true;

TextBuffer textBuffer = {};

LRESULT CALLBACK win32MainWindowCallback(HWND window,
										UINT message,
										WPARAM wParam,
										LPARAM lParam)
{
	LRESULT result = 0;
	switch (message)
	{	
		case WM_KEYDOWN:
		case WM_CHAR:
			
			//we've don't disptach them here. I hope windows won't go over our head... You never know..
			//oh windows sure do sometimes lol...
			//I'll have to handle it..
			
			//ASSERT(false&&"windows is messing with us");	
		break;
	
		case WM_SIZE:
		{
			win32WindowDimensions dims = getWindowDimensions(window);
			win32ResizeDIBSection(&bitmap, dims.width, dims.height);
			
		} break; 
		case WM_DESTROY:
		{
			running =false;
		} break;
		case WM_CLOSE:
		{
			running = false;
		} break;
		case WM_ACTIVATEAPP:
		{
		} break;
		case WM_PAINT:
		{
#if 1
			redraw = true;
			redrawAll = true;
#else
			textBuffer.bitmap = bitmap.bitmap;
			Update(&textBuffer, 5, 0, {}, false);
			
			PAINTSTRUCT paint;
			HDC deviceContext = BeginPaint(window,&paint);
			win32WindowDimensions windowDims = getWindowDimensions(window);
		
			win32UpdateWindow(deviceContext,windowDims.width,windowDims.height,bitmap);
			EndPaint(window, &paint);
#endif
		}
		default:
		{
			result = DefWindowProc(window,message,wParam,lParam);
		} break;
	}
	return result;	
}

internal void setMods(Input *input)
{
	input->shift   = GetKeyState(VK_SHIFT) & 1 << 7;
	input->control = GetKeyState(VK_CONTROL) & 1 << 7;
	input->caps    = GetKeyState(VK_CAPITAL) & 1; //yes it should not be shifted, this is a toggle key... Also VK_CAPITAL..REALLY?
	input->alt	   = GetKeyState(VK_MENU) & 1<<7; //ok menu... everyone's fired.

}

global_variable unsigned int CUSTOM_CLIPBOARD_ID = RegisterClipboardFormat("DH_TE_ClipboardFormat");

internal void win32_StoreInClipboard(InternalClipboard *clipboard)
{
	//@pollish_me (check errors on global alloc and global locks and display them.)
	// this is not secure but we really don't care about security.
	// worst case sombody can maybe crash us by spoofing to be us and then not null terminating.
	// I just don't care about that.
	
	HGLOBAL customFormatMem;
	{ //custom_format

		DynamicArray_char buffer = DHDS_constructDA(char, 200,default_allocator);
		serialize(&buffer, *clipboard);

		//this only workis if GMEM_FIXED is set. why? (GPTR = GMEM_FIXED | GMEM_ZEROINIT)
		customFormatMem = GlobalAlloc(GPTR, buffer.length);
		GlobalLock(customFormatMem);
			memcpy(customFormatMem, buffer.start, buffer.length);
		GlobalUnlock(customFormatMem);
		free_(buffer.start);
		// dude we're not freeing... 
	}

	HGLOBAL unicodeFormatMem;
	{ //utf_16
		char16_t *utf_16_string = getAsUnicode(clipboard);
		size_t size = DHSTR_strlen(utf_16_string)*sizeof(char);
		unicodeFormatMem = GlobalAlloc(GPTR, size);
		GlobalLock(unicodeFormatMem);
		{
			memcpy(unicodeFormatMem, utf_16_string, size);
		}GlobalUnlock(unicodeFormatMem);
		free_(utf_16_string);
	}
		
	HGLOBAL asciiFormatMem;
	{	//ascii
		char *ascii_string = getAsAscii(clipboard);
		size_t size = strlen(ascii_string)*sizeof(char);
		asciiFormatMem = GlobalAlloc(GPTR, size);
		GlobalLock(asciiFormatMem);
		{
			memcpy(asciiFormatMem, ascii_string, size);
		}GlobalUnlock(asciiFormatMem);
		free_(ascii_string);
	}
	
	// appearnlty calling open clipboard with a null handle does not get the current window
	// but instead causes it to fail when empty clipboard is called
	// somehow the code is working with it set to null.
	// that seams to be pure luck though. msdn explicitly tells us it should fail.
	
	if(OpenClipboard(GetActiveWindow()))
	{
		if (EmptyClipboard())
		{
			SetClipboardData(CUSTOM_CLIPBOARD_ID, customFormatMem);
			//SetClipboardData(CF_UNICODETEXT, unicodeFormatMem); //has problems with zero len lines
			SetClipboardData(CF_TEXT, asciiFormatMem);
		}
		else
		{
			assert(false && "could not empty the clipboard");
		}
		CloseClipboard();
	}
	else
	{
		assert(false && "could not open the clipboard");
	}
}

internal InternalClipboard win32_LoadFromClipboard()
{
	InternalClipboard ret = {};
	if (IsClipboardFormatAvailable(CUSTOM_CLIPBOARD_ID)) 
	{
		OpenClipboard(0);
		{
			char *p = (char *) GetClipboardData(CUSTOM_CLIPBOARD_ID);
			GlobalLock(p);
			{
				ret = deSerialize_InternalClipboard(&p);
			} GlobalUnlock(p);
		} CloseClipboard();
	}
	else if (IsClipboardFormatAvailable(CF_UNICODETEXT))
	{
		ret.clips = DHDS_constructDA(ClipboardItem, 3,default_allocator);
		DynamicArray_char16_t tmp = DHDS_constructDA(char16_t, 100,default_allocator);
		OpenClipboard(0);
		{
			char *p = (char *)GetClipboardData(CF_UNICODETEXT);
			GlobalLock(p);
			{
				while (*p)
				{
					//if (*p == '\r' && *(p + 1) == '\n')break;
					Add(&tmp, *p++);
				}
				Add(&tmp, 0);//null terminating
			} GlobalUnlock(p);
		} CloseClipboard();
		ClipboardItem item = {};
		item.onSeperateLine = false;
		item.string = tmp.start;
		Add(&ret.clips, item);
	}

	else if (IsClipboardFormatAvailable(CF_TEXT))
	{
		ret.clips = DHDS_constructDA(ClipboardItem, 10,default_allocator);
		DynamicArray_char16_t tmp = DHDS_constructDA(char16_t, 100, default_allocator);
		OpenClipboard(0);
		{
			char *p = (char *)GetClipboardData(CF_TEXT);
			GlobalLock(p);
			{
				while (*p)
				{
					//if (*p == '\r' && *(p + 1) == '\n')break;
					Add(&tmp, *p++);
				}
				Add(&tmp, 0); //null terminating
			} GlobalUnlock(p);
		} CloseClipboard();
		ClipboardItem item = {};
		item.onSeperateLine = false;
		item.string = tmp.start;
		Add(&ret.clips, item);
	}

	return ret;
}

static void GetOutlineMetrics(HDC hdc)
{
	uint32 cbBuffer = GetOutlineTextMetrics(hdc, 0, 0);
	if (cbBuffer == 0)
		return;
	OUTLINETEXTMETRICA *buffer =(OUTLINETEXTMETRICA *) GlobalAlloc(GPTR,(int)cbBuffer);
	if (GetOutlineTextMetrics(hdc, cbBuffer, buffer) != 0)
	{
		int i = 0;
	}
	GlobalFree(buffer);
}


void loadFonts() 
{
	static const LPCSTR fontRegistryPath = "Software\\Microsoft\\Windows NT\\CurrentVersion\\Fonts";
	HKEY hKey;
	LONG result;


	DH_Allocator macro_used_allocator = default_allocator;

	result = RegOpenKeyEx(HKEY_LOCAL_MACHINE, fontRegistryPath, 0, KEY_READ, &hKey);
	if (result != ERROR_SUCCESS) {
		return;
	}

	DWORD maxValueNameSize, maxValueDataSize;
	result = RegQueryInfoKey(hKey, 0, 0, 0, 0, 0, 0, 0, &maxValueNameSize, &maxValueDataSize, 0, 0);
	if (result != ERROR_SUCCESS) {
		return;
	}

	DWORD valueIndex = 0;
	char *valueName = (char *)alloca(maxValueNameSize* sizeof(char));
	char *valueData = (char *)alloca(maxValueDataSize* sizeof(char));
	DWORD valueNameSize, valueDataSize, valueType;
	char *wsFontFile = "";

	char winDir[MAX_PATH]; //the maxpath isn't really maxpath anymore but I'm fairly confident that the windows file is inside of maxdir though...
	GetWindowsDirectory(winDir, MAX_PATH);
	DHSTR_String system_fonts_file = DHSTR_MERGE(DHSTR_MAKE_STRING(winDir), DHSTR_MAKE_STRING("\\FONTS\\"),alloca);
	//load the font
	do {
		wsFontFile = "";
		valueDataSize = maxValueDataSize;
		valueNameSize = maxValueNameSize;

		result = RegEnumValue(hKey, valueIndex, valueName, &valueNameSize, 0, &valueType, (BYTE *)valueData, &valueDataSize);
		valueIndex++;
		bool success;
		char *extension= getLast(valueData, '.', &success);
		if (strcmp(".ttf", extension) && strcmp(".TTF", extension)){
			continue; //something other than ttf
		}
		char *paren = getLast(valueName, '(', &success);
		*paren = 0;
		trimEnd(valueName, "Regular ");
		trimEnd(valueName, "regular ");

		AvailableFont typeface = {};
		typeface.name = DHSTR_CPY(DHSTR_MAKE_STRING(valueName),ALLOCATE);
		
		if ((valueData[1] == ':') && false)
		{
			typeface.path = DHSTR_CPY(DHSTR_MAKE_STRING(valueData), ALLOCATE);
		} else
		{
			typeface.path = DHSTR_MERGE(system_fonts_file, DHSTR_MAKE_STRING(valueData),ALLOCATE);
		}

		Insert(&availableFonts, typeface, ordered_insert_dont_care);
	} while (result != ERROR_NO_MORE_ITEMS);
	RegCloseKey(hKey);

	//merge fonts into typefaces
	int base=0;
	int c=1;
	for (int i = 1; i < availableFonts.length; i++)
	{
		if (!DHSTR_eq(availableFonts.start[base].name, availableFonts.start[i].name,string_eq_length_dont_care)) //this is a bit wrong, it groups some stuff it shouldn't... (but so does the win32 api) ie. Arial Unicode MS is a style, whatever
		{
			++c;
		}
		else
		{
			AvailableTypeface typeface = {};
			typeface.member_len = c;
			typeface.members = (AvailableFont *)alloc_(sizeof(AvailableFont)* c,"typeface members");
			typeface.typefaceName = DHSTR_UTF8_FROM_STRING(availableFonts.start[base].name,ALLOCATE);
			int len = strlen(typeface.typefaceName);
			
			for (int j = 0; j < c; j++)
			{
				typeface.members[j] = availableFonts.start[base + j];
				DHSTR_String memberName = typeface.members[base + j].name;
				typeface.members[j].name = DHSTR_subString(memberName, 0, memberName.length-1);
			}
			typeface.members[0].name = DHSTR_MAKE_STRING("Regular");
			Add(&availableTypefaces, typeface);
			c = 1;
			base = i;
		}
	}

	return;
}





internal bool charDown(int character, Input *input)
{
	setMods(input);
	input->inputType = input_character;
	input->character = character;
	return true;
}


uint64_t frequency;

internal uint64_t get_MicroSeconds_Since(uint64_t previousTimeStamp, uint64_t *currentTime)
{	//frequency better be setup!
	LARGE_INTEGER timeStamp_li;
	QueryPerformanceCounter(&timeStamp_li);
	if(currentTime)
		*currentTime = timeStamp_li.QuadPart;
	return (timeStamp_li.QuadPart - previousTimeStamp) / (frequency / 1000000);
}




uint64_t previousTimeStamp;
uint64_t MicorSeconds_Since_StartUP; //should not overflow... but let's not rely on us.
uint64_t getMicros() { return MicorSeconds_Since_StartUP; }

int targetFrameWait = 16667;

float QueryFrameUsed()
{
	uint64_t timePassed = get_MicroSeconds_Since(previousTimeStamp, 0);
	return (float)timePassed / (float)targetFrameWait;
};

int CALLBACK
WinMain(HINSTANCE instance,
		HINSTANCE prevInstance,
		LPSTR args,
		int showCode)
{
	test();
	if (GetCaretBlinkTime() == INFINITE)			
	{
		blinkTimePerS = 0;
	}
	else
	{
		blinkTimePerS = 1000 / GetCaretBlinkTime() ;
	}
	LARGE_INTEGER  li;
	QueryPerformanceFrequency(&li);
	frequency = li.QuadPart;
	QueryPerformanceCounter(&li);
	previousTimeStamp = li.QuadPart;

	uint64_t previousTimeStamp_2 = li.QuadPart;

	HANDLE iconSmall = LoadImage(NULL, "icon.ico", IMAGE_ICON, 0, 0, LR_LOADFROMFILE);
	
	WNDCLASS windowClass = {};
	windowClass.hIcon = (HICON)iconSmall;
	windowClass.style = CS_OWNDC|CS_HREDRAW|CS_VREDRAW;
	windowClass.lpfnWndProc = win32MainWindowCallback;
	windowClass.hInstance = instance;
	windowClass.lpszClassName = "TEWindowClass";
	
	RegisterClass(&windowClass);
#if 1
	HWND window = CreateWindowEx(
		0,	windowClass.lpszClassName,
		"+T", WS_OVERLAPPEDWINDOW|WS_VISIBLE,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
		0, 0, instance, 0);
#else
	HWND window = CreateWindowEx(
		0, windowClass.lpszClassName,
		"+T", WS_OVERLAPPEDWINDOW | WS_VISIBLE,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
		0, 0, instance, 0);
#endif

	win32WindowDimensions dims = getWindowDimensions(window);
	win32ResizeDIBSection(&bitmap,dims.width,dims.height);
	
	//textBuffer = openFileIntoNewBuffer();
	bool success;

	//textBuffer = openFileIntoNewBuffer(u"notes.txt", &success);
	//textBuffer.fileName = u"hello.c";
	DynamicArray_TextBuffer textBuffers = DHDS_constructDA(TextBuffer,2, default_allocator);
	
	loadFonts();

	//Add(&textBuffers, textBuffer);
	
	Data data = {};
	data.textBuffers = textBuffers;
	TextBuffer *buffer = (TextBuffer *) alloc_(sizeof(TextBuffer), "commandLineBuffer struct");
	*buffer = openCommanLine();
	data.commandLine = buffer;
	data.activeTextBufferIndex = 0;
	data.menu = DHDS_constructDA(MenuItem, 50,default_allocator);
	//Layout *split_a = CREATE_LAYOUT(layout_type_x, 1, CREATE_LAYOUT(layout_type_y,1,leaf,0.3,leaf,0.7,leaf), 0.7f, leaf);
	//Layout *split_b = CREATE_LAYOUT(layout_type_x, 1, leaf, 0.3f, leaf);

	data.layout = CREATE_LAYOUT(layout_type_y, 2, leaf, .5, leaf);
	uint16_t timeSinceStartUpInMs;
		
	//test_binsumtree_();
	if (window)
	{
		Input input = {};

		MSG message;
		running = true; 

		while(running)
		{
			//GetMessage(&message, 0, 0, 0);
			redrawAll = false;
			data.updateAllBuffers= false;
			
			while (PeekMessage(&message, 0, 0, 0, PM_REMOVE))
			{
				bool hasInput = false;		//if this is set outside of this loop we bug out 
											// so there is something wrong in here!
				TranslateMessage(&message);
				UINT msgType = message.message;


				char buffer[100];
				sprintf(buffer, "Message is %X\n", msgType);
				//OutputDebugString(buffer);

				if (msgType == WM_QUIT)
				{
					running = false;
				}
				else if (msgType == WM_KEYDOWN || msgType == WM_SYSKEYDOWN)
				{
					//hasInput = keyDown((uint8_t)message.wParam, &input);
					hasInput = true;
					input.VK_Code = message.wParam;
					input.inputType = input_key;
					setMods(&input);
					redraw = redraw || hasInput;
				}
				else if (msgType == WM_SYSCHAR)
				{
					// doshitall..what is this even. 
					// stops that annoying windows sound from being played at least
				}
				else if (msgType== WM_CHAR)
				{
					hasInput = charDown(message.wParam, &input);
					redraw = redraw || hasInput;
				}
				else if (msgType == WM_MOUSEWHEEL)
				{
					setMods(&input);
					int zDelta = GET_WHEEL_DELTA_WPARAM(message.wParam);
					hasInput = true;
					input.inputType = input_mouseWheel;
					
					input.mouseWheel = zDelta;
				}
				else
				{
					DispatchMessage(&message);
				}
				if(hasInput)
					updateInput(&data, &input,bitmap.bitmap, MicorSeconds_Since_StartUP);
			}
			
			if (redraw||true) //since we do smoothscrolling and stuff we need to improve the simple redraw thing.
			{
				//data.updateAllBuffers |= redrawAll;
				data.updateAllBuffers = true;
				//need to handle dt!
				renderFrame(&data, bitmap.bitmap, MicorSeconds_Since_StartUP);
				HDC deviceContext = GetDC(window);
				win32WindowDimensions windowDims = getWindowDimensions(window);
				win32UpdateWindow(deviceContext, windowDims.width, windowDims.height, bitmap);
				ReleaseDC(window, deviceContext);
				redraw = false;
			}

			//no need to update more frequently than the screen fps.
			uint64_t timeStamp;
			uint64_t timePassed = get_MicroSeconds_Since(previousTimeStamp,&timeStamp);
			MicorSeconds_Since_StartUP += timePassed;
			
			if (timePassed < targetFrameWait)
			{
				Sleep((targetFrameWait - timePassed)/1000); // erm system clock is 15,6ms. which is waaaay to much. all our frames may wait between 0 and 15,6 ms. Which is way to much. 
			}
			
			previousTimeStamp = timeStamp;
		}
    }
	return 0;
}

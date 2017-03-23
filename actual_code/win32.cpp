#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cstdio>


#define PAGES 800;

//technically KiB, MiB, Gib but I don't fucking care
#define KB(value)(1024*value)
#define MB(value)(1024*KB(value))
#define GB(value)(1024*MB(value))


#define STB_IMAGE_IMPLEMENTATION 
#include "..\\libs\\stb_image.h" 

#include "win32.hpp"

global_variable bool running;
global_variable win32OffScreenBuffer bitmap;
global_variable Data data = {};

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
	return VirtualAlloc(NULL, bytes_to_alloc, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);
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
		case WM_SIZE:
			win32ResizeDIBSection(&bitmap, LOWORD(lParam), HIWORD(lParam));
		break;
		case WM_INPUT:
		{
			//this documentation is retarded, msdn implies that the size can be any size whatsoever, 
			//however it will be *one* of the *possible* unions, so unless we like to dynamically allocate to save like two bytes.
			//we can just point to our own rawinput structure. Come on MSDN. I dislike allocating shit that I don't have to.

			RAWINPUT raw;
			UINT dw_size = sizeof(RAWINPUT);
			int ret = GetRawInputData((HRAWINPUT)lParam, RID_INPUT, &raw, &dw_size,sizeof(RAWINPUTHEADER));
			assert(ret > 0);
			RAWKEYBOARD kb = raw.data.keyboard;
			UINT scanCode = kb.MakeCode;
			bool e0 = false;
			static unsigned char keyState[256] = {};
			{
				
				e0 = kb.Flags & RI_KEY_E0;
				bool e1 = kb.Flags & RI_KEY_E1;
				if (e1)
				{
					scanCode = MapVirtualKey(kb.VKey, MAPVK_VK_TO_VSC);
				}
					
				// these are unasigned but not reserved as of now.
				// this is bad but, you know, we'll fix it if it ever breaks.
				#define VK_LRETURN         0x9E
				#define VK_RRETURN         0x9F

				#define UPDATE_KEY_STATE(key) do{keyState[key] = (kb.Flags & 1) ? 0 : 0xff;}while(0)
				if (kb.VKey == VK_CONTROL)
				{
					if (e0)	UPDATE_KEY_STATE(VK_RCONTROL);
					else	UPDATE_KEY_STATE(VK_LCONTROL);
					keyState[VK_CONTROL] = keyState[VK_RCONTROL] | keyState[VK_LCONTROL];
				}
				else if (kb.VKey == VK_SHIFT)
				{
					// because why should any api be consistant lol
					// (because we get different scancodes for l/r-shift but not for l/r ctrl etc... but still)
					UPDATE_KEY_STATE(MapVirtualKey(kb.MakeCode, MAPVK_VSC_TO_VK_EX));
					keyState[VK_SHIFT] = keyState[VK_LSHIFT] | keyState[VK_RSHIFT];
				}
				else if (kb.VKey == VK_MENU)
				{
					if (e0)	UPDATE_KEY_STATE(VK_RMENU);
					else	UPDATE_KEY_STATE(VK_LMENU);
					keyState[VK_MENU] = keyState[VK_RMENU] | keyState[VK_LMENU];
				}
				else if (kb.VKey == VK_RETURN)
				{
					// this is currently not sent through to the application 
					// but I thought while I remembered what the hell this crap is about I might as well write it.
					if (e0) UPDATE_KEY_STATE(VK_RRETURN);
					else	UPDATE_KEY_STATE(VK_LRETURN);
					keyState[VK_RETURN] = keyState[VK_RRETURN] | keyState[VK_LRETURN];
				}
				else
				{
					UPDATE_KEY_STATE(kb.VKey);
				}
				#undef UPDATE_KEY_STATE
			}

			bool window_is_ontop = !wParam;
			if (raw.header.dwType == RIM_TYPEKEYBOARD && window_is_ontop)
			{
				assert(!(kb.VKey >> 8));
				
				if(!(kb.Flags & 0x1))
				{
					char utf8_buffer[32];
					char *buff=0;
					int utf8_len = 0;
					
					wchar_t utf16_buffer[16];
					unsigned char ctrl = keyState[VK_CONTROL];
					// note this is only true on euro keyboards ie. where left alt is alt gr
					// however this appears to work on US-keyboards as well since control is ignored in the ToUnicode function

					keyState[VK_CONTROL]|= keyState[VK_RMENU];
					int utf16_len = ToUnicode(kb.VKey, kb.MakeCode, keyState, utf16_buffer, ARRAY_LENGTH(utf16_buffer), 0);
					keyState[VK_CONTROL] = ctrl;
					

					if (utf16_len > 0) //this is a bad check, errs on multiple bytes!
					{
						if (!iswcntrl(utf16_buffer[0]))
						{
							int32_t errors;
							utf8_len= utf16toutf8((utf16_t *)utf16_buffer, utf16_len*sizeof(char16_t), utf8_buffer, ARRAY_LENGTH(utf8_buffer), &errors);
							if (!errors)
							{
								buff = utf8_buffer;
							}
							else
							{
								utf8_len = 0;
							}
						}
					}
					
					if (kb.VKey == VK_RETURN)
					{
						utf8_len = 1;
						utf8_buffer[0] = '\n';
						buff = utf8_buffer;
					}

					Input input = {};
					input.caps = keyState[VK_CAPITAL];
					input.inputType = input_keyboard;
					input.utf8 = buff;
					input.utf8_len = utf8_len;
					input.VK_Code = kb.VKey;
				
					input.shift_left    = keyState[VK_LSHIFT];
					input.shift_right   = keyState[VK_RSHIFT];
					input.control_right = keyState[VK_RCONTROL];
					input.control_left  = keyState[VK_LCONTROL];
					input.alt_right     = keyState[VK_RMENU];
					input.alt_left      = keyState[VK_LMENU];

					updateInput(&data, &input, bitmap.bitmap, 0);

					result = DefWindowProc(window, message, wParam, lParam);
				}
			}
			break;
		}
		case WM_DESTROY: 
		{//kill application
			running = false;
			break;
		}
		default:
		{
			result = DefWindowProc(window,message,wParam,lParam);
		} break;
	}
	return result;	
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

		DA_char buffer = DA_char::make(general_allocator);
		serialize(&buffer, *clipboard);
		
		//this only workis if GMEM_FIXED is set. why? (GPTR = GMEM_FIXED | GMEM_ZEROINIT)
		customFormatMem = GlobalAlloc(GPTR, buffer.length);
		GlobalLock(customFormatMem);
			memcpy(customFormatMem, buffer.start, buffer.length);
		GlobalUnlock(customFormatMem);
		buffer.destroy();
	}

	String utf8_string = string_from_clipboard(clipboard);

	HGLOBAL unicodeFormatMem;
	{ //utf_16
		int len;
		char16_t *utf16_string = utf8_string.ss_utf16(&len);
		size_t size = len * sizeof(char16_t);
		unicodeFormatMem = GlobalAlloc(GPTR, size);
		GlobalLock(unicodeFormatMem);
		{
			memcpy(unicodeFormatMem, utf16_string, size);
		}GlobalUnlock(unicodeFormatMem);
	}
		
	HGLOBAL asciiFormatMem;
	{	//utf8
		size_t size = utf8_string.length*sizeof(char);
		asciiFormatMem = GlobalAlloc(GPTR, size);
		GlobalLock(asciiFormatMem);
		{
			memcpy(asciiFormatMem, utf8_string.start, size);
		}GlobalUnlock(asciiFormatMem);
	}
	utf8_string.destroy(general_allocator);

	
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
		ret.clips = DA_ClipboardItem::make(general_allocator);
		OpenClipboard(0);
		char *endless_buffer = (char *)MemStack_GetTop();
		int len;
		{
			char16_t *p = (char16_t *)GetClipboardData(CF_UNICODETEXT);
			int32_t errors;
			GlobalLock(p);
			{
				len=utf16toutf8((utf16_t *)p, lstrlenW((wchar_t *)p), endless_buffer, 2000, &errors);
			} GlobalUnlock(p);
		} CloseClipboard();
		ClipboardItem item = {};
		item.onSeperateLine = false;
		String s = { endless_buffer,len };
		item.string = s.copy(general_allocator);
		ret.clips.add(item);
	}
	else if (IsClipboardFormatAvailable(CF_TEXT))
	{
		ret.clips = DA_ClipboardItem::make(general_allocator);
		ClipboardItem item = {};
		OpenClipboard(0);
		{
			char *p = (char *)GetClipboardData(CF_TEXT);
			GlobalLock(p);
			{
				item.string = String::make(p).copy(general_allocator);
			} GlobalUnlock(p);
		} CloseClipboard();
		item.onSeperateLine = false;
		ret.clips.add(item);
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


enum ttf_platform {
	unicode = 0,
	mac = 1,
	ISO = 2,
	windows = 3,
	custum = 4,
};
enum ttf_encoding {
	uni_1_0 = 0,
	uni_1_1 = 1,
	iso_iec10646 = 2,
	uni_2_0_bmp = 3,
	uni_2_0_full = 4,
	uni_var_seq = 5,
	uni_full = 6,
};
union TMP {
	struct {
		ttf_platform platform;
		ttf_encoding encoding;
	};
	uint64_t val;
};

#define HT_KEY TMP
#define HT_HASH(x) silly_hash(x.val)
#define HT_NAME ll_set
#define HT_EQUAL(a,b) a.val == b.val
#include "L:\HashTable.h"
ll_set set = ll_set::make(font_allocator,8);
void iterate_strings(const stbtt_fontinfo *font) {
	stbtt_int32 i, count, stringOffset;
	stbtt_uint8 *fc = font->data;
	stbtt_uint32 offset = font->fontstart;
	stbtt_uint32 nm = stbtt__find_table(fc, offset, "name");
	if (!nm) return ;

	count = ttUSHORT(fc + nm + 2);
	stringOffset = nm + ttUSHORT(fc + nm + 4);
	for (i = 0; i < count; ++i) {
		stbtt_uint32 loc = nm + 6 + 12 * i;
		int length = ttUSHORT(fc + loc + 8);
		char *c = (char *)(fc + stringOffset + ttUSHORT(fc + loc + 10));
		int platformID = ttUSHORT(fc + loc + 0), encodingID = ttUSHORT(fc + loc + 2), languageID = ttUSHORT(fc + loc + 4), nameID = ttUSHORT(fc + loc + 6);
		//if (platformID > 1  || !(nameID == 1 || nameID == 2 || nameID == 16 || nameID == 17))continue;
		char16_t *buffer = (char16_t*)malloc(length*sizeof(char16_t));
		TMP tmp;
		tmp.platform = (ttf_platform)platformID;
		tmp.encoding = (ttf_encoding)encodingID;
		
		set.insert(tmp);
		for (int i = 0; i < length; i++) {
			if (i & 1) ((char *)buffer)[i] = c[i-1];
			else       ((char *)buffer)[i] = c[i+1];
		}
		buffer[length/2] = 0;
		c[length] = 0;
		int q = 0;
		free(buffer);
	}
}

void loadFonts() 
{
	static const LPCSTR fontRegistryPath = "Software\\Microsoft\\Windows NT\\CurrentVersion\\Fonts";
	HKEY hKey;
	LONG result;

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

	char winDir[MAX_PATH]; //the maxpath isn't really maxpath anymore but I'm fairly confident that the windows file is inside of max_path though...
	GetWindowsDirectory(winDir, MAX_PATH);
	String system_fonts_file = String::make(winDir);
	//load the font
	do {
		wsFontFile = "";
		valueDataSize = maxValueDataSize;
		valueNameSize = maxValueNameSize;


		result = RegEnumValue(hKey, valueIndex, valueName, &valueNameSize, 0, &valueType, (BYTE *)valueData, &valueDataSize);
		valueIndex++;
		bool success;
		String value_data = String::make(valueData);
		int dot_idx = value_data.find_last('.');
		char *extension = valueData + dot_idx;
		if (strcmp(".ttf", extension) && strcmp(".TTF", extension)){
			continue; //something other than ttf
		}
		String str = String::make(valueName); // why though?
		int paren_idx;
		if ((paren_idx = str.find_last('(')) != -1) {
			str.trim_end(str.length - paren_idx);
		}
		str.trim_end(String::make(" "));
		str.trim_end(String::make("Regular"));
		str.trim_end(String::make("regular"));

		AvailableFont typeface = {};
		typeface.name = str.copy(font_allocator);
		
		if ((valueData[1] == ':') && false)
		{
			typeface.path = value_data.copy(font_allocator);
		} else
		{
			// slow isch... add a merge?? add ss.copy cause these are actually permanent
			typeface.path = ss_sprintf("%s\\FONTS\\%s", winDir, value_data.start).copy(font_allocator);
		}
		#if 0
				stbtt_fontinfo info = {};
				info.userdata = "stbtt internal allocation";
				setUpTT(&info, typeface.path);
				if (!info.data)continue;
		#endif
		availableFonts.add(typeface);
	} while (result != ERROR_NO_MORE_ITEMS);
	RegCloseKey(hKey);
#if 0
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
#endif
}





internal bool charDown(int character, Input *input)
{
	/*
	setMods(input);
	input->inputType = input_character;
	input->character = character;
	*/
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
	
	RAWINPUTDEVICE rid = {};
	rid.usUsagePage = 0x01;
	rid.usUsage = 0x06;
	rid.dwFlags = RIDEV_NOLEGACY| RIDEV_INPUTSINK;   // adds HID keyboard and also ignores legacy keyboard messages
	rid.hwndTarget = window;

	assert(RegisterRawInputDevices(&rid, 1, sizeof(rid)));
	

	win32WindowDimensions dims = getWindowDimensions(window);
	win32ResizeDIBSection(&bitmap,dims.width,dims.height);
	
	//textBuffer = openFileIntoNewBuffer();
	bool success;

	//textBuffer = openFileIntoNewBuffer(u"notes.txt", &success);
	//textBuffer.fileName = u"hello.c";
	DA_PTextBuffer textBuffers = DA_PTextBuffer::make(general_allocator);
	
	loadFonts();

	//Add(&textBuffers, textBuffer);
	
	data.textBuffers = textBuffers;
	TextBuffer *buffer= openCommanLine();
	data.commandLine = buffer;
	data.activeTextBufferIndex = 0;
	data.menu = {};
	data.menu.allocator= general_allocator->make_new<DH_SlowTrackingArena, DH_TrackingAllocator *, char *>(&tracking_raw, "backing buffer user");

	data.menu.items = DA_MenuItem::make(general_allocator);
	//Layout *split_a = CREATE_LAYOUT(layout_type_x, 1, CREATE_LAYOUT(layout_type_y,1,leaf,0.3,leaf,0.7,leaf), 0.7f, leaf);
	//Layout *split_b = CREATE_LAYOUT(layout_type_x, 1, leaf, 0.3f, leaf);

	data.layout = CREATE_LAYOUT(layout_type_y, 2, leaf, .5, CREATE_LAYOUT(layout_type_x,2,leaf,.5,leaf));

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
				DispatchMessage(&message);
			}

			{
				//data.updateAllBuffers |= redrawAll;
				data.updateAllBuffers = true;
				//need to handle dt!
				renderFrame(&data, bitmap.bitmap, MicorSeconds_Since_StartUP);
				HDC deviceContext = GetDC(window);
				win32WindowDimensions windowDims = getWindowDimensions(window);
				win32UpdateWindow(deviceContext, windowDims.width, windowDims.height, bitmap);
				ReleaseDC(window, deviceContext);
			}

			uint64_t timeStamp;
			uint64_t timePassed = get_MicroSeconds_Since(previousTimeStamp,&timeStamp);
			MicorSeconds_Since_StartUP += timePassed;
			
			if (timePassed < targetFrameWait)
			{
				Sleep((targetFrameWait - timePassed)/1000); // erm system clock is granular to 15,6ms. which is waaaay to much. all our frames may wait between 0 and 15,6 ms. Which is way to much. 
			}
			
			previousTimeStamp = timeStamp;
		}
    }
	return 0;
}

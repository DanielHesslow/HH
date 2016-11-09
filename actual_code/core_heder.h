#ifndef CORE_HEADER
#define CORE_HEADER

#include "stdint.h"
#include "math.h"
//#include "History.h"

#define internal static 

#define clamp(num, lower, upper) ((num < lower) ? lower : ((num > upper) ? upper : num))

struct Location
{
	int line;
	int column;
};


//---Bindings
enum Mods
{
	mod_none = 0,
	mod_alt = 1 << 1,
	mod_control = 1 << 2,
	mod_shift = 1 << 3,
	mod_shift_left = 1 << 4 | mod_shift,
	mod_shift_right = 1 << 5 | mod_shift,
	mod_control_left = 1 << 6 | mod_control,
	mod_control_right = 1 << 7 | mod_control,
	mod_alt_right = 1 << 8 | mod_alt,
	mod_alt_left = 1 << 9 | mod_alt,
};

Mods operator | (Mods a, Mods b) { return static_cast<Mods>(static_cast<int>(a) | static_cast<int>(b)); }



enum ModMode
{
	atLeast,
	atMost,
	precisely,
};



struct Color
{
	float a, r, g, b;
	Color operator * (float f)
	{
		return{ a*f, r*f, g*f, b*f };
	}
	Color operator / (float f)
	{
		return{ a / f ,r / f, g / f, b / f, };
	}
	Color operator + (Color c)
	{
		return{ a + c.a, r + c.r, g + c.g, b + c.b };
	}
	Color operator * (Color c)
	{
		return{ a * c.a, r * c.r, g * c.g, b * c.b };
	}

	bool operator == (Color c)
	{
		return r == c.r && g == c.g && b == c.b && a == c.a;
	}
	explicit operator int()
	{
		return (((uint8_t)(a * 255)) << 24) |
			(((uint8_t)(r * 255)) << 16) |
			(((uint8_t)(g * 255)) << 8) |
			(((uint8_t)(b * 255)) << 0);
	}
};
bool color_eq(void *a, void *b)
{
	return ((*(Color *)a) == (*(Color *)b));
}


Color rgb(float red, float green, float blue)
{
	return{ 1.0f, red, green, blue };
}
Color argb(float alpha, float red, float green, float blue)
{
	return{ alpha, red, green, blue };
}

float _hue_to_rgb(float p, float q, float t)
{
	if (t < 0.0) t += 1;
	if (t > 1.0) t -= 1;
	if (t < (1.0 / 6.0))
		return p + (q - p) * 6.0 * t;
	if (t < (1.0 / 2.0))
		return q;
	if (t < (2.0 / 3.0))
		return p + (q - p) * (2.0 / 3.0 - t) * 6.0;
	return p;
}

Color hsl(float h, float s, float l) {
	float r, g, b;
	h = fmod(h, 1);
	h = h > 0 ? h : 1 - h;
	s = clamp(s, 0, 1);
	l = clamp(l, 0, 1);

	if (s == 0) {
		r = g = b = l; // achromatic
	}
	else {
		float q = l < 0.5 ? l * (1 + s) : l + s - l * s;
		float p = 2 * l - q;
		r = _hue_to_rgb(p, q, h + 1.0 / 3.0);
		g = _hue_to_rgb(p, q, h);
		b = _hue_to_rgb(p, q, h - 1.0 / 3.0);
	}

	return{ 1.0f, r,g,b, };
}
Color colorFromInt(int i)
{
	float a = ((i & 0xff000000) >> 24) / (255.0f);
	float r = ((i & 0x00ff0000) >> 16) / (255.0f);
	float g = ((i & 0x0000ff00) >> 8) / (255.0f);
	float b = ((i & 0x000000ff) >> 0) / (255.0f);
	return argb(a, r, g, b);
}




enum BufferChangeAction
{
	buffer_change_add,
	buffer_change_remove,
};

struct BufferChange
{
	BufferChangeAction action;
	char character; // should be char32_t but for now I don't want to dig into that.
	Location location;
};


#include "colorScheme.h"

#endif
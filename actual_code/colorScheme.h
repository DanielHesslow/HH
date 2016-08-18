

struct ColorScheme
{
	Color *background_colors;
	int number_of_background_colors;
	Color highlightColor, foregroundColor, caretLight, caretDark, active_color;
};


Color _highlightColor = rgb(.25,.25,.25);
Color _foregroundColor = rgb(.8f,.8f,.8f);

Color _caret_light = rgb(.1f,.6f,.8f);
Color _caret_dark = rgb(.2f,.3f,.8f );
Color _active_color = hsl(.09,.9f,.5);

float _s = .4;
float _i = .1;
Color _backgroundColors[] = { hsl(1.0 / 2.0,_s,_i), hsl(1.0/3.0,_s,_i), hsl(4.0 / 5.0,_s,_i) };

global_variable ColorScheme active_colorScheme = 
{
	_backgroundColors,
	3,
	_highlightColor,
	_foregroundColor,
	_caret_light,
	_caret_dark,
	_active_color
};


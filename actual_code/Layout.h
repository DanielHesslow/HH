#ifndef LayoutHeader
#define LayoutHeader
#include "DH_MacroAbuse.h"

enum LayoutType
{
	layout_type_x,
	layout_type_y,
	layout_type_z,
};

#define CREATE_LAYOUT(layout_type, width, ...) createLayout(layout_type, width, DHMA_NUMBER_OF_ARGS(__VA_ARGS__),__VA_ARGS__)


struct Divider
{
	float relative_position;
	int width;
};

struct Layout
{
	LayoutType type;
	Layout *children[10];
	int number_of_children; 
	Divider dividers[9];
	int favourite_child;
	//know that these may not be able to tell the future
	int last_width;
	int last_height; 
};

struct LayoutLocator
{
	Layout *parent;
	int child_index;
};

int total_number_of_children(Layout *layout);
bool parentLayout(Layout *root, LayoutLocator locator, LayoutLocator *result);
LayoutLocator locateLayout(Layout *root, int target_index);
Layout *layoutFromLocator(LayoutLocator locator);
int number_of_leafs(Layout *layout);
int total_number_of_children(Layout *layout);
int favourite_descendant(Layout *layout);
Layout *createLayout(LayoutType type, int width, int number_of_args, ...);


#endif

#include "Layout.h"
//#include "Rendering.h"
#include "DH_MacroAbuse.h"


Layout static_layouts[100];
static int running_index = 0;

Layout *createLayout(LayoutType type, int width,  int number_of_args, ...)
{
	Layout *ret = &static_layouts[running_index++];
	va_list args;
	va_start(args, number_of_args);
	for (int i = 0; i < number_of_args/2; i++)
	{
		ret->children[i] = va_arg(args, Layout *);
		ret->dividers[i].relative_position = va_arg(args, double); //floats auto conv to double through varargs..
		ret->dividers[i].width = width; 
	}
	ret->number_of_children = number_of_args / 2 +1;
	ret->children[number_of_args/2] = va_arg(args, Layout *);
	ret->type = type;
	return ret;
}





int favourite_descendant(Layout *layout)
{
	if (layout == 0)
	{
		return 0;
	}
	else
	{
		int ack = 0;
		for (int i = 0; i < layout->favourite_child; i++)
			ack += number_of_leafs(layout->children[i]);
		return favourite_descendant(layout->children[layout->favourite_child]) + ack;
	}
}



int total_number_of_children(Layout *layout)
{
	int ack = 0;
	if (!layout) return 0;
	for (int i = 0; i < layout->number_of_children;i++)
	{
		ack += 1+total_number_of_children(layout->children[i]);
	}
	return ack;
}

int number_of_leafs(Layout *layout)
{
	int ack = 0;
	if (!layout) return 1;
	for (int i = 0; i < layout->number_of_children; i++)
	{
		ack += number_of_leafs(layout->children[i]);
	}
	return ack;
}




Layout *layoutFromLocator(LayoutLocator locator)
{
	return &locator.parent[locator.child_index];
}

LayoutLocator locateLayout(Layout *root, int target_index)
{
	for (int i = 0; i < root->number_of_children; i++) 
	{
		if (target_index == 0 && root->children[i] == 0) return{ root, i };
		int leafs = number_of_leafs(root->children[i]);
		if (target_index<leafs)
		{
			return locateLayout(root->children[i], target_index);
		}
		else
		{
			target_index -= leafs;
		}
	}
	return{ 0,0 }; // nothing we can do.
}

bool parentLayout(Layout *root, LayoutLocator locator, LayoutLocator *result)
{
	for (int i = 0; i < root->number_of_children; i++)
	{
		if (root->children[i] == locator.parent)
		{
			*result = { root, i };
			return true;
		}
		else
		{
			LayoutLocator pot;
			if (root->children[i]&&parentLayout(root->children[i], locator, &pot))
			{
				*result = pot;
				return true;
			}
		}
	}
	*result = { 0,0 };
	return false;
}

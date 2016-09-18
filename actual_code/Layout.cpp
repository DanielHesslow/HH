
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

void renderWithLayout(Bitmap bitmap, Layout *layout, char **items_ptr, char *items_end, void (*render_and_advance)(Bitmap bitmap, char **ptr))
{
	bool ended = items_end == *items_ptr;

	if (ended)
	{
		clearBitmap(bitmap, (int)active_colorScheme.background_colors[0]);
	}
	 
	if (!layout || layout->number_of_children == 0)
	{
		if(!ended) render_and_advance(bitmap, items_ptr);
		return;
	}

	int total_width = 0;
	for (int i = 0; i < layout->number_of_children- 1; i++)
	{
		total_width += layout->dividers[i].width;
	}

	switch (layout->type)
	{
		case layout_type_x:
		{
			int ack_border_width = 0;
			float last_rel_pos = 0;
			int working_width = bitmap.width - total_width;
			for (int i = 0; i < layout->number_of_children-1; i++)
			{
				int start_px = ack_border_width + last_rel_pos * working_width;
				last_rel_pos = layout->dividers[i].relative_position;
				int end_px = ack_border_width + last_rel_pos * working_width;
				ack_border_width += layout->dividers[i].width;
				renderRect(bitmap, end_px, 0, layout->dividers[i].width, bitmap.height,(int)active_colorScheme.active_color);
				renderWithLayout(subBitmap(bitmap, end_px - start_px, bitmap.height, start_px, 0), layout->children[i], items_ptr, items_end, render_and_advance);
			}
			int start_px = ack_border_width + last_rel_pos * working_width;
			int end_px = bitmap.width;
			renderWithLayout(subBitmap(bitmap, end_px - start_px, bitmap.height, start_px, 0), layout->children[layout->number_of_children - 1], items_ptr, items_end, render_and_advance);
		}break;

		case layout_type_y:
		{
			int ack_border_width = 0;
			float last_rel_pos = 0;
			int working_height = bitmap.height - total_width;
			for (int i = 0; i < layout->number_of_children - 1; i++)
			{
				int start_px = ack_border_width + last_rel_pos * working_height;
				last_rel_pos = layout->dividers[i].relative_position;
				int end_px = ack_border_width + last_rel_pos * working_height;
				ack_border_width += layout->dividers[i].width;
				renderRect(bitmap, 0, end_px, bitmap.width, layout->dividers[i].width, (int)active_colorScheme.active_color);
				renderWithLayout(subBitmap(bitmap, bitmap.width, end_px - start_px, 0, start_px),  layout->children[i], items_ptr, items_end, render_and_advance);
			}
			int start_px = ack_border_width + last_rel_pos * working_height;
			int end_px = bitmap.width;
			renderWithLayout(subBitmap(bitmap, bitmap.width, end_px - start_px, 0, start_px), layout->children[layout->number_of_children - 1], items_ptr, items_end, render_and_advance);
		}break;

		case layout_type_z: 
		{
			for (int i = 0; i < layout->number_of_children; i++)
			{	//probably don't always need to render on top... Need to handle that.
				renderWithLayout(bitmap, layout->children[i], items_ptr, items_end, render_and_advance);
			}
		}break;
	}

	layout->last_width = bitmap.width;
	layout->last_height = bitmap.height;
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


void test()
{
	Layout *leaf = (Layout *)0;
	assert(total_number_of_children(CREATE_LAYOUT(layout_type_x, 2, leaf, .5f, leaf)) == 2);
	assert(total_number_of_children(CREATE_LAYOUT(layout_type_x, 2, leaf)) == 1);
	assert(total_number_of_children(CREATE_LAYOUT(layout_type_x, 2, leaf, 0.5f, leaf, 0.6f, leaf)) == 3);
	assert(total_number_of_children(CREATE_LAYOUT(layout_type_x, 2, CREATE_LAYOUT(layout_type_x, 2, leaf, 0.5f, leaf), 0.5f, leaf, 0.6f, leaf)) == 5);

	assert(number_of_leafs(CREATE_LAYOUT(layout_type_x, 2, leaf, .5f, leaf)) == 2);
	assert(number_of_leafs(CREATE_LAYOUT(layout_type_x, 2, leaf)) == 1);
	assert(number_of_leafs(CREATE_LAYOUT(layout_type_x, 2, leaf, 0.5f, leaf, 0.6f, leaf)) == 3);
	assert(number_of_leafs(CREATE_LAYOUT(layout_type_x, 2, CREATE_LAYOUT(layout_type_x, 2, leaf, 0.5f, leaf), 0.5f, leaf, 0.6f, leaf)) == 4);

	assert(total_number_of_children(leaf) == 0);
	assert(number_of_leafs(leaf) == 1);


	Layout *l = CREATE_LAYOUT(layout_type_x, 1, leaf, 0.5, leaf);
	Layout *r = CREATE_LAYOUT(layout_type_x, 1, leaf, 0.5, leaf);
	Layout p = {};
	p.children[0] = l;
	p.children[1] = r;
	p.number_of_children = 2;
	LayoutLocator loc;
	loc = locateLayout(&p,0);
	assert(loc.child_index == 0);
	assert(loc.parent == l);

	loc = locateLayout(&p, 1);
	assert(loc.child_index == 1);
	assert(loc.parent == l);

	loc = locateLayout(&p, 2);
	assert(loc.child_index == 0);
	assert(loc.parent == r);
	
	loc = locateLayout(&p, 3);
	assert(loc.child_index == 1);
	assert(loc.parent == r);
}


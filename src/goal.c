/* Atomix -- a little mind game about atoms and molecules.
 * Copyright (C) 1999 Jens Finke
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <libxml/parser.h>
#include <libxml/tree.h>

#include "global.h"
#include "main.h"
#include "theme.h"
#include "goal.h"
#include "canvas_helper.h"

#define SCALE_FACTOR  0.6

void
goal_print_offset(gpointer ptr, gpointer data);

GnomeCanvasItem*
create_small_moveable(GnomeCanvasGroup *group, gdouble x, gdouble y, Tile* tile);

GnomeCanvasItem*
create_small_item(GdkPixbuf *img, gdouble x, gdouble y, GnomeCanvasGroup *group);

Goal*
goal_new(PlayField* pf)
{
	Goal* goal;
	gint row,col;
	Tile *tile;
	GSList* list;
	gint tile_id = 0;
	gint current_id = 0;
	ItemOffset* offset;
	gint max_id = 0; 

	goal = g_malloc(sizeof(Goal));    
    
	goal->pf = pf;
	goal->item_group = NULL;
    
	/* initialise index */
	goal->index = g_ptr_array_new();
	g_assert(goal->index);
    
	/* 
	 * save for every tile id the possible offsets  
	 * from the top left corner of the playfield
	 */
	while(current_id <= max_id)
	{
		list = NULL;
	
		for(row = 0; row < pf->n_rows; row++)
		{
			for(col = 0; col < pf->n_cols; col++)
			{
				tile = playfield_get_tile(pf, row, col);
				if(tile_get_type(tile) == TILE_MOVEABLE)
				{
					tile_id = tile_get_unique_id(tile);
		    
					if(tile_id == current_id)
					{
						offset = g_malloc(sizeof(ItemOffset));
			
						offset->horiz = col;
						offset->vert = row;
		    
						list = g_slist_append(list, offset);
					}

					max_id = MAX(max_id, tile_id);
		    
				}
			}
		}
		g_ptr_array_add(goal->index, (gpointer)list);
	    
		current_id++;
	}

	return goal;
}

void
goal_destroy(Goal* goal)
{
	if(goal->item_group)
	{
		gtk_object_destroy(GTK_OBJECT(goal->item_group));
	}
	g_ptr_array_free(goal->index, TRUE);
	g_free(goal);
}

void 
goal_print(Goal* goal)
{
	guint i;
	GSList* list;
    
	if(goal->pf)
	{
		g_print("GOAL:\n");
		playfield_print(goal->pf);
	}
#ifdef DEBUG
	else 
	{
		g_print("No goal specified.\n");
	}
#endif

	if(goal->index)
	{
		for(i = 0; i < goal->index->len; i++)
		{
			list = (GSList*)g_ptr_array_index(goal->index,i);
			g_print("%x: ",i);
			if(list)
			{
				g_slist_foreach(list, (GFunc)goal_print_offset, NULL);
			}
			else
			{
				g_print("list NULL.");
			}
			g_print("\n");
		}
	}
#ifdef DEBUG
	else
	{
		g_print("Index is somehow NULL!\n");
	}
#endif
}

void
goal_print_offset(gpointer ptr, gpointer data)
{
	ItemOffset* off = (ItemOffset*) ptr;
	g_print("{%x|%x} ",off->horiz, off->vert);
}

gboolean
goal_reached(Goal* goal, PlayField* pf, guint row_anchor, guint col_anchor)
{
	guint tile_id;
	gint i;
	GSList* list;
	gboolean result = FALSE;
	Tile *tile;

	tile = playfield_get_tile(pf, row_anchor, col_anchor);
	tile_id = tile_get_unique_id(tile);
	list = (GSList*) g_ptr_array_index(goal->index, tile_id);
    
	for(i = 0; i < g_slist_length(list) && !result; i++)
	{
		ItemOffset* offset = (ItemOffset*) g_slist_nth_data(list, i);
		gboolean item_checked = FALSE;
		gboolean item_result = TRUE;
		gint start_row =  row_anchor - offset->vert;
		gint start_col =  col_anchor - offset->horiz;

		if((start_row >= 0) && (start_col >= 0))
		{
			gint pf_row;
			gint goal_row = 0;
			gint end_row = start_row + goal->pf->n_rows;
			gint end_col = start_col + goal->pf->n_cols;;
			item_checked = TRUE;

			for(pf_row = start_row; (pf_row < end_row) && item_result; pf_row++)
			{
				gint goal_col = 0;
				gint pf_col;

				for(pf_col = start_col; (pf_col < end_col) && item_result; pf_col++)
				{
					Tile *pf_tile;
					Tile *goal_tile;
					goal_tile = playfield_get_tile(goal->pf,
								       goal_row,
								       goal_col);
					pf_tile = playfield_get_tile(pf, pf_row , pf_col);

					if(tile_get_type(goal_tile) == TILE_MOVEABLE)
					{
						if((tile_get_unique_id(goal_tile) != 
						    tile_get_unique_id(pf_tile))) 
						{
							item_result = FALSE;
						}		    
					}
					goal_col++;
				}
				goal_row++;
			}

			result = item_checked && item_result;	    
		}
	}

	return result;
}

GnomeCanvasItem*
create_small_moveable(GnomeCanvasGroup *group, gdouble x, gdouble y, Tile* tile)
{
	GdkPixbuf *img = NULL;
	GdkPixbuf *link_img;
	GnomeCanvasItem *item = NULL;
	GnomeCanvasGroup *moveable_group = NULL;
	Theme* theme;
	GSList *link_images;
    
	theme = get_actual_theme ();
	img = theme_get_tile_image (theme, tile);
	link_images = theme_get_tile_link_images (theme, tile);
    
	if(img) 
	{
		gint i;
		/* create new group at (0,0) */
		moveable_group = create_group("ca_matrix", group);

		/* add connection and moveable images */
		if(link_images)
		{
			for(i = 0; i < g_slist_length(link_images); i++)
			{
				link_img = (GdkPixbuf*) g_slist_nth_data(link_images, i);
				item = create_small_item(link_img, 0.0, 0.0, moveable_group);
			}
		}		
		item = create_small_item(img, 0.0, 0.0, moveable_group);

		/* move to the right location */
		gnome_canvas_item_move(GNOME_CANVAS_ITEM(moveable_group), x, y);
	
	} 
#ifdef DEBUG
	else {
		g_print("Atom image not found!\n");
	}
#endif    
	g_slist_free(link_images);
	
	return GNOME_CANVAS_ITEM(moveable_group);
}

GnomeCanvasItem*
create_small_item(GdkPixbuf *img, gdouble x, gdouble y, 
		  GnomeCanvasGroup *group)
{
	GnomeCanvasItem *item;

	item = gnome_canvas_item_new(group,
				     gnome_canvas_pixbuf_get_type(),
				     "pixbuf", img,
				     "x", x,
				     "x_in_pixels", TRUE,
				     "y", y,
				     "y_in_pixels", TRUE,
				     "width", 
				     (gdouble)(gdk_pixbuf_get_width (img))*SCALE_FACTOR,
				     "height", 
				     (gdouble)(gdk_pixbuf_get_height(img))*SCALE_FACTOR,
				     "anchor", GTK_ANCHOR_NW,
				     NULL);                              
	return item;
}

void
goal_render(Goal* goal)
{
	GnomeCanvasItem *item;
	GnomeCanvas *canvas;
	gint row,col;
	gdouble x;
	gdouble y;
	Tile *tile;
	TileType type;
	gint tile_width, tile_height;
	gint width, height;

	canvas = GNOME_CANVAS(glade_xml_get_widget (get_gui (), "ca_goal"));
	if(goal->item_group == NULL)
	{
		goal->item_group = create_group_ref(canvas, NULL);
	}


	theme_get_tile_size(get_actual_theme(), 
			    &tile_width, 
			    &tile_height);

	for(row=0; row < goal->pf->n_rows; row++) 
	{
		for(col=0; col < goal->pf->n_cols; col++)
		{
			tile = playfield_get_tile(goal->pf, row, col);
			type = tile_get_type(tile);
			switch(type)
			{
			case TILE_MOVEABLE:
				x = col * tile_width * SCALE_FACTOR;
				y = row * tile_height * SCALE_FACTOR;
				item = create_small_moveable(goal->item_group, 
							     x, y, tile);

				break;
		
			case TILE_OBSTACLE:
			case TILE_NONE:
			default:
			}
		}
	}

	set_background_color_ref(GTK_WIDGET(canvas), 
				 theme_get_background_color(get_actual_theme()));
	
	width = tile_width * goal->pf->n_cols * SCALE_FACTOR;
	height = tile_height * goal->pf->n_rows * SCALE_FACTOR;
	gnome_canvas_set_scroll_region(canvas, 0, 0, width, height);
}

void
goal_clear(Goal* goal)
{
	gtk_object_destroy(GTK_OBJECT(goal->item_group));
	goal->item_group = NULL;
}

void goal_init (AtomixApp *app)
{
	GdkColor color;

	color.red = 60928;   /* this is the X11 color "cornsilk2" */
  	color.green = 59392;
	color.blue = 52480;

	set_background_color_ref(app->ca_goal, &color);
}

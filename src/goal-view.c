/* Atomix -- a little mind game about atoms and molecules.
 * Copyright (C) 2001 Jens Finke
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

#include "goal-view.h"
#include "canvas_helper.h"

static GnomeCanvas *goal_canvas;
static Theme       *goal_theme;
static GnomeCanvasGroup*  item_group;

#define SCALE_FACTOR  0.7

static GnomeCanvasItem* create_small_item (GnomeCanvasGroup *group, gdouble x, gdouble y, Tile* tile);
static void render_view (Goal *goal);
static void clear_view (void);

void 
goal_view_init (Theme *theme, GnomeCanvas *canvas)
{
	g_return_if_fail (IS_THEME (theme));
	g_return_if_fail (GNOME_IS_CANVAS (canvas));

	set_background_color (GTK_WIDGET (canvas), theme_get_background_color (theme));

	goal_canvas = canvas;
	goal_theme = theme;
}


void
goal_view_render (Goal* goal)
{
	clear_view ();

	if (goal != NULL)
		render_view (goal);
}

static void
render_view (Goal *goal)
{
	GnomeCanvasItem *item;
	PlayField *pf;
	gint row,col;
	gdouble x;
	gdouble y;
	Tile *tile;
	TileType type;
	gint tile_width, tile_height;
	gint width, height;

	g_return_if_fail (IS_GOAL (goal));
	g_return_if_fail (GNOME_IS_CANVAS (goal_canvas));
	g_return_if_fail (IS_THEME (goal_theme));

	if(item_group == NULL)
		item_group = create_group (goal_canvas, NULL);

	theme_get_tile_size (goal_theme, 
			     &tile_width, 
			     &tile_height);

	pf = goal_get_playfield (goal);
	for(row = 0; row < playfield_get_n_rows (pf); row++) 
	{
		for(col = 0; col < playfield_get_n_cols (pf); col++)
		{
			tile = playfield_get_tile (pf, row, col);
			if (!tile) continue;

			type = tile_get_tile_type(tile);
			switch(type)
			{
			case TILE_TYPE_ATOM:
				x = col * tile_width * SCALE_FACTOR;
				y = row * tile_height * SCALE_FACTOR;
				item = create_small_item (item_group, 
							  x, y, tile);

				break;
		
			case TILE_TYPE_WALL:
			case TILE_TYPE_UNKNOWN:
			default:
			}
			g_object_unref (tile);
		}
	}
	set_background_color  (GTK_WIDGET(goal_canvas), 
			       theme_get_background_color (goal_theme));
	
	width = tile_width * playfield_get_n_cols (pf) * SCALE_FACTOR;
	height = tile_height * playfield_get_n_rows (pf) * SCALE_FACTOR;
	gnome_canvas_set_scroll_region(goal_canvas, 0, 0, width, height);

	g_object_unref (pf);
}

static void
clear_view ()
{
	if (item_group)
		gtk_object_destroy (GTK_OBJECT(item_group));
	item_group = NULL;
}

static GnomeCanvasItem*
create_small_item (GnomeCanvasGroup *group, gdouble x, gdouble y, Tile* tile)
{
	GdkPixbuf *pixbuf = NULL;
	GdkPixbuf *small_pb = NULL;
	GnomeCanvasItem *item = NULL;

	g_return_val_if_fail (IS_TILE (tile), NULL);

	pixbuf = theme_get_tile_image (goal_theme, tile);

	small_pb = gdk_pixbuf_scale_simple (pixbuf, 
					    gdk_pixbuf_get_width (pixbuf) * SCALE_FACTOR,
					    gdk_pixbuf_get_height (pixbuf) * SCALE_FACTOR,
					    GDK_INTERP_BILINEAR);

	item = gnome_canvas_item_new(group,
				     gnome_canvas_pixbuf_get_type(),
				     "pixbuf", small_pb,
				     "x", x,
				     "x_in_pixels", TRUE,
				     "y", y,
				     "y_in_pixels", TRUE,
				     "width", 
				     (gdouble)(gdk_pixbuf_get_width (small_pb)),
				     "height", 
				     (gdouble)(gdk_pixbuf_get_height(small_pb)),
				     "anchor", GTK_ANCHOR_NW,
				     NULL);            
	gdk_pixbuf_unref (pixbuf);
	
	return GNOME_CANVAS_ITEM(item);
}


/* Atomix -- a little mind game about atoms and molecules.
 * Copyright (C) 1999-2001 Jens Finke
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

GnomeCanvasItem* create_small_item (GnomeCanvasGroup *group, gdouble x, gdouble y, Tile* tile);

static void destroy_hash_value (gpointer value);
static gboolean compare_playfield_with_goal (Goal *goal, PlayField *pf, 
					     guint start_row, guint start_col);


typedef struct 
{
	gint horiz;
	gint vert;
} TileOffset;



Goal*
goal_new (PlayField* goal_pf)
{
	Goal* goal;
	gint row,col;
	Tile *tile;

	goal = g_new0 (Goal, 1);    
    
	goal->pf = goal_pf;
	g_object_ref (goal->pf);
	goal->item_group = NULL;
    
	/* initialise index */
	goal->index = g_hash_table_new_full ((GHashFunc) g_direct_hash, 
					     (GCompareFunc) g_direct_equal,
					     NULL, (GDestroyNotify) destroy_hash_value);
	
	for(row = 0; row < playfield_get_n_rows (goal_pf); row++)
	{
		for(col = 0; col < playfield_get_n_cols (goal_pf); col++)
		{
			tile = playfield_get_tile(goal_pf, row, col);

			if (tile &&
			    tile_get_tile_type (tile) == TILE_TYPE_MOVEABLE)
			{
				gint tile_id;
				TileOffset *off;
				GSList *list = NULL;

				tile_id = tile_get_hash_value (tile);
				
				off = g_new0 (TileOffset, 1);
				off->horiz = col;
				off->vert = row;
				
				list = g_hash_table_lookup (goal->index, 
							    GINT_TO_POINTER (tile_id));

				list = g_slist_append(list, off);
				g_hash_table_insert (goal->index, GINT_TO_POINTER (tile_id),
						      (gpointer) list);
				
			}
			g_object_unref (tile);
		}
	}

	return goal;
}

static void
destroy_hash_value (gpointer value)
{
	GSList *list = (GSList*) value;
	GSList *it;

	for (it = list; it != NULL; it = it->next)
		g_free (it->data);

	g_free (list);
}


void
goal_destroy (Goal* goal)
{
	g_object_unref (goal->pf);

	if(goal->item_group)
		gtk_object_destroy(GTK_OBJECT(goal->item_group));

	g_hash_table_destroy ((GHashTable*) goal->index);
	g_free (goal);
}

static void
print_hash_table (gpointer key, gpointer value, gpointer data)
{
	GSList *list = (GSList*) value;
	GSList *it;

	g_print ("HASH: %i ", GPOINTER_TO_INT (key));
	for (it = list; it != NULL; it = it->next) {
		TileOffset *off = (TileOffset*) it->data;
		g_print("{%x|%x} ",off->horiz, off->vert);
	}
}

void 
goal_print (Goal* goal)
{
	g_return_if_fail (goal != NULL);
	g_return_if_fail (goal->pf != NULL);
	g_return_if_fail (goal->index != NULL);

	g_print("GOAL:\n");
	playfield_print(goal->pf);

	g_hash_table_foreach (goal->index, (GHFunc) print_hash_table, NULL);

	g_print ("\n");
}

gboolean
goal_reached(Goal* goal, PlayField* pf, guint row_anchor, guint col_anchor)
{
	guint tile_id;
	GSList *list;
	GSList *it;
	gboolean result = FALSE;
	Tile *tile;

	g_return_val_if_fail (goal != NULL, FALSE);
	g_return_val_if_fail (goal->index != NULL, FALSE);
	g_return_val_if_fail (IS_PLAYFIELD (pf), FALSE);

	tile = playfield_get_tile (pf, row_anchor, col_anchor);
	if (tile == NULL) return FALSE;

	tile_id = tile_get_hash_value (tile);
	list = (GSList*) g_hash_table_lookup (goal->index, GINT_TO_POINTER (tile_id));
    
	for (it = list; it != NULL && !result; it = it->next) {
		TileOffset *offset = (TileOffset*) it->data;
		gint start_row =  row_anchor - offset->vert;
		gint start_col =  col_anchor - offset->horiz;

		result = compare_playfield_with_goal (goal, pf, start_row, start_col);
	}

	return result;
}

static gboolean
compare_playfield_with_goal (Goal *goal, PlayField *pf, guint start_row, guint start_col) 
{
	gint pf_row;
	gint pf_col;
	gint goal_row;
	gint goal_col; 
	gint end_row;
	gint end_col;
	gboolean result;
	
	end_row = start_row + playfield_get_n_rows (goal->pf);
	end_col = start_col + playfield_get_n_cols (goal->pf);
	result = TRUE;
	
	for(pf_row = start_row, goal_row = 0; 
	    pf_row < end_row && result; 
	    pf_row++, goal_row++)
	{
		for(pf_col = start_col, goal_col = 0; 
		    pf_col < end_col && result; 
		    pf_col++, goal_col++)
		{
			Tile *pf_tile;
			Tile *goal_tile;

			goal_tile = playfield_get_tile(goal->pf, goal_row, goal_col);
			pf_tile = playfield_get_tile (pf, pf_row , pf_col);

			if (goal_tile) {
				if (tile_get_tile_type (goal_tile) == TILE_TYPE_MOVEABLE)
				{
					if (!pf_tile) 
						result = FALSE;
					else if (!tile_is_equal (goal_tile, pf_tile))
						result = FALSE;
				}
			}

			g_object_unref (goal_tile);
			g_object_unref (pf_tile);
		}
	}

	return result;
}


GnomeCanvasItem*
create_small_item (GnomeCanvasGroup *group, gdouble x, gdouble y, Tile* tile)
{
	GdkPixbuf *pixbuf = NULL;
	GnomeCanvasItem *item = NULL;
	Theme* theme;
    
	theme = get_actual_theme ();
	pixbuf = theme_get_tile_image (theme, tile);

	item = gnome_canvas_item_new(group,
				     gnome_canvas_pixbuf_get_type(),
				     "pixbuf", pixbuf,
				     "x", x,
				     "x_in_pixels", TRUE,
				     "y", y,
				     "y_in_pixels", TRUE,
				     "width", 
				     (gdouble)(gdk_pixbuf_get_width (pixbuf))*SCALE_FACTOR,
				     "height", 
				     (gdouble)(gdk_pixbuf_get_height(pixbuf))*SCALE_FACTOR,
				     "anchor", GTK_ANCHOR_NW,
				     NULL);                              
	
	return GNOME_CANVAS_ITEM(item);
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

	canvas = GNOME_CANVAS (get_app ()->ca_goal);
	if(goal->item_group == NULL)
	{
		goal->item_group = create_group_ref(canvas, NULL);
	}


	theme_get_tile_size(get_actual_theme(), 
			    &tile_width, 
			    &tile_height);

	for(row = 0; row < playfield_get_n_rows (goal->pf); row++) 
	{
		for(col = 0; col < playfield_get_n_cols (goal->pf); col++)
		{
			tile = playfield_get_tile (goal->pf, row, col);
			if (!tile) continue;

			type = tile_get_tile_type(tile);
			switch(type)
			{
			case TILE_TYPE_MOVEABLE:
				x = col * tile_width * SCALE_FACTOR;
				y = row * tile_height * SCALE_FACTOR;
				item = create_small_item (goal->item_group, 
							  x, y, tile);

				break;
		
			case TILE_TYPE_OBSTACLE:
			case TILE_TYPE_UNKNOWN:
			default:
			}
			g_object_unref (tile);
		}
	}

	set_background_color_ref (GTK_WIDGET(canvas), 
				  theme_get_background_color (get_actual_theme()));
	
	width = tile_width * playfield_get_n_cols (goal->pf) * SCALE_FACTOR;
	height = tile_height * playfield_get_n_rows (goal->pf) * SCALE_FACTOR;
	gnome_canvas_set_scroll_region(canvas, 0, 0, width, height);
}

void
goal_clear(Goal* goal)
{
	gtk_object_destroy (GTK_OBJECT(goal->item_group));
	goal->item_group = NULL;
}

void goal_init (AtomixApp *app)
{
	set_background_color_ref (app->ca_goal, theme_get_background_color (app->theme));
}

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
#include "main.h"
#include "math.h"
#include "canvas_helper.h"

void set_background_color (GtkWidget *canvas, GdkColor *color)
{
	/* try to alloc color */
	if(gdk_color_alloc(gdk_colormap_get_system(), color))
	{
		GtkStyle *style;
		style = gtk_style_copy(gtk_widget_get_style(GTK_WIDGET(canvas)));

		/* set new style */
		style->bg[GTK_STATE_NORMAL] = *color;
		gtk_widget_set_style(GTK_WIDGET(canvas), style);
	}
}

void convert_to_playfield (Theme *theme, 
			   gdouble x, gdouble y, 
			   guint *row, guint *col)
{
	guint int_y, int_x;
	gint tile_width, tile_height;
	
	theme_get_tile_size(theme, &tile_width, &tile_height);
	
	int_y = (guint) ceil(y);
	*row = (int_y / tile_height);
	
	int_x = (guint) ceil(x);
	*col = (int_x / tile_width);
}


void convert_to_canvas(Theme *theme, 
		       guint row, guint col, 
		       gdouble *x, gdouble *y)
{
	gint tile_width, tile_height;
	
	theme_get_tile_size(theme, &tile_width, &tile_height);
	
	*x = col * tile_width;
	*y = row * tile_height;
}

void set_canvas_dimensions (GnomeCanvas *canvas, gint width, gint height)
{
	gnome_canvas_set_scroll_region(canvas, 0, 0, width, height);    
}

GnomeCanvasGroup* create_group (GnomeCanvas *canvas,
				GnomeCanvasGroup *parent)
{
	GnomeCanvasGroup *group;
	if(parent == NULL)
	{
		parent = gnome_canvas_root(canvas);
	}
	group = GNOME_CANVAS_GROUP(gnome_canvas_item_new(
		parent,
		gnome_canvas_group_get_type(),
		"x", 0.0,
		"y", 0.0,
		NULL));
	return group;
}


CanvasMap* canvas_map_new(void)
{
	CanvasMap *map = g_malloc(sizeof(CanvasMap));

	g_return_val_if_fail(map!=NULL, NULL);

	map->table = g_hash_table_new((GHashFunc) g_direct_hash, 
				      (GCompareFunc) g_direct_equal);

	g_return_val_if_fail(map->table != NULL, NULL);

	return map;
}

void canvas_map_destroy(CanvasMap *map)
{
	g_return_if_fail(map!=NULL);

	g_hash_table_destroy(map->table);
	g_free(map);
}

void canvas_map_set_item(CanvasMap *map, guint row, guint col, GnomeCanvasItem *item)
{
	gint key;

	g_return_if_fail(map != NULL);
	g_return_if_fail(map->table != NULL);
	g_return_if_fail(item != NULL);

	key = (row+1)*100 + col+1;

	g_hash_table_insert(map->table, GINT_TO_POINTER(key), (gpointer) item);
}

GnomeCanvasItem* canvas_map_get_item(CanvasMap *map, guint row, guint col)
{
	GnomeCanvasItem *item;
	gint key;

	g_return_val_if_fail(map != NULL, NULL);
	g_return_val_if_fail(map->table != NULL, NULL);

	key = (row+1)*100 + col+1;
	item = (GnomeCanvasItem*) g_hash_table_lookup(map->table, GINT_TO_POINTER(key));

	return item;
}

void canvas_map_move_item(CanvasMap *map, 
			  guint src_row, guint src_col, 
			  guint dest_row, guint dest_col)
{
	GnomeCanvasItem *item;
	item = canvas_map_get_item(map, src_row, src_col);
	if(item != NULL)
	{
		canvas_map_set_item(map, dest_row, dest_col, item);
		canvas_map_remove_item(map, src_row, src_col);
	}
}

void canvas_map_remove_item(CanvasMap *map, guint row, guint col)
{
	gint key;

	g_return_if_fail(map!=NULL);
	g_return_if_fail(map->table!=NULL);

	key = (row+1)*100 + (col+1);
	
	g_hash_table_remove(map->table, GINT_TO_POINTER(key));
}


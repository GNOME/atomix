/* Atomix -- a little puzzle game about atoms and molecules.
 * Copyright (C) 1999 Jens Finke
 * Copyright (C) 2005 Guilherme de S. Pastore
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "board_gtk.h"

static GtkFixed *board_canvas = NULL;
static Theme *board_theme = NULL;

#define BGR_FLOOR_ROWS 15
#define BGR_FLOOR_COLS 15

static void create_background_floor (void)
{
  int row, col;
  Tile *tile;
  GQuark quark;
  int tile_width, tile_height;
  double x, y;
  GdkPixbuf *pixbuf = NULL;
  GtkWidget *item;
  int ca_width, ca_height;
  int width, height;
  GtkAllocation allocation;

  quark = g_quark_from_static_string ("floor");
  theme_get_tile_size (board_theme, &tile_width, &tile_height);

  tile = tile_new (TILE_TYPE_FLOOR);
  tile_set_base_id (tile, quark);
  pixbuf = theme_get_tile_image (board_theme, tile);
  g_object_unref (tile);

  for (row = 0; row < BGR_FLOOR_ROWS; row++)
    for (col = 0; col < BGR_FLOOR_COLS; col++)
      {
	x = col * tile_width;
	y = row * tile_height;

    item = gtk_image_new_from_pixbuf (pixbuf);
    gtk_fixed_put (GTK_FIXED (board_canvas), item, x, y);
	//item = gnome_canvas_item_new (level_items->floor,
	//			      gnome_canvas_pixbuf_get_type (),
	//			      "pixbuf", pixbuf, "x", x, "x_in_pixels",
	//			      TRUE, "y", y, "y_in_pixels", TRUE,
	//			      "width",
	//			      (double) gdk_pixbuf_get_width (pixbuf),
	//			      "height",
	//			      (double) gdk_pixbuf_get_height (pixbuf),
	//			      "anchor", GTK_ANCHOR_NW, NULL);
      }

  g_object_unref (pixbuf);

  /* center the whole thing */
  gtk_widget_get_allocation (GTK_WIDGET (board_canvas), &allocation);
  ca_width = allocation.width;
  ca_height = allocation.height;

  width = tile_width * BGR_FLOOR_COLS;
  height = tile_height * BGR_FLOOR_ROWS;

  if (width > ca_width)
    {
      x = (width / 2) - (ca_width / 2);
      width = ca_width;
    }
  else
    x = 0;

  if (height > ca_height)
    {
      y = (height / 2) - (ca_height / 2);
      height = ca_height;
    }
  else
    y = 0;
}

void board_gtk_init (Theme * theme, gpointer canvas)
{
  board_theme = theme;
  board_canvas = canvas;

  create_background_floor ();
  gtk_widget_show_all (GTK_WIDGET(board_canvas));
}

void board_gtk_init_level (PlayField * env, PlayField * sce, Goal * goal)
{
}

void board_gtk_destroy (void)
{
}

void board_gtk_clear (void)
{
}

void board_gtk_print (void)
{
}

void board_gtk_hide (void)
{
}

void board_gtk_show (void)
{
}

gboolean board_gtk_undo_move (void)
{
    return FALSE;
}

void board_gtk_show_logo (gboolean visible)
{
}

void board_gtk_handle_key_event (GObject * canvas, GdkEventKey * event,
			     gpointer data)
{
}


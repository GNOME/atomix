/* Atomix -- a little puzzle game about atoms and molecules.
 * Copyright (C) 2001 Jens Finke
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

#include "goal-view.h"

static GtkFixed *goal_fixed;
static Theme *goal_theme;
static GSList *goal_items = NULL;

#define SCALE_FACTOR 0.9

static GtkImage *create_small_item (gdouble x, gdouble y, Tile *tile);
static void render_view (Goal *goal);

void goal_view_init (Theme *theme, GtkFixed *fixed)
{
  g_return_if_fail (IS_THEME (theme));
  g_return_if_fail (GTK_IS_FIXED (fixed));

  goal_fixed = fixed;
  goal_theme = theme;
}

void goal_view_render (Goal *goal)
{
  goal_view_clear ();

  if (goal != NULL)
    render_view (goal);
}

static void render_view (Goal *goal)
{
  PlayField *pf;
  guint row, col;
  gdouble x;
  gdouble y;
  Tile *tile;
  TileType type;
  gint tile_width, tile_height;

  g_return_if_fail (IS_GOAL (goal));
  g_return_if_fail (GTK_IS_FIXED (goal_fixed));
  g_return_if_fail (IS_THEME (goal_theme));

  theme_get_tile_size (goal_theme, &tile_width, &tile_height);

  pf = goal_get_playfield (goal);

  for (row = 0; row < playfield_get_n_rows (pf); row++)
    {
      for (col = 0; col < playfield_get_n_cols (pf); col++)
	{
	  tile = playfield_get_tile (pf, row, col);
	  if (!tile)
	    continue;

	  type = tile_get_tile_type (tile);

	  switch (type)
	    {
	    case TILE_TYPE_ATOM:
	      x = col * tile_width * SCALE_FACTOR;
	      y = row * tile_height * SCALE_FACTOR;
	      create_small_item (x, y, tile);

	      break;

	    case TILE_TYPE_WALL:
	    case TILE_TYPE_UNKNOWN:
	    case TILE_TYPE_NONE:
	    case TILE_TYPE_FLOOR:
	    case TILE_TYPE_SHADOW:
	    case TILE_TYPE_LAST:
	    default:
	      break;
	    }
	  g_object_unref (tile);
	}
    }

  g_object_unref (pf);
}

static void remove_child (GtkWidget *widget, gpointer data)
{
  gtk_container_remove (GTK_CONTAINER (goal_fixed), widget);
}

void goal_view_clear (void)
{
  g_slist_foreach (goal_items, (GFunc) remove_child, NULL);
  g_slist_free (goal_items);
  goal_items = NULL;
}

static GtkImage *create_small_item (gdouble x, gdouble y, Tile *tile)
{
  GdkPixbuf *pixbuf = NULL;
  GdkPixbuf *small_pb = NULL;
  GtkWidget *item = NULL;

  g_return_val_if_fail (IS_TILE (tile), NULL);

  pixbuf = theme_get_tile_image (goal_theme, tile);

  small_pb = gdk_pixbuf_scale_simple (pixbuf,
				      gdk_pixbuf_get_width (pixbuf) *
				      SCALE_FACTOR,
				      gdk_pixbuf_get_height (pixbuf) *
				      SCALE_FACTOR, GDK_INTERP_BILINEAR);

  item = gtk_image_new_from_pixbuf (small_pb);
  gtk_widget_set_visible (item, TRUE);
  goal_items = g_slist_prepend (goal_items, item);
  gtk_fixed_put (goal_fixed, item, x, y);

  g_object_unref (pixbuf);

  return GTK_IMAGE (item);
}

/* Atomix -- a little puzzle game about atoms and molecules.
 * Copyright (C) 1999-2001 Jens Finke
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
#include "main.h"
#include "math.h"
#include "canvas_helper.h"

void set_background_color (GtkWidget *canvas, GdkColor *color)
{
  /* try to alloc color */
  if (gdk_colormap_alloc_color (gdk_colormap_get_system (), color, FALSE, TRUE))
    {
      GtkStyle *style;
      style = gtk_style_copy (gtk_widget_get_style (GTK_WIDGET (canvas)));

      /* set new style */
      style->bg[GTK_STATE_NORMAL] = *color;
      gtk_widget_set_style (GTK_WIDGET (canvas), style);
    }
}

void convert_to_playfield (Theme *theme, gdouble x, gdouble y,
			   guint *row, guint *col)
{
  guint int_y, int_x;
  gint tile_width, tile_height;

  theme_get_tile_size (theme, &tile_width, &tile_height);

  int_y = (guint) ceil (y);
  *row = (int_y / tile_height);

  int_x = (guint) ceil (x);
  *col = (int_x / tile_width);
}

void convert_to_canvas (Theme *theme, guint row, guint col,
			gdouble *x, gdouble *y)
{
  gint tile_width, tile_height;

  theme_get_tile_size (theme, &tile_width, &tile_height);

  *x = col * tile_width;
  *y = row * tile_height;
}

GnomeCanvasGroup *create_group (GnomeCanvas *canvas, GnomeCanvasGroup *parent)
{
  GnomeCanvasGroup *group;

  if (parent == NULL)
    {
      parent = gnome_canvas_root (canvas);
    }
  group = GNOME_CANVAS_GROUP (gnome_canvas_item_new (parent,
						     gnome_canvas_group_get_type
						     (), "x", 0.0, "y", 0.0,
						     NULL));
  return group;
}

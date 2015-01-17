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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#ifndef _ATOMIX_THEME_H_
#define _ATOMIX_THEME_H_

#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gdk/gdk.h>
#include "tile.h"


#define THEME_TYPE        (theme_get_type ())
#define THEME(o)          (G_TYPE_CHECK_INSTANCE_CAST ((o), THEME_TYPE, Theme))
#define THEME_CLASS(k)    (G_TYPE_CHECK_CLASS_CAST((k), THEME_TYPE, ThemeClass))
#define IS_THEME(o)       (G_TYPE_CHECK_INSTANCE_TYPE ((o), THEME_TYPE))
#define IS_THEME_CLASS(k) (G_TYPE_CHECK_CLASS_TYPE ((k), THEME_TYPE))
#define THEME_GET_CLASS(o)(G_TYPE_INSTANCE_GET_CLASS ((o), THEME_TYPE, ThemeClass))

typedef struct _ThemePrivate ThemePrivate;

typedef struct
{
  GObject parent;
  ThemePrivate *priv;
} Theme;

typedef struct
{
  GObjectClass parent_class;
} ThemeClass;

GType theme_get_type (void);

/* This class can't be directyl instantiated. Use
 * ThemeManager to get an Theme object.
 */

GdkPixbuf *theme_get_tile_image (Theme * theme, Tile * tile);

GdkRGBA *theme_get_background_color (Theme * theme);

GdkPixbuf *theme_get_selector_image (Theme * theme);

void theme_get_selector_arrow_images (Theme * theme,
				      GdkPixbuf ** arrow_images);

void theme_get_tile_size (Theme * theme, gint * width, gint * height);
gchar *theme_get_name (Theme * theme);
gint theme_get_animstep (Theme * theme);

gboolean theme_apply_decoration (Theme * theme, Tile * tile);

#endif /* _ATOMIX_THEME_H_ */

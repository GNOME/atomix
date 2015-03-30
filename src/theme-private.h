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

#ifndef _ATOMIX_THEME_PRIVATE_H_
#define _ATOMIX_THEME_PRIVATE_H_

#include <gdk/gdk.h>
#include "tile.h"

typedef struct
{
  GQuark id;			/* the id */
  gchar *file;			/* file name */
  gboolean loading_failed;	/* if once the image loading failed. */
  GdkPixbuf *image;		/* image */
  gint alpha;			/* alpha value to use for overy-/underlay */
  GSList *decorations;		/* possible decoration images */
} ThemeImage;

struct _ThemePrivate
{
  gchar *name;			/* name of the theme */
  gchar *path;			/* full qualified path to the theme directory */

  gint tile_width;		/* width of each tile */
  gint tile_height;		/* height of each tile */
  gint animstep;		/* number of pixels to move a atom in one 
				   animation step */
  GdkRGBA bg_color;		/* background color */

  GData *images;		/* key/data list for all the images */
};


Theme *theme_new (void);

GQuark theme_add_image (Theme * theme, const gchar * src, gint alpha);

void theme_add_image_decoration (Theme * theme, GQuark base, GQuark decor);

#endif /* _ATOMIX_THEME_PRIVATE_H_ */

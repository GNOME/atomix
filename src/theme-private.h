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

#ifndef _ATOMIX_THEME_PRIVATE_H_
#define _ATOMIX_THEME_PRIVATE_H_

#include "tile.h"

typedef struct 
{
	gint          id;             /* the id */
	gchar         *name;          /* symbolic name */
	gchar         *file;          /* file name */
	gboolean      loading_failed; /* if once the image loading failed.*/
        GdkPixbuf     *image;         /* image */
} ThemeImage;

struct _ThemePrivate 
{
	gchar         *name;          /* name of the theme */
	gchar         *path;          /* full qualified path to the theme directory */

	gint          tile_width;     /* width of each tile */
	gint          tile_height;    /* height of each tile */
	gint          animstep;       /* number of pixels to move a atom in one 
				         animation step */
	GdkColor      bg_color;       /* background color */

	GHashTable    *image_list[TILE_TYPE_UNKNOWN]; /* the different image lists */
	GHashTable    *link_image_list; /* the different link images */
	ThemeImage    *selector;      /* selector image */
		
	/* the following fields are only used by atomixed */
	gboolean      modified;       /* whether the theme is modified */
	gboolean      need_update;    /* whether the referenced levels needs an update */
	gint          last_id[TILE_TYPE_UNKNOWN];  /* the appropriate last id for each image list */
};


Theme* theme_new (void);

void theme_add_link_image (Theme *theme, 
			   const gchar *name, 
			   const gchar *file, 
			   TileLink link);

void theme_add_base_image (Theme *theme, 
			   const gchar *name, 
			   const gchar *file, 
			   TileType tile_type);

void theme_add_base_image_with_id (Theme *theme, 
				   const gchar *name, 
				   const gchar *file, 
				   TileType tile_type,
				   int id);

void theme_set_selector_image (Theme *theme, const gchar *file_name);

#endif /* _ATOMIX_THEME_PRIVATE_H_ */

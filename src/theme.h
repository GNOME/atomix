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

#ifndef _ATOMIX_THEME_H_
#define _ATOMIX_THEME_H_

#include <gnome.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include "tile.h"
#include "global.h"

typedef struct _Theme     Theme;
typedef struct _ThemeElement ThemeElement;

typedef enum
{
	THEME_IMAGE_OBSTACLE,
	THEME_IMAGE_MOVEABLE,
	THEME_IMAGE_LINK,
	THEME_IMAGE_UNDEFINED
} ThemeImageKind;

static const int N_IMG_LISTS = 3;

/**
 * Theme structure.
 */
struct _Theme 
{
	gchar *name;              /* name of the theme */
	gchar *path;              /* full qualified path to the theme directory */
	GHashTable *image_list[3]; /* the different image lists */
	ThemeElement *selector;   /* selector image */
	
	gint tile_width;        /* width of each tile */
	gint tile_height;       /* height of each tile */
	gint animstep;          /* number of pixels to move a atom in one 
				   animation step */
	GdkColor bg_color;      /* background color */

	/* the following fields are only used by atomixed */
	gboolean modified;       /* whether the theme is modified */
	gboolean need_update;    /* whether the referenced levels needs an update */
	gint last_id[3];         /* the appropriate last id for each image list*/
};

struct _ThemeElement
{
	gint id;                /* the id */
	gchar *file;            /* file name */
	gboolean loading_failed;/* if once the image loading failed.*/
        GdkPixbuf *image;       /* image */
	gchar *name;            /* symbolic name */
};

Theme* theme_new(void);

void theme_destroy(Theme* theme);

void theme_destroy_element(ThemeElement *element);

GdkPixbuf* theme_get_tile_image(Theme* theme, Tile *tile);

GdkPixbuf* theme_get_element_image(Theme *theme, ThemeElement *element);

GdkPixbuf* theme_get_selector_image(Theme *theme);

/* returns list of images */
GSList* theme_get_tile_link_images(Theme *theme, Tile *tile);

void theme_set_path(Theme* theme, gchar* path);

void theme_set_selector_image(Theme *theme, const gchar *file_name);

ThemeElement* theme_add_image(Theme *theme, const gchar *name, 
			      const gchar *file, ThemeImageKind kind);

void theme_remove_element(Theme *theme, ThemeImageKind kind, const ThemeElement *element);

gboolean
 theme_change_element_id(Theme *theme, ThemeImageKind kind, ThemeElement *element, gint id);

gboolean theme_does_id_exist(Theme *theme, ThemeImageKind kind, gint id);

Theme* theme_load_xml(char* name);

void theme_save_xml(Theme *theme, gchar *filename);

void theme_create_hash_table(void);

void theme_destroy_hash_table(void);

GdkColor* theme_get_background_color(Theme *theme);

void theme_get_tile_size(Theme *theme, gint *width, gint *height);

GList* theme_get_available_themes(void);

#endif /* _ATOMIX_THEME_H_ */

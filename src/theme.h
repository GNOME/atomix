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

#ifndef _ATOMIX_THEME_H_
#define _ATOMIX_THEME_H_

#include <gnome.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include "tile.h"


#define THEME_TYPE        (theme_get_type ())
#define THEME(o)          (G_TYPE_CHECK_INSTANCE_CAST ((o), THEME_TYPE, Theme))
#define THEME_CLASS(k)    (G_TYPE_CHECK_CLASS_CAST((k), THEME_TYPE, ThemeClass))
#define IS_THEME(o)       (G_TYPE_CHECK_INSTANCE_TYPE ((o), THEME_TYPE))
#define IS_THEME_CLASS(k) (G_TYPE_CHECK_CLASS_TYPE ((k), THEME_TYPE))
#define THEME_GET_CLASS(o)(G_TYPE_INSTANCE_GET_CLASS ((o), THEME_TYPE, ThemeClass))

typedef struct _ThemePrivate ThemePrivate;

typedef struct {
	GObject parent;
	ThemePrivate *priv;
} Theme;

typedef struct {
	GObjectClass parent_class;
} ThemeClass;

/* This class can't be directyl instantiated. Use
 * ThemeManager to get an Theme object.
 */

GdkPixbuf* theme_get_tile_image       (Theme* theme, 
				       Tile *tile);

GdkColor*  theme_get_background_color (Theme *theme);

GdkPixbuf* theme_get_selector_image   (Theme *theme);

void       theme_get_tile_size        (Theme *theme, 
				       gint *width, 
				       gint *height);
gchar*     theme_get_name (Theme *theme);
gint       theme_get_animstep (Theme *theme);

/* editor functions */
/* these aren't used yet */
#if 0
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

#endif

#endif /* _ATOMIX_THEME_H_ */

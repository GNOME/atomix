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

#ifndef _ATOMIX_THEME_MANAGER_H_
#define _ATOMIX_THEME_MANAGER_H_

#include <glib-object.h>
#include "theme.h"

#define THEME_MANAGER_TYPE        (theme_manager_get_type ())
#define THEME_MANAGER(o)          (G_TYPE_CHECK_INSTANCE_CAST ((o), THEME_MANAGER_TYPE, ThemeManager))
#define THEME_MANAGER_CLASS(k)    (G_TYPE_CHECK_CLASS_CAST((k), THEME_MANAGER_TYPE, ThemeManager))
#define IS_THEME_MANAGER(o)       (G_TYPE_CHECK_INSTANCE_TYPE ((o), THEME_MANAGER_TYPE))
#define IS_THEME_MANAGER_CLASS(k) (G_TYPE_CHECK_CLASS_TYPE ((k), THEME_MANAGER_TYPE))
#define THEME_MANAGER_GET_CLASS(o)(G_TYPE_INSTANCE_GET_CLASS ((o), THEME_MANAGER_TYPE, ThemeManagerClass))

typedef struct _ThemeManagerPrivate ThemeManagerPrivate;

typedef struct
{
  GObject parent;
  ThemeManagerPrivate *priv;
} ThemeManager;

typedef struct
{
  GObjectClass parent_class;
} ThemeManagerClass;

GType theme_manager_get_type (void);

ThemeManager *theme_manager_new (void);

void theme_manager_init_themes (ThemeManager * tm);

Theme *theme_manager_get_theme (ThemeManager * tm, const gchar * theme_name);

GList *theme_manager_get_available_themes (ThemeManager * tm);

#endif /* _ATOMIX_THEME_MANAGER_H_ */

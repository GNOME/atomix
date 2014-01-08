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
#ifndef _ATOMIX_LEVEL_MANAGER_H_
#define _ATOMIX_LEVEL_MANAGER_H_

#include <glib-object.h>
#include "level.h"

#define LEVEL_MANAGER_TYPE        (level_manager_get_type ())
#define LEVEL_MANAGER(o)          (G_TYPE_CHECK_INSTANCE_CAST ((o), LEVEL_MANAGER_TYPE, LevelManager))
#define LEVEL_MANAGER_CLASS(k)    (G_TYPE_CHECK_CLASS_CAST((k), LEVEL_MANAGER_TYPE, LevelManager))
#define IS_LEVEL_MANAGER(o)       (G_TYPE_CHECK_INSTANCE_TYPE ((o), LEVEL_MANAGER_TYPE))
#define IS_LEVEL_MANAGER_CLASS(k) (G_TYPE_CHECK_CLASS_TYPE ((k), LEVEL_MANAGER_TYPE))
#define LEVEL_MANAGER_GET_CLASS(o)(G_TYPE_INSTANCE_GET_CLASS ((o), LEVEL_MANAGER_TYPE, LevelManagerClass))

typedef struct _LevelManagerPrivate LevelManagerPrivate;

typedef struct
{
  GObject parent;
  LevelManagerPrivate *priv;
} LevelManager;

typedef struct
{
  GObjectClass parent_class;
} LevelManagerClass;

GType level_manager_get_type (void);

LevelManager *level_manager_new (void);

void level_manager_init_levels (LevelManager * lm);

Level *level_manager_get_next_level (LevelManager * lm,
				     Level * current_level);

GList *level_manager_get_available_levels (LevelManager * lm);

gboolean level_manager_is_last_level (LevelManager * lm, Level * level);

#endif /* _ATOMIX_LEVEL_MANAGER_H_ */

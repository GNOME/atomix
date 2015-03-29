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

#ifndef _ATOMIX_LEVEL_H 
#define _ATOMIX_LEVEL_H 

#include "playfield.h"

#define LEVEL_TYPE        (level_get_type ())
#define LEVEL(o)          (G_TYPE_CHECK_INSTANCE_CAST ((o), LEVEL_TYPE, Level))
#define LEVEL_CLASS(k)    (G_TYPE_CHECK_CLASS_CAST((k), LEVEL_TYPE, LevelClass))
#define IS_LEVEL(o)       (G_TYPE_CHECK_INSTANCE_TYPE ((o), LEVEL_TYPE))
#define IS_LEVEL_CLASS(k) (G_TYPE_CHECK_CLASS_TYPE ((k), LEVEL_TYPE))
#define LEVEL_GET_CLASS(o)(G_TYPE_INSTANCE_GET_CLASS ((o), LEVEL_TYPE, LevelClass))

typedef struct _LevelPrivate LevelPrivate;

typedef struct  {
	GObject parent;
	LevelPrivate *priv;
} Level;


typedef struct {
	GObjectClass parent_class;
} LevelClass;

GType level_get_type (void);

gchar *level_get_name (Level *level);

gchar *level_get_formula (Level *level);

PlayField* level_get_environment (Level *level);

PlayField* level_get_scenario (Level *level);

PlayField* level_get_goal (Level *level);

void
level_parser_start_element (GMarkupParseContext  *context,
                            const gchar          *element_name,
                            const gchar         **attribute_names,
                            const gchar         **attribute_values,
                            gpointer              user_data,
                            GError              **error);
void
level_parser_end_element (GMarkupParseContext  *context,
                          const gchar          *element_name,
                          gpointer              user_data,
                          GError              **error);
#endif /* _ATOMIX_LEVEL_H */

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

#ifndef _ATOMIX_LEVEL_H 
#define _ATOMIX_LEVEL_H 

#include <gnome.h>
#include "global.h"
#include "playfield.h"
#include "theme.h"

typedef struct _Level       Level;

struct _Level
{
	gchar*       name;         /* name of the level */
	guint        time;         /* time to solve */
	gchar*       theme_name;   /* name of the used theme */
	PlayField*   goal;         /* determines the end of the level */
	PlayField*   playfield;    /* starting situation */
	gchar*       next;         /* name of the following level */
	gboolean     first_level;  /* whether this is the first level */
	gboolean     bonus_level;  /* whether this is a bonus level */

	/* the following fields are only used by atomixed */
	gchar *file_name;          /* file name of the level */
	gboolean modified;         /* whether the level is modified */
};

GHashTable* available_levels;

Level* level_new(void);

void level_destroy(Level* l);

void level_set_last_level(Level *level);

gboolean level_is_last_level(Level *level);

Level* level_load_xml(gchar* name);

Level* level_load_xml_file(gchar *file_path);

void level_save_xml(Level* level, gchar* filename);

/*-----------------------------------------------*/

void level_create_hash_table(void);

void level_add_to_hash_table(gchar *name, gchar *file_path);

gboolean level_is_name_already_present(gchar *name);

void level_destroy_hash_table(void);

gchar* level_get_first_level(void);


#endif /* _ATOMIX_LEVEL_H */

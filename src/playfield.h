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

#ifndef _ATOMIX_PLAY_FIELD_H 
#define _ATOMIX_PLAY_FIELD_H 

#include <gnome.h>
#include <libxml/tree.h>
#include "global.h"
#include "tile.h"

#define PLAYFIELD_TYPE        (playfield_get_type ())
#define PLAYFIELD(o)          (G_TYPE_CHECK_INSTANCE_CAST ((o), PLAYFIELD_TYPE, PlayField))
#define PLAYFIELD_CLASS(k)    (G_TYPE_CHECK_CLASS_CAST((k), PLAYFIELD_TYPE, PlayFieldClass))
#define IS_PLAYFIELD(o)       (G_TYPE_CHECK_INSTANCE_TYPE ((o), PLAYFIELD_TYPE))
#define IS_PLAYFIELD_CLASS(k) (G_TYPE_CHECK_CLASS_TYPE ((k), PLAYFIELD_TYPE))
#define PLAYFIELD_GET_CLASS(o)(G_TYPE_INSTANCE_GET_CLASS ((o), PLAYFIELD_TYPE, PlayFieldClass))


typedef struct _PlayFieldPrivate  PlayFieldPrivate;

typedef struct
{
	GObject parent;
	PlayFieldPrivate *priv;
} PlayField;

typedef struct {
	GObjectClass parent_class;
} PlayFieldClass;

GType playfield_get_type (void);

PlayField* playfield_new(void);

guint playfield_get_n_rows (PlayField *pf);

guint playfield_get_n_cols (PlayField *pf);

void playfield_add_row(PlayField* pf);

void playfield_add_column(PlayField* pf);

Tile* playfield_get_tile(PlayField *pf, guint row, 
				   guint col);

void playfield_set_tile(PlayField* pf, 
			guint row, guint col, 
			Tile *tile);

void playfield_set_matrix_size(PlayField *pf, guint n_rows, guint n_cols);

PlayField* playfield_strip (PlayField *pf);

PlayField* playfield_copy (PlayField *pf);

Tile* playfield_clear_tile(PlayField *pf, guint row, guint col);

void playfield_swap_tiles(PlayField *pf, guint src_row, guint src_col,
			  guint dest_row, guint dest_col);

void playfield_print(PlayField *pf);

void playfield_clear(PlayField *pf);

PlayField* playfield_load_xml(xmlNodePtr pf_node, gint revision);

void playfield_save_xml(PlayField *pf, xmlNodePtr pf_node);

#endif /* _ATOMIX_PLAY_FIELD_H */

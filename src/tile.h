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

#ifndef _TILE_H_ 
#define _TILE_H_ 

#include <gnome.h>
#include <libxml/tree.h>

#define TILE_TYPE        (tile_get_type ())
#define TILE(o)          (G_TYPE_CHECK_INSTANCE_CAST ((o), TILE_TYPE, Tile))
#define TILE_CLASS(k)    (G_TYPE_CHECK_CLASS_CAST((k), TILE_TYPE, TileClass))
#define IS_TILE(o)       (G_TYPE_CHECK_INSTANCE_TYPE ((o), TILE_TYPE))
#define IS_TILE_CLASS(k) (G_TYPE_CHECK_CLASS_TYPE ((k), TILE_TYPE))
#define TILE_GET_CLASS(o)(G_TYPE_INSTANCE_GET_CLASS ((o), TILE_TYPE, TileClass))

typedef struct _TilePrivate  TilePrivate;

typedef enum 
{
	TILE_TYPE_NONE,
	TILE_TYPE_ATOM,
	TILE_TYPE_WALL,
	TILE_TYPE_FLOOR,
	TILE_TYPE_SHADOW,
	TILE_TYPE_UNKNOWN,
	TILE_TYPE_LAST
} TileType;

typedef enum {
	TILE_SUB_OVERLAY,
	TILE_SUB_UNDERLAY
} TileSubType;

typedef struct {
	GObject     parent;
	TilePrivate *priv;
} Tile;

typedef struct {
	GObjectClass parent_class;
} TileClass;

GType tile_get_type (void);

Tile* tile_new (TileType type);

Tile* tile_new_from_xml (xmlNodePtr node);

Tile* tile_copy (Tile *tile);

GSList* tile_get_sub_ids (Tile *tile, TileSubType sub_type);

GQuark tile_get_base_id (Tile *tile);

TileType tile_get_tile_type (Tile *tile);

void tile_add_sub_id (Tile *tile, GQuark id, TileSubType sub_type);

void tile_remove_sub_id (Tile *tile, guint id, TileSubType sub_type);

void tile_remove_all_sub_ids (Tile *tile, TileSubType sub_type);

void tile_set_base_id (Tile *tile, GQuark id);

void tile_set_tile_type (Tile *tile, TileType type);

void tile_print (Tile *tile);

gboolean tile_is_equal (Tile *tile, Tile *comp);

#if 0
void tile_save_xml (Tile *tile, xmlNodePtr parent);
#endif 

#endif /* _TILE_H */

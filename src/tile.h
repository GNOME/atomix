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

#ifndef _TILE_H_ 
#define _TILE_H_ 

#include <gnome.h>
#include <libxml/tree.h>


typedef struct _Tile  Tile;

typedef enum 
{
        TILE_NONE,
	TILE_MOVEABLE,
	TILE_OBSTACLE,
	TILE_DECOR    
} TileType;


struct _Tile
{
	TileType type;
	guint    img_id;
	GSList*  conn_ids;   
};


Tile* tile_new(void);

Tile* tile_new_args(TileType type, guint img_id, GSList *conn_id);

void tile_destroy(Tile *tile);

Tile* tile_copy(Tile *tile);

void tile_print(Tile *tile);

GSList* tile_get_link_ids(Tile *tile);

gint tile_get_image_id(Tile *tile);

guint tile_get_unique_id(Tile *tile);

TileType tile_get_type(Tile *tile);

void tile_add_link_id(Tile *tile, gint id);

void tile_remove_link_id(Tile *tile, gint id);

void tile_delete_all_links(Tile *tile);

void tile_set_image_id(Tile *tile, gint id);

void tile_set_type(Tile *tile, TileType type);

Tile* tile_load_xml(xmlNodePtr node, gint revision);

void tile_save_xml(Tile *tile, xmlNodePtr parent);

void tile_set_values(Tile *dest, Tile *src);
#endif /* _TILE_H */

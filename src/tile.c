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

#include "tile.h"

/*=================================================================
 
  Tile creation, initialisation and clean up

  ---------------------------------------------------------------*/

Tile*
tile_new(void)
{
	return tile_new_args(TILE_NONE, 0, 0);
}

Tile*
tile_new_args(TileType type, guint img_id, GSList *conn_ids)
{
	Tile *tile;

	tile = g_malloc(sizeof(Tile));

	if(tile)
	{
		gint i;
		tile->type = type;
		tile->img_id = img_id;

		/* copy link values */
		tile->conn_ids = NULL;
		if(conn_ids)
		{
			for(i = 0; i < g_slist_length(conn_ids); i++)
			{
				gint value = GPOINTER_TO_INT(g_slist_nth_data(conn_ids, i));
				tile->conn_ids = g_slist_append(tile->conn_ids, 
								GINT_TO_POINTER(value));
			}
		}
	}
#ifdef DEBUG
	else
	{
		g_print("Not enough memory for Tile allocation!\n");
	}
#endif

	return tile;
}

void
tile_destroy(Tile *tile)
{
	if(tile)
	{
		tile_delete_all_links(tile);
		g_free(tile);
	}
}

/*=================================================================
 
  Tile functions

  ---------------------------------------------------------------*/
Tile*
tile_copy(Tile *tile)
{
	Tile *tile_copy;
	tile_copy = tile_new_args(tile->type, 
				  tile->img_id, 
				  tile->conn_ids);
	g_assert(tile_copy!=NULL);

	return tile_copy;
}

GSList*
tile_get_link_ids(Tile *tile)
{
	if(tile)
	{
		return tile->conn_ids;
	}

	return 0;
}

gint 
tile_get_image_id(Tile *tile)
{
	if(tile)
	{
		return tile->img_id;
	}

	return 0;
}

TileType 
tile_get_type(Tile *tile)
{
	if(tile)
	{
		return tile->type;
	}

	return TILE_NONE;
}

guint tile_get_unique_id(Tile *tile)
{
	gint id;

	if(tile)
	{
		gint i;
		id = (1000 * (gint)tile->type) + (100 * tile->img_id);

		for(i = 0; i < g_slist_length(tile->conn_ids); i++)
		{
			gint value = GPOINTER_TO_INT(g_slist_nth_data(tile->conn_ids, i));
			id += value;
		}		
	}
	else
	{
		id = -1;
	}
	return id;
}

void tile_set_values(Tile *dest, Tile *src)
{
	int i;
	dest->type = src->type;
	dest->img_id = src->img_id;

	/* copy link values */
	dest->conn_ids = NULL;
	for(i = 0; i < g_slist_length(src->conn_ids); i++)
	{
		gint value = GPOINTER_TO_INT(g_slist_nth_data(src->conn_ids, i));
		dest->conn_ids = g_slist_append(dest->conn_ids, GINT_TO_POINTER(value));
	}
}

void tile_add_link_id(Tile *tile, gint id)
{
        if(tile)
	{
		GSList *element; 
		
		/* don't add a id twice */
		element = g_slist_find(tile->conn_ids, GINT_TO_POINTER(id));
		if(element == NULL)
		{
			tile->conn_ids = g_slist_append(tile->conn_ids, 
							GINT_TO_POINTER(id));
		}
	}
}

void tile_remove_link_id(Tile *tile, gint id)
{
        if(tile)
	{
		GSList *element;
		element = g_slist_find(tile->conn_ids, GINT_TO_POINTER(id));
		tile->conn_ids = g_slist_remove_link(tile->conn_ids, element);
	}
}


void tile_delete_all_links(Tile *tile)
{
	if(tile && tile->conn_ids)
	{
		g_slist_free(tile->conn_ids);			
	}
	tile->conn_ids = NULL;
}


void tile_set_image_id(Tile *tile, gint id)
{
	if(tile)
	{
		tile->img_id = id;
	}
}

void tile_set_type(Tile *tile, TileType type)
{
	if(tile)
	{
		tile->type = type;
	}
}

void
tile_print(Tile *tile)
{
	if(tile != NULL)
	{
		gchar *type;
		switch(tile->type)
		{
		case TILE_NONE:
			type = "NONE"; break;
		case TILE_MOVEABLE:
			type = "MOVEABLE"; break;
		case TILE_OBSTACLE:
			type = "OBSTACLE"; break;
		case TILE_DECOR:
			type = "DECOR"; break;
		default:
			type = "OTHER"; break;
		}
		
		g_print("Tile TYPE: %s  IMG_ID: %i", type, tile->img_id);
		if(tile->conn_ids)
		{
			gint length = g_slist_length(tile->conn_ids);
			GSList *list = tile_get_link_ids(tile);

			g_print("\n    LENGTH: %i", length);
			while(list)
			{
				gint value = GPOINTER_TO_INT(list->data);
				g_print("\n     CONN_ID: %i", value);
				list = list->next;
			}
			g_print("\n");
		}
		else
		{
			g_print("  No CONN_IDs");
		}

		g_print("  UNIQUE_ID: %i\n", tile_get_unique_id(tile)); 

	}
	else
	{
		g_print("tile_print(): Tile pointer is NULL.\n");
	}
}

/*=================================================================
 
  Tile load/save functions

  ---------------------------------------------------------------*/
Tile*
tile_load_xml(xmlNodePtr node, gint revision)
{
	xmlNodePtr child;
	Tile *tile = NULL;
	TileType type;
	gint img_id;
	gint conn_id;
	gchar *prop_value;

	if(revision < 2)
	{	    
		prop_value = xmlGetProp(node, "type");
		type = atoi(prop_value);
		    
		prop_value = xmlGetProp(node, "imgid");
		img_id = atoi(prop_value);

		tile = tile_new_args(type, img_id, 0);
	}
	else
	{
		gchar *content;
		child = node->xmlChildrenNode;
		tile = tile_new();

		while(child)
		{
			if(g_strcasecmp(child->name, "TYPE")==0)
			{
				/* handle tile type */
				content = xmlNodeGetContent(child);
				type = atoi(content);
				tile_set_type(tile, (TileType)type);
				g_free(content);
			}
			
			else if(g_strcasecmp(child->name,"IMG_ID")==0)
			{
				/* handle img id node */
				content = xmlNodeGetContent(child);
				img_id = atoi(content);
				tile_set_image_id(tile, img_id);
				g_free(content);
			}

			else if(g_strcasecmp(child->name,"CONN_ID")==0)
			{
				/* handle time node */
				content = xmlNodeGetContent(child);
				conn_id = atoi(content);
				tile_add_link_id(tile, conn_id);
				g_free(content);
			}
			
			else
			{
				g_print("tile.c: Unknown TAG, ignoring <%s>\n",
					child->name);
			}

			child = child->next;
		}
	}	

	return tile;
}


void tile_save_xml(Tile *tile, xmlNodePtr tile_node)
{	
	xmlNodePtr child;
	gchar *str_buffer; 
	gint length;
	GSList *conn_id;
	TileType type;
	gint img_id;

	str_buffer = g_malloc(5*sizeof(gchar));
	
	if(tile && tile_node)
	{
		/* add tile type */
		type = tile_get_type(tile);
		length = g_snprintf(str_buffer, 5, "%i", (gint)type);
		child = xmlNewChild(tile_node, NULL, "TYPE", str_buffer);
		
		/* add image id */
		img_id = tile_get_image_id(tile);
		length = g_snprintf(str_buffer, 5, "%i", img_id);
		child = xmlNewChild(tile_node, NULL, "IMG_ID", str_buffer);
		
		/* add link id */
		conn_id = tile->conn_ids;
		while(conn_id)
		{
			gint value = GPOINTER_TO_INT(conn_id->data);
			length = g_snprintf(str_buffer, 5, "%i", value);
			child = xmlNewChild(tile_node, NULL, "CONN_ID", str_buffer);
			conn_id = conn_id->next;
		}
	}

	g_free(str_buffer);
}




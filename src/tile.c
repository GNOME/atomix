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

#include "tile.h"

static GObjectClass     *parent_class = NULL;

static void tile_class_init (GObjectClass *class);
static void tile_init (Tile *tile);
static void tile_finalize (GObject *object);

struct _TilePrivate {
	TileType type;
	guint    base_id;
        gboolean link_id[TILE_LINK_LAST];
};


GType
tile_get_type (void)
{
  static GType object_type = 0;

  if (!object_type)
    {
      static const GTypeInfo object_info =
      {
	sizeof (TileClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) tile_class_init,
	NULL,   /* clas_finalize */
	NULL,   /* class_data */
	sizeof(Tile),
	0,      /* n_preallocs */
	(GInstanceInitFunc) tile_init,
      };

      object_type = g_type_register_static (G_TYPE_OBJECT,
					    "Tile",
					    &object_info, 0);
    }

  return object_type;
}

/*=================================================================
 
  Tile creation, initialisation and clean up

  ---------------------------------------------------------------*/

static void 
tile_class_init (GObjectClass *class)
{
	parent_class = g_type_class_ref (G_TYPE_OBJECT);
	class->finalize = tile_finalize;
}

static void 
tile_init (Tile *tile)
{
	TilePrivate *priv;
	int i;

	priv = g_new0 (TilePrivate, 1);
	priv->type = TILE_TYPE_UNKNOWN;
	priv->base_id = 0;
	for (i = 0; i < TILE_LINK_LAST; i++) priv->link_id[i] = FALSE;
	
	tile->priv = priv;
}

static void 
tile_finalize (GObject *object)
{
	Tile* tile = TILE (object);
	g_free (tile->priv);
	tile->priv = NULL;
}

Tile*
tile_new (TileType type)
{
	Tile *tile;
	tile = TILE (g_object_new (TILE_TYPE, NULL));
	tile->priv->type = type;
	return tile;
}

/*=================================================================
 
  Tile functions

  ---------------------------------------------------------------*/
Tile*
tile_copy(Tile *tile)
{
	Tile *tile_copy;
	int i;
	
	g_return_val_if_fail (IS_TILE (tile), NULL);

	tile_copy = tile_new (tile->priv->type); 
	
	tile_copy->priv->base_id = tile->priv->base_id;
	for (i = 0; i < TILE_LINK_LAST; i++) 
		tile_copy->priv->link_id[i] = tile->priv->link_id[i];

	return tile_copy;
}

gboolean
tile_has_link (Tile *tile, TileLink link)
{
	g_return_if_fail (IS_TILE (tile));

	if (tile->priv->type == TILE_TYPE_MOVEABLE)
		return tile->priv->link_id[link];
	else
		return FALSE;
}

gint 
tile_get_base_id(Tile *tile)
{
	g_return_val_if_fail (IS_TILE (tile), 0);

	return tile->priv->base_id;
}

TileType 
tile_get_tile_type (Tile *tile)
{
	g_return_val_if_fail (IS_TILE (tile), TILE_TYPE_UNKNOWN);

	return tile->priv->type;
}

void tile_add_link (Tile *tile, TileLink link)
{
	g_return_if_fail (IS_TILE (tile));
	if (tile->priv->type != TILE_TYPE_MOVEABLE) return;
	
	tile->priv->link_id[link] = TRUE;
}


void tile_remove_link (Tile *tile, TileLink link)
{
	g_return_if_fail (IS_TILE (tile));
	if (tile->priv->type != TILE_TYPE_MOVEABLE) return;

	tile->priv->link_id[link] = FALSE;
}


void tile_remove_all_links(Tile *tile)
{
	int i;

	g_return_if_fail (IS_TILE (tile));
	if (tile->priv->type != TILE_TYPE_MOVEABLE) return;

	for (i = 0; i < TILE_LINK_LAST; i++) 
		tile->priv->link_id[i] = FALSE;
}


void tile_set_base_id(Tile *tile, gint id)
{
	g_return_if_fail (IS_TILE (tile));
	
	tile->priv->base_id = id;
}

void tile_set_type(Tile *tile, TileType type)
{
	g_return_if_fail (IS_TILE (tile));
	
	tile->priv->type = type;

	if (type != TILE_TYPE_MOVEABLE)
		tile_remove_all_links (tile);
}

void
tile_print(Tile *tile)
{
	gchar *type_str;
	gint16 link;
	int i;
	
	g_return_if_fail (IS_TILE (tile));
	
	switch(tile->priv->type)
	{
	case TILE_TYPE_UNKNOWN:
		type_str = "NONE"; break;
	case TILE_TYPE_MOVEABLE:
		type_str = "MOVEABLE"; break;
	case TILE_TYPE_OBSTACLE:
		type_str = "OBSTACLE"; break;
	case TILE_TYPE_DECOR:
		type_str = "DECOR"; break;
	default:
		type_str = "UNKNOWN"; break;
	}
	
	g_print("Tile TYPE: %s  BASE_ID: %i  LINK_IDs: ", 
		type_str, tile->priv->base_id);

	for (link = 0; link < TILE_LINK_LAST ; link++) {
		gchar *tile_name;

		if (tile->priv->link_id[link]){
			switch (link) {
			case TILE_LINK_LEFT: 
				g_print ("left, "); break;
			case TILE_LINK_RIGHT: 
				g_print ("right, "); break;
			case TILE_LINK_TOP:
				g_print ("top, "); break;
			case TILE_LINK_BOTTOM: 
				g_print ("bottom,"); break;
			case TILE_LINK_TOP_LEFT: 
				g_print ("top-left, "); break;
			case TILE_LINK_TOP_RIGHT: 
				g_print ("top-right, "); break;
			case TILE_LINK_BOTTOM_LEFT: 
				g_print ("bottom-left, "); break;
			case TILE_LINK_BOTTOM_RIGHT: 
				g_print ("bottom-right, "); break;
			case TILE_LINK_LEFT_DOUBLE:
				g_print ("left-double, "); break;
			case TILE_LINK_RIGHT_DOUBLE:
				g_print ("right-double, "); break;
			case TILE_LINK_TOP_DOUBLE:
				g_print ("top-double, "); break;
			case TILE_LINK_BOTTOM_DOUBLE:
				g_print ("bottom-double, "); break;
			default:
				g_print ("Unknown, "); break;
			}
		}
	}
}


/*=================================================================
 
  Tile load/save functions

  ---------------------------------------------------------------*/
#if 0
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

#endif



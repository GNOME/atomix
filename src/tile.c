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

#define GPOINTER_TO_QUARK(p)  ((GQuark) (p))
#define GQUARK_TO_POINTER(p)  ((gpointer) (p))

static GObjectClass     *parent_class = NULL;

static void tile_class_init (GObjectClass *class);
static void tile_init (Tile *tile);
static void tile_finalize (GObject *object);

struct _TilePrivate {
	TileType type;
	GQuark   base_id;
	GSList   *sub_id_list[2];
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

	priv = g_new0 (TilePrivate, 1);
	priv->type = TILE_TYPE_UNKNOWN;
	priv->base_id = 0;
	priv->sub_id_list[TILE_SUB_OVERLAY] = NULL;
	priv->sub_id_list[TILE_SUB_UNDERLAY] = NULL;

	tile->priv = priv;
}

static void 
tile_finalize (GObject *object)
{
	Tile* tile = TILE (object);

	g_slist_free (tile->priv->sub_id_list[TILE_SUB_OVERLAY]);
	g_slist_free (tile->priv->sub_id_list[TILE_SUB_UNDERLAY]);
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
	GSList *sub_id;
	Tile *tile_copy;
	int id, i;
	
	g_return_val_if_fail (IS_TILE (tile), NULL);

	tile_copy = tile_new (tile->priv->type); 
	
	tile_copy->priv->base_id = tile->priv->base_id;

	for (i = 0; i < 2; i++) {
		sub_id = tile->priv->sub_id_list[i];
		for (; sub_id != NULL; sub_id = sub_id->next) {
			id = GPOINTER_TO_QUARK (sub_id->data);
			tile_copy->priv->sub_id_list[i] = 
				g_slist_append (tile_copy->priv->sub_id_list[i], 
						GQUARK_TO_POINTER (id));
		}
	}
		
	return tile_copy;
}

GSList*
tile_get_sub_ids (Tile *tile, TileSubType sub_type)
{
	g_return_val_if_fail (IS_TILE (tile), NULL);
	
	return tile->priv->sub_id_list[sub_type];
}

GQuark 
tile_get_base_id (Tile *tile)
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

static gint
list_compare_func (gconstpointer a, gconstpointer b)
{
	return GPOINTER_TO_QUARK (a) - GPOINTER_TO_QUARK (b);
}


void
tile_add_sub_id (Tile *tile, guint id, TileSubType sub_type)
{
	g_return_if_fail (IS_TILE (tile));

	tile->priv->sub_id_list[sub_type] = 
		g_slist_insert_sorted (tile->priv->sub_id_list[sub_type],
				       GQUARK_TO_POINTER (id),
				       list_compare_func);
}


void 
tile_remove_sub_id (Tile *tile, guint id, TileSubType sub_type)
{
	GSList *result;

	g_return_if_fail (IS_TILE (tile));

	result = g_slist_find (tile->priv->sub_id_list[sub_type],
			       GQUARK_TO_POINTER (id));
	if (result != NULL)
		tile->priv->sub_id_list[sub_type] =
			g_slist_delete_link (tile->priv->sub_id_list[sub_type],
					     result);
}


void 
tile_remove_all_sub_ids (Tile *tile, TileSubType sub_type)
{
	g_return_if_fail (IS_TILE (tile));

	g_slist_free (tile->priv->sub_id_list[sub_type]);
	tile->priv->sub_id_list[sub_type] = NULL;
}


void 
tile_set_base_id (Tile *tile, GQuark id)
{
	g_return_if_fail (IS_TILE (tile));
	
	tile->priv->base_id = id;
}

gboolean
tile_is_equal (Tile *tile, Tile *comp)
{
	GSList *elem;
	GSList *comp_elem;
	gint i;

	g_return_val_if_fail (IS_TILE (tile), FALSE);
	g_return_val_if_fail (IS_TILE (comp), FALSE);

	if (tile->priv->type != comp->priv->type)
		return FALSE;

	if (tile->priv->base_id != comp->priv->base_id) 
		return FALSE;

	for (i = 0; i < 2; i++) {
		elem = tile->priv->sub_id_list[i];
		comp_elem = comp->priv->sub_id_list[i];
		for (; elem != NULL && comp_elem != NULL; 
		     elem = elem->next, comp_elem = comp_elem->next) 
		{
			if (GPOINTER_TO_QUARK (elem->data) != 
			    GPOINTER_TO_QUARK (comp_elem->data))
			    return FALSE;
		}

		if (elem != NULL || comp_elem != NULL)
			return FALSE;
	}

	return TRUE;
}

void tile_set_tile_type (Tile *tile, TileType type)
{
	g_return_if_fail (IS_TILE (tile));
	
	tile->priv->type = type;
}

void
tile_print (Tile *tile)
{
	gchar *type_str;
	
	g_return_if_fail (IS_TILE (tile));
	
	switch(tile->priv->type)
	{
	case TILE_TYPE_NONE:
		type_str = "NONE"; break;
	case TILE_TYPE_ATOM:
		type_str = "ATOM"; break;
	case TILE_TYPE_WALL:
		type_str = "WALL"; break;
	case TILE_TYPE_FLOOR:
		type_str = "FLOR"; break;
	default:
		type_str = "UKWN"; break;
	}
	
	g_print ("%s ", type_str);
#if 0
	g_print("Tile TYPE: %s  BASE_ID: %i  LINK_IDs: ", 
		type_str, tile->priv->base_id);

	for (link = 0; link < TILE_LINK_LAST ; link++) {
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
#endif
}


/*=================================================================
 
  Tile load/save functions

  ---------------------------------------------------------------*/

static TileType
string_to_tile_type (gchar *str)
{
	TileType tile_type = TILE_TYPE_UNKNOWN;
	static int prefix_len = 0;
	if (!prefix_len) prefix_len = strlen ("TILE_TYPE_");
	
	g_return_val_if_fail (str != NULL, tile_type);

	str += prefix_len;
	
	if (!g_strcasecmp (str, "FLOOR"))
		tile_type = TILE_TYPE_FLOOR;
	else if (!g_strcasecmp (str, "WALL"))
		tile_type = TILE_TYPE_WALL;
	else if (!g_strcasecmp (str, "ATOM"))
		tile_type = TILE_TYPE_ATOM;
	else 
		tile_type = TILE_TYPE_UNKNOWN;
	
	return tile_type;
}

Tile*
tile_new_from_xml (xmlNodePtr node)
{
	xmlNodePtr child;
	Tile *tile = NULL;
	TileType type;
	GQuark base_id;
	GQuark sub_id;
	gchar *content;

	g_return_val_if_fail (node != NULL, NULL);
	g_return_val_if_fail (!g_strcasecmp (node->name, "tile"), NULL);

	tile = NULL; 

	for (child = node->xmlChildrenNode; 
	     child != NULL; child = child->next) 
	{
		if(!g_strcasecmp(child->name, "type"))
		{
			g_assert (tile == NULL);
			content = xmlNodeGetContent(child);
			type = string_to_tile_type (content);
			tile = tile_new (type);
		}
		else if(!g_strcasecmp(child->name,"base"))
		{
			g_assert (tile != NULL);
			content = xmlNodeGetContent (child);
			base_id = g_quark_from_string (content);
			tile_set_base_id (tile, base_id);
		}
		else if(!g_strcasecmp(child->name,"underlay"))
		{
			g_assert (tile != NULL);
			content = xmlNodeGetContent (child);
			sub_id = g_quark_from_string (content);
			tile_add_sub_id (tile, sub_id, TILE_SUB_UNDERLAY);
		}
		else if(!g_strcasecmp(child->name,"overlay"))
		{
			g_assert (tile != NULL);
			content = xmlNodeGetContent (child);
			base_id = g_quark_from_string (content);
			tile_add_sub_id (tile, base_id, TILE_SUB_OVERLAY);
		}
		else if (!g_strcasecmp (child->name, "text")) {
		}
		else
		{
			g_warning ("Skipping unknown tag: %s.",
				   child->name);
		}
	}

	return tile;
}

#if 0
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
			gint value = GPOINTER_TO_QUARK(conn_id->data);
			length = g_snprintf(str_buffer, 5, "%i", value);
			child = xmlNewChild(tile_node, NULL, "CONN_ID", str_buffer);
			conn_id = conn_id->next;
		}
	}

	g_free(str_buffer);
}

#endif



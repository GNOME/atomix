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

#include <string.h>

#include "tile.h"
#include "xml-util.h"

static GObjectClass *parent_class = NULL;

static void tile_class_init (GObjectClass *class);
static void tile_init (Tile *tile);
static void tile_finalize (GObject *object);

struct _TilePrivate
{
  TileType type;
  GQuark base_id;
  GSList *sub_id_list[2];
};

GType tile_get_type (void)
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
	  NULL,			/* clas_finalize */
	  NULL,			/* class_data */
	  sizeof (Tile),
	  0,			/* n_preallocs */
	  (GInstanceInitFunc) tile_init,
	};

      object_type = g_type_register_static (G_TYPE_OBJECT,
					    "Tile", &object_info, 0);
    }

  return object_type;
}

/*=================================================================
 
  Tile creation, initialisation and clean up

  ---------------------------------------------------------------*/

static void tile_class_init (GObjectClass *class)
{
  parent_class = g_type_class_ref (G_TYPE_OBJECT);
  class->finalize = tile_finalize;
}

static void tile_init (Tile *tile)
{
  TilePrivate *priv;

  priv = g_new0 (TilePrivate, 1);
  priv->type = TILE_TYPE_UNKNOWN;
  priv->base_id = 0;
  priv->sub_id_list[TILE_SUB_OVERLAY] = NULL;
  priv->sub_id_list[TILE_SUB_UNDERLAY] = NULL;

  tile->priv = priv;
}

static void tile_finalize (GObject *object)
{
  Tile *tile = TILE (object);

  g_slist_free (tile->priv->sub_id_list[TILE_SUB_OVERLAY]);
  g_slist_free (tile->priv->sub_id_list[TILE_SUB_UNDERLAY]);
  g_free (tile->priv);
  tile->priv = NULL;
}

Tile *tile_new (TileType type)
{
  Tile *tile;
  tile = TILE (g_object_new (TILE_TYPE, NULL));
  tile->priv->type = type;

  return tile;
}

/*=================================================================
 
  Tile functions

  ---------------------------------------------------------------*/
Tile *tile_copy (Tile *tile)
{
  GSList *sub_id;
  Tile *tile_copy;
  int id, i;

  g_return_val_if_fail (IS_TILE (tile), NULL);

  tile_copy = tile_new (tile->priv->type);
  tile_copy->priv->base_id = tile->priv->base_id;

  for (i = 0; i < 2; i++)
    {
      sub_id = tile->priv->sub_id_list[i];
      for (; sub_id != NULL; sub_id = sub_id->next)
	{
	  id = GPOINTER_TO_QUARK (sub_id->data);
	  tile_copy->priv->sub_id_list[i] =
	    g_slist_append (tile_copy->priv->sub_id_list[i],
			    GQUARK_TO_POINTER (id));
	}
    }

  return tile_copy;
}

GSList *tile_get_sub_ids (Tile *tile, TileSubType sub_type)
{
  g_return_val_if_fail (IS_TILE (tile), NULL);

  return tile->priv->sub_id_list[sub_type];
}

GQuark tile_get_base_id (Tile *tile)
{
  g_return_val_if_fail (IS_TILE (tile), 0);

  return tile->priv->base_id;
}

TileType tile_get_tile_type (Tile *tile)
{
  g_return_val_if_fail (IS_TILE (tile), TILE_TYPE_UNKNOWN);

  return tile->priv->type;
}

static gint list_compare_func (gconstpointer a, gconstpointer b)
{
  return GPOINTER_TO_QUARK (a) - GPOINTER_TO_QUARK (b);
}

void tile_add_sub_id (Tile *tile, guint id, TileSubType sub_type)
{
  g_return_if_fail (IS_TILE (tile));

  tile->priv->sub_id_list[sub_type] =
    g_slist_insert_sorted (tile->priv->sub_id_list[sub_type],
			   GQUARK_TO_POINTER (id), list_compare_func);
}

void tile_remove_sub_id (Tile *tile, guint id, TileSubType sub_type)
{
  GSList *result;

  g_return_if_fail (IS_TILE (tile));

  result = g_slist_find (tile->priv->sub_id_list[sub_type],
			 GQUARK_TO_POINTER (id));

  if (result != NULL)
    tile->priv->sub_id_list[sub_type] =
      g_slist_delete_link (tile->priv->sub_id_list[sub_type], result);
}

void tile_remove_all_sub_ids (Tile *tile, TileSubType sub_type)
{
  g_return_if_fail (IS_TILE (tile));

  g_slist_free (tile->priv->sub_id_list[sub_type]);
  tile->priv->sub_id_list[sub_type] = NULL;
}

void tile_set_base_id (Tile *tile, GQuark id)
{
  g_return_if_fail (IS_TILE (tile));

  tile->priv->base_id = id;
}

gboolean tile_is_equal (Tile *tile, Tile *comp)
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

  for (i = 0; i < 2; i++)
    {
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

void tile_print (Tile *tile)
{
  const gchar *type_str;

  g_return_if_fail (IS_TILE (tile));

  switch (tile->priv->type)
    {
    case TILE_TYPE_NONE:
      type_str = "NONE";
      break;

    case TILE_TYPE_ATOM:
      type_str = "ATOM";
      break;

    case TILE_TYPE_WALL:
      type_str = "WALL";
      break;

    case TILE_TYPE_FLOOR:
      type_str = "FLOR";
      break;

    case TILE_TYPE_SHADOW:
    case TILE_TYPE_UNKNOWN:
    case TILE_TYPE_LAST:
    default:
      type_str = "UKWN";
      break;
    }

  g_print ("%s ", type_str);

}


/*=================================================================
 
  Tile load/parse functions

  ---------------------------------------------------------------*/

TileType tile_type_from_string (const gchar *str)
{
  TileType tile_type = TILE_TYPE_UNKNOWN;
  static int prefix_len = 0;

  if (!prefix_len)
    prefix_len = strlen ("TILE_TYPE_");

  g_return_val_if_fail (str != NULL, tile_type);

  str += prefix_len;

  if (!g_ascii_strcasecmp (str, "FLOOR"))
    tile_type = TILE_TYPE_FLOOR;

  else if (!g_ascii_strcasecmp (str, "WALL"))
    tile_type = TILE_TYPE_WALL;

  else if (!g_ascii_strcasecmp (str, "ATOM"))
    tile_type = TILE_TYPE_ATOM;

  else
    tile_type = TILE_TYPE_UNKNOWN;

  return tile_type;
}

typedef struct
{
  gchar* text;
} TextData;

static void
single_tag_parser_text (GMarkupParseContext  *context,
                        const gchar          *text,
                        gsize                 text_len,
                        gpointer              user_data,
                        GError              **error)
{
//  printf ("tile: text %s\n", text);
  TextData *text_data = user_data;
  text_data->text = g_strdup(text);
}

static GMarkupParser text_parser =
{
  NULL,
  NULL,
  single_tag_parser_text,
  NULL,
  xml_parser_log_error
};

void
tile_parser_start_element (GMarkupParseContext  *context,
                           const gchar          *element_name,
                           const gchar         **attribute_names,
                           const gchar         **attribute_values,
                           gpointer              user_data,
                           GError              **error)
{
  TextData *text_data = NULL;

  if (!g_strcmp0 (element_name, "underlay") || 
      !g_strcmp0 (element_name, "overlay"))
  {
    text_data = g_slice_new (TextData);
    g_markup_parse_context_push(context, &text_parser, text_data);
  } else
    g_print ("tile: starting %s\n", element_name);
}

void
tile_parser_end_element (GMarkupParseContext  *context,
                         const gchar          *element_name,
                         gpointer              user_data,
                         GError              **error)
{
  TextData *text_data;
  GQuark sub_id;
  Tile *tile = user_data;

  if (!g_strcmp0 (element_name, "underlay") || 
      !g_strcmp0 (element_name, "overlay"))
  {
    text_data = g_markup_parse_context_pop (context);
    sub_id = g_quark_from_string (text_data->text);
    g_free (text_data->text);
    g_slice_free (TextData, text_data);

    if (!g_strcmp0 (element_name, "underlay"))
      tile_add_sub_id (tile, sub_id, TILE_SUB_UNDERLAY);
    else
      tile_add_sub_id (tile, sub_id, TILE_SUB_OVERLAY);
  } else
    g_print ("tile: ending %s\n", element_name);
}

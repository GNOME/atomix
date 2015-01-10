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

#include <glib.h>
#include <libxml/tree.h>
#include <libxml/parser.h>

#include "playfield.h"
#include "tile.h"
#include "level.h"
#include "level-private.h"

enum
  {
    OLD_TILE_TYPE_NONE,
    OLD_TILE_TYPE_MOVEABLE,
    OLD_TILE_TYPE_OBSTACLE,
    OLD_TILE_TYPE_DECOR    
  };

typedef struct
{
  GQuark id;
  gchar *string;
} TranslationItem;

static TranslationItem atom_map [] =
  {
    { 0, "fucked-up" },
    { 1, "atom-h" },
    { 2, "atom-o" },
    { 3, "atom-c" },
    { 0, NULL }
  };

static TranslationItem link_map [] =
  {
    {  0, "fucked-up" },
    {  1, "link-top" },
    {  2, "link-top-right" },
    {  3, "link-right" },
    {  4, "link-bottom-right" },
    {  5, "link-bottom" },
    {  6, "link-bottom-left" },
    {  7, "link-left" },
    {  8, "link-top-left" },
    {  9, "link-top-double" },
    { 10, "link-right-double" },
    { 11, "link-bottom-double" },
    { 12, "link-left-double" },
    { 0, NULL }
  };

static TranslationItem wall_map [] =
  {
    {  0, "wall-single" },
    {  1, "wall-vertical-bottom-end" },
    {  2, "wall-single" },
    {  3, "wall-bottom-left" },
    {  4, "wall-vertical-top-end" },
    {  5, "wall-vertical" },
    {  6, "wall-top-left" },
    {  7, "wall-vertical-rightx" },
    {  8, "wall-single" },
    {  9, "wall-bottom-right" },
    { 10, "wall-horizontal" },
    { 11, "wall-horizontal-topx" },
    { 12, "wall-top-right" },
    { 13, "wall-vertical-leftx" },
    { 14, "wall-horizontal-bottomx" },
    { 15, "wall-single"},
    { 0, NULL }
  };

static void calculate_translation_quark (TranslationItem array[]);
static Tile* update_tile (Tile *tile);
static Tile* old_tile_load_xml (xmlNodePtr node, gint revision);
static Level* old_level_load_xml_file (gchar *file_path);
static PlayField* old_playfield_load_xml (xmlNodePtr pf_node, gint revision, gboolean is_env);
static void save_environment (PlayField *pf, xmlNodePtr parent);
static void save_tile (xmlNodePtr parent, Tile *tile);
static void save_only_atoms (PlayField *pf, xmlNodePtr parent);

static void calculate_translation_quark (TranslationItem array[])
{
  gint i = 0;
  for (; array[i].string != NULL; i++)
    {
      array[i].id = g_quark_from_string (array[i].string);
    }
}

static Level *old_level_load_xml_file (gchar *file_path)
{
  xmlDocPtr doc;
  xmlNodePtr node;
  Level *level;
  gchar *prop_value;
  gint revision = 1;

  /* read file */
  doc = xmlParseFile (file_path);

  if (doc == NULL)
    {
      g_warning ("Couldn't parse xml file.");
      return NULL;
    }

  level = level_new ();
  node = doc->xmlRootNode;

  while (node != NULL)
    {
      if (g_ascii_strcasecmp(node->name,"LEVEL") == 0)
	{
	  /* handle level node */
	  prop_value = xmlGetProp (node, "name");
	  level->priv->name = g_strdup (prop_value);
	  node = node->xmlChildrenNode;
	}
      else 
	{
	  if (g_ascii_strcasecmp(node->name, "REVISION") == 0)
	    {
	      /* handle revision number */
	      gchar *content = xmlNodeGetContent (node);
	      revision = atoi (content);
	      g_free (content);
	    }
	  else if (!g_ascii_strcasecmp (node->name, "TIME"))
	    {
	      /* deprecated tag, overread it */
	    }
	  else if (!g_ascii_strcasecmp (node->name, "THEME"))
	    {
	      /* deprecated tag, overread it */
	    }
	  else if (!g_ascii_strcasecmp (node->name, "NEXT"))
	    {
	      /* deprecated tag, overread it */
	    }

	  else if (!g_ascii_strcasecmp (node->name, "PLAYFIELD"))
	    {
	      /* handle playfield node */
	      level->priv->environment = 
		old_playfield_load_xml (node, revision, TRUE);
	      level->priv->scenario = 
		old_playfield_load_xml (node, revision, FALSE);
	    }

	  else if (!g_ascii_strcasecmp (node->name, "GOAL"))
	    {
	      /* handle goal node */
	      level->priv->goal = 
		old_playfield_load_xml (node, revision, FALSE);
	    }

	  else if (!g_ascii_strcasecmp (node->name, "FIRST_LEVEL"))
	    {
	      /* deprecated tag, overread it */
	    }
	  else if (g_ascii_strcasecmp(node->name, "BONUS_LEVEL") == 0)
	    {
	      /* deprecated tag, overread it */
	    }
	  else if (!g_ascii_strcasecmp (node->name, "text"))
	    {
	    }
	  else
	    {
	      g_message ("Unknown TAG, ignoring <%s>.",
			 node->name);
	    }
	  node = node->next;
	}	    
    }
  xmlFreeDoc (doc);	    

  return level;
}

static PlayField *old_playfield_load_xml (xmlNodePtr pf_node, gint revision,
					  gboolean is_env)
{
  xmlNodePtr tile_node;
  PlayField *pf;
  gint row, col;
  gchar *prop_value;
  gint n_rows, n_cols;
  Tile *tile;
  TileType tile_type;

  pf = playfield_new ();

  /* read the matrix */
  row = col = 0;

  /* reading number of columns and rows */
  prop_value = xmlGetProp (pf_node, "rows");
  n_rows = atoi (prop_value);
  prop_value = xmlGetProp (pf_node, "cols");
  n_cols = atoi (prop_value);

  playfield_set_matrix_size (pf, n_rows, n_cols);

  /* reading non empty tiles */
  tile_node = pf_node->xmlChildrenNode;
  while (tile_node)
    {
      if (g_ascii_strcasecmp (tile_node->name, "TILE") == 0)
	{
	  prop_value = xmlGetProp (tile_node, "row");
	  row = atoi (prop_value);
	  prop_value = xmlGetProp (tile_node, "col");
	  col = atoi (prop_value);

	  tile = old_tile_load_xml (tile_node, revision);
	  tile_type = tile_get_tile_type (tile);
	  if (is_env && tile_type == TILE_TYPE_WALL)
	    playfield_set_tile (pf, row, col, tile);
	  else if (!is_env && tile_type == TILE_TYPE_ATOM)
	    playfield_set_tile (pf, row, col, tile);
	  g_object_unref (tile);
	}
      else if (!g_ascii_strcasecmp (tile_node->name, "text"))
	{
	}
      else
	{
	  g_print ("playfield.: Unexpected Tag (<%s>), ignoring.\n", 
		   tile_node->name);
	}

      tile_node = tile_node->next;
    }

  return pf;
}

static Tile *old_tile_load_xml (xmlNodePtr node, gint revision)
{
  xmlNodePtr child;
  Tile *tile = NULL;
  TileType type = TILE_TYPE_UNKNOWN;
  gint img_id;
  gint conn_id;

  gchar *content;
  child = node->xmlChildrenNode;

  while (child)
    {
      if (g_ascii_strcasecmp (child->name, "TYPE") == 0)
	{
	  int otype;

	  g_assert (tile == NULL);
	  /* handle tile type */
	  content = xmlNodeGetContent (child);
	  otype = (atoi(content));
	  switch (otype)
	    {
	    case OLD_TILE_TYPE_NONE:
	      type = TILE_TYPE_NONE;
	      break;
	    case OLD_TILE_TYPE_MOVEABLE:
	      type = TILE_TYPE_ATOM;
	      break;
	    case OLD_TILE_TYPE_OBSTACLE:
	      type = TILE_TYPE_WALL;
	      break;
	    case OLD_TILE_TYPE_DECOR:
	      type = TILE_TYPE_NONE;
	      break;
	    default:
	      type = TILE_TYPE_UNKNOWN;
	      break;
	    };
	  tile = tile_new (type);
	  g_free(content);
	}

      else if (g_ascii_strcasecmp (child->name, "IMG_ID") == 0)
	{
	  /* handle img id node */
	  g_assert (tile != NULL);
	  content = xmlNodeGetContent (child);
	  img_id = atoi (content);
	  if (type == TILE_TYPE_ATOM) 
	    tile_set_base_id (tile, atom_map[img_id].id);
	  else if (type == TILE_TYPE_WALL)
	    tile_set_base_id (tile, wall_map[img_id].id);
	  else
	    tile_set_base_id (tile, (GQuark) img_id);

	  g_free(content);
	}

      else if (g_ascii_strcasecmp (child->name, "CONN_ID") == 0)
	{
	  g_assert (tile != NULL);

	  content = xmlNodeGetContent (child);
	  conn_id = atoi (content);

	  tile_add_sub_id (tile, link_map[conn_id].id, 
			   TILE_SUB_UNDERLAY);
	  g_free (content);
	}
      else if (!g_ascii_strcasecmp (child->name, "text"))
	{
	}
      else
	{
	  g_print ("tile.c: Unknown TAG, ignoring <%s>\n",
		   child->name);
	}

      child = child->next;
    }	

  return tile;
}

static void new_level_write_file (Level *level)
{
  xmlDocPtr doc;
  xmlAttrPtr attr;
  xmlNodePtr level_node;
  xmlNodePtr node;

  g_return_if_fail (IS_LEVEL (level));

  /* create xml doc */    
  doc = xmlNewDoc ("1.0");

  /* level name */
  level_node = xmlNewDocNode (doc, NULL, "level", NULL);
  doc->xmlRootNode = level_node;
  attr = xmlSetProp (level_node, "name", g_strdup (level->priv->name));

  /* Playfield */
  node = xmlNewChild (level_node, NULL, "environment", NULL);
  save_environment (level->priv->environment, node);

  node = xmlNewChild (level_node, NULL, "scenario", NULL);
  save_only_atoms (level->priv->scenario, node);	

  /* Goal */
  node = xmlNewChild(level_node, NULL, "goal", NULL);
  save_only_atoms (level->priv->goal, node);

  xmlSaveFormatFile ("-", doc, 1);

  xmlFreeDoc(doc);
}

static PlayField *convert_environment (PlayField *pf)
{
  int r,c;
  Tile *tile;
  Tile *new_tile;
  PlayField *result;
  PlayField *pf_border;

  g_return_val_if_fail (IS_PLAYFIELD (pf), NULL);

  pf_border = playfield_new ();
  playfield_set_matrix_size (pf_border, 
			     playfield_get_n_rows (pf)+2, 
			     playfield_get_n_cols (pf)+2);

  for (r = 0; r < playfield_get_n_rows (pf); r++)
    {
      for (c = 0; c < playfield_get_n_cols (pf); c++)
	{
	  tile = playfield_get_tile (pf, r, c);
	  playfield_set_tile (pf_border, r+1, c+1, tile);
	  if (tile)
	    g_object_unref (tile);
	}
    }

  result = playfield_new ();
  playfield_set_matrix_size (result, 
			     playfield_get_n_rows (pf_border), 
			     playfield_get_n_cols (pf_border));

  for (r = 0; r < playfield_get_n_rows (result); r++)
    {
      for (c = 0; c < playfield_get_n_cols (result); c++)
	{
	  tile = playfield_get_tile (pf_border, r, c);
	  new_tile = update_tile (tile);

	  if (new_tile)
	    {
	      playfield_set_tile (result, r, c, new_tile);
	      g_object_unref (new_tile);
	    }

	  if (tile)
	    g_object_unref (tile);
	}
    }

  g_object_unref (pf_border);

  return result;
}

static PlayField *convert_scenario (PlayField *pf)
{
  int r,c;
  Tile *tile;
  PlayField *pf_border;

  pf_border = playfield_new ();
  playfield_set_matrix_size (pf_border, 
			     playfield_get_n_rows (pf)+2, 
			     playfield_get_n_cols (pf)+2);

  for (r = 0; r < playfield_get_n_rows (pf); r++)
    {
      for (c = 0; c < playfield_get_n_cols (pf); c++)
	{
	  tile = playfield_get_tile (pf, r, c);
	  playfield_set_tile (pf_border, r+1, c+1, tile);
	  if (tile)
	    g_object_unref (tile);
	}
    }

  return pf_border;
}

static Tile *update_tile (Tile *tile)
{
  static GQuark wall_id = 0;
  Tile *new_tile = NULL;
  TileType type;

  if (!wall_id)
    wall_id = g_quark_from_static_string ("wall-single");

  if (tile == NULL)
    return NULL;

  type = tile_get_tile_type (tile);
  if (type == TILE_TYPE_WALL)
    {
      new_tile = tile_copy (tile);
      tile_set_base_id (new_tile, wall_id);
    }

  return new_tile;
}

static void save_environment (PlayField *pf, xmlNodePtr parent)
{
  xmlNodePtr node;
  xmlNodePtr row_node;
  xmlNodePtr col_node;
  gchar *buffer;
  gint r,c;
  Tile *tile;

  g_return_if_fail (IS_PLAYFIELD (pf));
  g_return_if_fail (parent != NULL);

  buffer = g_new0 (gchar, 5);

  g_snprintf (buffer, 5, "%i", playfield_get_n_rows (pf));
  node = xmlNewChild (parent, NULL, "n_rows", buffer);
  g_snprintf (buffer, 5, "%i", playfield_get_n_cols (pf));
  node = xmlNewChild (parent, NULL, "n_columns", buffer);

  for (r = 0; r < playfield_get_n_rows (pf); r++)
    {
      row_node = NULL; 

      for (c = 0; c < playfield_get_n_cols (pf); c++)
	{
	  tile = playfield_get_tile (pf, r, c);
	  if (tile != NULL && tile_get_tile_type (tile) != TILE_TYPE_ATOM)
	    {
	      if (row_node == NULL)
		{
		  row_node = xmlNewChild (parent, NULL, "row", NULL);
		  g_snprintf (buffer, 5, "%i", r);
		  xmlSetProp (row_node, "no", buffer);
		}

	      col_node = xmlNewChild (row_node, NULL, "col", NULL);
	      g_snprintf (buffer, 5, "%i", c);
	      xmlSetProp (col_node, "no", buffer);

	      save_tile (col_node, tile);
	    }

	  if (tile != NULL) 
	    g_object_unref (tile);
	}
    }
}

static void save_only_atoms (PlayField *pf, xmlNodePtr parent)
{
  xmlNodePtr node = NULL;
  xmlNodePtr row_node  = NULL;
  xmlNodePtr col_node = NULL;
  gchar *buffer;
  gint r,c;
  gint last_written_r  = -1; 
  Tile *tile;

  g_return_if_fail (IS_PLAYFIELD (pf));
  g_return_if_fail (parent != NULL);

  buffer = g_new0 (gchar, 5);

  g_snprintf (buffer, 5, "%i", playfield_get_n_rows (pf));
  node = xmlNewChild (parent, NULL, "n_rows", buffer);
  g_snprintf (buffer, 5, "%i", playfield_get_n_cols (pf));
  node = xmlNewChild (parent, NULL, "n_columns", buffer);

  for (r = 0; r < playfield_get_n_rows (pf); r++)
    {
      for (c = 0; c < playfield_get_n_cols (pf); c++)
	{
	  tile = playfield_get_tile (pf, r, c);
	  if (tile != NULL && tile_get_tile_type (tile) == TILE_TYPE_ATOM)
	    {
	      if (r != last_written_r)
		{
		  row_node = xmlNewChild (parent, NULL, "row", NULL);
		  g_snprintf (buffer, 5, "%i", r);
		  xmlSetProp (row_node, "no", buffer);
		}

	      col_node = xmlNewChild (row_node, NULL, "col", NULL);
	      g_snprintf (buffer, 5, "%i", c);
	      xmlSetProp (col_node, "no", buffer);

	      save_tile (col_node, tile);
	      last_written_r = r;
	    }

	  if (tile)
	    g_object_unref (tile);
	}
    }

  g_free (buffer);
}

static void save_tile (xmlNodePtr parent, Tile *tile)
{
  xmlNodePtr tile_node;
  GSList *elem;
  TileType type;
  gchar *type_str = NULL;

  if (tile == NULL)
    return;

  tile_node = xmlNewChild (parent, NULL, "tile", NULL);

  type = tile_get_tile_type (tile);

  switch (type)
    {
    case TILE_TYPE_ATOM:
      type_str = "TILE_TYPE_ATOM";
      break;
    case TILE_TYPE_WALL:
      type_str = "TILE_TYPE_WALL";
      break;
    case TILE_TYPE_FLOOR:
      type_str = "TILE_TYPE_FLOOR";
      break;
    default:
      break;
    }

  if (type_str == NULL)
    return;

  xmlNewChild (tile_node, NULL, "type", type_str);
  xmlNewChild (tile_node, NULL, "base", g_quark_to_string (tile_get_base_id (tile)));

  elem = tile_get_sub_ids (tile, TILE_SUB_UNDERLAY);
  for (; elem != NULL; elem = elem->next)
    {
      xmlNewChild (tile_node, NULL, "underlay", 
		   g_quark_to_string ((GQuark) elem->data));
    }
  elem = tile_get_sub_ids (tile, TILE_SUB_OVERLAY);
  for (; elem != NULL; elem = elem->next)
    {
      xmlNewChild (tile_node, NULL, "overlay", 
		   g_quark_to_string ((GQuark) elem->data));
    }
}

int main (int argc, char** argv)
{
  Level *level;
  gchar *src_file = NULL;
  PlayField *npf;

  calculate_translation_quark (atom_map);
  calculate_translation_quark (link_map);
  calculate_translation_quark (wall_map);

  if (argc < 2)
    {
      g_error ("level-convert <in-file> <out-file");
    }
  src_file = argv[1];

  if (!g_file_test (src_file, G_FILE_TEST_IS_REGULAR))
    g_error ("Couldn't find file %s.", src_file);

  level = old_level_load_xml_file (src_file);
  if (level == NULL)
    g_error ("Error on level loading.");

#if 0
  playfield_print (level->priv->environment);
  playfield_print (level->priv->scenario);
#endif

  npf = convert_environment (level->priv->environment);
  g_object_unref (level->priv->environment);
  level->priv->environment = npf;

  npf = convert_scenario (level->priv->scenario);
  g_object_unref (level->priv->scenario);
  level->priv->scenario = npf;

  new_level_write_file (level);

  g_object_unref (level);

  return 0;
}

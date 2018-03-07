/* Atomix -- a little puzzle game about atoms and molecules.
 * Copyright (C) 2001 Jens Finke
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

#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <fcntl.h>

#include "level-manager.h"
#include "level-private.h"
#include "xml-util.h"

static void search_level_in_dir (LevelManager *lm, gchar *dir_path);
static gchar *lookup_level_name (gchar *filename);
static void add_level (LevelManager *lm, gchar *levelname, gchar *filename);
static Level *load_level (gchar *filename);

static GObjectClass *parent_class = NULL;

static void level_manager_class_init (GObjectClass *class);
static void level_manager_init (LevelManager *tm);
static void level_manager_finalize (GObject *object);

struct _LevelManagerPrivate
{
  gboolean initialized;
  GList *level_seq;
  GHashTable *levels;
};

GType level_manager_get_type (void)
{
  static GType object_type = 0;

  if (!object_type)
    {
      static const GTypeInfo object_info =
	{
	  sizeof (LevelManagerClass),
	  (GBaseInitFunc) NULL,
	  (GBaseFinalizeFunc) NULL,
	  (GClassInitFunc) level_manager_class_init,
	  NULL,			/* clas_finalize */
	  NULL,			/* class_data */
	  sizeof (LevelManager),
	  0,			/* n_preallocs */
	  (GInstanceInitFunc) level_manager_init,
	};

      object_type = g_type_register_static (G_TYPE_OBJECT,
					    "LevelManager", &object_info, 0);
    }

  return object_type;
}

/*=================================================================
 
  Level_Manager creation, initialisation and clean up

  ---------------------------------------------------------------*/

static void level_manager_class_init (GObjectClass *class)
{
  parent_class = g_type_class_ref (G_TYPE_OBJECT);
  class->finalize = level_manager_finalize;
}

static void level_manager_init (LevelManager *tm)
{
  LevelManagerPrivate *priv;

  priv = g_new0 (LevelManagerPrivate, 1);
  priv->initialized = FALSE;
  priv->level_seq = NULL;
  priv->levels = g_hash_table_new_full ((GHashFunc) g_str_hash,
					(GCompareFunc) g_str_equal,
					(GDestroyNotify) g_free,
					(GDestroyNotify) g_free);
  tm->priv = priv;
}

static void level_manager_finalize (GObject *object)
{
  LevelManager *tm = LEVEL_MANAGER (object);
  g_hash_table_destroy (tm->priv->levels);
  g_list_free_full (tm->priv->level_seq, g_free);
  g_free (tm->priv);
  tm->priv = NULL;
}

LevelManager *level_manager_new (void)
{
  LevelManager *lm;
  lm = LEVEL_MANAGER (g_object_new (LEVEL_MANAGER_TYPE, NULL));
  return lm;
}

/*=================================================================
 
  Level_Manager level sequence parsing

  ---------------------------------------------------------------*/
static void
sequence_parser_start_element (GMarkupParseContext  *context,
                               const gchar          *element_name,
                               const gchar         **attribute_names,
                               const gchar         **attribute_values,
                               gpointer              user_data,
                               GError              **error)
{
  const gchar *prop_value;
  LevelManager *lm = LEVEL_MANAGER (user_data);
  LevelManagerPrivate *priv = lm->priv;

  if (!g_strcmp0 (element_name, "level")) {
    prop_value = get_attribute_value ("name", attribute_names, attribute_values);
    priv->level_seq = g_list_append (priv->level_seq, 
                                     g_strdup (prop_value));
  } else if (g_strcmp0 (element_name, "levelsequence") && 
             g_strcmp0 (element_name, "text")) {
    g_warning ("Ignoring sequence xml tag: %s", element_name);
  }
}

static GMarkupParser sequence_parser =
{
  sequence_parser_start_element,
  NULL,
  NULL,
  NULL,
  xml_parser_log_error
};

static void create_level_sequence (LevelManager *lm, gchar *file)
{
  GFile *sequence_file;
  gchar *sequence_contents;
  gsize sequence_length;
  GMarkupParseContext *parse_context;

  g_return_if_fail (IS_LEVEL_MANAGER (lm));

  if (!g_file_test (file, G_FILE_TEST_IS_REGULAR))  {
    g_warning ("File not found: %s.", file);
    return;
  }

  sequence_file = g_file_new_for_path (file);
  if (g_file_load_contents (sequence_file, NULL, &sequence_contents, &sequence_length, NULL, NULL)) {
    parse_context = g_markup_parse_context_new (&sequence_parser,
                                                G_MARKUP_TREAT_CDATA_AS_TEXT,
                                                lm,
                                                NULL);
    g_markup_parse_context_parse (parse_context, sequence_contents, sequence_length, NULL);
    g_markup_parse_context_unref (parse_context);
    g_free (sequence_contents);
  }

  g_object_unref (sequence_file);

}

void level_manager_init_levels (LevelManager *lm)
{
  gchar *dir_path;
  gchar *sequence_file;

  g_return_if_fail (IS_LEVEL_MANAGER (lm));
  g_return_if_fail (!lm->priv->initialized);

  /* load the sequence of the levels */

  if (g_list_length (lm->priv->level_seq) == 0)
    {
      sequence_file = g_build_filename (DATADIR,
					"atomix", "level", "sequence", NULL);
      create_level_sequence (lm, sequence_file);
      g_free (sequence_file);

      if (g_list_length (lm->priv->level_seq) == 0)
	{
	  g_warning ("Couldn’t find level sequence description.");
	}
    }

  /* search for all levels */
  dir_path = g_build_filename (g_get_home_dir (), ".atomix", "level", NULL);
  search_level_in_dir (lm, dir_path);
  g_free (dir_path);

  dir_path = g_build_filename (DATADIR, "atomix", "level", NULL);
  search_level_in_dir (lm, dir_path);
  g_free (dir_path);

  if (g_hash_table_size (lm->priv->levels) == 0)
    g_warning ("No level found.");

  lm->priv->initialized = TRUE;
}

gboolean level_manager_is_last_level (LevelManager *lm, Level *level)
{
  GList *last;

  g_return_val_if_fail (IS_LEVEL_MANAGER (lm), TRUE);
  g_return_val_if_fail (lm->priv->initialized, TRUE);
  g_return_val_if_fail (IS_LEVEL (level), TRUE);

  last = g_list_last (lm->priv->level_seq);
  if (last == NULL)
    return TRUE;

  return !g_ascii_strcasecmp ((gchar *) last->data, level_get_name (level));
}

static void search_level_in_dir (LevelManager *lm, gchar *dir_path)
{
  struct dirent *dent = NULL;
  DIR *dir;

  g_return_if_fail (IS_LEVEL_MANAGER (lm));

  dir = opendir (dir_path);
  if (dir)
    {
      gchar *filename;

      while ((dent = readdir (dir)) != NULL)
	{
	  if (g_ascii_strcasecmp (".", dent->d_name) &&
	      g_ascii_strcasecmp ("..", dent->d_name) &&
	      g_ascii_strcasecmp ("sequence", dent->d_name))
	    {
	      gchar *levelname;

	      filename = g_build_filename (dir_path, dent->d_name, NULL);
	      levelname = lookup_level_name (filename);
	      add_level (lm, levelname, filename);
	      g_free (filename);
	      g_free (levelname);
	    }
	}
      g_free (dent);
      closedir (dir);
    }
}

static void add_level (LevelManager *lm, gchar *levelname, gchar *filename)
{
  gchar *search_result;

  g_return_if_fail (IS_LEVEL_MANAGER (lm));
  g_return_if_fail (levelname != NULL);
  g_return_if_fail (filename != NULL);

  /* don't add a theme twice */
  search_result = g_hash_table_lookup (lm->priv->levels, levelname);

  if (search_result == NULL)
    {
      g_hash_table_insert (lm->priv->levels,
			   g_strdup (levelname), g_strdup (filename));

#ifdef DEBUG
      g_message ("Found level “%s” in: %s", levelname, filename);
#endif
    }
}


Level *level_manager_get_next_level (LevelManager *lm, Level *current_level)
{
  LevelManagerPrivate *priv;
  gchar *levelname = NULL;
  gchar *filename = NULL;
  Level *level = NULL;

  g_return_val_if_fail (IS_LEVEL_MANAGER (lm), NULL);
  g_return_val_if_fail (lm->priv->initialized, NULL);

  if (g_list_length (lm->priv->level_seq) == 0)
    return NULL;

  priv = lm->priv;

  if (current_level == NULL)
    {
      levelname = (gchar *) g_list_first (priv->level_seq)->data;
    }
  else
    {
      GList *result;
      result =
	g_list_find_custom (priv->level_seq, level_get_name (current_level),
			    (GCompareFunc) g_ascii_strcasecmp);
      if (result != NULL)
	{
	  result = result->next;
	  if (result != NULL)
	    levelname = (gchar *) result->data;
	}
    }

  if (levelname != NULL)
    {
      filename = g_hash_table_lookup (lm->priv->levels, levelname);
      if (filename != NULL)
	    level = load_level (filename);
    }

  return level;
}

static void add_level_to_list (gchar *key, gpointer value, GList **list)
{
  *list =
    g_list_insert_sorted (*list, key, (GCompareFunc) g_ascii_strcasecmp);
}

GList *level_manager_get_available_levels (LevelManager *lm)
{
  GList *list = NULL;

  g_return_val_if_fail (IS_LEVEL_MANAGER (lm), NULL);

  g_hash_table_foreach (lm->priv->levels, (GHFunc) add_level_to_list, &list);

  return list;
}

/*=================================================================

  Level_Manager level parsing

  ---------------------------------------------------------------*/

static GMarkupParser level_parser =
{
  level_parser_start_element,
  level_parser_end_element,
  NULL,
  NULL,
  xml_parser_log_error
};

static gchar *lookup_level_name (gchar *filename)
{

  gchar *name = NULL;
  Level *level = NULL;

  g_return_val_if_fail (filename != NULL, NULL);
  g_return_val_if_fail (g_file_test (filename, G_FILE_TEST_EXISTS), NULL);

  level = load_level (filename);
  name = level_get_name (level);

  return name;
}

static Level *load_level (gchar *filename)
{
  GFile *level_file;
  gchar *level_contents;
  gsize level_length;
  GMarkupParseContext *parse_context;
  Level *level = NULL;

  g_return_val_if_fail (filename != NULL, NULL);

  if (!g_file_test (filename, G_FILE_TEST_IS_REGULAR))
    {
      g_warning ("Level not found %s.", filename);
      return NULL;
    }

  // markup-based loading here
  level_file = g_file_new_for_path (filename);
  if (g_file_load_contents (level_file, NULL, &level_contents, &level_length, NULL, NULL)) {
    level = level_new ();
    level->priv->file_name = g_path_get_basename (filename);
    parse_context = g_markup_parse_context_new (&level_parser,
                                                G_MARKUP_TREAT_CDATA_AS_TEXT,
                                                level,
                                                NULL);
    g_markup_parse_context_parse (parse_context, level_contents, level_length, NULL);
    g_markup_parse_context_unref (parse_context);
    g_free (level_contents);
  }

  return level;

}

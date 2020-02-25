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

#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <fcntl.h>
#include <dirent.h>

#include "level.h"
#include "xml-util.h"
#include "level-private.h"

static void level_class_init (GObjectClass *class);
static void level_init (Level *level);
static void level_finalize (GObject *object);

static GObjectClass *parent_class;

/*=================================================================
 
  Level creation, initialisation and clean up

  ---------------------------------------------------------------*/

GType level_get_type (void)
{
  static GType object_type = 0;

  if (!object_type)
    {
      static const GTypeInfo object_info =
	{
	  sizeof (LevelClass),
	  (GBaseInitFunc) NULL,
	  (GBaseFinalizeFunc) NULL,
	  (GClassInitFunc) level_class_init,
	  NULL,   /* clas_finalize */
	  NULL,   /* class_data */
	  sizeof(Level),
	  0,      /* n_preallocs */
	  (GInstanceInitFunc) level_init,
	};

      object_type = g_type_register_static (G_TYPE_OBJECT,
					    "Level",
					    &object_info, 0);
    }

  return object_type;
}

/*=================================================================
 
  Level creation, initialisation and clean up

  ---------------------------------------------------------------*/

static void level_class_init (GObjectClass *class)
{
  parent_class = g_type_class_ref (G_TYPE_OBJECT);
  class->finalize = level_finalize;
}

static void level_init (Level *level)
{
  LevelPrivate *priv;

  priv = g_new0 (LevelPrivate, 1);

  priv->name = NULL;
  priv->formula = NULL;
  priv->goal = NULL;
  priv->environment = NULL;
  priv->scenario = NULL;
  priv->file_name = NULL;
  priv->modified = FALSE;

  level->priv = priv;
}

static void level_finalize (GObject *object)
{
  LevelPrivate *priv;
  Level* level = LEVEL (object);
  priv = level->priv;

  if (priv->name)
    g_free (priv->name);
  if (priv->formula)
    g_free (priv->formula);
  if (priv->goal)
    g_object_unref (priv->goal);
  if (priv->environment)
    g_object_unref (priv->environment);
  if (priv->scenario)
    g_object_unref (priv->scenario);
  if (priv->file_name)
    g_free (priv->file_name);

  g_free (level->priv);
  level->priv = NULL;
}

Level *level_new (void)
{
  Level *level;

  level = LEVEL (g_object_new (LEVEL_TYPE, NULL));

  return level;
}


gchar *level_get_name (Level *level)
{
  g_return_val_if_fail (IS_LEVEL (level), NULL);

  return level->priv->name;
}

gchar *level_get_formula (Level *level)
{
  g_return_val_if_fail (IS_LEVEL (level), NULL);

  return level->priv->formula;
}

PlayField *level_get_environment (Level *level)
{
  g_return_val_if_fail (IS_LEVEL (level), NULL);

  g_object_ref (level->priv->environment);

  return level->priv->environment;
}

PlayField *level_get_goal (Level *level)
{
  g_return_val_if_fail (IS_LEVEL (level), NULL);

  g_object_ref (level->priv->goal);

  return level->priv->goal;
}

PlayField *level_get_scenario (Level *level)
{
  g_return_val_if_fail (IS_LEVEL (level), NULL);

  g_object_ref (level->priv->scenario);

  return level->priv->scenario;
}

static GMarkupParser playfield_parser =
{
  playfield_parser_start_element,
  playfield_parser_end_element,
  playfield_parser_text,
  NULL,
  xml_parser_log_error
};

void
level_parser_start_element (GMarkupParseContext  *context,
                            const gchar          *element_name,
                            const gchar         **attribute_names,
                            const gchar         **attribute_values,
                            gpointer              user_data,
                            GError              **error)
{
  const gchar *prop_value;
  Level *level = LEVEL (user_data);
  PlayField *playfield = NULL;
  guint rows = 0;
  guint cols = 0;

  if (!g_strcmp0 (element_name, "level"))
  {
    prop_value = get_attribute_value ("_name", attribute_names, attribute_values);
    level->priv->name = g_strdup (prop_value);

    prop_value = get_attribute_value ("formula", attribute_names, attribute_values);
    level->priv->formula = g_strdup (prop_value); 
  } else if (!g_strcmp0 (element_name, "environment") ||
             !g_strcmp0 (element_name, "goal") ||
             !g_strcmp0 (element_name, "scenario"))
  {
    playfield = playfield_new ();
    prop_value = get_attribute_value ("n_rows", attribute_names, attribute_values);
    rows = (guint) atoi (prop_value);
    prop_value = get_attribute_value ("n_columns", attribute_names, attribute_values);
    cols = (guint) atoi (prop_value);

    playfield_set_matrix_size (playfield, rows, cols);

    g_markup_parse_context_push (context, &playfield_parser, playfield);
  }
  
}

void
level_parser_end_element (GMarkupParseContext  *context,
                          const gchar          *element_name,
                          gpointer              user_data,
                          GError              **error)
{
  PlayField* playfield = NULL;
  Level *level = LEVEL (user_data);

  if (!g_strcmp0 (element_name, "environment") ||
      !g_strcmp0 (element_name, "goal") ||
      !g_strcmp0 (element_name, "scenario"))
  {
    playfield = g_markup_parse_context_pop (context);
    if (!g_strcmp0 (element_name, "environment"))
    {
      level->priv->environment = playfield;
    } else if (!g_strcmp0 (element_name, "scenario"))
    {
      level->priv->scenario = playfield;
    } else if (!g_strcmp0 (element_name, "goal"))
    {
      level->priv->goal = playfield;
    }
  }
}


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

#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <fcntl.h>
#include <dirent.h>

#include "level.h"
#include "level-private.h"

static void level_class_init (GObjectClass *class);
static void level_init (Level *level);
static void level_finalize (GObject *object);

GObjectClass *parent_class;

/*=================================================================
 
  Level creation, initialisation and clean up

  ---------------------------------------------------------------*/

GType
level_get_type (void)
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

static void 
level_class_init (GObjectClass *class)
{
	parent_class = g_type_class_ref (G_TYPE_OBJECT);
	class->finalize = level_finalize;
}

static void 
level_init (Level *level)
{
	LevelPrivate *priv;

	priv = g_new0 (LevelPrivate, 1);

	priv->name = NULL;
	priv->goal = NULL;
	priv->playfield = 0;
	priv->file_name = 0;
	priv->modified = FALSE;

	level->priv = priv;
}

static void 
level_finalize (GObject *object)
{
	LevelPrivate *priv;
	Level* level = LEVEL (object);

	priv = level->priv;

	if (priv->name)
		g_free (priv->name);
	if (priv->goal)
		g_object_unref (priv->goal);
	if (priv->playfield)
		g_object_unref (priv->playfield);
	if (priv->file_name)
		g_free (priv->file_name);

	g_free (level->priv);
	level->priv = NULL;
}

Level*
level_new (void)
{
	Level *level;

	level = LEVEL (g_object_new (LEVEL_TYPE, NULL));

	return level;
}


gchar*
level_get_name (Level *level)
{
	g_return_val_if_fail (IS_LEVEL (level), NULL);
	return level->priv->name;
}

PlayField* 
level_get_playfield (Level *level)
{
	g_return_val_if_fail (IS_LEVEL (level), NULL);
	return level->priv->playfield;
}

PlayField* 
level_get_goal (Level *level)
{
	g_return_val_if_fail (IS_LEVEL (level), NULL);
	return level->priv->goal;
}



#if 0
void
level_save_xml(Level* level, gchar* filename)
{
	xmlDocPtr doc;
	xmlAttrPtr attr;
	xmlNodePtr level_node;
	xmlNodePtr playfield_node;
	xmlNodePtr goal_node;
	xmlNodePtr child_node;
	gchar *str_buffer; 
	gint length;

	str_buffer = g_malloc(5*sizeof(gchar));

	/* create xml doc */    
	doc = xmlNewDoc("1.0");
	xmlSetDocCompressMode(doc, 9);

	/* level name */
	level_node = xmlNewDocNode(doc, NULL, "LEVEL", NULL);
	doc->xmlRootNode = level_node;
	attr = xmlSetProp(level_node, "name", g_strdup(level->name));

	/* set revision number */
	child_node = xmlNewChild(level_node, NULL, "REVISION", "2");

	/* time */
	child_node = xmlNewChild(level_node, NULL, "TIME", NULL);
	length = g_snprintf(str_buffer, 5, "%i", level->time);
	attr = xmlSetProp(child_node, "secs", g_strdup(str_buffer));
    
	/* level */
	child_node = xmlNewChild(level_node, NULL, "LEVEL", NULL);
	attr = xmlSetProp(child_node, "name", g_strdup(level->level_name));

	/* next */
	child_node = xmlNewChild(level_node, NULL, "NEXT", NULL);
	attr = xmlSetProp(child_node, "level", g_strdup(level->next));

	/* first level */
	if(level->first_level)
	{
		child_node = xmlNewChild(level_node, NULL, "FIRST_LEVEL", NULL);
	}

	/* bonus level */
	if(level->bonus_level)
	{
		child_node = xmlNewChild(level_node, NULL, "BONUS_LEVEL", NULL);
	}

	/* Playfield */
	playfield_node = xmlNewChild(level_node, NULL, "PLAYFIELD", NULL);
	playfield_save_xml(level->playfield, playfield_node);

	/* Goal */
	goal_node = xmlNewChild(level_node, NULL, "GOAL", NULL);
	playfield_save_xml(level->goal, goal_node);
    
	xmlSaveFile(filename, doc);

	xmlFreeDoc(doc);
	g_free(str_buffer);
}
#endif 

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

#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <fcntl.h>
#include <dirent.h>
#include <libxml/parser.h>
#include <libxml/tree.h>

#include "level.h"

/*=================================================================
 
  Declaration of internal structures.

  ---------------------------------------------------------------*/
typedef struct _LevelInfo LevelInfo;

struct _LevelInfo
{
	gchar *name;
	gboolean is_first_level;
};

/*=================================================================
 
  Declaration of internal functions

  ---------------------------------------------------------------*/

gchar *first_level;

void 
level_check_and_add_filename(gchar *path);

LevelInfo*
level_get_level_info_xml(gchar *filename);

void
level_print_hash_table(gpointer key, gpointer value, gpointer user_data);

void
level_destroy_hash_table_item(gpointer key, gpointer value, gpointer user_data);

/*=================================================================
 
  Level creation, initialisation and clean up

  ---------------------------------------------------------------*/

Level* 
level_new(void)
{
	Level* p_level;
	p_level = g_malloc(sizeof(Level));    

	p_level->name = NULL;
	p_level->next = NULL;
	p_level->theme_name = NULL;
	p_level->time = 0;
	p_level->playfield = NULL;
	p_level->goal = NULL;
	p_level->first_level = FALSE;
	p_level->file_name = NULL;
	p_level->modified = FALSE;
	p_level->bonus_level = FALSE;
	
	return p_level;
}

void
level_destroy(Level* l) 
{
	if(l!=NULL)
	{
		if(l->playfield) 
		{
			playfield_destroy(l->playfield);
		}
		if(l->goal)
		{
			playfield_destroy(l->goal);
		}
		if(l->name)
		{
			g_free(l->name);
		}
		if(l->next)
		{
			g_free(l->next);
		}
		if(l->theme_name)
		{
			g_free(l->theme_name);
		}    
		if(l->file_name)
		{
			g_free(l->file_name);
		}    
		g_free(l);
	}
}

void
level_create_hash_table(void)
{
	gint i, n_dirs;
	struct dirent *dent = NULL;
	DIR* dir;
	gchar *search_dirs[2];

	available_levels = g_hash_table_new((GHashFunc) g_str_hash, 
					    (GCompareFunc)g_str_equal);
	first_level = NULL;
    
	/* define directorys where to search for levels */
	search_dirs[0] = g_strjoin("/", g_get_home_dir (), ".atomix/levels", NULL);
	search_dirs[1] = g_strjoin("/", DATADIR, "atomix/levels", NULL);
	n_dirs = 2;
    
	/* search the dirs */
	for(i = 0; (i < n_dirs); i++)
	{
		dir = opendir(search_dirs[i]);
		if(dir)
		{
			gchar *filepath;
			g_print("looking for levels in %s\n", search_dirs[i]);

			while((dent = readdir(dir)) != NULL)
			{
				if((g_strcasecmp(".", dent->d_name)!=0) &&
				   (g_strcasecmp("..", dent->d_name)!=0))
				{
					filepath = g_concat_dir_and_file(search_dirs[i],
									 dent->d_name);
					level_check_and_add_filename(filepath);
					g_free(filepath);
				}
			}
		}

		//g_free(dent);
		closedir(dir);
	}
    
	if(g_hash_table_size(available_levels) == 0)
	{
		g_print("** Warning no levels found.\n");
	}

	for(i = 0; i < n_dirs; i++) g_free(search_dirs[i]);

}

void level_check_and_add_filename(gchar *path)
{					
	gchar *rev_path;

	/* check for right file suffix (.atomix) */
	rev_path = g_strdup(path);
	g_strreverse(rev_path);	
	if(g_strncasecmp(rev_path, "ximota.", 7) == 0)
	{
		LevelInfo *level_info = NULL;

		/* get level infos */
		level_info = level_get_level_info_xml(path);
		    
		if(level_info)
		{
			/* don't insert a level twice */
			if(!level_is_name_already_present(level_info->name))
			{
				level_add_to_hash_table(level_info->name,
							path);

				/* check whether it is the first level */
				if(level_info->is_first_level)
				{
					first_level = g_strdup(level_info->name);
				}
				g_print("found level: %s \n", level_info->name);

			}
			g_free(level_info);
		}
		
	}
	else
	{
		// g_print(" -> ignoring: %s\n", path);
	}

					
	g_free(rev_path);
}

void
level_destroy_hash_table(void)
{
	if(available_levels)
	{
		g_hash_table_foreach(available_levels, 
				     (GHFunc) level_destroy_hash_table_item, NULL);
		g_hash_table_destroy(available_levels);
	}

	if(first_level)
	{
		g_free(first_level);
	}
}

/*=================================================================
 
  Level functions

  ---------------------------------------------------------------*/

gchar*
level_get_first_level(void)
{
	return first_level;
}

void level_set_last_level(Level *level)
{
	g_return_if_fail(level!=NULL);
	level->next = g_strdup("game_end");
}

gboolean level_is_last_level(Level *level)
{
	g_return_val_if_fail(level!=NULL, TRUE);
	
	if(level->next != NULL)
	{	
		return (g_strcasecmp(level->next, "game_end") == 0);
	}
	else
	{
		return FALSE;
	}
}

/*=================================================================
 
  Level hashtable functions

  ---------------------------------------------------------------*/

void 
level_add_to_hash_table(gchar *name, gchar *file_path)
{
	g_return_if_fail(name!=NULL);
	g_return_if_fail(file_path!=NULL);

	// don't insert a level twice
	if(!level_is_name_already_present(name))
	{
		/* insert level */
		g_hash_table_insert(available_levels, 
				    g_strdup(name),
				    g_strdup(file_path));
	}
}

gboolean 
level_is_name_already_present(gchar *name)
{
	gchar *result = NULL;
	result = g_hash_table_lookup(available_levels, name);
	return (result != NULL);
}


/*=================================================================
 
  Level load/save functions

  ---------------------------------------------------------------*/

Level* 
level_load_xml(gchar* name)
{
	Level *level;
	gchar *path;

	/* check if name == NULL */
	if(name == NULL)
	{
		return NULL;
	}

	path = (gchar*) g_hash_table_lookup(available_levels, name);
	level = level_load_xml_file(path);

	return level;
}

Level*
level_load_xml_file(gchar *file_path)
{
	xmlDocPtr doc;
	xmlNodePtr node;
	Level *level;
	gchar *prop_value;
	gchar **splitted_path;
	gchar *previous_part;
	gint part = 0;
	gint revision = 1;

	/* read file */
	level = level_new();
	doc = xmlParseFile(file_path); 
       
	if(doc != NULL)
	{
		node = doc->xmlRootNode;
	
		while(node!=NULL)
		{
			if(g_strcasecmp(node->name,"LEVEL")==0)
			{
				/* handle level node */
				prop_value = xmlGetProp(node, "name");
				level->name = g_strdup(prop_value);
				node = node->xmlChildrenNode;
			}			
			else 
			{
				if(g_strcasecmp(node->name, "REVISION")==0)
				{
					/* handle revision number */
					gchar *content = xmlNodeGetContent(node);
					revision = atoi(content);
					g_free(content);
				}
				else if(g_strcasecmp(node->name,"TIME")==0)
				{
					/* handle time node */
					prop_value = xmlGetProp(node, "secs");
					level->time = atoi(prop_value);
				}
				else if(g_strcasecmp(node->name,"THEME")==0)
				{
					/* handle theme node */
					prop_value = xmlGetProp(node, "name");
					level->theme_name = g_strdup(prop_value);
				}
				else if(g_strcasecmp(node->name,"NEXT")==0)
				{
					/* handle next node */
					prop_value = xmlGetProp(node, "level");
					level->next = g_strdup(prop_value);
				}
		
				else if(g_strcasecmp(node->name,"PLAYFIELD")==0)
				{
					/* handle playfield node */
					level->playfield = 
						playfield_load_xml(node, revision);
				}
		
				else if(g_strcasecmp(node->name,"GOAL")==0)
				{
					/* handle goal node */
					level->goal = 
						playfield_load_xml(node, revision);
				}

				else if(g_strcasecmp(node->name, "FIRST_LEVEL")==0)
				{
					/* handle first level node */
					level->first_level = TRUE;
				}
				else if(g_strcasecmp(node->name, "BONUS_LEVEL")==0)
				{
					/* handle bonus level node */
					level->bonus_level = TRUE;
				}
				else
				{
					g_print("Unknown TAG, ignoring <%s>\n",
						node->name);
				}
		
				node = node->next;
			}	    
		}
		xmlFreeDoc(doc);	    
	}
	else
	{
		level_destroy(level);
		return NULL;
	}

	/* obtain the file name of the level, without path */
	splitted_path = g_strsplit(file_path, "/", 0);
	previous_part = splitted_path[0];
	part = 1;
	if(previous_part != NULL)
	{
		while(splitted_path[part] != NULL)
		{
			previous_part = splitted_path[part];
			part++;
		}
		level->file_name = g_strdup(previous_part);		
	}
	g_strfreev(splitted_path);

	return level;
}

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
    
	/* theme */
	child_node = xmlNewChild(level_node, NULL, "THEME", NULL);
	attr = xmlSetProp(child_node, "name", g_strdup(level->theme_name));

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

LevelInfo* level_get_level_info_xml(gchar* filename)
{
	LevelInfo *info = NULL;
	xmlDocPtr doc;
	xmlNodePtr node;
	gchar *prop_value;

	/* read file */
	doc = xmlParseFile(filename); 
       
	if(doc != NULL)
	{
		info = g_malloc(sizeof(LevelInfo));
		info->name = NULL;
		info->is_first_level = FALSE;

		node = doc->xmlRootNode;
	
		while(node!=NULL)
		{
			if(g_strcasecmp(node->name,"LEVEL")==0)
			{
				/* get level name */
				prop_value = xmlGetProp(node, "name");
				info->name = g_strdup(prop_value);
				node = node->xmlChildrenNode;
			}
			else 
			{
				if(g_strcasecmp(node->name, "FIRST_LEVEL")==0)
				{
					/* is first level */
					info->is_first_level = TRUE;
				}
				node = node->next;
			}	    
		}
		xmlFreeDoc(doc);
	}

	return info;
}

/*=================================================================
 
  Internal helper functions

  ---------------------------------------------------------------*/

void
level_print_hash_table(gpointer key, gpointer value, gpointer user_data)
{
	g_print("Key: %s, Value: %s\n",(gchar*) key, (gchar*) value);
}


void
level_destroy_hash_table_item(gpointer key, gpointer value, gpointer user_data)
{
	g_free(key);
	g_free(value);
}


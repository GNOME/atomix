/* Atomix -- a little mind game about atoms and molecules.
 * Copyright (C) 2001 Jens Finke
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
#include <dirent.h>
#include <fcntl.h>
#include <libxml/parser.h>
#include <libxml/tree.h>

#include "theme-manager.h"
#include "theme-private.h"

static GObjectClass     *parent_class = NULL;

static void search_themes_in_dir (ThemeManager *tm, const gchar *dir_path);
static void add_theme (ThemeManager *tm, gchar *themename, gchar *dirpath);
static void add_theme_to_list (gchar* key, gpointer value, GList **list);
static Theme* load_theme (gchar *theme_dir);
static void handle_tile_type_node (Theme *theme, xmlNodePtr node, TileType type);
static gchar* lookup_theme_name (gchar *theme_file);

static void theme_manager_class_init (GObjectClass *class);
static void theme_manager_init (ThemeManager *tm);
static void theme_manager_finalize (GObject *object);

struct _ThemeManagerPrivate {
	gboolean   initialized;
	GHashTable *themes;
};


GType
theme_manager_get_type (void)
{
  static GType object_type = 0;

  if (!object_type)
    {
      static const GTypeInfo object_info =
      {
	sizeof (ThemeManagerClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) theme_manager_class_init,
	NULL,   /* clas_finalize */
	NULL,   /* class_data */
	sizeof(ThemeManager),
	0,      /* n_preallocs */
	(GInstanceInitFunc) theme_manager_init,
      };

      object_type = g_type_register_static (G_TYPE_OBJECT,
					    "ThemeManager",
					    &object_info, 0);
    }

  return object_type;
}

/*=================================================================
 
  Theme_Manager creation, initialisation and clean up

  ---------------------------------------------------------------*/

static void 
theme_manager_class_init (GObjectClass *class)
{
	parent_class = g_type_class_ref (G_TYPE_OBJECT);
	class->finalize = theme_manager_finalize;
}

static void 
theme_manager_init (ThemeManager *tm)
{
	ThemeManagerPrivate *priv;

	priv = g_new0 (ThemeManagerPrivate, 1);
	priv->initialized = FALSE;
	priv->themes = g_hash_table_new_full ((GHashFunc) g_str_hash, 
					      (GCompareFunc) g_str_equal,
					      (GDestroyNotify) g_free,
					      (GDestroyNotify) g_free);
	
	tm->priv = priv;
}

static void 
theme_manager_finalize (GObject *object)
{
	ThemeManager* tm = THEME_MANAGER (object);
	g_hash_table_destroy(tm->priv->themes);
	
	g_free (tm->priv);
	tm->priv = NULL;
}

ThemeManager*
theme_manager_new (void)
{
	ThemeManager *tm;
	tm = THEME_MANAGER (g_object_new (THEME_MANAGER_TYPE, NULL));
	return tm;
}


void
theme_manager_init_themes (ThemeManager *tm)
{
	gchar* dir_path;

	g_return_if_fail (IS_THEME_MANAGER (tm));
	g_return_if_fail (!tm->priv->initialized);

        dir_path =  g_build_filename (g_get_home_dir (), ".atomix/themes", NULL);
	search_themes_in_dir (tm, dir_path);
	g_free (dir_path);

	dir_path = g_build_filename (DATADIR, "atomix/themes", NULL);
	search_themes_in_dir (tm, dir_path);
	g_free (dir_path);

	if (g_hash_table_size (tm->priv->themes) == 0)
		g_warning(_("No themes found."));

	tm->priv->initialized = TRUE;
}
    
static void 
search_themes_in_dir (ThemeManager *tm, const gchar *dir_path)
{
	struct dirent *dent = NULL;
	DIR* dir;
	
	g_message ("looking for themes in %s", dir_path);

	dir = opendir (dir_path);
	if(dir)
	{
		char* filename;
		char* themename;
		char* subdirpath;
		
		while((dent = readdir(dir)) != NULL)
		{
			if((g_strcasecmp(".", dent->d_name)!=0) &&
			   (g_strcasecmp("..", dent->d_name)!=0))
			{
				/* is current file a directory? */
				subdirpath = g_build_filename (dir_path,
							       dent->d_name, NULL);
				if(g_file_test(subdirpath, G_FILE_TEST_IS_DIR))
				{
					/* try to load file */
					filename = g_build_filename (subdirpath, 
								     "theme", NULL);
					themename = lookup_theme_name (filename);
					add_theme (tm, themename, subdirpath);
					g_free (filename);
					g_free (themename);
				}
				g_free (subdirpath);
			}
		}
	}
	
	g_free(dent);
	closedir(dir);
}

static void 
add_theme (ThemeManager *tm, gchar *themename, gchar *dirpath)
{
	gchar *search_result;

	g_return_if_fail (IS_THEME_MANAGER (tm));
	g_return_if_fail (themename != NULL);
	g_return_if_fail (dirpath != NULL);

	/* don't add a theme twice */
	search_result = g_hash_table_lookup (tm->priv->themes, themename);
	
	if(search_result == NULL)
	{
		g_hash_table_insert(tm->priv->themes,
				    g_strdup(themename),
				    g_strdup(dirpath));
		
		g_message ("Found theme: %s\n", themename);
	}
	else 
		g_warning (_("Found theme %s twice."), themename);
}


GList* 
theme_manager_get_available_themes (ThemeManager *tm)
{
	GList *list = NULL;
	
	g_return_val_if_fail (IS_THEME_MANAGER (tm), NULL);
		
	g_hash_table_foreach(tm->priv->themes,
			     (GHFunc) add_theme_to_list, &list);
			     
	return list;
}

static void
add_theme_to_list (gchar* key, gpointer value, GList **list)
{
	*list = g_list_insert_sorted (*list, key, (GCompareFunc) g_strcasecmp);	
}

/* =======================================================================
  
                      Theme loading stuff

  ======================================================================== */


Theme* 
theme_manager_get_theme (ThemeManager *tm, const gchar *theme_name)
{
	Theme *theme;
	gchar *theme_dir;

	g_return_val_if_fail (IS_THEME_MANAGER (tm), NULL);
	g_return_val_if_fail (tm->priv->initialized, NULL);
	
	theme_dir = g_hash_table_lookup (tm->priv->themes, theme_name);
	if (theme_dir == NULL) return NULL;
		
	theme = load_theme (theme_dir);
	g_free (theme_dir);

	return theme;
}

static Theme*
load_theme (gchar *theme_dir)
{	
	Theme *theme;
	ThemePrivate *priv;
	gchar *theme_file;
	gchar *prop_value;
	xmlDocPtr doc;
	xmlNodePtr node;

	g_return_val_if_fail (theme_dir != NULL, NULL);

	theme_file = g_build_filename (theme_dir, "theme", NULL);
	if (!g_file_test (theme_file, G_FILE_TEST_EXISTS | G_FILE_TEST_IS_REGULAR)) {
		g_warning ("File theme not found in %s", theme_dir);
		return NULL;
	}

	g_message ("parse file: %s", theme_file);

	doc = xmlParseFile (theme_file); 
	if (doc == NULL) {
		g_warning ("XML file %s couldn't be parsed.", theme_file);
		g_free (theme_file);
		return NULL;
	}

	theme = theme_new ();
	priv = theme->priv;
	priv->path = g_strdup (theme_dir);
	
	node = doc->xmlRootNode;
	while (node != NULL)
	{
		if(!g_strcasecmp (node->name,"theme"))
		{
			/* handle theme node */
			priv->name = g_strdup (xmlGetProp (node, "name"));
			node = node->xmlChildrenNode;
		}
		else 
		{	
			if (!g_strcasecmp (node->name, "floor"))
			{
				handle_tile_type_node (theme, node->xmlChildrenNode,
						       TILE_TYPE_FLOOR);
			}
			else if (!g_strcasecmp (node->name, "atom"))
			{
				handle_tile_type_node (theme, node->xmlChildrenNode,
						       TILE_TYPE_ATOM);
			}
			else if (!g_strcasecmp (node->name, "wall"))
			{
				handle_tile_type_node (theme, node->xmlChildrenNode,
						       TILE_TYPE_WALL);
			}
			else if (!g_strcasecmp (node->name,"animstep"))
			{
				priv->animstep = atoi (xmlGetProp(node, "dist"));
			}
			else if (!g_strcasecmp(node->name,"bgcolor"))
			{					
				/* handle background color */
				prop_value = xmlGetProp(node, "color");
				gdk_color_parse (prop_value, &(priv->bg_color));
			}
			else if (!g_strcasecmp(node->name, "bgcolor_rgb"))
			{
				/* handle rgb color node */
				prop_value = xmlGetProp(node, "red");
				priv->bg_color.red = (atof(prop_value)/255.0) * 65536;
				prop_value = xmlGetProp(node, "green");
				priv->bg_color.green = (atof(prop_value)/255.0) * 65536;
				prop_value = xmlGetProp(node, "blue");
				priv->bg_color.blue = (atof(prop_value)/255.0) * 65536;
			}
			else if (!g_strcasecmp(node->name,"selector"))
			{
				gchar *src;
				src = g_build_filename (theme_dir, xmlGetProp(node, "src"), NULL);
				theme_set_selector_image(theme, src);
			}
			else if (!g_strcasecmp (node->name, "text")) {
			}
			else					
			{
				g_warning("Unknown theme tag, ignoring <%s>.", node->name);
			}
			
			node = node->next;
		}
	}
	
	xmlFreeDoc(doc);

	return theme;
}

static void
handle_tile_type_node (Theme *theme, xmlNodePtr node, TileType type)
{
	gint id;
	gchar *src;
	gchar *name;

	g_return_if_fail (IS_THEME (theme));
	
	for (; node != NULL; node = node->next) {
		if (!g_strcasecmp (node->name, "base")) {
			id = atoi (xmlGetProp (node, "id"));
			src = g_build_filename (theme->priv->path, 
						xmlGetProp (node, "src"), NULL);
			name = xmlGetProp (node, "name");
		
			theme_add_base_image_with_id (theme, name, src, type, id);
			g_free (src);
		}
		else if (!g_strcasecmp (node->name, "underlay")) {
			id = atoi (xmlGetProp (node, "id"));
			src = g_build_filename (theme->priv->path, 
						xmlGetProp (node, "src"), NULL);
			name = xmlGetProp (node, "name");
		
			theme_add_sub_image_with_id (theme, name, src, type, TRUE, id);
			g_free (src);
		}
		else if (!g_strcasecmp (node->name, "overlay")) {
			id = atoi (xmlGetProp (node, "id"));
			src = g_build_filename (theme->priv->path, 
						xmlGetProp (node, "src"), NULL);
			name = xmlGetProp (node, "name");
		
			theme_add_sub_image_with_id (theme, name, src, type, FALSE, id);
			g_free (src);
		}
		else if (!g_strcasecmp (node->name, "text")) {
		}
		else 
			g_warning("Unknown theme tag, ignoring <%s>.", node->name);
	}
}


static gchar*
lookup_theme_name (gchar *theme_file)
{
	xmlDocPtr doc;
	xmlNodePtr node;
	gchar* name = NULL;

	g_return_val_if_fail (theme_file != NULL, NULL);
	g_return_val_if_fail (g_file_test (theme_file, G_FILE_TEST_EXISTS), NULL);

	/* read file */
	doc = xmlParseFile (theme_file); 
	if (doc == NULL) {
		g_warning ("Couldn't parse theme file: %s", theme_file);
		return NULL;
	}

	node = doc->xmlRootNode;
	
	if (node && !g_strcasecmp (node->name, "theme"))
	        name = g_strdup (xmlGetProp (node, "name"));
	
	xmlFreeDoc(doc);

	return name;
}


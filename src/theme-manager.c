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
static void destroy_theme_item (gpointer key, gpointer value, gpointer user_data);
static void add_theme_to_list (gchar* key, gpointer value, GList **list);
static Theme* load_theme (gchar *theme_dir);
static void handle_link_image_node (Theme *theme, xmlNodePtr node);
static void handle_base_image_node (Theme *theme, xmlNodePtr node);
static gchar* lookup_theme_name (gchar *theme_file);
static TileLink string_to_tile_link (gchar *str);
static TileType string_to_tile_type (gchar *str);


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
	priv->themes = g_hash_table_new ((GHashFunc) g_str_hash, 
					 (GCompareFunc) g_str_equal);
	
	tm->priv = priv;
}

static void 
theme_manager_finalize (GObject *object)
{
	ThemeManager* tm = THEME_MANAGER (object);
	g_hash_table_foreach(tm->priv->themes, 
			     (GHFunc) destroy_theme_item, NULL);
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

static void
destroy_theme_item (gpointer key, gpointer value, gpointer user_data)
{
	g_free (key);
	g_free (value);
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
	guint revision = 3;
	xmlDocPtr doc;
	xmlNodePtr node;

	g_return_val_if_fail (theme_dir != NULL, NULL);

	theme_file = g_build_filename (theme_dir, "theme", NULL);
	if (!g_file_test (theme_file, G_FILE_TEST_EXISTS)) {
		g_warning ("File theme not found in %s", theme_dir);
		return NULL;
	}

	theme = theme_new ();
	priv = theme->priv;
	priv->path = g_strdup (theme_dir);
    
	doc = xmlParseFile (theme_file); 
	if (doc == NULL) {
		g_warning ("XML file %s couldn't be parsed.", theme_file);
		g_free (theme_file);
		g_object_unref (theme);
		return NULL;
	}
	
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
			if(!g_strcasecmp (node->name, "revision"))
			{
				/* handle revision node */
				gchar *content = xmlNodeGetContent(node);
				revision = atoi(content);
				g_free(content);
			}
			else if(!g_strcasecmp (node->name, "baseimages"))
			{
				handle_base_image_node (theme, node->xmlChildrenNode);
			}
			else if (!g_strcasecmp (node->name, "linkimages"))
			{
				handle_link_image_node (theme, node->xmlChildrenNode);
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
				priv->bg_color.red = atoi(prop_value);
				prop_value = xmlGetProp(node, "green");
				priv->bg_color.green = atoi(prop_value);
				prop_value = xmlGetProp(node, "blue");
				priv->bg_color.blue = atoi(prop_value);
			}
			else if (!g_strcasecmp(node->name,"selector"))
			{
				gchar *src;
				src = g_build_filename (theme_dir, xmlGetProp(node, "src"));
				theme_set_selector_image(theme, src);
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
handle_base_image_node (Theme *theme, xmlNodePtr node)
{
	TileType tile_type;
	gint id;
	gchar *src;
	gchar *name;

	for (; node != NULL; node = node->next) {
		tile_type = string_to_tile_type (xmlGetProp (node, "type"));
		id = atoi (xmlGetProp (node, "id"));
		src = g_build_filename (theme->priv->path, xmlGetProp (node, "src"), NULL);
		name = xmlGetProp (node, "name");
		
		theme_add_base_image_with_id (theme, name, src, tile_type, id);
		g_free (src);
	}
}

static TileType
string_to_tile_type (gchar *str)
{
	TileType tile_type = TILE_TYPE_UNKNOWN;
	static int prefix_len = 0;
	if (!prefix_len) prefix_len = strlen ("TILE_TYPE_");
	
	str += prefix_len;

	if (!g_strcasecmp (str, "OBSTACLE"))
		tile_type = TILE_TYPE_OBSTACLE;
	else if (!g_strcasecmp (str, "MOVEABLE"))
		tile_type = TILE_TYPE_MOVEABLE;
	else if (!g_strcasecmp (str, "DECOR"))
		tile_type = TILE_TYPE_DECOR;
	
	return tile_type;
}

static void
handle_link_image_node (Theme *theme, xmlNodePtr node)
{
	TileLink tile_link;
	gchar *src;
	gchar *name;
	
	for (; node != NULL; node = node->next ) {
		tile_link = string_to_tile_link (xmlGetProp (node, "type"));
		src = g_build_filename (theme->priv->path, xmlGetProp (node, "src"), NULL);
		name = xmlGetProp (node, "name");
	
		theme_add_link_image (theme, name, src, tile_link);
		g_free (src);
	}
}

static TileLink
string_to_tile_link (gchar *str)
{
	TileLink link = TILE_LINK_LAST;
	static int prefix_len = 0;
	if (!prefix_len) prefix_len = strlen ("TILE_LINK_");
	
	str += prefix_len;
	
	switch (str[0]) {
	case 'T':
		if (!g_strcasecmp (str, "TOP"))
			link = TILE_LINK_TOP;
		else if (!g_strcasecmp (str, "TOP_RIGHT"))
			link = TILE_LINK_TOP_RIGHT;
		else if (!g_strcasecmp (str, "TOP_LEFT"))
			link = TILE_LINK_TOP_LEFT;
		else if (!g_strcasecmp (str, "TOP_DOUBLE"))
			link = TILE_LINK_TOP_DOUBLE;
		break;
	case 'R':
		if (!g_strcasecmp (str, "RIGHT"))
			link = TILE_LINK_RIGHT;
		else if (!g_strcasecmp (str, "RIGHT_DOUBLE"))
			link = TILE_LINK_RIGHT_DOUBLE;
		break;
	case 'L':
		if (!g_strcasecmp (str, "LEFT"))
			link = TILE_LINK_LEFT;
		else if (!g_strcasecmp (str, "LEFT_DOUBLE"))
			link = TILE_LINK_LEFT_DOUBLE;
		break;
	case 'B':
		if (!g_strcasecmp (str, "BOTTOM"))
			link = TILE_LINK_BOTTOM;
		else if (!g_strcasecmp (str, "BOTTOM_RIGHT"))
			link = TILE_LINK_BOTTOM_RIGHT;
		else if (!g_strcasecmp (str, "BOTTOM_LEFT"))
			link = TILE_LINK_BOTTOM_LEFT;
		else if (!g_strcasecmp (str, "BOTTOM_DOUBLE"))
			link = TILE_LINK_BOTTOM_DOUBLE;
		break;		
	}

	return link;
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


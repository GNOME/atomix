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
#include <dirent.h>
#include <fcntl.h>
#include <libxml/parser.h>
#include <libxml/tree.h>

#include "theme.h"

GHashTable* available_themes;

/*=================================================================
 
  Declaration of internal functions

  ---------------------------------------------------------------*/
void 
theme_fill_img_array(Theme *theme, xmlNodePtr node, TileType type,
		     guint revision);

gint 
theme_load_last_id(xmlNodePtr node, gint revision);

gchar*
theme_get_theme_name(gchar *filename);

gchar*
theme_get_theme_name_xml(gchar *filename);

void 
theme_save_image_element_xml(gpointer key, gpointer value, gpointer user_data);

void
theme_print_hash_table(gpointer key, gpointer value, gpointer user_data);

void
theme_destroy_name_table_item(gpointer key, gpointer value, gpointer user_data);

void
theme_destroy_image_table_item(gpointer key, gpointer value, gpointer user_data);

void 
theme_add_tile_image(Theme *theme, TileType type, ThemeElement *element);

void
theme_add_theme_name(gchar *themename, gchar *directory);

void
add_theme_to_list(gchar* key, gpointer value, GList **list);

GdkPixbuf* theme_create_default_image(Theme *theme);

/*=================================================================
 
  Theme creation, initialisation and clean up

  ---------------------------------------------------------------*/
Theme*
theme_new()
{
	Theme* theme;
	theme = g_malloc(sizeof(Theme));

	theme->name = NULL;
	theme->path = NULL;
	theme->image_list[THEME_IMAGE_OBSTACLE] = 
		g_hash_table_new((GHashFunc) g_direct_hash, 
				 (GCompareFunc) g_direct_equal);

	theme->image_list[THEME_IMAGE_MOVEABLE] =
		g_hash_table_new((GHashFunc) g_direct_hash, 
				 (GCompareFunc) g_direct_equal);

	theme->image_list[THEME_IMAGE_LINK] =
		g_hash_table_new((GHashFunc) g_direct_hash, 
				 (GCompareFunc) g_direct_equal);

	theme->selector = NULL;
	theme->tile_width = 0;
	theme->tile_height = 0;
	theme->animstep = 4;
	theme->bg_color.red = 0;
	theme->bg_color.green = 0;
	theme->bg_color.blue = 0;

	theme->modified = FALSE;
	theme->need_update = FALSE;
	theme->last_id[THEME_IMAGE_OBSTACLE] = 0;
	theme->last_id[THEME_IMAGE_MOVEABLE] = 0;
	theme->last_id[THEME_IMAGE_LINK] = 0;

	return theme;
}

void
theme_destroy(Theme* theme)
{
	if(theme) 
	{	
		int i;

		if(theme->name)
		{
			g_free(theme->name);
		}
		if(theme->path)
		{
			g_free(theme->path);
		}
		for(i = 0; i < N_IMG_LISTS; i++)
		{
			if(theme->image_list[i]) 
			{
				g_hash_table_foreach(theme->image_list[i], 
						     (GHFunc) theme_destroy_image_table_item, NULL);
				g_hash_table_destroy(theme->image_list[i]);
			}
		}

		if(theme->selector)
		{
			theme_destroy_element(theme->selector);
		}
		g_free(theme);
	}
}

void
theme_create_hash_table(void)
{
	gint i, n_dirs;
	struct dirent *dent = NULL;
	DIR* dir;
	gchar* search_dirs[2];

	/* init hash table */
	available_themes = g_hash_table_new((GHashFunc) g_str_hash, 
					    (GCompareFunc) g_str_equal);

	/* define directorys where to search for themes */
	search_dirs[0] = g_strjoin("/", g_get_home_dir (), ".atomix/themes", NULL);
	search_dirs[1] = g_strjoin("/", DATADIR, "atomix/themes", NULL);
	n_dirs = 2;
    
	/* search the dirs */
	for(i = 0; i < n_dirs; i++)
	{
		dir = opendir(search_dirs[i]);
		if(dir)
		{
			char* filename;
			char* themename;
			char* directory;
			
			g_print("looking for themes in %s\n", search_dirs[i]);

			while((dent = readdir(dir)) != NULL)
			{
				if((g_strcasecmp(".", dent->d_name)!=0) &&
				   (g_strcasecmp("..", dent->d_name)!=0))
				{
					/* is current file a directory? */
					directory = g_concat_dir_and_file(search_dirs[i],
									  dent->d_name);
					if(g_file_test(directory, G_FILE_TEST_IS_DIR))
					{
						/* try to load file */
						filename = g_concat_dir_and_file(directory, 
										 "theme");
						themename = theme_get_theme_name_xml(filename);
						theme_add_theme_name(themename, directory);
						g_free(filename);
						g_free(themename);
					}
					g_free(directory);
				}
			}
		}

		g_free(dent);
		closedir(dir);
	}

	if(g_hash_table_size(available_themes) == 0)
	{
		g_print("** Warning no themes found.\n");
	}

	for(i = 0; i < n_dirs; i++) g_free(search_dirs[i]);
}

void theme_add_theme_name(gchar *themename, gchar *directory)
{
	gchar *search_result;

	if((themename != NULL) && 
	   (directory != NULL))
	{
	
		/* don't add a theme twice */
		search_result = g_hash_table_lookup(available_themes, themename);
		
		if(search_result == NULL)
		{
			g_hash_table_insert(available_themes,
					    g_strdup(themename),
					    g_strdup(directory));

			g_print("found theme: %s\n", themename);
		}
	}
}

void
theme_destroy_hash_table(void)
{
	if(available_themes)
	{
		g_hash_table_foreach(available_themes, 
				     (GHFunc) theme_destroy_name_table_item, NULL);
		g_hash_table_destroy(available_themes);
	}
}

/*=================================================================
 
  Theme functions

  ---------------------------------------------------------------*/

GList* theme_get_available_themes(void)
{
	GList *list = NULL;
	
	g_hash_table_foreach(available_themes,
			     (GHFunc) add_theme_to_list, &list);
			     
	return list;
}

GdkPixbuf*
theme_get_tile_image(Theme* theme, Tile *tile)
{
	gint type; 
	gint image_id;
	ThemeElement *element = NULL;

	g_return_val_if_fail(theme != NULL, NULL);
	g_return_val_if_fail(tile != NULL, NULL);

	type = tile_get_type(tile);
	image_id = tile_get_image_id(tile);

	switch(type)
	{
	case TILE_OBSTACLE:
		if(theme->image_list[THEME_IMAGE_OBSTACLE]!=NULL) 
		{
			element = g_hash_table_lookup(theme->image_list[THEME_IMAGE_OBSTACLE], 
						      GINT_TO_POINTER(image_id));
		}
		break;
	    
	case TILE_MOVEABLE:
		if(theme->image_list[THEME_IMAGE_MOVEABLE])
		{
			element = g_hash_table_lookup(theme->image_list[THEME_IMAGE_MOVEABLE], 
						      GINT_TO_POINTER(image_id));
		}
		break;
	default:
	}

	return theme_get_element_image(theme, element);
}


GSList* theme_get_tile_link_images(Theme *theme, Tile *tile)
{
	gint i;
	gint length;
	GSList *image_list = NULL;
	GdkPixbuf *image;
	ThemeElement *element;
	GSList *id_list;

	g_return_val_if_fail(theme!=NULL, NULL);
	g_return_val_if_fail(tile!=NULL, NULL);

	id_list = tile_get_link_ids (tile);
	
	length = g_slist_length(id_list);
	for(i = 0; i  < length; i++)
	{
		/* get id of link image */
		gint id = GPOINTER_TO_INT(g_slist_nth_data(id_list, i));

		/* get appropriate image name */
		element = g_hash_table_lookup(theme->image_list[THEME_IMAGE_LINK],
					      GINT_TO_POINTER(id));
		image = theme_get_element_image(theme, element);

		if(image)
		{
                       /* add image to list */
			image_list = g_slist_append(image_list, (gpointer) image);
		}
	}

	return image_list;
}

GdkPixbuf* theme_get_selector_image(Theme *theme)
{
	g_return_val_if_fail(theme!=NULL, NULL);

	return theme_get_element_image(theme, theme->selector);
}

GdkPixbuf* theme_get_element_image(Theme *theme, ThemeElement *element)
{
	GdkPixbuf *image = NULL;

	g_return_val_if_fail(theme!=NULL, NULL);       

	if(element == NULL)
	{
		return theme_create_default_image(theme);
	}

	if(element->image == NULL)
	{
		if(!element->loading_failed)
		{
			GError *error = NULL;
			/* try to load the image */
			gchar *full_path;

			/* add full path */
			full_path = g_concat_dir_and_file (theme->path, element->file);
			element->image = gdk_pixbuf_new_from_file (full_path, &error);

			if(element->image == NULL)
			{
				/* loading failed */
				g_print("** Warning: couldn't find file %s.\n", full_path);
				element->loading_failed = TRUE;
				element->image = theme_create_default_image(theme); 
			}
			image = element->image;

			g_free(full_path);
		}
	}
	else
	{
		image = element->image;
	}
	
	return image;
}

void
theme_get_tile_size(Theme *theme, gint *width, gint *height)
{
	if(theme)
	{
		*width = theme->tile_width;
		*height = theme->tile_height;
	}
	else
	{
		*width = 0;
		*height = 0;
	}
}

GdkColor* theme_get_background_color(Theme *theme)
{
	return &(theme->bg_color);
}


ThemeElement* theme_add_image(Theme *theme, const gchar *name, 
			      const gchar *file, ThemeImageKind kind)
{
	ThemeElement *element;

	g_return_val_if_fail(theme != NULL, NULL);
	g_return_val_if_fail(name != NULL, NULL);
	g_return_val_if_fail(file != NULL, NULL);
	
	/* create new theme element */
	element = g_malloc(sizeof(ThemeElement));

	element->id = ++(theme->last_id[kind]);
	element->file = g_strdup(file);
	element->name = g_strdup(name);
	element->loading_failed = FALSE;
	element->image = NULL;     

	g_print("Adding image:\n");
	g_print("  id: %i\n", element->id);
	g_print("  file: %s\n", element->file);
	g_print("  name: %s\n", element->name);

	g_hash_table_insert(theme->image_list[kind], 
			    GINT_TO_POINTER(element->id),
			    (gpointer) element);

	/* obtain the width and height of the images */
	if((theme->tile_width == 0) || (theme->tile_height == 0))
	{
		GError *error = NULL;
		gchar *full_path;

		full_path = g_concat_dir_and_file(theme->path, file);
		element->image = gdk_pixbuf_new_from_file (full_path, &error);
		if(element->image != NULL)
		{
			theme->tile_width = gdk_pixbuf_get_width (element->image);
			theme->tile_height = gdk_pixbuf_get_height (element->image);
		}

		g_error_free (error);
		g_free(full_path);
	}
	
	return element;
}

void 
theme_remove_element(Theme *theme, ThemeImageKind kind, const ThemeElement *element)
{
	g_return_if_fail(theme != NULL);
	g_return_if_fail(element != NULL);
	
	g_hash_table_remove(theme->image_list[kind], GINT_TO_POINTER(element->id));	
}

gboolean
theme_change_element_id(Theme *theme, ThemeImageKind kind, ThemeElement *element, gint id)
{
	gboolean result = FALSE;

	g_return_val_if_fail(theme != NULL, FALSE);
	g_return_val_if_fail(element != NULL, FALSE);


	if(!theme_does_id_exist(theme, kind, id))
	{
		theme_remove_element(theme, kind, element);
		element->id = id;

		g_hash_table_insert(theme->image_list[kind], GINT_TO_POINTER(element->id),
				    (gpointer)element);
		if(theme->last_id[kind] < id)
		{
			theme->last_id[kind] = id;
		}
		result = TRUE;
	}

	return result;
}

gboolean theme_does_id_exist(Theme *theme, ThemeImageKind kind, gint id)
{
	g_return_val_if_fail(theme != NULL, FALSE);
	
	return (g_hash_table_lookup(theme->image_list[kind], 
			    GINT_TO_POINTER(id)) != NULL);
}

void theme_set_selector_image(Theme *theme, const gchar *file_name)
{
	ThemeElement *element;
	
	g_return_if_fail(theme != NULL);
	g_return_if_fail(file_name != NULL);

	if(theme->selector != NULL)
	{
		theme_destroy_element(theme->selector);
	}

	element = g_malloc(sizeof(ThemeElement));
	element->id = 1;
	element->file = g_strdup(file_name);
	element->name = g_strdup("selector");
	element->loading_failed = FALSE;
	element->image = NULL;
	theme->selector = element;
}

/*=================================================================
 
  Theme load/save functions

  ---------------------------------------------------------------*/
Theme*
theme_load_xml(gchar *name/*, LoadResult *result*/)
{
	xmlDocPtr doc;
	xmlNodePtr node;
	Theme *theme;
	gchar *prop_value;
	gchar *dir_path;
	gchar *file_path;
	guint revision = 1;

	theme = theme_new();

	/* read file */
	dir_path = (gchar*) g_hash_table_lookup(available_themes, name);
	theme_set_path(theme, dir_path);
	file_path = g_strjoin("/", dir_path, "theme", NULL);
    
	doc = xmlParseFile(file_path); 
       
	if(doc != NULL)
	{
		node = doc->xmlRootNode;
		while(node!=NULL)
		{
			if(g_strcasecmp(node->name,"THEME")==0)
			{
				/* handle theme node */
				prop_value = xmlGetProp(node, "name");
				theme->name = g_strdup(prop_value);
				node = node->xmlChildrenNode;
			}
			else 
			{	
				if(g_strcasecmp(node->name,"REVISION")==0)
				{
					/* handle revision node */
					gchar *content = xmlNodeGetContent(node);
					revision = atoi(content);
					g_free(content);
				}
				else if(g_strcasecmp(node->name,"MOVEABLE")==0)
				{
					/* handle moveable node */
					theme->last_id[THEME_IMAGE_MOVEABLE] = 
						theme_load_last_id(node, revision);
					theme_fill_img_array(theme, node->xmlChildrenNode, 
							     THEME_IMAGE_MOVEABLE,
							     revision);
				}
				else if(g_strcasecmp(node->name,"LINK")==0)
				{
					/* handle link node */
					theme->last_id[THEME_IMAGE_LINK] = 
						theme_load_last_id(node, revision);
					theme_fill_img_array(theme, node->xmlChildrenNode,
							     THEME_IMAGE_LINK,
							     revision);
				}
				else if(g_strcasecmp(node->name,"OBSTACLE")==0)
				{
					/* handle obstacle node */
					theme->last_id[THEME_IMAGE_OBSTACLE] = 
						theme_load_last_id(node, revision);
					theme_fill_img_array(theme, node->xmlChildrenNode,
							     THEME_IMAGE_OBSTACLE,
							     revision);
				}
				else if(g_strcasecmp(node->name,"ANIMSTEP")==0)
				{
					/* handle animstep node */
					prop_value = xmlGetProp(node, "dist");
					theme->animstep = atoi(prop_value);
				}
				else if(g_strcasecmp(node->name,"BGCOLOR")==0)
				{					
					/* handle background color */
					prop_value = xmlGetProp(node, "color");
					gdk_color_parse(prop_value, &(theme->bg_color));
				}
				else if(g_strcasecmp(node->name, "BGCOLOR_RGB")==0)
				{
					/* handle rgb color node */
					prop_value = xmlGetProp(node, "red");
					theme->bg_color.red = atoi(prop_value);
					prop_value = xmlGetProp(node, "green");
					theme->bg_color.green = atoi(prop_value);
					prop_value = xmlGetProp(node, "blue");
					theme->bg_color.blue = atoi(prop_value);
				}
				else if(g_strcasecmp(node->name,"SELECTOR")==0)
				{
					/* handle selector image */
					prop_value = xmlGetProp(node, "src");
					theme_set_selector_image(theme, prop_value);
				}
				else					
				{
					g_print("theme.c: Unknown TAG, ignoring <%s>\n",
						node->name);
				}
		
				node = node->next;
			}
		}
	
		xmlFreeDoc(doc);
	}
	else
	{
		return NULL;
	}

	return theme;
}

void
theme_save_xml(Theme *theme, gchar *filename)
{
	xmlDocPtr doc;
	xmlAttrPtr attr;
	xmlNodePtr node;
	xmlNodePtr theme_node;
	xmlNodePtr section_node;
	gint dist;
	gint i;
	gchar *str_buffer;

	gchar *node_name[N_IMG_LISTS];
	node_name[THEME_IMAGE_MOVEABLE] = "MOVEABLE";
	node_name[THEME_IMAGE_OBSTACLE] = "OBSTACLE";
	node_name[THEME_IMAGE_LINK] = "LINK";

	str_buffer = g_malloc(6*sizeof(gchar));
    
	/* create xml doc */    
	doc = xmlNewDoc("1.0");
	// xmlSetDocCompressMode(doc, 9);

	/* theme name */
	theme_node = xmlNewDocNode(doc, NULL, "THEME", NULL);
	doc->xmlRootNode = theme_node;
	attr = xmlSetProp(theme_node, "name", g_strdup(theme->name));

	/* revision */
	node = xmlNewChild(theme_node, NULL, "REVISION", "2");

	/* bg_color */
	node = xmlNewChild(theme_node, NULL, "BGCOLOR_RGB", NULL);
	dist = g_snprintf(str_buffer, 6, "%i", theme->bg_color.red);
	attr = xmlSetProp(node, "red", g_strdup(str_buffer));
	dist = g_snprintf(str_buffer, 6, "%i", theme->bg_color.green);
	attr = xmlSetProp(node, "green", g_strdup(str_buffer));
	dist = g_snprintf(str_buffer, 6, "%i", theme->bg_color.blue);
	attr = xmlSetProp(node, "blue", g_strdup(str_buffer));

	/* animstep */
	node = xmlNewChild(theme_node, NULL, "ANIMSTEP", NULL);
	dist = g_snprintf(str_buffer, 5, "%i", theme->animstep);
	attr = xmlSetProp(node, "dist", g_strdup(str_buffer));

	/* the different image lists */
	for(i = 0; i < N_IMG_LISTS; i++)
	{
		section_node = xmlNewChild(theme_node, NULL, node_name[i], NULL);
		dist = g_snprintf(str_buffer, 5, "%i", theme->last_id[i]);
		attr = xmlSetProp(section_node, "last_id", g_strdup(str_buffer));
		g_hash_table_foreach(theme->image_list[i], 
				     (GHFunc) theme_save_image_element_xml,
				     (gpointer) section_node);
	}

	/* selector section */
	if(theme->selector)
	{
		xmlNodePtr selector_node;
		selector_node = xmlNewChild(theme_node, NULL, "SELECTOR", NULL);
		attr = xmlSetProp(selector_node, "src", g_strdup(theme->selector->file));
	}
	
	xmlSaveFile(filename, doc);

	xmlFreeDoc(doc);
	g_free(str_buffer);
}

void theme_save_image_element_xml(gpointer key, gpointer value, gpointer user_data)
{
	ThemeElement *element = (ThemeElement*) value;
	
	if(!element->loading_failed)
	{
		gchar *str_buffer;
		gint length;
		gint id = GPOINTER_TO_INT(key);
		xmlNodePtr child;
		xmlNodePtr parent = (xmlNodePtr) user_data;
		xmlAttrPtr attr;

		str_buffer = g_malloc(5*sizeof(gchar));
	
		child = xmlNewChild(parent, NULL, "IMAGE", NULL);
		length = g_snprintf(str_buffer, 5, "%i", id);
		attr = xmlSetProp(child, "no", g_strdup(str_buffer));
		attr = xmlSetProp(child, "src", g_strdup(element->file));
		attr = xmlSetProp(child, "name", g_strdup(element->name));

		g_free(str_buffer);	
	}
}

gchar*
theme_get_theme_name_xml(gchar *filename)
{
	xmlDocPtr doc;
	xmlNodePtr node;
	gchar* name = NULL;
	gchar *prop_value;

	/* read file */
	doc = xmlParseFile(filename); 
       
	if(doc != NULL)
	{
		node = doc->xmlRootNode;
	
		if((node!=NULL) &&
		   (g_strcasecmp(node->name,"THEME")==0))
		{
			/* handle level node */
			prop_value = xmlGetProp(node, "name");
			name = g_strdup(prop_value);
		}
		
		xmlFreeDoc(doc);
	}

	return name;
}

/*=================================================================
 
  Internal helper functions

  ---------------------------------------------------------------*/

GdkPixbuf* 
theme_create_default_image(Theme *theme)
{
	GdkColor white;
	GdkColor red;

	if(gdk_color_parse("red", &red) && 
	   gdk_color_alloc(gdk_colormap_get_system(), &red) &&
	   gdk_color_parse("white", &white) &&
	   gdk_color_alloc(gdk_colormap_get_system(), &white))
	{	  
		GdkPixmap *pixmap;
		GdkPixbuf *image;
		GdkGC *gc;
		GdkVisual *visual;
		gint tile_width, tile_height;

		theme_get_tile_size(theme, &tile_width, &tile_height);
		visual = gdk_visual_get_system();
		pixmap = gdk_pixmap_new(NULL,
					tile_width, tile_height,
				        visual->depth);
		gc = gdk_gc_new(pixmap);

		if((tile_width > 5) && (tile_height > 5))
		{
			/* fill with red */
			gdk_gc_set_foreground(gc, &red);
			gdk_draw_rectangle(pixmap, gc,
					   TRUE,
					   0,0, tile_width, tile_height);
		
			/* draw white rectangle into the red area */
			gdk_gc_set_foreground(gc, &white);
			gdk_draw_rectangle(pixmap, gc,
					   TRUE,
					   2, 2, tile_width-4, tile_height-4);

			/* draw X inside the white area */
			gdk_gc_set_foreground(gc, &red);
			gdk_draw_line(pixmap, gc,
				      2, tile_height-3,
				      tile_width-3, 2);
			gdk_draw_line(pixmap, gc,
				      2, 2,
				      tile_width-3, tile_height-3);
		}
		else
		{
			gdk_gc_set_foreground(gc, &white);
			gdk_draw_rectangle(pixmap, gc,
					   TRUE,
					   2, 2, tile_width-2, tile_height-2);
		}

		image = gdk_pixbuf_get_from_drawable (NULL, 
						      pixmap,
						      gdk_colormap_get_system (),
						      0, 0, 0, 0,
						      tile_width, tile_height);

		gdk_pixmap_unref(pixmap);
		gdk_gc_unref(gc);

		return image;
	}
	
	return NULL;
}

void
theme_add_tile_image(Theme* theme, ThemeImageKind kind, ThemeElement *element)
{
	g_return_if_fail(theme!=NULL);	
	g_return_if_fail(element!=NULL);

		
	/* check whether the id already exists */
	if(g_hash_table_lookup(theme->image_list[kind], 
			       GINT_TO_POINTER(element->id)) != NULL)
	{
		g_print("theme.c: Warning, duplicate id %i ... ignoring.\n", element->id);
		theme_destroy_element(element);
		return;
	}

	/* add element with key id */
	g_hash_table_insert(theme->image_list[kind], GINT_TO_POINTER(element->id),
			    (gpointer) element);

	/* set last id counter right */
	if(theme->last_id[kind] < element->id)
	{
		theme->last_id[kind] = element->id;
	}


	/* obtain the width and height of the images */
	if((theme->tile_width == 0) || (theme->tile_height == 0))
	{
		GError *error = NULL;
		gchar *full_path;

		full_path = g_strjoin("/", theme->path, element->file, NULL);
		element->image = gdk_pixbuf_new_from_file (full_path, &error);
		if(element->image != NULL)
		{
			theme->tile_width = gdk_pixbuf_get_width (element->image);
			theme->tile_height = gdk_pixbuf_get_height (element->image);
		}

		g_error_free (error);
		g_free(full_path);
	}
}

void
theme_set_path(Theme* theme, gchar* path)
{
	theme->path = g_strdup(path);
}

void 
theme_fill_img_array(Theme *theme, xmlNodePtr node, ThemeImageKind kind, guint revision)
{
	gchar *img_src;
	gchar *id_str; 
	gchar *img_name;
	ThemeElement *element;
	gint id; 

	while(node != NULL)
	{
		/* get image number */
		id_str = xmlGetProp(node, "no");
		id = atoi(id_str);
		g_free(id_str);

		/* get image file */
		img_src = xmlGetProp(node, "src");

		/* get name */
		img_name = xmlGetProp(node, "name");
		if(img_name == NULL)
		{
			img_name = g_strdup("test");
		}

		/* create new theme element */
		element = g_malloc(sizeof(ThemeElement));
		element->id = id;
		element->file = img_src;
		element->name = img_name;
		element->loading_failed = FALSE;
		element->image = NULL;

		theme_add_tile_image(theme, kind, element);
		node = node->next;
	}
}

gint 
theme_load_last_id(xmlNodePtr node, gint revision)
{
	gchar *last_id_str;
	gint value;
	
	last_id_str = xmlGetProp(node, "last_id");
	if(last_id_str == NULL)
	{
		value = 0;
	}
	else
	{
		value = atoi(last_id_str);
	}

	return value;
}

void
theme_print_hash_table(gpointer key, gpointer value, gpointer user_data)
{
	g_print("Key: %s, Value: %s\n",(gchar*) key, (gchar*) value);
}



void
theme_destroy_name_table_item(gpointer key, gpointer value, gpointer user_data)
{
	g_free(key);
	g_free(value);
}

void
theme_destroy_image_table_item(gpointer key, gpointer value, gpointer user_data)
{
	theme_destroy_element((ThemeElement*) value);
}


void
theme_destroy_element(ThemeElement *element)
{
	g_return_if_fail(element != NULL);

	if(element->file)
	{
		g_free(element->file);
	}
	if(element->image)
	{
		gdk_pixbuf_unref (element->image);
	}
	if(element->name)
	{
		g_free(element->name);
	}
	g_free(element);
}

void
add_theme_to_list(gchar* key, gpointer value, GList **list)
{
	*list = g_list_insert_sorted(*list, key, (GCompareFunc) g_strcasecmp);	
}



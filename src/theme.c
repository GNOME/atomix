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

#include "theme.h"
#include "theme-private.h"


/*=================================================================
 
  Declaration of internal functions

  ---------------------------------------------------------------*/

static void destroy_theme_image (ThemeImage *ti);

static void theme_class_init (GObjectClass *class);
static void theme_init (Theme *theme);
static void theme_finalize (GObject *object);

GObjectClass *parent_class;

GType
theme_get_type (void)
{
  static GType object_type = 0;

  if (!object_type)
    {
      static const GTypeInfo object_info =
      {
	sizeof (ThemeClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) theme_class_init,
	NULL,   /* clas_finalize */
	NULL,   /* class_data */
	sizeof(Theme),
	0,      /* n_preallocs */
	(GInstanceInitFunc) theme_init,
      };

      object_type = g_type_register_static (G_TYPE_OBJECT,
					    "Theme",
					    &object_info, 0);
    }

  return object_type;
}


/*=================================================================
 
  Theme creation, initialisation and clean up

  ---------------------------------------------------------------*/

static void 
theme_class_init (GObjectClass *class)
{
	parent_class = g_type_class_ref (G_TYPE_OBJECT);
	class->finalize = theme_finalize;
}

static void 
theme_init (Theme *theme)
{
	ThemePrivate *priv;
	gint i;

	priv = g_new0 (ThemePrivate, 1);

	priv->name = NULL;
	priv->path = NULL;
	priv->tile_width = 0;
	priv->tile_height = 0;
	priv->animstep = 0;
	priv->bg_color.red  = 0;
	priv->bg_color.green = 0;
	priv->bg_color.blue = 0;
	for (i = 0; i < TILE_TYPE_UNKNOWN; i++) priv->image_list[i] = NULL;
	priv->selector = NULL;
	priv->modified = FALSE;
	priv->need_update = FALSE;
	for (i = 0; i < TILE_TYPE_UNKNOWN; i++) priv->last_id[i] = 0; 
	
	theme->priv = priv;
}

static void 
theme_finalize (GObject *object)
{
	int i;
	ThemePrivate *priv;
	Theme* theme = THEME (object);
	
	g_return_if_fail (theme != NULL);

	priv = theme->priv;

	if (priv->name)
		g_free (priv->name);
	priv->name = NULL;
	
	if (priv->path)
		g_free (priv->path);
	priv->path = NULL;
	
	for (i = 0; i < TILE_TYPE_UNKNOWN; i++) {
		if (priv->image_list[i]) {
			g_hash_table_foreach (priv->image_list[i], 
					     (GHFunc) destroy_theme_image, NULL);
			g_hash_table_destroy (priv->image_list[i]);
			priv->image_list[i] = NULL;
		}
	}

	if (priv->link_image_list) {
		g_hash_table_foreach (priv->link_image_list, 
				      (GHFunc) destroy_theme_image, NULL);
		g_hash_table_destroy (priv->link_image_list);
		priv->link_image_list = NULL;
	}

	if (priv->selector)
		destroy_theme_image (priv->selector);
	priv->selector = NULL;

	g_free (theme->priv);
	theme->priv = NULL;
}

Theme*
theme_new (void)
{
	Theme *theme;
	ThemePrivate *priv;
	gint i;
	theme = THEME (g_object_new (THEME_TYPE, NULL));
	
	priv = theme->priv;
	
	for (i = 0; i < TILE_TYPE_UNKNOWN; i++) {
		priv->image_list[i] = 
			g_hash_table_new((GHashFunc) g_direct_hash, 
					 (GCompareFunc) g_direct_equal);
	}

	priv->link_image_list = 
		g_hash_table_new((GHashFunc) g_direct_hash, 
				 (GCompareFunc) g_direct_equal);

	return theme;
}



/*=================================================================
 
  Theme functions

  ---------------------------------------------------------------*/

gchar*  
theme_get_name (Theme *theme)
{
	g_return_val_if_fail (IS_THEME (theme), NULL);

	return theme->priv->name;
}

gint
theme_get_animstep (Theme *theme)
{
	g_return_val_if_fail (IS_THEME (theme), 0);

	return theme->priv->animstep;
}

static void
destroy_theme_image (ThemeImage *ti)
{
	if (ti == NULL) return;
	
	if (ti->name)
		g_free (ti->name);
	if (ti->file)
		g_free (ti->file);
	if (ti->image)
		gdk_pixbuf_unref (ti->image);
	g_free (ti);
}

static GdkPixbuf*
get_theme_image_pixbuf (ThemeImage *ti)
{
	g_return_val_if_fail (ti != NULL, NULL);
	
	if (ti->loading_failed)
		return NULL;
	
	if (ti->image == NULL) {
		ti->image = gdk_pixbuf_new_from_file (ti->file, NULL);
		if (ti->image == NULL) ti->loading_failed = TRUE;
	}

	return ti->image;
}

static GdkPixbuf*
create_link_image (Theme *theme, Tile *tile)
{
	ThemePrivate *priv;
	gint link;
	ThemeImage *ti;
	GdkPixbuf *pixbuf = NULL;
	GdkPixbuf *link_pb;

	g_return_val_if_fail(IS_THEME (theme), NULL);
	g_return_val_if_fail(IS_TILE (tile), NULL);

	priv = theme->priv;

	for (link = 0; link < TILE_LINK_LAST; link++) {
		if (tile_has_link (tile, link)) {

			ti = g_hash_table_lookup (priv->link_image_list, 
						    GINT_TO_POINTER (link));

			link_pb = get_theme_image_pixbuf (ti);
			if (link_pb == NULL) {
				g_warning ("Couldn't load link image: %i", link);
				continue;
			}

			if (pixbuf == NULL)
				pixbuf = gdk_pixbuf_copy (link_pb);
			
			else {
				gdk_pixbuf_composite (link_pb,
						      pixbuf,
						      0, 0,
						      gdk_pixbuf_get_width (pixbuf),
						      gdk_pixbuf_get_height (pixbuf),
						      0.0, 0.0, 1.0, 1.0,
						      GDK_INTERP_BILINEAR, 255);
			}
			gdk_pixbuf_unref (link_pb);
		}
	}

	return pixbuf;
}


GdkPixbuf*
theme_get_tile_image (Theme* theme, Tile *tile)
{
	ThemePrivate *priv;
	TileType type; 
	gint image_id;
	ThemeImage *timg = NULL;
	GdkPixbuf *link_pb = NULL;
	GdkPixbuf *base_pb = NULL;

	g_return_val_if_fail(IS_THEME (theme), NULL);
	g_return_val_if_fail(IS_TILE (tile), NULL);

	priv = theme->priv;

	type = tile_get_tile_type (tile);
	image_id = tile_get_base_id(tile);

	link_pb = create_link_image (theme, tile);
	
	timg = g_hash_table_lookup (priv->image_list[type], GINT_TO_POINTER (image_id));

	base_pb = get_theme_image_pixbuf (timg);

	if (link_pb != NULL) {
		gdk_pixbuf_composite (base_pb,
				      link_pb,
				      0, 0,
				      gdk_pixbuf_get_width (link_pb),
				      gdk_pixbuf_get_height (link_pb),
				      0.0, 0.0, 1.0, 1.0,
				      GDK_INTERP_BILINEAR, 255);
		gdk_pixbuf_unref (base_pb);
		return link_pb;
	}

	return base_pb;
}

GdkPixbuf* theme_get_selector_image(Theme *theme)
{
	g_return_val_if_fail ( IS_THEME (theme), NULL);

	return get_theme_image_pixbuf (theme->priv->selector);
}

void
theme_get_tile_size(Theme *theme, gint *width, gint *height)
{
	*width = *height = 0;

	g_return_if_fail (IS_THEME (theme));

	*width = theme->priv->tile_width;
	*height = theme->priv->tile_height;
}

GdkColor* theme_get_background_color(Theme *theme)
{
	return &(theme->priv->bg_color);
}



void
theme_add_base_image (Theme *theme, 
		      const gchar *name, 
		      const gchar *file, 
		      TileType tile_type)
{
	g_return_if_fail (IS_THEME (theme));

	theme_add_base_image_with_id (theme, name, file, tile_type,
				      theme->priv->last_id[tile_type] + 1);
}


void
theme_add_base_image_with_id (Theme *theme, 
			      const gchar *name, 
			      const gchar *file, 
			      TileType tile_type,
			      int id)
{
	ThemePrivate *priv;
	ThemeImage *ti;
	
	g_return_if_fail (IS_THEME (theme));
	g_return_if_fail (name != NULL);
	g_return_if_fail (file != NULL);
	
	priv = theme->priv;

	/* create new theme element */
	ti = g_new0 (ThemeImage, 1);

	ti->id = id;
	ti->file = g_strdup(file);
	ti->name = g_strdup(name);
	ti->loading_failed = FALSE;
	ti->image = NULL;     

        if(g_hash_table_lookup (priv->image_list[tile_type], GINT_TO_POINTER (id)) != NULL)
		id = priv->last_id[tile_type] + 1;

	/* insert into hash table */
	g_hash_table_insert(priv->image_list[tile_type], 
			    GINT_TO_POINTER(id),
			    (gpointer) ti);

	/* Obtain the width and height of the images. 
	 * We assume here that all images of this theme have
	 * the same dimension.
	 */
	if((priv->tile_width == 0) || (priv->tile_height == 0))
	{
		ti->image = gdk_pixbuf_new_from_file (file, NULL);
		if(ti->image != NULL)
		{
			priv->tile_width = gdk_pixbuf_get_width (ti->image);
			priv->tile_height = gdk_pixbuf_get_height (ti->image);
		}
		else
			ti->loading_failed = TRUE;
	}

	priv->last_id[tile_type] = MAX (priv->last_id[tile_type], id);
}

void
theme_add_link_image (Theme *theme, 
		      const gchar *name, 
		      const gchar *file, 
		      TileLink link)
{
	ThemePrivate *priv;
	ThemeImage *ti;
	
	g_return_if_fail (IS_THEME (theme));
	g_return_if_fail (name != NULL);
	g_return_if_fail (file != NULL);
	
	priv = theme->priv;

	
        ti = g_hash_table_lookup (priv->link_image_list, GINT_TO_POINTER (link));
	if (ti != NULL)
		destroy_theme_image (ti);

	/* create new theme element */
	ti = g_new0 (ThemeImage, 1);

	ti->id = link;
	ti->file = g_strdup(file);
	ti->name = g_strdup(name);
	ti->loading_failed = FALSE;
	ti->image = NULL;     


	/* insert into hash table */
	g_hash_table_insert(priv->link_image_list, 
			    GINT_TO_POINTER(link),
			    (gpointer) ti);
}


void 
theme_set_selector_image(Theme *theme, const gchar *file_name)
{
	ThemeImage *ti;
	
	g_return_if_fail(IS_THEME (theme));
	g_return_if_fail(file_name != NULL);

	if(theme->priv->selector != NULL)
		destroy_theme_image (theme->priv->selector);

	ti = g_new0 (ThemeImage, 1);
	ti->id = 1;
	ti->file = g_strdup(file_name);
	ti->name = g_strdup("selector");
	ti->loading_failed = FALSE;
	ti->image = NULL;
	theme->priv->selector = ti;
}

#if 0

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


#endif


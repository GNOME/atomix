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

static void destroy_theme_image (gpointer data);
static void add_theme_image_to_hashtable (GHashTable **hash_table, ThemeImage *ti);


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
	gint i, j;

	priv = g_new0 (ThemePrivate, 1);

	priv->name = NULL;
	priv->path = NULL;
	priv->tile_width = 0;
	priv->tile_height = 0;
	priv->animstep = 0;
	priv->bg_color.red  = 0;
	priv->bg_color.green = 0;
	priv->bg_color.blue = 0;
	for (i = 0; i < TILE_TYPE_LAST; i++) priv->base_list[i] = NULL;
	for (i = 0; i < TILE_TYPE_LAST; i++) for (j = 0; j < 2; j++) priv->sub_list[j][i] = NULL;
	priv->selector = NULL;
	for (i = 0; i < TILE_TYPE_LAST; i++) priv->base_last_id[i] = 0; 
	for (i = 0; i < TILE_TYPE_LAST; i++) for (j = 0; j < 2; j++) priv->sub_last_id[j][i] = 0; 
	
	theme->priv = priv;
}

static void 
theme_finalize (GObject *object)
{
	int i, j;
	ThemePrivate *priv;
	Theme* theme = THEME (object);
	
	g_return_if_fail (theme != NULL);

	g_message ("Finalize theme");
	priv = theme->priv;

	if (priv->name)
		g_free (priv->name);
	priv->name = NULL;
	
	if (priv->path)
		g_free (priv->path);
	priv->path = NULL;
	
	for (i = 0; i < TILE_TYPE_LAST; i++) {
		if (priv->base_list[i]) {
			g_hash_table_destroy (priv->base_list[i]);
			priv->base_list[i] = NULL;
		}
		for (j = 0; j < 2; j++) {
			if (priv->sub_list[j][i]) {
				g_hash_table_destroy (priv->sub_list[j][i]);
				priv->sub_list[j][i] = NULL;
			}
		}
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
	theme = THEME (g_object_new (THEME_TYPE, NULL));
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
destroy_theme_image (gpointer data)
{
	ThemeImage *ti; 

	ti = (ThemeImage*) data;

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

	if (ti->image != NULL)
		gdk_pixbuf_ref (ti->image);

	return ti->image;
}

static GdkPixbuf*
create_sub_images (Theme *theme, Tile *tile, TileSubType sub_type)
{
	ThemePrivate *priv;
	GSList *elem;
	ThemeImage *ti;
	GdkPixbuf *pixbuf = NULL;
	GdkPixbuf *pb;

	g_return_val_if_fail(IS_THEME (theme), NULL);
	g_return_val_if_fail(IS_TILE (tile), NULL);

	priv = theme->priv;

	elem = tile_get_sub_ids (tile, sub_type);

	for (; elem != NULL; elem = elem->next) {
		ti = g_hash_table_lookup (priv->sub_list[sub_type][tile_get_tile_type(tile)], 
					  elem->data);
		
		pb = get_theme_image_pixbuf (ti);
		if (pb == NULL) {
			g_warning ("Couldn't load sub image: %i", 
				   GPOINTER_TO_INT (elem->data));
			continue;
		}
		
		if (pixbuf == NULL)
			pixbuf = gdk_pixbuf_copy (pb);
		
		else {
			gdk_pixbuf_composite (pb,
					      pixbuf,
					      0, 0,
					      gdk_pixbuf_get_width (pixbuf),
					      gdk_pixbuf_get_height (pixbuf),
					      0.0, 0.0, 1.0, 1.0,
					      GDK_INTERP_BILINEAR, 255);
		}
		gdk_pixbuf_unref (pb);
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
	GdkPixbuf *underlay_pb = NULL;
	GdkPixbuf *overlay_pb = NULL;
	GdkPixbuf *base_pb = NULL;
	GdkPixbuf *result = NULL;

	g_return_val_if_fail(IS_THEME (theme), NULL);
	g_return_val_if_fail(IS_TILE (tile), NULL);

	priv = theme->priv;

	type = tile_get_tile_type (tile);
	image_id = tile_get_base_id (tile);

	underlay_pb = create_sub_images (theme, tile, TILE_SUB_UNDERLAY);
	overlay_pb = create_sub_images (theme, tile, TILE_SUB_OVERLAY);

	timg = g_hash_table_lookup (priv->base_list[type], GINT_TO_POINTER (image_id));
	base_pb = get_theme_image_pixbuf (timg);
	g_assert (base_pb != NULL);

	if (underlay_pb && overlay_pb) {
		result = gdk_pixbuf_copy (underlay_pb);
		gdk_pixbuf_composite (base_pb,
				      result,
				      0, 0,
				      gdk_pixbuf_get_width (result),
				      gdk_pixbuf_get_height (result),
				      0.0, 0.0, 1.0, 1.0,
				      GDK_INTERP_BILINEAR, 255);
		gdk_pixbuf_composite (overlay_pb,
				      result,
				      0, 0,
				      gdk_pixbuf_get_width (result),
				      gdk_pixbuf_get_height (result),
				      0.0, 0.0, 1.0, 1.0,
				      GDK_INTERP_BILINEAR, 255);	
		gdk_pixbuf_unref (overlay_pb);
		gdk_pixbuf_unref (underlay_pb);
	}
	else if (!underlay_pb && overlay_pb) {
		result = gdk_pixbuf_copy (base_pb);
		gdk_pixbuf_composite (overlay_pb,
				      result,
				      0, 0,
				      gdk_pixbuf_get_width (result),
				      gdk_pixbuf_get_height (result),
				      0.0, 0.0, 1.0, 1.0,
				      GDK_INTERP_BILINEAR, 255);	
		gdk_pixbuf_unref (overlay_pb);
	}
	else if (underlay_pb && !overlay_pb) {
		result = gdk_pixbuf_copy (underlay_pb);
		gdk_pixbuf_composite (base_pb,
				      result,
				      0, 0,
				      gdk_pixbuf_get_width (result),
				      gdk_pixbuf_get_height (result),
				      0.0, 0.0, 1.0, 1.0,
				      GDK_INTERP_BILINEAR, 255);	
		gdk_pixbuf_unref (underlay_pb);
	}
	else
		result = gdk_pixbuf_ref (base_pb);
	
	gdk_pixbuf_unref (base_pb);
		
	return result;
}

GdkPixbuf* 
theme_get_selector_image(Theme *theme)
{
	g_return_val_if_fail ( IS_THEME (theme), NULL);

	return get_theme_image_pixbuf (theme->priv->selector);
}

void
theme_get_selector_arrow_images (Theme *theme, GdkPixbuf **arrows)
{
	int i;

	for (i = 0; i < 4; i++) {
		arrows[i] = get_theme_image_pixbuf (theme->priv->selector_arrows[i]);
	}
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
				      theme->priv->base_last_id[tile_type] + 1);
}


void
theme_add_base_image_with_id (Theme *theme, 
			      const gchar *name, 
			      const gchar *file, 
			      TileType tile_type,
			      guint id)
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

	add_theme_image_to_hashtable (&priv->base_list[tile_type], ti);

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

	priv->base_last_id[tile_type] = MAX (priv->base_last_id[tile_type], id);
}

static void
add_theme_image_to_hashtable (GHashTable **hash_table, ThemeImage *ti)
{
	if (*hash_table == NULL)
		*hash_table = g_hash_table_new_full ((GHashFunc) g_direct_hash, 
						     (GCompareFunc) g_direct_equal,
						     (GDestroyNotify) NULL,
						     (GDestroyNotify) destroy_theme_image);

        if (g_hash_table_lookup (*hash_table, GINT_TO_POINTER (ti->id)) == NULL) {
		g_hash_table_insert(*hash_table, 
				    GINT_TO_POINTER (ti->id),
				    (gpointer) ti);
	}
	else {
		g_warning ("Couldn't register file %s, id %i already in use.",
			   ti->file, ti->id);
	}
}

void
theme_add_sub_image_with_id (Theme *theme, 
			     const gchar *name, 
			     const gchar *file, 
			     TileType type,
			     gboolean underlay,
			     guint id)
{
	ThemePrivate *priv;
	ThemeImage *ti;
	gint layer;
	
	g_return_if_fail (IS_THEME (theme));
	g_return_if_fail (name != NULL);
	g_return_if_fail (file != NULL);
	
	priv = theme->priv;
	layer = underlay ? 1 : 0;
	
	/* create new theme element */
	ti = g_new0 (ThemeImage, 1);

	ti->id = id;
	ti->file = g_strdup(file);
	ti->name = g_strdup(name);
	ti->loading_failed = FALSE;
	ti->image = NULL;     

	add_theme_image_to_hashtable (&priv->sub_list [layer][type], ti);
	
	priv->sub_last_id[layer][type] = 
		MAX (priv->sub_last_id[layer][type], id);	
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

void 
theme_set_selector_arrow_image (Theme *theme, const gchar *type, const gchar *file_name)
{
	ThemeImage *ti;
	gint n;
	
	g_return_if_fail(IS_THEME (theme));
	g_return_if_fail(file_name != NULL);
	g_return_if_fail(type != NULL);
	
	if (!g_strcasecmp (type, "top")) {
		n = 0;
	}
	else if (!g_strcasecmp (type, "right")) {
		n = 1;
	}
	else if (!g_strcasecmp (type, "bottom")) {
		n = 2;
	}
	else if (!g_strcasecmp (type, "left")) {
		n = 3;
	}
	else
		return;


	ti = g_new0 (ThemeImage, 1);
	ti->id = 0;
	ti->file = g_strdup(file_name);
	ti->name = g_strdup("arrow");
	ti->loading_failed = FALSE;
	ti->image = NULL;

	theme->priv->selector_arrows[n] = ti;	
}


#if 0
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
#endif


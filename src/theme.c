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

#include "theme.h"
#include "theme-private.h"


/*=================================================================
 
  Declaration of internal functions

  ---------------------------------------------------------------*/

static void theme_class_init (GObjectClass *class);
static void theme_init (Theme *theme);
static void theme_finalize (GObject *object);
static void destroy_theme_image (gpointer data);

static GObjectClass *parent_class;

GType theme_get_type (void)
{
  static GType object_type = 0;

  if (!object_type)
    {
      static const GTypeInfo object_info = {
	sizeof (ThemeClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) theme_class_init,
	NULL,			/* clas_finalize */
	NULL,			/* class_data */
	sizeof (Theme),
	0,			/* n_preallocs */
	(GInstanceInitFunc) theme_init,
      };

      object_type = g_type_register_static (G_TYPE_OBJECT,
					    "Theme", &object_info, 0);
    }

  return object_type;
}


/*=================================================================
 
  Theme creation, initialisation and clean up

  ---------------------------------------------------------------*/

static void theme_class_init (GObjectClass *class)
{
  parent_class = g_type_class_ref (G_TYPE_OBJECT);
  class->finalize = theme_finalize;
}

static void theme_init (Theme *theme)
{
  ThemePrivate *priv;

  priv = g_new0 (ThemePrivate, 1);

  priv->name = NULL;
  priv->path = NULL;
  priv->tile_width = 0;
  priv->tile_height = 0;
  priv->animstep = 0;
  priv->bg_color.red = 0;
  priv->bg_color.green = 0;
  priv->bg_color.blue = 0;
  g_datalist_init (&priv->images);

  theme->priv = priv;
}

static void theme_finalize (GObject *object)
{
  ThemePrivate *priv;
  Theme *theme = THEME (object);

  g_return_if_fail (theme != NULL);

#ifdef DEBUG
  g_message ("Finalize theme");
#endif
  priv = theme->priv;

  if (priv->name)
    g_free (priv->name);
  priv->name = NULL;

  if (priv->path)
    g_free (priv->path);
  priv->path = NULL;

  if (priv->images)
    g_datalist_clear (&priv->images);

  g_free (theme->priv);
  theme->priv = NULL;
}

Theme *theme_new (void)
{
  Theme *theme;
  theme = THEME (g_object_new (THEME_TYPE, NULL));
  return theme;
}


/*=================================================================
 
  Theme functions

  ---------------------------------------------------------------*/

gchar *theme_get_name (Theme *theme)
{
  g_return_val_if_fail (IS_THEME (theme), NULL);

  return theme->priv->name;
}

gint theme_get_animstep (Theme *theme)
{
  g_return_val_if_fail (IS_THEME (theme), 0);

  return theme->priv->animstep;
}

static void destroy_theme_image (gpointer data)
{
  ThemeImage *ti;

  ti = (ThemeImage *) data;

  if (ti == NULL)
    return;

  if (ti->file)
    g_free (ti->file);

  if (ti->image)
    g_object_unref (ti->image);

  if (ti->decorations)
    g_slist_free (ti->decorations);

  g_free (ti);
}

static GdkPixbuf *get_theme_image_pixbuf (ThemeImage *ti)
{
  GdkPixbuf *pixbuf;

  if (ti == NULL)
    return NULL;

  if (ti->loading_failed)
    return NULL;

  if (ti->image == NULL)
    {
      ti->image = gdk_pixbuf_new_from_file (ti->file, NULL);
      if (ti->image == NULL)
	ti->loading_failed = TRUE;
      else if (ti->alpha < 255)
	{
	  pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB, TRUE, 8,
				   gdk_pixbuf_get_width (ti->image),
				   gdk_pixbuf_get_height (ti->image));
	  gdk_pixbuf_fill (pixbuf, 0x00000000);
	  gdk_pixbuf_composite (ti->image,
				pixbuf,
				0, 0,
				gdk_pixbuf_get_width (pixbuf),
				gdk_pixbuf_get_height (pixbuf),
				0.0, 0.0, 1.0, 1.0,
				GDK_INTERP_NEAREST, ti->alpha);
	  g_object_unref (ti->image);
	  ti->image = pixbuf;
	}
    }

  if (ti->image != NULL)
    g_object_ref (ti->image);

  return ti->image;
}

static GdkPixbuf *create_sub_images (Theme *theme, Tile *tile,
				     TileSubType sub_type)
{
  ThemePrivate *priv;
  GSList *elem;
  ThemeImage *ti;
  GdkPixbuf *pixbuf = NULL;
  GdkPixbuf *pb;

  g_return_val_if_fail (IS_THEME (theme), NULL);
  g_return_val_if_fail (IS_TILE (tile), NULL);

  priv = theme->priv;

  elem = tile_get_sub_ids (tile, sub_type);

  for (; elem != NULL; elem = elem->next)
    {
      ti = g_datalist_id_get_data (&priv->images, GPOINTER_TO_QUARK (elem->data));
      if (ti == NULL)
	continue;

      pb = get_theme_image_pixbuf (ti);
      if (pb == NULL)
	{
#ifdef DEBUG
	  g_warning ("Couldn't load sub image: %s",
		     g_quark_to_string (GPOINTER_TO_QUARK (elem->data)));
#endif
	  continue;
	}

      if (pixbuf == NULL)
	{
	  pixbuf = gdk_pixbuf_copy (pb);
	}
      else
	{
	  gdk_pixbuf_composite (pb,
				pixbuf,
				0, 0,
				gdk_pixbuf_get_width (pixbuf),
				gdk_pixbuf_get_height (pixbuf),
				0.0, 0.0, 1.0, 1.0, GDK_INTERP_NEAREST, 255);
	}
      g_object_unref (pb);
    }

  return pixbuf;
}

gboolean theme_apply_decoration (Theme *theme, Tile *tile)
{
  static int counter = 0;
  ThemeImage *ti;
  gint n_decorations;
  GQuark decor_id;

  if (tile == NULL)
    return FALSE;

  g_return_val_if_fail (IS_THEME (theme), FALSE);
  g_return_val_if_fail (IS_TILE (tile), FALSE);

  ti = g_datalist_id_get_data (&theme->priv->images, tile_get_base_id (tile));
  if (ti->decorations == NULL)
    return FALSE;

  n_decorations = g_slist_length (ti->decorations);
  decor_id = GPOINTER_TO_QUARK (g_slist_nth_data (ti->decorations,
					counter++ % n_decorations));
  tile_add_sub_id (tile, decor_id, TILE_SUB_OVERLAY);

  return TRUE;
}

GdkPixbuf *theme_get_tile_image (Theme *theme, Tile *tile)
{
  ThemePrivate *priv;
  GQuark image_id;
  ThemeImage *ti = NULL;
  GdkPixbuf *underlay_pb = NULL;
  GdkPixbuf *overlay_pb = NULL;
  GdkPixbuf *base_pb = NULL;
  GdkPixbuf *result = NULL;
  gint tw, th;

  g_return_val_if_fail (IS_THEME (theme), NULL);
  g_return_val_if_fail (IS_TILE (tile), NULL);

  priv = theme->priv;

  image_id = tile_get_base_id (tile);

  underlay_pb = create_sub_images (theme, tile, TILE_SUB_UNDERLAY);
  overlay_pb = create_sub_images (theme, tile, TILE_SUB_OVERLAY);

  ti = g_datalist_id_get_data (&priv->images, image_id);
  base_pb = get_theme_image_pixbuf (ti);
  if (base_pb == NULL)
    {
      theme_get_tile_size (theme, &tw, &th);
      base_pb = gdk_pixbuf_new (GDK_COLORSPACE_RGB, TRUE, 8, tw, th);
      gdk_pixbuf_fill (base_pb, 0);
    }

  if (underlay_pb && overlay_pb)
    {
      result = gdk_pixbuf_copy (underlay_pb);
      gdk_pixbuf_composite (base_pb,
			    result,
			    0, 0,
			    gdk_pixbuf_get_width (result),
			    gdk_pixbuf_get_height (result),
			    0.0, 0.0, 1.0, 1.0, GDK_INTERP_NEAREST, 255);
      gdk_pixbuf_composite (overlay_pb,
			    result,
			    0, 0,
			    gdk_pixbuf_get_width (result),
			    gdk_pixbuf_get_height (result),
			    0.0, 0.0, 1.0, 1.0, GDK_INTERP_NEAREST, 255);
      g_object_unref (overlay_pb);
      g_object_unref (underlay_pb);
    }
  else if (!underlay_pb && overlay_pb)
    {
      result = gdk_pixbuf_copy (base_pb);
      gdk_pixbuf_composite (overlay_pb,
			    result,
			    0, 0,
			    gdk_pixbuf_get_width (result),
			    gdk_pixbuf_get_height (result),
			    0.0, 0.0, 1.0, 1.0, GDK_INTERP_NEAREST, 255);
      g_object_unref (overlay_pb);
    }
  else if (underlay_pb && !overlay_pb)
    {
      result = gdk_pixbuf_copy (underlay_pb);
      gdk_pixbuf_composite (base_pb,
			    result,
			    0, 0,
			    gdk_pixbuf_get_width (result),
			    gdk_pixbuf_get_height (result),
			    0.0, 0.0, 1.0, 1.0, GDK_INTERP_NEAREST, 255);
      g_object_unref (underlay_pb);
    }
  else
    result = g_object_ref (base_pb);

  g_object_unref (base_pb);

  return result;
}

GdkPixbuf *theme_get_selector_image (Theme *theme)
{
  static GQuark cursor_id = 0;
  ThemeImage *ti;

  if (!cursor_id)
    cursor_id = g_quark_from_static_string ("cursor");

  g_return_val_if_fail (IS_THEME (theme), NULL);

  ti = g_datalist_id_get_data (&theme->priv->images, cursor_id);

  return get_theme_image_pixbuf (ti);
}

void theme_get_selector_arrow_images (Theme *theme, GdkPixbuf **arrows)
{
  gint i;
  ThemeImage *ti;
  static GQuark arrow_id[4] = { 0, 0, 0, 0 };

  if (!arrow_id[0])
    {
      arrow_id[0] = g_quark_from_static_string ("arrow-top");
      arrow_id[1] = g_quark_from_static_string ("arrow-right");
      arrow_id[2] = g_quark_from_static_string ("arrow-bottom");
      arrow_id[3] = g_quark_from_static_string ("arrow-left");
    }

  for (i = 0; i < 4; i++)
    {
      ti = g_datalist_id_get_data (&theme->priv->images, arrow_id[i]);
      arrows[i] = get_theme_image_pixbuf (ti);
    }
}

void theme_get_tile_size (Theme *theme, gint *width, gint *height)
{
  *width = *height = 0;

  g_return_if_fail (IS_THEME (theme));

  *width = theme->priv->tile_width;
  *height = theme->priv->tile_height;
}

GdkRGBA *theme_get_background_color (Theme *theme)
{
  return &(theme->priv->bg_color);
}


GQuark theme_add_image (Theme *theme, const gchar *src, gint alpha)
{
  ThemeImage *ti;
  gchar *filename;
  gchar *suffix;

  g_return_val_if_fail (IS_THEME (theme), 0);
  g_return_val_if_fail (src != NULL, 0);
  g_return_val_if_fail (0 <= alpha && alpha <= 255, 0);

  ti = g_new0 (ThemeImage, 1);
  ti->file = g_strdup (src);
  ti->loading_failed = FALSE;
  ti->image = NULL;
  ti->alpha = alpha;
  ti->decorations = NULL;

  filename = g_path_get_basename (src);
  suffix = g_strrstr (filename, ".png");
  if (suffix != NULL)
    *suffix = '\0';
  else
    {
      suffix = g_strrstr (filename, ".gif");
      if (suffix != NULL)
	*suffix = '\0';
    }

  ti->id = g_quark_from_string (filename);

  g_datalist_id_set_data_full (&theme->priv->images,
			       ti->id, ti, destroy_theme_image);

  g_free (filename);

  return ti->id;
}

void theme_add_image_decoration (Theme *theme, GQuark base, GQuark decor)
{
  ThemeImage *ti;

  g_return_if_fail (IS_THEME (theme));

  ti = g_datalist_id_get_data (&theme->priv->images, base);
  if (ti == NULL)
    return;

  ti->decorations = g_slist_append (ti->decorations, GQUARK_TO_POINTER (decor));
}

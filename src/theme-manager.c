/* -*- tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
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

#include <glib.h>
#include <stdlib.h>

#include "theme-manager.h"
#include "theme-private.h"
#include "xml-util.h"

static GObjectClass *parent_class = NULL;

static void search_themes_in_dir (ThemeManager *tm, const gchar *dir_path);
static void add_theme (ThemeManager *tm, gchar *themename, gchar *dirpath);
static void add_theme_to_list (gchar *key, gpointer value, GList **list);
static Theme *load_theme (gchar *theme_dir);
static gchar *lookup_theme_name (gchar *theme_file);

static void theme_manager_class_init (GObjectClass *class);
static void theme_manager_init (ThemeManager *tm);
static void theme_manager_finalize (GObject *object);

struct _ThemeManagerPrivate
{
  gboolean initialized;
  GHashTable *themes;
};

static void
theme_parser_start_element (GMarkupParseContext  *context,
                            const gchar          *element_name,
                            const gchar         **attribute_names,
                            const gchar         **attribute_values,
                            gpointer              user_data,
                            GError              **error)
{
  const gchar *prop_value;
  Theme *theme = THEME (user_data);
  ThemePrivate *priv = theme->priv;

  if (!g_strcmp0 (element_name, "theme")) {
    prop_value = get_attribute_value ("name", attribute_names, attribute_values);
    priv->name = g_strdup (prop_value);
  } else if (!g_strcmp0 (element_name, "icon")) {
    gchar *src;
    GdkPixbuf *pixbuf;
    gint alpha = 255;

    src = g_build_filename (theme->priv->path, 
                            get_attribute_value ("src", attribute_names, attribute_values),
                            NULL);

    prop_value = get_attribute_value ("alpha", attribute_names, attribute_values);

    if (prop_value != NULL)
      alpha = CLAMP (atoi (prop_value), 0, 255);

    if (theme->priv->tile_width == 0) {
      pixbuf = gdk_pixbuf_new_from_file (src, NULL);
      if (pixbuf != NULL) {
        theme->priv->tile_width = gdk_pixbuf_get_width (pixbuf);
        theme->priv->tile_height = gdk_pixbuf_get_height (pixbuf);
        g_object_unref (pixbuf);
      }
    }

    theme_add_image (theme, src, alpha);

    g_free (src);
  } else if (!g_strcmp0 (element_name, "animstep")) {
    priv->animstep = atoi (get_attribute_value ("dist", attribute_names, 
                                                    attribute_values));
  } else if (!g_strcmp0 (element_name, "bgcolor")) {
    /* handle background color */
    prop_value = get_attribute_value ("color", attribute_names, attribute_values);
    gdk_rgba_parse (&(priv->bg_color), prop_value);
  } else if (!g_strcmp0 (element_name, "bgcolor_rgb")) {
    /* handle rgb color node */
    prop_value = get_attribute_value ("red", attribute_names, attribute_values);
    priv->bg_color.red = (atof (prop_value) / 255.0) * 65536;
    prop_value = get_attribute_value ("blue", attribute_names, attribute_values);
    priv->bg_color.blue = (atof (prop_value) / 255.0) * 65536;
    prop_value = get_attribute_value ("green", attribute_names, attribute_values);
    priv->bg_color.green = (atof (prop_value) / 255.0) * 65536;
  } else if (!g_strcmp0 (element_name, "decor")) {
    gchar *src;
    const gchar *base;
    GQuark base_id;
    GQuark decor_id;
    gint alpha = 255;

    src = g_build_filename (theme->priv->path, 
                            get_attribute_value ("src", attribute_names, attribute_values),
                            NULL);

    prop_value = get_attribute_value ("alpha", attribute_names, attribute_values);
    if (prop_value != NULL) {
      alpha = CLAMP (atoi (prop_value), 0, 255);
    }

    decor_id = theme_add_image (theme, src, alpha);
    g_free (src);

    base = get_attribute_value ("base", attribute_names, attribute_values);
    if (base == NULL)
      return;

    base_id = g_quark_from_string (base);
    theme_add_image_decoration (theme, base_id, decor_id);
  }
}

static GMarkupParser theme_parser =
{
  theme_parser_start_element,
  NULL,
  NULL,
  NULL,
  xml_parser_log_error
};

GType theme_manager_get_type (void)
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
	  NULL,			/* clas_finalize */
	  NULL,			/* class_data */
	  sizeof (ThemeManager),
	  0,			/* n_preallocs */
	  (GInstanceInitFunc) theme_manager_init,
	};

      object_type = g_type_register_static (G_TYPE_OBJECT,
					    "ThemeManager", &object_info, 0);
    }

  return object_type;
}

/*=================================================================
 
  Theme_Manager creation, initialisation and clean up

  ---------------------------------------------------------------*/

static void theme_manager_class_init (GObjectClass *class)
{
  parent_class = g_type_class_ref (G_TYPE_OBJECT);
  class->finalize = theme_manager_finalize;
}

static void theme_manager_init (ThemeManager *tm)
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

static void theme_manager_finalize (GObject *object)
{
  ThemeManager *tm = THEME_MANAGER (object);
  g_hash_table_destroy (tm->priv->themes);

  g_free (tm->priv);
  tm->priv = NULL;
}

ThemeManager *theme_manager_new (void)
{
  ThemeManager *tm;

  tm = THEME_MANAGER (g_object_new (THEME_MANAGER_TYPE, NULL));
  return tm;
}

void theme_manager_init_themes (ThemeManager *tm)
{
  gchar *dir_path;

  g_return_if_fail (IS_THEME_MANAGER (tm));
  g_return_if_fail (!tm->priv->initialized);

  dir_path = g_build_filename (g_get_home_dir (), ".atomix", "themes", NULL);
  search_themes_in_dir (tm, dir_path);
  g_free (dir_path);

  dir_path = g_build_filename (DATADIR, "atomix", "themes", NULL);
  search_themes_in_dir (tm, dir_path);
  g_free (dir_path);

  if (g_hash_table_size (tm->priv->themes) == 0)
    g_warning ("No themes found.");

  tm->priv->initialized = TRUE;
}

static void search_themes_in_dir (ThemeManager *tm, const gchar *dir_path)
{
  struct dirent *dent = NULL;
  DIR *dir;

  dir = opendir (dir_path);
  if (dir)
    {
      char *filename;
      char *themename;
      char *subdirpath;

      while ((dent = readdir (dir)) != NULL)
	{
	  if ((g_ascii_strcasecmp (".", dent->d_name) != 0) &&
	      (g_ascii_strcasecmp ("..", dent->d_name) != 0))
	    {
	      /* is current file a directory? */
	      subdirpath = g_build_filename (dir_path, dent->d_name, NULL);
	      if (g_file_test (subdirpath, G_FILE_TEST_IS_DIR))
		{
		  /* try to load file */
		  filename = g_build_filename (subdirpath, "theme", NULL);
		  themename = lookup_theme_name (filename);
		  add_theme (tm, themename, subdirpath);
		  g_free (filename);
		  g_free (themename);
		}
	      g_free (subdirpath);
	    }
	}
      g_free (dent);
      closedir (dir);
    }
}

static void add_theme (ThemeManager *tm, gchar *themename, gchar *dirpath)
{
  gchar *search_result;

  g_return_if_fail (IS_THEME_MANAGER (tm));
  g_return_if_fail (themename != NULL);
  g_return_if_fail (dirpath != NULL);

  /* don't add a theme twice */
  search_result = g_hash_table_lookup (tm->priv->themes, themename);

  if (search_result == NULL)
    {
      g_hash_table_insert (tm->priv->themes,
			   g_strdup (themename), g_strdup (dirpath));

#ifdef DEBUG
      g_message ("Found theme “%s” in: %s", themename, dirpath);
#endif
    }
}

GList *theme_manager_get_available_themes (ThemeManager *tm)
{
  GList *list = NULL;

  g_return_val_if_fail (IS_THEME_MANAGER (tm), NULL);

  g_hash_table_foreach (tm->priv->themes, (GHFunc) add_theme_to_list, &list);

  return list;
}

static void add_theme_to_list (gchar *key, gpointer value, GList **list)
{
  *list =
    g_list_insert_sorted (*list, key, (GCompareFunc) g_ascii_strcasecmp);
}

/* =======================================================================
  
                      Theme loading stuff

  ======================================================================== */


Theme *theme_manager_get_theme (ThemeManager *tm, const gchar *theme_name)
{
  Theme *theme;
  gchar *theme_dir;

  g_return_val_if_fail (IS_THEME_MANAGER (tm), NULL);
  g_return_val_if_fail (tm->priv->initialized, NULL);

  theme_dir = g_hash_table_lookup (tm->priv->themes, theme_name);
  if (theme_dir == NULL)
    return NULL;

  theme = load_theme (theme_dir);

  return theme;
}


static Theme *load_theme (gchar *theme_dir)
{
  Theme *theme = NULL;
  ThemePrivate *priv;
  gchar *theme_file_path;
  GFile *theme_file;
  gchar *theme_contents;
  gsize theme_length;
  GMarkupParseContext *parse_context;

  g_return_val_if_fail (theme_dir != NULL, NULL);

  theme_file_path = g_build_filename (theme_dir, "theme", NULL);

  if (!g_file_test (theme_file_path, G_FILE_TEST_IS_REGULAR))
    {
      g_warning ("File not found: %s.", theme_file_path);
      g_free (theme_file_path);
      return NULL;
    }

  theme_file = g_file_new_for_path (theme_file_path);
  g_free (theme_file_path);

  if (g_file_load_contents (theme_file, NULL, &theme_contents, &theme_length, NULL, NULL)) {
    theme = theme_new ();
    priv = theme->priv;
    priv->path = g_strdup (theme_dir);
    parse_context = g_markup_parse_context_new (&theme_parser,
                                                G_MARKUP_TREAT_CDATA_AS_TEXT,
                                                theme,
                                                NULL);
    g_markup_parse_context_parse (parse_context, theme_contents, theme_length, NULL);
    g_markup_parse_context_unref (parse_context);
    g_free (theme_contents);
  }

  g_object_unref (theme_file);

  return theme;
}

static gchar *lookup_theme_name (gchar *theme_file)
{
  gchar *name = NULL;
  GFile *file;
  GFile *folder;
  Theme *theme = NULL;
  gchar *theme_path = NULL;

  file = g_file_new_for_path (theme_file);
  folder = g_file_get_parent (file);
  theme_path = g_file_get_path (folder);
  theme = load_theme (theme_path);

  name = theme_get_name (theme);

  g_free (theme_path);
  g_object_unref (file);
  g_object_unref (folder);

  return name;
}

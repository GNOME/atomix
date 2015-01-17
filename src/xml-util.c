/* Atomix -- a little puzzle game about atoms and molecules.
 * Copyright (C) 2001 Jens Finke
 * Copyright (C) 2005 Guilherme de S. Pastore
 * Copyright (C) 2015 Robert Roth
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

#include "xml-util.h"

const gchar* get_attribute_value (const gchar *attribute,
                                  const gchar **attribute_names,
                                  const gchar **attribute_values)
{
  gint i = 0;

  while (attribute_names[i]) {
    if (!g_strcmp0 (attribute_names[i], attribute))
      return attribute_values[i];
    i++;
  }

  return NULL;
}

void xml_parser_log_error (GMarkupParseContext *context,
                           GError *error,
                           gpointer user_data)
{
  g_print ("Error while parsing XML: %s with user data of type %s\n", error->message, G_OBJECT_TYPE_NAME (user_data));
}

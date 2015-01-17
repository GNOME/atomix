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

#ifndef _ATOMIX_XML_UTIL_H_
#define _ATOMIX_XML_UTIL_H_

#include <glib-object.h>

const gchar* get_attribute_value (const gchar *attribute,
                                  const gchar **attribute_names,
                                  const gchar **attribute_values);

void xml_parser_log_error (GMarkupParseContext *context,
                           GError *error,
                           gpointer user_data);

#endif //_ATOMIX_XML_UTIL_H_

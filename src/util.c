/* Atomix -- a little mind game about atoms and molecules.
 * Copyright (C) 1999-2000 Jens Finke
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
#include "main.h"
#include "util.h"

void set_appbar_temporary(gchar *txt)
{
#if 0
	static gint id = -1;
	GnomeAppBar* appbar = GNOME_APPBAR(glade_xml_get_widget (get_gui (), "appbar"));
	if(id != -1) gtk_timeout_remove(id);
	gnome_appbar_set_status(appbar, txt);
	id = gtk_timeout_add(2000, clear_appbar, &id);    
#endif
}

int
clear_appbar(void *data)
{
#if 0
	int *id = data;
	GnomeAppBar* appbar = GNOME_APPBAR(glade_xml_get_widget (get_gui (), "appbar"));
	gnome_appbar_refresh(appbar);
	gtk_timeout_remove(*id);
	*id = -1;

#endif
	return TRUE;
}

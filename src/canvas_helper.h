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
#ifndef _ATOMIX_CANVAS_HELPER_H_
#define _ATOMIX_CANVAS_HELPER_H_

#include <gnome.h>
#include "theme.h"

void set_background_color (GtkWidget * canvas, GdkColor * color);

void convert_to_playfield (Theme * theme, gdouble x, gdouble y,
			   guint * row, guint * col);

void convert_to_canvas (Theme * theme, guint row, guint col,
			gdouble * x, gdouble * y);

GnomeCanvasGroup *create_group (GnomeCanvas * canvas,
				GnomeCanvasGroup * parent);

#endif /* _ATOMIX_CANVAS_HELPER_H_ */

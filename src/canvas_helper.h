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
#ifndef _ATOMIX_CANVAS_HELPER_H_
#define _ATOMIX_CANVAS_HELPER_H_

#include "theme.h"
#include "playfield.h"

#define BGR_FLOOR_ROWS 15
#define BGR_FLOOR_COLS 15


void convert_to_playfield (Theme * theme, PlayField * playfield, gint x, gint y,
			   guint * row, guint * col);

void convert_to_canvas (Theme * theme, PlayField * playfield, guint row, guint col,
			gint * x, gint * y);

#endif /* _ATOMIX_CANVAS_HELPER_H_ */

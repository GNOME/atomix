/* Atomix -- a little puzzle game about atoms and molecules.
 * Copyright (C) 1999 Jens Finke
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

#ifndef _ATOMIX_BOARD_GTK_H_
#define _ATOMIX_BOARD_GTK_H_

#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <math.h>
#include "theme.h"
#include "playfield.h"
#include "goal.h"
#include "undo.h"
#include "canvas_helper.h"

void board_gtk_init (Theme * theme, gpointer canvas);

void board_gtk_init_level (PlayField * env, PlayField * sce, Goal * goal);

void board_gtk_destroy (void);

void board_gtk_clear (void);

void board_gtk_print (void);

void board_gtk_hide (void);

void board_gtk_show (void);

gboolean board_gtk_undo_move (void);

void board_gtk_show_logo (gboolean visible);

#endif /* _ATOMIX_BOARD_GTK_H_ */

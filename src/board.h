/* Atomix -- a little puzzle game about atoms and molecules.
 * Copyright (C) 1999 Jens Finke
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

#ifndef _ATOMIX_BOARD_H_
#define _ATOMIX_BOARD_H_

#include "theme.h"
#include "playfield.h"
#include "goal.h"

enum
{
  BOARD_MSG_NONE,
  BOARD_MSG_GAME_PAUSED,
  BOARD_MSG_NEW_GAME,
  BOARD_MSG_GAME_OVER
};

void board_init (Theme * theme, GnomeCanvas * canvas);

void board_init_level (PlayField * env, PlayField * sce, Goal * goal);

void board_destroy (void);

void board_clear (void);

void board_print (void);

void board_hide (void);

void board_show (void);

void board_show_normal_cursor (void);

gboolean board_undo_move (void);

void board_view_message (gint msg_id);

void board_hide_message (gint msg_id);

void board_show_logo (gboolean visible);

void board_handle_key_event (GObject * canvas, GdkEventKey * event,
			     gpointer data);

#endif /* _ATOMIX_BOARD_H_ */

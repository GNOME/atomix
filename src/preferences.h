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

#ifndef _ATOMIX_PREFERENCES_H_
#define _ATOMIX_PREFERENCES_H_

#include <gnome.h>

typedef struct _Preferences Preferences;

struct _Preferences
{
  gboolean mouse_control;
  gboolean keyboard_control;
  gboolean hide_cursor;
  gboolean lazy_dragging;
  gint mouse_sensitivity;
  gboolean score_time_enabled;
};


void preferences_init (void);

void preferences_destroy (void);

void preferences_save (void);

Preferences *preferences_get (void);

void preferences_show_dialog (void);

#endif /* _ATOMIX_PREFERENCES_H_ */

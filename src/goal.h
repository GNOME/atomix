/* Atomix -- a little mind game about atoms and molecules.
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

#ifndef _ATOMIX_GOAL_H_
#define _ATOMIX_GOAL_H_

#include <gnome.h>
#include "playfield.h"
#include "main.h"

typedef struct _Goal        Goal;

struct _Goal
{
	PlayField*   pf;
	GHashTable*  index;
	GnomeCanvasGroup*  item_group;
};


Goal* goal_new(PlayField* pf);

void goal_destroy(Goal* goal);

gboolean goal_reached(Goal* goal, PlayField* pf, guint row_anchor, 
		      guint col_anchor);

void goal_print(Goal* goal);

void goal_render(Goal* goal);

void goal_clear(Goal* goal);

void goal_init (AtomixApp *app);

#endif /* _ATOMIX_GOAL_H_ */

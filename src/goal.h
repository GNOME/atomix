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

#ifndef _ATOMIX_GOAL_H_
#define _ATOMIX_GOAL_H_

#include <glib-object.h>
#include "playfield.h"

#define GOAL_TYPE        (goal_get_type ())
#define GOAL(o)          (G_TYPE_CHECK_INSTANCE_CAST ((o), GOAL_TYPE, Goal))
#define GOAL_CLASS(k)    (G_TYPE_CHECK_CLASS_CAST((k), GOAL_TYPE, GoalClass))
#define IS_GOAL(o)       (G_TYPE_CHECK_INSTANCE_TYPE ((o), GOAL_TYPE))
#define IS_GOAL_CLASS(k) (G_TYPE_CHECK_CLASS_TYPE ((k), GOAL_TYPE))
#define GOAL_GET_CLASS(o)(G_TYPE_INSTANCE_GET_CLASS ((o), GOAL_TYPE, GoalClass))


typedef struct _GoalPrivate GoalPrivate;

typedef struct
{
  GObject parent;
  GoalPrivate *priv;
} Goal;

typedef struct
{
  GObjectClass parent_class;
} GoalClass;

GType goal_get_type (void);

Goal *goal_new (PlayField * pf);

PlayField *goal_get_playfield (Goal * goal);

gboolean goal_reached (Goal * goal, PlayField * pf,
		       guint row_anchor, guint col_anchor);

void goal_print (Goal * goal);

#endif /* _ATOMIX_GOAL_H_ */

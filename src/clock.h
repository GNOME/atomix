/* Atomix -- a little puzzle game about atoms and molecules.
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

#ifndef _ATOMIX_CLOCK_H_
#define _ATOMIX_CLOCK_H_

#include <gtk/gtk.h>

G_BEGIN_DECLS
#define TYPE_CLOCK		(clock_get_type ())
#define CLOCK(obj)		(G_TYPE_CHECK_INSTANCE_CAST((obj), TYPE_CLOCK, Clock))
#define CLOCK_CLASS(klass)	(G_TYPE_CHECK_CAST_CLASS((klass), TYPE_CLOCK, ClockClass))

typedef struct
{
  GtkLabel widget;
  GTimer *timer;
  gint timer_id;
  gchar *fmt;
} Clock;

typedef struct
{
  GtkLabelClass parent_class;
} ClockClass;

GType clock_get_type (void);
GtkWidget *clock_new (void);
void clock_reset (Clock *);
void clock_start (Clock *);
void clock_resume (Clock *);
gint clock_get_elapsed (Clock *);
void clock_stop (Clock *);

G_END_DECLS

#endif /* _ATOMIX_CLOCK_H_ */

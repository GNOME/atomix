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

#include <time.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS
#define TYPE_CLOCK		(clock_get_type ())
#define CLOCK(obj)		(G_TYPE_CHECK_INSTANCE_CAST((obj), TYPE_CLOCK, Clock))
#define CLOCK_CLASS(klass)	(G_TYPE_CHECK_CAST_CLASS((klass), TYPE_CLOCK, ClockClass))

typedef struct
{
  GtkLabel widget;
  gint timer_id;
  gint update_interval;
  time_t seconds;
  time_t stopped;
  gchar *fmt;
  struct tm *tm;
} Clock;

typedef struct
{
  GtkLabelClass parent_class;
} ClockClass;

GType clock_get_type (void);
GtkWidget *clock_new (void);
void clock_set_format (Clock *, const gchar *);
void clock_set_seconds (Clock *, time_t);
void clock_set_update_interval (Clock *, gint);
void clock_start (Clock *);
void clock_stop (Clock *);

G_END_DECLS

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
#include <string.h>
#include "clock.h"

static void clock_class_init (ClockClass *klass);
static void clock_init (Clock *clock);

static GtkLabelClass *parent_class = NULL;

GType clock_get_type (void)
{
  static GType object_type = 0;

  if (!object_type)
    {
      static const GTypeInfo object_info =
	{
	  sizeof (ClockClass),
	  (GBaseInitFunc) NULL,
	  (GBaseFinalizeFunc) NULL,
	  (GClassInitFunc) clock_class_init,
	  NULL,			/* clas_finalize */
	  NULL,			/* class_data */
	  sizeof (Clock),
	  0,			/* n_preallocs */
	  (GInstanceInitFunc) clock_init,
	};

      object_type = g_type_register_static (GTK_TYPE_LABEL,
					    "Clock", &object_info, 0);
    }

  return object_type;
}

static void clock_destroy (GtkObject *object)
{
  g_return_if_fail (object != NULL);

  clock_stop (CLOCK (object));
  GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

static void clock_class_init (ClockClass *klass)
{
  GtkObjectClass *object_class = (GtkObjectClass *) klass;

  object_class->destroy = clock_destroy;
  parent_class = g_type_class_peek (gtk_label_get_type ());
}

static void clock_init (Clock *clock)
{
  clock->timer_id = -1;
  clock->update_interval = 1;
  clock->seconds = time (NULL);
  clock->stopped = 0;
}

static void clock_gen_str (Clock *clock)
{
  gchar timestr[64];
  time_t secs;

  secs = time (NULL) - clock->seconds;

  clock->tm->tm_hour = secs / 3600;
  secs -= clock->tm->tm_hour * 3600;
  clock->tm->tm_min = secs / 60;
  clock->tm->tm_sec = secs - clock->tm->tm_min * 60;

  strftime (timestr, 64, clock->fmt, clock->tm);
  gtk_label_set_text (GTK_LABEL (clock), timestr);
}

static gint clock_timer_callback (gpointer data)
{
  Clock *clock = (Clock *) data;

  GDK_THREADS_ENTER ();
  clock_gen_str (clock);
  GDK_THREADS_LEAVE ();

  return TRUE;
}

GtkWidget *clock_new ()
{
  Clock *clock = CLOCK (g_object_new (TYPE_CLOCK, NULL));

  clock->fmt = g_strdup ("%H:%M:%S");
  clock->tm = g_new (struct tm, 1);
  memset (clock->tm, 0, sizeof (struct tm));
  clock->update_interval = 1;

  clock_gen_str (clock);

  return GTK_WIDGET (clock);
}

void clock_set_format (Clock *clock, const gchar *fmt)
{
  g_return_if_fail (clock != NULL);
  g_return_if_fail (fmt != NULL);

  g_free (clock->fmt);
  clock->fmt = g_strdup (fmt);
}

void clock_set_seconds (Clock *clock, time_t seconds)
{
  g_return_if_fail (clock != NULL);

  clock->seconds = time (NULL) - seconds;

  if (clock->timer_id == -1)
    clock->stopped = seconds;

  clock_gen_str (clock);
}

void clock_start (Clock *clock)
{
  g_return_if_fail (clock != NULL);

  if (clock->timer_id != -1)
    return;

  clock_set_seconds (clock, clock->stopped);
  clock->timer_id = g_timeout_add (1000 * clock->update_interval,
				     clock_timer_callback, clock);
}

void clock_stop (Clock *clock)
{
  g_return_if_fail (clock != NULL);

  if (clock->timer_id == -1)
    return;

  clock->stopped = time (NULL) - clock->seconds;

  g_source_remove (clock->timer_id);
  clock->timer_id = -1;
}

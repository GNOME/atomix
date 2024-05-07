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

static void clock_destroy (GtkWidget *object)
{
  Clock *clock = CLOCK (object);

  g_return_if_fail (object != NULL);
  clock_stop (clock);

  GTK_WIDGET_CLASS (parent_class)->destroy (object);
}

static void clock_class_init (ClockClass *klass)
{
  GtkWidgetClass *object_class = (GtkWidgetClass *) klass;

  object_class->destroy = clock_destroy;
  parent_class = g_type_class_peek (gtk_label_get_type ());
}

static void clock_init (Clock *clock)
{
  clock->timer_id = -1;
  clock->timer = NULL;
}

static void clock_gen_str (Clock *clock)
{
  gchar *timestr;
  gchar *markup;
  gint secs = 0;
  GDateTime *dtm;

  if (clock->timer)
    secs = g_timer_elapsed (clock->timer, NULL);

  dtm = g_date_time_new_from_unix_utc (secs);
  timestr = g_date_time_format (dtm, clock->fmt);
  markup = g_strdup_printf("<tt>%s</tt>", timestr);
  g_date_time_unref (dtm);

  gtk_label_set_markup (GTK_LABEL (clock), markup);
  g_free (timestr);
  g_free (markup);
}

static gint clock_timer_callback (gpointer data)
{
  Clock *clock = (Clock *) data;

  clock_gen_str (clock);

  return TRUE;
}

GtkWidget *clock_new (void)
{
  Clock *clock = CLOCK (g_object_new (TYPE_CLOCK, NULL));

  clock->fmt = g_strdup ("%M:%S");

  clock_gen_str (clock);
  gtk_label_set_xalign (GTK_LABEL (clock), 0.0);

  return GTK_WIDGET (clock);
}

void clock_reset (Clock *clock)
{
  g_return_if_fail (clock != NULL);

  if (clock->timer)
    g_timer_start (clock->timer);

  clock_gen_str (clock);
}

static void start_timer (Clock *clock)
{
  if (clock->timer_id != -1)
    return;

  clock->timer_id = g_timeout_add_seconds (1, clock_timer_callback, clock);
}

void clock_resume (Clock *clock)
{
  g_return_if_fail (clock != NULL);

  if (clock->timer != NULL)
    g_timer_continue (clock->timer);

  start_timer (clock);
}

void clock_start (Clock *clock)
{
  g_return_if_fail (clock != NULL);

  if (clock->timer)
    g_timer_destroy (clock->timer);

  clock->timer = g_timer_new ();

  start_timer (clock);
}

void clock_stop (Clock *clock)
{
  g_return_if_fail (clock != NULL);

  if (clock->timer)
    g_timer_stop (clock->timer);

  if (clock->timer_id == -1)
    return;

  g_source_remove (clock->timer_id);
  clock->timer_id = -1;
}

gint clock_get_elapsed (Clock *clock)
{
  if (clock->timer)
    return g_timer_elapsed (clock->timer, NULL);
  return 0;
}

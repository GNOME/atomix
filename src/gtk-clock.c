/*
 * gtk-clock: The GTK clock widget
 * (C)1998 The Free Software Foundation
 *
 * Author: Szekeres István (szekeres@cyberspace.mht.bme.hu)
 *
 * Gnome 2.0 port: Jens Finke <jens@triq.net>
 */

#include <time.h>
#include <gtk/gtk.h>
#include <string.h>
#include "gtk-clock.h"

static void gtk_clock_class_init (GtkClockClass *klass);
static void gtk_clock_init (GtkClock *clock);

static GtkLabelClass *parent_class = NULL;

GType gtk_clock_get_type (void)
{
  static GType object_type = 0;

  if (!object_type)
    {
      static const GTypeInfo object_info =
	{
	  sizeof (GtkClockClass),
	  (GBaseInitFunc) NULL,
	  (GBaseFinalizeFunc) NULL,
	  (GClassInitFunc) gtk_clock_class_init,
	  NULL,			/* clas_finalize */
	  NULL,			/* class_data */
	  sizeof (GtkClock),
	  0,			/* n_preallocs */
	  (GInstanceInitFunc) gtk_clock_init,
	};

      object_type = g_type_register_static (GTK_TYPE_LABEL,
					    "GtkClock", &object_info, 0);
    }

  return object_type;
}

static void gtk_clock_destroy (GtkObject *object)
{
  g_return_if_fail (object != NULL);

  gtk_clock_stop (GTK_CLOCK (object));
  GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

static void gtk_clock_class_init (GtkClockClass *klass)
{
  GtkObjectClass *object_class = (GtkObjectClass *) klass;

  object_class->destroy = gtk_clock_destroy;
  parent_class = gtk_type_class (gtk_label_get_type ());
}

static void gtk_clock_init (GtkClock *clock)
{
  clock->timer_id = -1;
  clock->update_interval = 1;
  clock->seconds = time (NULL);
  clock->stopped = 0;
}

static void gtk_clock_gen_str (GtkClock *clock)
{
  gchar timestr[64];
  time_t secs;

  switch (clock->type)
    {
    case GTK_CLOCK_DECREASING:
      secs = clock->seconds - time (NULL);
      break;
    case GTK_CLOCK_INCREASING:
      secs = time (NULL) - clock->seconds;
      break;
    case GTK_CLOCK_REALTIME:
      secs = time (NULL);
      break;
    }

  if (clock->type == GTK_CLOCK_REALTIME)
    {
      clock->tm = localtime (&secs);
    }
  else
    {
      clock->tm->tm_hour = secs / 3600;
      secs -= clock->tm->tm_hour * 3600;
      clock->tm->tm_min = secs / 60;
      clock->tm->tm_sec = secs - clock->tm->tm_min * 60;
    }

  strftime (timestr, 64, clock->fmt, clock->tm);
  gtk_label_set_text (GTK_LABEL (clock), timestr);
}

static gint gtk_clock_timer_callback (gpointer data)
{
  GtkClock *clock = (GtkClock *) data;

  GDK_THREADS_ENTER ();
  gtk_clock_gen_str (clock);
  GDK_THREADS_LEAVE ();

  return TRUE;
}

static gint gtk_clock_timer_first_callback (gpointer data)
{
  GtkClock *clock = (GtkClock *) data;
  gint tmpid;

  GDK_THREADS_ENTER ();

  gtk_clock_gen_str (clock);

  tmpid = gtk_timeout_add (1000 * clock->update_interval,
			   gtk_clock_timer_callback, clock);

  gtk_clock_stop (clock);

  clock->timer_id = tmpid;

  GDK_THREADS_LEAVE ();

  return FALSE;
}

GtkWidget *gtk_clock_new (GtkClockType type)
{
  GtkClock *clock = GTK_CLOCK (g_object_new (GTK_TYPE_CLOCK, NULL));

  clock->type = type;

  if (type == GTK_CLOCK_REALTIME)
    {
      clock->fmt = g_strdup ("%H:%M");
      clock->update_interval = 60;
      clock->tm = localtime (&clock->seconds);
      clock->timer_id = gtk_timeout_add (1000 * (60 - clock->tm->tm_sec),
					 gtk_clock_timer_first_callback,
					 clock);
    }
  else
    {
      clock->fmt = g_strdup ("%H:%M:%S");
      clock->tm = g_new (struct tm, 1);
      memset (clock->tm, 0, sizeof (struct tm));
      clock->update_interval = 1;
    }

  gtk_clock_gen_str (clock);

  return GTK_WIDGET (clock);
}

void gtk_clock_set_format (GtkClock *clock, const gchar *fmt)
{
  g_return_if_fail (clock != NULL);
  g_return_if_fail (fmt != NULL);

  g_free (clock->fmt);
  clock->fmt = g_strdup (fmt);
}

void gtk_clock_set_seconds (GtkClock *clock, time_t seconds)
{
  g_return_if_fail (clock != NULL);

  if (clock->type == GTK_CLOCK_INCREASING)
    {
      clock->seconds = time (NULL) - seconds;
    }
  else if (clock->type == GTK_CLOCK_DECREASING)
    {
      clock->seconds = time (NULL) + seconds;
    }

  if (clock->timer_id == -1)
    clock->stopped = seconds;

  gtk_clock_gen_str (clock);
}

void gtk_clock_set_update_interval (GtkClock *clock, gint seconds)
{
  guint tmp;

  g_return_if_fail (clock != NULL);

  tmp = clock->update_interval;

  clock->update_interval = seconds;

  if (tmp > seconds && clock->timer_id != -1)
    {
      gtk_clock_stop (clock);
      gtk_clock_start (clock);
    }
}

void gtk_clock_start (GtkClock *clock)
{
  g_return_if_fail (clock != NULL);

  if (clock->timer_id != -1)
    return;

  gtk_clock_set_seconds (clock, clock->stopped);
  clock->timer_id = gtk_timeout_add (1000 * clock->update_interval,
				     gtk_clock_timer_callback, clock);
}

void gtk_clock_stop (GtkClock *clock)
{
  g_return_if_fail (clock != NULL);

  if (clock->timer_id == -1)
    return;

  if (clock->type == GTK_CLOCK_INCREASING)
    {
      clock->stopped = time (NULL) - clock->seconds;
    }
  else if (clock->type == GTK_CLOCK_DECREASING)
    {
      clock->stopped = clock->seconds - time (NULL);
    }

  gtk_timeout_remove (clock->timer_id);
  clock->timer_id = -1;
}

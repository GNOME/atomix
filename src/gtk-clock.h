
/*
 * gtk-clock: The GTK clock widget
 * (C)1998 The Free Software Foundation
 *
 * Author: Szekeres István (szekeres@cyberspace.mht.bme.hu)
 */

#ifndef __GTK_CLOCK_H__
#define __GTK_CLOCK_H__

#include <time.h>
#include <gtk/gtk.h>
#include <libgnome/libgnome.h>

G_BEGIN_DECLS

#define GTK_TYPE_CLOCK            (gtk_clock_get_type ())
#define GTK_CLOCK(obj)            (G_TYPE_CHECK_INSTANCE_CAST((obj), GTK_TYPE_CLOCK, GtkClock))
#define GTK_CLOCK_CLASS(klass)    (G_TYPE_CHECK_CAST_CLASS((klass), GTK_TYPE_CLOCK, GtkClockClass))
#define GTK_IS_CLOCK(obj)         (G_TYPE_CHECK_INSTANCE_TYPE((obj), GTK_TYPE_CLOCK))
#define GTK_IS_CLOCK_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GTK_TYPE_CLOCK))
#define GTK_CLOCK_GET_CLASS(o)    (G_TYPE_INSTANCE_GET_CLASS ((o), GTK_TYPE_CLOCK, GtkClockClass))

typedef struct _GtkClock GtkClock;
typedef struct _GtkClockClass GtkClockClass;

typedef enum
{
	GTK_CLOCK_INCREASING,
	GTK_CLOCK_DECREASING,
	GTK_CLOCK_REALTIME
} GtkClockType;

struct _GtkClock {
	GtkLabel widget;
	GtkClockType type;
	gint timer_id;
	gint update_interval;
	time_t seconds;
	time_t stopped;
	gchar *fmt;
	struct tm *tm;
};

struct _GtkClockClass {
	GtkLabelClass parent_class;
};

GType      gtk_clock_get_type            (void);
GtkWidget* gtk_clock_new                 (GtkClockType type);
void       gtk_clock_set_format          (GtkClock *gclock, const gchar *fmt);
void       gtk_clock_set_seconds         (GtkClock *gclock, time_t seconds);
void       gtk_clock_set_update_interval (GtkClock *gclock, gint seconds);
void       gtk_clock_start               (GtkClock *gclock);
void       gtk_clock_stop                (GtkClock *gclock);

G_END_DECLS

#endif /* __GTK_CLOCK_H__ */

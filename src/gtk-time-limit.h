#ifndef __GTK_TIME_LIMIT_H__
#define __GTK_TIME_LIMIT_H__

#include <libgnomeui/gtk-clock.h>

#ifdef __cplusplus
extern "C" {
#endif 
	
#define GTK_TIME_LIMIT(obj)         (GTK_CHECK_CAST(obj, gtk_time_limit_get_type(), GtkTimeLimit))
#define GTK_TIME_LIMIT_CLASS(class) (GTK_CHECK_CAST_CLASS(class, gtk_time_limit_get_type(), GtkTimeLimitClass))
#define GTK_IS_STOP_CLOCK(obj)      (GTK_CHECK_TYPE(obj, gtk_time_limit_get_type()))
	
	
typedef struct _GtkTimeLimit GtkTimeLimit;
typedef struct _GtkTimeLimitClass GtkTimeLimitClass;         

struct _GtkTimeLimit
{
	GtkClock clock;
	
	gint timeout_secs;
	gint timer_id;
};

struct _GtkTimeLimitClass
{
	GtkClockClass parent_class;
	
	/* timeout signal */
	void (* timeout)    (GtkTimeLimit *tl);
};

/* methods */
GtkType gtk_time_limit_get_type(void);

GtkWidget* gtk_time_limit_new(void);

void gtk_time_limit_start(GtkTimeLimit *tl);

void gtk_time_limit_stop(GtkTimeLimit *tl);

gint gtk_time_limit_get_seconds(GtkTimeLimit *tl);

void gtk_time_limit_set_timeout(GtkTimeLimit *tl, gint secs);

#ifdef __cplusplus
}
#endif

#endif /* __GTK_TIME_LIMIT_H__ */

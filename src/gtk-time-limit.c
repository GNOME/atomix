#include <time.h>
#include "gtk-time-limit.h"

static void gtk_time_limit_init(GtkTimeLimit* tl);
static void gtk_time_limit_class_init(GtkTimeLimitClass* klass);
static gint gtk_time_limit_timer_callback(gpointer data);

static GtkClockClass* parent_class = NULL;

enum {
	TIMEOUT,
	LAST_SIGNAL
};

static guint time_limit_signals[LAST_SIGNAL] = { 0 };

guint gtk_time_limit_get_type(void)
{
	static guint gtk_time_limit_type = 0;
	if (!gtk_time_limit_type) {
		GtkTypeInfo gtk_time_limit_info = {
			"GtkTimeLimit",
			sizeof(GtkTimeLimit),
			sizeof(GtkTimeLimitClass),
			(GtkClassInitFunc) gtk_time_limit_class_init,
			(GtkObjectInitFunc) gtk_time_limit_init,
			(GtkArgSetFunc) NULL,
			(GtkArgGetFunc) NULL,
			(GtkClassInitFunc) NULL,
		};
		gtk_time_limit_type = gtk_type_unique(gtk_clock_get_type(), 
						      &gtk_time_limit_info);
	}
    
	return gtk_time_limit_type;
}

static void gtk_time_limit_class_init(GtkTimeLimitClass *klass)
{
	GtkObjectClass *object_class;

	object_class = (GtkObjectClass*) klass;

	parent_class = gtk_type_class (gtk_clock_get_type());

	time_limit_signals[TIMEOUT] =
		gtk_signal_new ("timeout",
				GTK_RUN_FIRST,
				object_class->type,
				GTK_SIGNAL_OFFSET (GtkTimeLimitClass, timeout),
				gtk_marshal_NONE__NONE,
				GTK_TYPE_NONE, 0);


	gtk_object_class_add_signals (object_class, time_limit_signals, LAST_SIGNAL);

	klass->timeout = NULL;
}

static void gtk_time_limit_init(GtkTimeLimit *tl)
{
	GtkClock* clock = GTK_CLOCK(tl);

	tl->timeout_secs = 0;
	tl->timer_id = -1;

	clock->type = GTK_CLOCK_DECREASING;

	clock->fmt = g_strdup("%M:%S");
	clock->tm = g_new(struct tm, 1);
	memset(clock->tm, 0, sizeof(struct tm));
	clock->update_interval = 1;
}

static gint gtk_time_limit_timer_callback(gpointer data)
{
	GtkTimeLimit *tl;
	gboolean equal;

	if(data == NULL) return FALSE;

	tl = GTK_TIME_LIMIT(data);

	GDK_THREADS_ENTER ();    
	equal = (gtk_time_limit_get_seconds(tl) == tl->timeout_secs);
	GDK_THREADS_LEAVE();

	if(equal)
	{
		gtk_signal_emit (GTK_OBJECT(tl), time_limit_signals[TIMEOUT]);
	}

	return TRUE;
}

GtkWidget* gtk_time_limit_new(void)
{
	GtkTimeLimit *tl = gtk_type_new(gtk_time_limit_get_type());

	tl->timer_id = -1;

	return GTK_WIDGET(tl);
}

void gtk_time_limit_start(GtkTimeLimit* tl) 
{
	g_return_if_fail(tl != NULL);

	if (tl->timer_id != -1) return;

	tl->timer_id = gtk_timeout_add(1000, gtk_time_limit_timer_callback, tl);
	gtk_clock_start(GTK_CLOCK(tl));
    
}

void gtk_time_limit_stop(GtkTimeLimit* tl) 
{
	g_return_if_fail(tl != NULL);

	if (tl->timer_id == -1) return;

	gtk_timeout_remove(tl->timer_id);
	tl->timer_id = -1; 
    
	gtk_clock_stop(GTK_CLOCK(tl));
}

gint gtk_time_limit_get_seconds(GtkTimeLimit* tl)
{
	if(tl==NULL) return -1;

	if(tl->timer_id == -1)
	{
		return GTK_CLOCK(tl)->stopped;
	}
	else
	{		
		return (GTK_CLOCK(tl)->seconds - time(NULL));
	}
}

void gtk_time_limit_set_timeout(GtkTimeLimit* tl, gint secs)
{
	g_return_if_fail(tl!=NULL);    
	tl->timeout_secs = secs;
}

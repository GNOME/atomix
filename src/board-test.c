#include <gtk/gtk.h>

#include "board_gtk.h"
#include "theme-manager.h"
#include "level-manager.h"


static Theme *theme = NULL;

static void hello( GtkWidget *widget,
                   gpointer   data )
{
    g_print ("Hello World\n");
}

static gboolean delete_event( GtkWidget *widget,
                              GdkEvent  *event,
                              gpointer   data )
{

    g_print ("delete event occurred\n");
    gtk_widget_destroy (widget);
    return TRUE;
}

/* Another callback */
static void destroy( GtkWidget *widget,
                     gpointer   data )
{
    gtk_main_quit ();
}

static void setup_level (void)
{
  Level *next_level;
  PlayField *goal_pf;
  LevelManager *lm;
  Goal *goal;
  PlayField *env_pf;
  PlayField *sce_pf;

  lm = level_manager_new ();
  level_manager_init_levels (lm);

  next_level = level_manager_get_next_level (lm, NULL);
  goal_pf = level_get_goal (next_level);

  goal = goal_new (goal_pf);

  g_object_unref (goal_pf);
  g_object_unref (lm);

  /* init board */
  env_pf = level_get_environment (next_level);
  sce_pf = level_get_scenario (next_level);
  board_gtk_init_level (env_pf, sce_pf, goal);

  g_object_unref (env_pf);
  g_object_unref (sce_pf);
}

int main( int   argc,
          char *argv[] )
{
    GtkWidget *window;
    GtkWidget *board;
    ThemeManager *tm;

    gtk_init (&argc, &argv);
    /* init theme manager */
    tm = theme_manager_new ();
    theme_manager_init_themes (tm);
    theme = theme_manager_get_theme (tm, "default");

    window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
    
    g_signal_connect (window, "delete-event",
		      G_CALLBACK (delete_event), NULL);
    
    g_signal_connect (window, "destroy",
		      G_CALLBACK (destroy), NULL);
    
    board = gtk_fixed_new ();
    gtk_widget_show (board);
    board_gtk_init (theme, GTK_FIXED (board)); 
    setup_level ();   
    /* This packs the button into the window (a gtk container). */
    gtk_container_add (GTK_CONTAINER (window), board);
    
    /* and the window */
    gtk_widget_show (window);
    gtk_widget_set_size_request (GTK_WIDGET (window), 478, 478);
    /* All GTK applications must have a gtk_main(). Control ends here
     * and waits for an event to occur (like a key press or
     * mouse event). */
    gtk_main ();
    
    return 0;
}

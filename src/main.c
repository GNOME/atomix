/* Atomix -- a little mind game about atoms and molecules.
 * Copyright (C) 1999-2001 Jens Finke
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include "board.h"
#include "playfield.h"
#include "main.h"
#include "goal.h"
#include "level.h"
#include "goal-view.h"
#include "gtk-clock.h"

static AtomixApp    *app; 

typedef enum {
	GAME_ACTION_NEW,
	GAME_ACTION_END,
	GAME_ACTION_PAUSE,
	GAME_ACTION_CONTINUE,
	GAME_ACTION_SKIP,
	GAME_ACTION_UNDO,
	GAME_ACTION_FINISHED,
	GAME_ACTION_RESTART,
} GameAction;

static void controller_handle_action (AtomixApp *app, GameAction action);
static void set_game_not_running_state (AtomixApp *app);
static gboolean set_next_level (AtomixApp *app);
static void setup_level (AtomixApp *app);
static void level_cleanup_view (AtomixApp *app);
static void atomix_exit (AtomixApp *app);
static gboolean on_key_press_event           (GtkWidget       *widget,
					      GdkEventKey     *event,
					      gpointer         user_data);
static void game_init (AtomixApp *app);
static void update_statistics (AtomixApp *app);
static void update_menu_item_state (AtomixApp *app);
static void view_congratulations (void);
static void calculate_score (AtomixApp *app);


/* ===============================================================
      
             Menu callback  functions 

-------------------------------------------------------------- */
void
verb_GameNew_cb (BonoboUIComponent *uic, gpointer user_data, const char *cname)
{
        controller_handle_action ((AtomixApp*) user_data, GAME_ACTION_NEW);
}

void
verb_GameEnd_cb (BonoboUIComponent *uic, gpointer user_data, const char *cname)
{
	controller_handle_action ((AtomixApp*) user_data, GAME_ACTION_END);
}


void
verb_GameSkip_cb (BonoboUIComponent *uic, gpointer user_data, const char *cname)
{
	controller_handle_action ((AtomixApp*) user_data, GAME_ACTION_SKIP);
}

void
verb_GameReset_cb (BonoboUIComponent *uic, gpointer user_data, const char *cname)
{
	controller_handle_action ((AtomixApp*) user_data, GAME_ACTION_RESTART);
}

void
verb_GamePause_cb (BonoboUIComponent *uic, gpointer user_data, const char *cname)
{
	controller_handle_action ((AtomixApp*) user_data, GAME_ACTION_PAUSE);
}

void
verb_GameContinue_cb (BonoboUIComponent *uic, gpointer user_data, const char *cname)
{
	controller_handle_action ((AtomixApp*) user_data, GAME_ACTION_CONTINUE);
}

void
verb_GameUndo_cb (BonoboUIComponent *uic, gpointer user_data, const char *cname)
{
	controller_handle_action ((AtomixApp*) user_data, GAME_ACTION_UNDO);
}


void
verb_GameScores_cb (BonoboUIComponent *uic, gpointer user_data, const char *cname)
{
#if 0
	gnome_scores_display("Atomix", "atomix", NULL, 0);	

#endif
}

void
verb_GameExit_cb (BonoboUIComponent *uic, gpointer user_data, const char *cname)
{
	atomix_exit ((AtomixApp*) user_data);
}

void
verb_EditPreferences_cb (BonoboUIComponent *uic, gpointer user_data, const char *cname)
{
#if 0
	preferences_show_dialog();
#endif
}


void
verb_HelpAbout_cb (BonoboUIComponent *uic, gpointer user_data, const char *cname)
{
#if 0
	show_about_dlg ();
#endif
}

gboolean
on_app_destroy_event(GtkWidget       *widget,
                     GdkEvent        *event,
                     gpointer         user_data)
{
	AtomixApp *app = (AtomixApp*) user_data;
	atomix_exit (app);
	
	return TRUE;
}

/* ===============================================================
      
             Game steering functions

-------------------------------------------------------------- */

static void
controller_handle_action (AtomixApp *app, GameAction action)
{
	switch (app->state) {
	case GAME_STATE_NOT_RUNNING:
		if (action == GAME_ACTION_NEW) {
			if (set_next_level (app)) {
				app->level_no = 1;
				app->score = 0;
				setup_level (app);
				app->state = GAME_STATE_LEVEL_RUNNING;
			}
		}
		break;

	case GAME_STATE_LEVEL_RUNNING:
		switch (action) {
		case GAME_ACTION_END:
			level_cleanup_view (app);
			set_game_not_running_state (app);
			break;

		case GAME_ACTION_PAUSE:
			gtk_clock_stop (GTK_CLOCK (app->clock));
			board_hide ();
			app->state = GAME_STATE_PAUSED;
			break;

		case GAME_ACTION_SKIP:
			level_cleanup_view (app);
			if (set_next_level (app)) {
				setup_level (app);
			}
			else {
				set_game_not_running_state (app);
			}
			break;

		case GAME_ACTION_FINISHED:
			calculate_score (app);
			if (level_manager_is_last_level (app->lm, app->level)) {
				view_congratulations (); 
				level_cleanup_view (app);
				set_game_not_running_state (app);
			}
			else {
				level_cleanup_view (app);
				set_next_level (app);
				setup_level (app);
			}
			break;

		case GAME_ACTION_RESTART:
			level_cleanup_view (app);
			setup_level (app);
			break;

		case GAME_ACTION_UNDO:
			board_undo_move ();
			break;

		default:
			break;
		}
		break;


	case GAME_STATE_PAUSED:
		if (action == GAME_ACTION_CONTINUE) {
			gtk_clock_start (GTK_CLOCK (app->clock));
			board_show ();
			app->state = GAME_STATE_LEVEL_RUNNING;
		}
		break;
	default:
		g_assert_not_reached ();
	}

	update_menu_item_state (app);
	update_statistics (app);
}

static void
set_game_not_running_state (AtomixApp *app)
{
	if (app->level)
		g_object_unref (app->level);
	if (app->goal)
		g_object_unref (app->goal);
	app->level = NULL;
	app->goal = NULL;
	app->score = 0;
	app->state = GAME_STATE_NOT_RUNNING;
}

static gboolean
set_next_level (AtomixApp *app)
{
	Level *next_level;
	PlayField *goal_pf;

	next_level = level_manager_get_next_level (app->lm, app->level);

	if (app->level)
		g_object_unref (app->level);
	app->level = NULL;

	if (app->goal)
		g_object_unref (app->goal);
	app->goal = NULL;

	if (next_level == NULL) return FALSE;

	app->level = next_level;
	app->level_no++;
 	goal_pf = level_get_goal (app->level);
 	app->goal = goal_new (goal_pf);

	g_object_unref (goal_pf);

	return TRUE;
}

static void 
setup_level (AtomixApp *app)
{
	PlayField *env_pf;
	PlayField *sce_pf;

	g_return_if_fail (app != NULL);

	if (app->level == NULL) return;
	g_return_if_fail (app->goal != NULL);

	/* init board */
	env_pf = level_get_environment (app->level);
	sce_pf = level_get_scenario (app->level);
	board_init_level (env_pf, sce_pf, app->goal);

	/* init goal */
 	goal_view_render (app->goal);

	/* init clock */
	gtk_clock_set_seconds (GTK_CLOCK (app->clock), 0);
	gtk_clock_start (GTK_CLOCK (app->clock));

	g_object_unref (env_pf);
	g_object_unref (sce_pf);
}

static void 
level_cleanup_view (AtomixApp *app)
{
	board_clear ();
	goal_view_clear ();

	gtk_clock_stop (GTK_CLOCK (app->clock));
}


static void
atomix_exit (AtomixApp *app)
{
	g_message ("Destroy application");

	g_return_if_fail (app != NULL);

	board_destroy ();
	
	if (app->level)
		g_object_unref (app->level);
	if (app->lm)
		g_object_unref (app->lm);

	if (app->theme)
		g_object_unref (app->theme);
	if (app->tm)
		g_object_unref (app->tm);

	/* quit application */
	bonobo_object_unref (BONOBO_OBJECT (app->ui_component));
	gtk_widget_destroy (app->mainwin);

	gtk_main_quit();
}

static gboolean
on_key_press_event           (GtkWidget       *widget,
			      GdkEventKey     *event,
			      gpointer         user_data)
{
	AtomixApp *app = (AtomixApp*) user_data;

	if(app->state == GAME_STATE_LEVEL_RUNNING)
	{
		board_handle_key_event (event);
	}
	
	return TRUE;
}


static void 
game_init (AtomixApp *app)
{
	g_return_if_fail (app != NULL);

	/* init theme manager */
	app->tm = theme_manager_new ();
	theme_manager_init_themes (app->tm);
	app->theme = theme_manager_get_theme (app->tm, "default");
	g_assert (app->theme != NULL);

	/* init level manager */
	app->lm = level_manager_new ();
	level_manager_init_levels (app->lm);

	/* init level statistics */ 
	app->level = NULL;
	app->level_no = 0;
	app->score = 0;
	gtk_clock_set_format (GTK_CLOCK (app->clock), "%M:%S");
	gtk_clock_set_seconds (GTK_CLOCK (app->clock), 0);

	/* init the board */
	board_init (app->theme, GNOME_CANVAS (app->ca_matrix));

	/* init goal */
	goal_view_init (app->theme, GNOME_CANVAS (app->ca_goal));

	/* update user visible information */
	app->state = GAME_STATE_NOT_RUNNING;
	update_menu_item_state (app);
	update_statistics (app);

	gtk_widget_grab_focus (GTK_WIDGET (app->ca_matrix));
}

void
game_level_finished (AtomixApp *tmp_app)
{
	controller_handle_action (app, GAME_ACTION_FINISHED);
}

static void 
calculate_score (AtomixApp *app)
{
	gint seconds;

	seconds = time (NULL) - GTK_CLOCK (app->clock)->seconds;
	if (seconds > 300) return;

	if (app->score == 0) 
		app->score = 300 - seconds;
	else
		app->score = app->score * (2 - (seconds/300));
}

static void
view_congratulations (void)
{
	GtkWidget *dlg;
	dlg = gtk_message_dialog_new (GTK_WINDOW (app->mainwin),
				      GTK_DIALOG_MODAL,
				      GTK_MESSAGE_INFO,
				      GTK_BUTTONS_CLOSE,
				      "%s",
				      _("Congratulations! You have finished all Atomix levels."));
	gtk_dialog_run (GTK_DIALOG (dlg));
	gtk_widget_destroy (GTK_WIDGET (dlg));
}

#if 0	
		dlg = gtk_message_dialog_new (GTK_WINDOW (app->mainwin),
					      GTK_DIALOG_MODAL,
					      GTK_MESSAGE_ERROR,
					      GTK_BUTTONS_OK,
					      "%s",
					      _("Couldn't find at least one level."));
		gtk_dialog_run (GTK_DIALOG (dlg));
		gtk_widget_destroy (GTK_WIDGET (dlg));


				      _("Do you want to finish the game?"));
#endif 

/* ===============================================================
      
             UI update functions

-------------------------------------------------------------- */
static void 
update_statistics (AtomixApp *app)
{
	gchar *str_buffer;
    
	g_return_if_fail (app != NULL);
	
	if (app->state == GAME_STATE_NOT_RUNNING) {
		/* don't show anything */
		gtk_label_set_text(GTK_LABEL(app->lb_level), "");
		gtk_label_set_text(GTK_LABEL(app->lb_name), "");
		gtk_label_set_text(GTK_LABEL(app->lb_score), "");
		gtk_widget_hide (GTK_WIDGET (app->clock));
	}
	else {
		/* set level number */
		str_buffer = g_new0 (gchar, 10);
		g_snprintf (str_buffer, 10, "%i", app->level_no);
		gtk_label_set_text (GTK_LABEL (app->lb_level), str_buffer);
		
		/* set levelname */
		gtk_label_set_text (GTK_LABEL (app->lb_name), 
				    level_get_name (app->level));

		/* set score */
		g_snprintf (str_buffer, 10, "%i", app->score);
		gtk_label_set_text (GTK_LABEL (app->lb_score), 
				    str_buffer);
		
		/* show clock */
		gtk_widget_show (GTK_WIDGET (app->clock));

		g_free (str_buffer);
	}
}


typedef struct {
	gchar *cmd;
	gboolean enabled;
} CmdEnable;

static const CmdEnable not_running[] = 
{
	{ "GameNew", TRUE },
	{ "GameEnd", FALSE }, 
	{ "GameSkip", FALSE },
	{ "GameReset", FALSE },
	{ "GameUndo", FALSE },
	{ "GamePause", FALSE },
	{ "GameContinue", FALSE },
	{ "EditPreferences", TRUE },
	{ NULL, FALSE }
};

static const CmdEnable running[] = 
{
	{ "GameNew", FALSE },
	{ "GameEnd", TRUE }, 
	{ "GameSkip", TRUE },
	{ "GameReset", TRUE },
	{ "GameUndo", TRUE },
	{ "GamePause", TRUE },
	{ "GameContinue", FALSE },
	{ "EditPreferences", TRUE },
	{ NULL, FALSE }
};

static const CmdEnable paused[] = 
{
	{ "GameNew", FALSE },
	{ "GameEnd", FALSE }, 
	{ "GameSkip", FALSE },
	{ "GameReset", FALSE },
	{ "GameUndo", FALSE },
	{ "GamePause", FALSE },
	{ "GameContinue", TRUE },
	{ "EditPreferences", TRUE },
	{ NULL, FALSE }
};

static const CmdEnable* state_sensitivity[] = { not_running, running, paused };

static void 
update_menu_item_state (AtomixApp *app)
{
	gchar *path;
	gint i;
	const CmdEnable *cmd_list = state_sensitivity [app->state];

	for (i = 0; cmd_list[i].cmd != NULL; i++) {
		path = g_strconcat ("/commands/", cmd_list[i].cmd, NULL);
		bonobo_ui_component_set_prop (app->ui_component, path, "sensitive",
					      cmd_list[i].enabled ? "1" : "0", NULL);
		g_free (path);
	}
}

#if 0
static void 
save_score (AtomixApp *app)
{
	if(score > 0.0)
	{
		gint highscore_pos;
		highscore_pos = gnome_score_log(score, NULL, TRUE);
		gnome_scores_display("Atomix", "atomix", NULL, highscore_pos);	    
	}
}
#endif

static void
create_user_config_dir (void)
{
	gchar* atomix_dir;
	gchar* themes_dir;
	gchar* level_dir;

	atomix_dir = g_build_filename (g_get_home_dir (), ".atomix", NULL);
	themes_dir = g_build_filename (atomix_dir, "themes", NULL);
	level_dir = g_build_filename (atomix_dir, "level", NULL);

	if (!g_file_test (atomix_dir, G_FILE_TEST_EXISTS | G_FILE_TEST_IS_DIR)) 
		if (mkdir (atomix_dir, 0755) != 0) {
			g_error (_("Couldn't create directory: %s"), atomix_dir);
			return;
		}

	if (!g_file_test (themes_dir, G_FILE_TEST_EXISTS | G_FILE_TEST_IS_DIR))
		if (mkdir (themes_dir, 0755) != 0) {
			g_error (_("Couldn't create directory: %s"), themes_dir);
		}
		
	if (!g_file_test (level_dir, G_FILE_TEST_EXISTS | G_FILE_TEST_IS_DIR))
		if (mkdir (level_dir, 0755) != 0) {
			g_error (_("Couldn't create directory: %s"), level_dir);
		}

	g_free(atomix_dir);
	g_free(themes_dir);
	g_free(level_dir);
}

/* ===============================================================
      
             GUI creation  functions 

-------------------------------------------------------------- */
static BonoboUIVerb verbs[] = {
	BONOBO_UI_VERB ("GameNew", verb_GameNew_cb),
	BONOBO_UI_VERB ("GameEnd", verb_GameEnd_cb),
	BONOBO_UI_VERB ("GameSkip", verb_GameSkip_cb),
	BONOBO_UI_VERB ("GameReset", verb_GameReset_cb),
	BONOBO_UI_VERB ("GameUndo", verb_GameUndo_cb),
	BONOBO_UI_VERB ("GamePause", verb_GamePause_cb),
	BONOBO_UI_VERB ("GameContinue", verb_GameContinue_cb),
	BONOBO_UI_VERB ("GameScores", verb_GameScores_cb),
	BONOBO_UI_VERB ("GameExit", verb_GameExit_cb),
	BONOBO_UI_VERB ("EditPreferences", verb_EditPreferences_cb),
#if 0
	BONOBO_UI_VERB ("HelpManual", verb_HelpManual_cb),
#endif
	BONOBO_UI_VERB ("HelpAbout", verb_HelpAbout_cb),
	BONOBO_UI_VERB_END
};

static GtkWidget*
create_canvas_widget (GtkWidget **canvas)
{
	GtkWidget *frame;

        gtk_widget_push_visual (gdk_rgb_get_visual ());
        gtk_widget_push_colormap (gdk_rgb_get_cmap ());
        *canvas = gnome_canvas_new_aa ();
        gtk_widget_pop_visual ();
        gtk_widget_pop_colormap ();

	frame = gtk_frame_new (NULL);
	gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
	gtk_container_add (GTK_CONTAINER (frame), GTK_WIDGET (*canvas));

	return frame;
}

static void
add_statistics_table_entry (GtkWidget *table, gint row, gchar *label_str, gboolean is_clock,
			   GtkWidget **return_widget)
{
	GtkWidget *label;
	GtkWidget *lb_align;
	GtkWidget *align;

	label = gtk_label_new (label_str);
	lb_align = gtk_alignment_new (1.0, 0.5, 0.0, 0.0);
	gtk_container_add (GTK_CONTAINER (lb_align), GTK_WIDGET (label));
	gtk_table_attach (GTK_TABLE (table), GTK_WIDGET (lb_align),
			  0, 1, row, row + 1,
			  GTK_FILL, GTK_FILL, 0, 0); 

	if (is_clock)
		*return_widget = gtk_clock_new (GTK_CLOCK_INCREASING);
	else
		*return_widget = gtk_label_new ("NO CONTENT");
	align = gtk_alignment_new (0.0, 0.5, 0.0, 0.0);
	gtk_container_add (GTK_CONTAINER (align), GTK_WIDGET (*return_widget));
	gtk_table_attach (GTK_TABLE (table), GTK_WIDGET (align),
			  1, 2, row, row + 1,
			  GTK_FILL, GTK_FILL, 6, 0);
}


static GtkWidget*
create_mainwin_content (AtomixApp *app)
{
	GtkWidget *hbox;
	GtkWidget *vbox;
	GtkWidget *pf;
	GtkWidget *goal;
	GtkWidget *frame;
	GtkWidget *table;

	/* create canvas widgets */
	pf = create_canvas_widget (&app->ca_matrix);
	goal = create_canvas_widget (&app->ca_goal);
	gtk_widget_set_size_request (GTK_WIDGET (goal), 180, 50);

	/* add playfield canvas to left side */
	hbox = gtk_hbox_new (FALSE, 6);
	gtk_container_set_border_width (GTK_CONTAINER (hbox), 6);
	gtk_box_pack_start (GTK_BOX (hbox), GTK_WIDGET (pf), TRUE, TRUE, 0);
	gtk_signal_connect (GTK_OBJECT (app->ca_matrix), "key_press_event", 
			    (GtkSignalFunc) on_key_press_event, app);
	
	/* create right window side */
	vbox = gtk_vbox_new (FALSE, 6);
	gtk_box_pack_start (GTK_BOX (hbox), GTK_WIDGET (vbox), FALSE, TRUE, 0);

	/* create statistics frame */
	frame = gtk_frame_new (_("Statistics"));
	table = gtk_table_new (4, 2, FALSE);
	gtk_table_set_row_spacings (GTK_TABLE (table), 6);
	gtk_table_set_col_spacings (GTK_TABLE (table), 6);
	gtk_container_set_border_width (GTK_CONTAINER (table), 6);
	
	add_statistics_table_entry (table, 0, N_("Level:"), FALSE, &app->lb_level);
	add_statistics_table_entry (table, 1, N_("Molecule:"), FALSE, &app->lb_name);
	add_statistics_table_entry (table, 2, N_("Score:"), FALSE, &app->lb_score);
	add_statistics_table_entry (table, 3, N_("Time:"), TRUE, &app->clock);

	gtk_container_add (GTK_CONTAINER (frame), GTK_WIDGET (table));

	/* add frame and goal canvas to left side */
	gtk_box_pack_start (GTK_BOX (vbox), GTK_WIDGET (frame), FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (vbox), GTK_WIDGET (goal), TRUE, TRUE, 0);

	/* show all */
	gtk_widget_show_all (GTK_WIDGET (hbox));
	return hbox;
}

static AtomixApp*
create_gui (GnomeProgram *prog)
{
	AtomixApp *app;
	gchar *ui_file = NULL;

	GtkWidget *content;

	app = g_new0 (AtomixApp, 1);
	app->prog = prog;
	app->level = NULL;

	app->mainwin = bonobo_window_new ("atomix", "Atomix");
	g_signal_connect(G_OBJECT(app->mainwin), "delete_event",
			 (GCallback) on_app_destroy_event, app);

	app->ui_container = bonobo_ui_engine_get_ui_container (
		bonobo_window_get_ui_engine (BONOBO_WINDOW (app->mainwin)));

	app->ui_component = bonobo_ui_component_new ("atomix");
	bonobo_ui_component_set_container (app->ui_component,
					   BONOBO_OBJREF(app->ui_container),
					   NULL);

	/* find xml menu description */
	ui_file = g_build_filename (DATADIR, "atomix", "ui", "atomix-ui.xml", NULL);
	if (!g_file_test (ui_file, G_FILE_TEST_EXISTS)) {
		ui_file = g_build_filename (".", "atomix-ui.xml", NULL);
		if (!g_file_test (ui_file, G_FILE_TEST_EXISTS))
			g_error (_("Couldn't find file: %s"), ui_file);
	}

	/* set menus */
	bonobo_ui_util_set_ui (app->ui_component,
			       "", ui_file,
			       "atomix", NULL);
	g_free (ui_file);

	bonobo_ui_component_add_verb_list_with_data (app->ui_component, verbs, app);

	/* create window contents */
	content = create_mainwin_content (app);
	gtk_widget_show (GTK_WIDGET (content));
	
	bonobo_window_set_contents (BONOBO_WINDOW (app->mainwin), content);

	return app;
}

static gboolean
splash_destroy (GtkWidget *widget)
{
	gtk_widget_destroy (widget);
	return FALSE;
}

static gboolean
splash_button_event (gpointer data)
{
	gtk_widget_hide (GTK_WIDGET (data));
	return TRUE;
}

static GtkWidget*
create_splash (void)
{
	GtkWidget *splash_window;
	gchar *file;
	GtkWidget *img;

	file = g_build_filename (DATADIR, "atomix", "atomix-splash.png", NULL);
	g_print ("%s", file);
	if (!g_file_test (file, G_FILE_TEST_EXISTS)) {
		g_free (file);
		return NULL;
	}

	splash_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_widget_set_events (GTK_WIDGET (splash_window), GDK_BUTTON_PRESS_MASK);
	g_signal_connect (G_OBJECT (splash_window), "button-press-event",
			  (GCallback) splash_button_event, 
			  splash_window);

	img = gtk_image_new_from_file (file);
	gtk_container_add (GTK_CONTAINER (splash_window), img);

	gtk_window_set_policy (GTK_WINDOW (splash_window), FALSE, FALSE, TRUE);
	gtk_window_set_title (GTK_WINDOW (splash_window), _("Atomix Splash"));
 	gtk_window_set_gravity (GTK_WINDOW (splash_window), GDK_GRAVITY_CENTER);

	gtk_widget_show_all (splash_window);

	gtk_timeout_add (5000, (GtkFunction) splash_destroy, splash_window);

	while (gtk_events_pending ())
		gtk_main_iteration ();
	
	g_free (file);
	return splash_window;
}

int
main (int argc, char *argv[])
{
	GnomeProgram *prog;
	GtkWidget *splash;

	gnome_score_init (PACKAGE);

	bindtextdomain (PACKAGE, GNOMELOCALEDIR);
	textdomain (PACKAGE);

	prog = gnome_program_init (PACKAGE, VERSION, LIBGNOMEUI_MODULE, 
				   argc, argv, NULL);

	splash = create_splash ();

	/* init preferences */
	/* preferences_init(); */

	/* make a few initalisations here */
	create_user_config_dir();

	app = create_gui (prog);

	game_init (app);

	gtk_widget_set_size_request (GTK_WIDGET (app->mainwin), 660, 480); 
	gtk_widget_show (app->mainwin);

	gtk_window_present (GTK_WINDOW (splash));

	gtk_main ();
	return 0;
}


/* Atomix -- a little mind game about atoms and molecules.
 * Copyright (C) 1999 Jens Finke
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
#include "util.h"
#include "goal-view.h"

/****************************
 *    global variables
 */

static AtomixApp    *app; 

/**
 *  dialog callbacks
 */
static void save_score (AtomixApp *app);
static void game_clean_up (AtomixApp *app);

static void update_menu_item_state (AtomixApp *app);
static void update_player_info (AtomixApp *app);
static void clear_player_info (AtomixApp *app);
void create_user_config_dir (void);

static void game_new (AtomixApp *app);
static void game_init (AtomixApp *app);
static void game_prepare_level (AtomixApp *app, Level *next_level);
static void game_undo_move (AtomixApp *app);
static void game_pause (AtomixApp *app);
static void game_continue (AtomixApp *app);
static void game_skip_level (AtomixApp *app);
static void game_reload_level (AtomixApp *app);
static void game_finished (AtomixApp *app);
static void atomix_exit (AtomixApp *app);
static void game_user_quits (AtomixApp *app);

/*
 *     Menu callbacks 
 */

void
verb_GameNew_cb (BonoboUIComponent *uic, gpointer user_data, const char *cname)
{
	game_new ((AtomixApp*) user_data);
}

void
verb_GameEnd_cb (BonoboUIComponent *uic, gpointer user_data, const char *cname)
{
	game_user_quits ((AtomixApp*) user_data);
}


void
verb_GameSkip_cb (BonoboUIComponent *uic, gpointer user_data, const char *cname)
{
	game_skip_level ((AtomixApp*) user_data);
}

void
verb_GamePause_cb (BonoboUIComponent *uic, gpointer user_data, const char *cname)
{
	game_pause ((AtomixApp*) user_data);

}

void
verb_GameContinue_cb (BonoboUIComponent *uic, gpointer user_data, const char *cname)
{
	game_continue ((AtomixApp*) user_data);
}

void
verb_GameUndo_cb (BonoboUIComponent *uic, gpointer user_data, const char *cname)
{
	game_undo_move ((AtomixApp*) user_data);
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


/****************************************************************************/

AtomixApp* 
get_app ()
{
	return app;
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
	app->level = NULL;
	app->level_no = 0;
	app->score = 0.0;

	/* init the board */
	board_init (app->theme, GNOME_CANVAS (app->ca_matrix));

	/* init goal */
	goal_view_init (app->theme, GNOME_CANVAS (app->ca_goal));

	/* enable only usable menu items according to state */
	app->state = GAME_NOT_RUNNING;
	update_menu_item_state (app);

	/* clear the level info */
	clear_player_info (app);
}

static void 
game_new (AtomixApp *app)
{
	Level *level;

	g_return_if_fail (app != NULL);
	
	level = level_manager_get_next_level (app->lm, NULL);

	if (level) 
	{
		board_hide_message (BOARD_MSG_NEW_GAME);

		/* init level data */
		app->level_no = 1;
		app->score = 0.0;
		app->state = GAME_RUNNING;
		
		/* update game state */
		update_menu_item_state (app);

		/* load the level */
		game_prepare_level (app, level);    
	}
	else
	{
		GtkWidget *dlg;
		dlg = gtk_message_dialog_new (GTK_WINDOW (app->mainwin),
					      GTK_DIALOG_MODAL,
					      GTK_MESSAGE_ERROR,
					      GTK_BUTTONS_OK,
					      "%s",
					      _("Couldn't find at least one level."));
		gtk_dialog_run (GTK_DIALOG (dlg));
		gtk_widget_destroy (GTK_WIDGET (dlg));
	}
}

static void 
game_undo_move (AtomixApp *app)
{
	g_return_if_fail (app != NULL);

	if (board_undo_move()) {
		set_appbar_temporary(_("No more undo levels."));
	}
}

static void 
game_skip_level (AtomixApp *app)
{
	g_return_if_fail (app != NULL);
	g_assert (app->state == GAME_RUNNING);
	if (level_manager_is_last_level (app->lm, app->level)) {
		set_appbar_temporary(_("This is the last level. You can't skip any further."));
	}
	else {
		Level *next_level = level_manager_get_next_level (app->lm, app->level);
		
		/* load level */
		game_prepare_level (app, next_level);
	}
}

static void 
game_prepare_level (AtomixApp *app, Level *next_level)
{
	PlayField *goal_pf;
	PlayField *level_pf;

	g_return_if_fail (app != NULL);
	g_return_if_fail (IS_LEVEL (next_level));

	if (app->level)
		g_object_unref (app->level);
	app->level = next_level;

	if (app->goal)
		g_object_unref (app->goal);
	goal_pf = level_get_goal (app->level);
	app->goal = goal_new (goal_pf);

	/* init board */
	board_clear ();
	level_pf = level_get_playfield (app->level);
	board_init_level (level_pf, app->goal);

	/* init goal */
	goal_view_render (app->goal);

	update_player_info (app);

	g_object_unref (goal_pf);
	g_object_unref (level_pf);
}

static void 
game_reload_level (AtomixApp *app)
{
	g_object_ref (app->level);
	game_prepare_level (app, app->level);
}

#if 0
void game_level_timeout(GtkWidget *widget, gpointer data)
{
	GtkWidget *dlg;
	GladeXML *dlg_xml;
	GtkWidget* clock;
	gint button_nr;

	/* stop the clock */
	clock = get_time_limit_widget();
	gtk_time_limit_stop(GTK_TIME_LIMIT(clock));      
        gtk_clock_set_seconds(GTK_CLOCK(clock), 0);

	/* show the cursor */
	board_show_normal_cursor();

	/* handle bonus level if neccessary */
	if(level_data->level->bonus_level)
	{
		game_bonus_level_timeout(widget, data);
		return;
	}

	/* update score */
	level_data->score = level_data->score * 0.9;
	update_player_info(level_data);

	/* show dialog "Do you want to try again?" */
	dlg_xml = glade_xml_new ("../atomix2.glade", "dlg_timeout", NULL);
	dlg =  glade_xml_get_widget (dlg_xml, "dlg_timeout");
	button_nr = gnome_dialog_run(GNOME_DIALOG (dlg));

	if(button_nr == 0)
	{
		/* restart level */
		game_reload_level();
	}
	else
	{	
		/* player want to quit */
		game_clean_up();
		save_score(level_data->score);
		game_state = GAME_NOT_RUNNING;
		update_menu_item_state(game_state);
		board_view_message(BOARD_MSG_NEW_GAME);
	}
}

void game_bonus_level_timeout(GtkWidget *widget, gpointer user_data)
{
	GladeXML *dlg_xml;
        GtkWidget *dlg;
	
	/* show dialog to inform the player */
	dlg_xml = glade_xml_new ("../atomix2.glade", "dlg_bonus_timeout", NULL);
	dlg = glade_xml_get_widget (dlg_xml, "dlg_bonus_timeout");
	gnome_dialog_run(GNOME_DIALOG(dlg));

	if(!level_is_last_level(level_data->level))
	{
		gchar *next;

		/* retrieve next level */
		next = g_strdup(level_data->level->next);    
		
		/* update level data */
		level_destroy(level_data->level);
		level_data->level = level_load_xml(next);
		level_data->no_level++;
		g_free(next);

		/* load level */
		game_prepare_level(level_data);
	}
	else
	{
		game_finished();
	}
}
#endif 

void 
game_level_finished (AtomixApp *app)
{
	Level *next_level;

	g_return_if_fail (app != NULL);

	/* show cursur */
	board_show_normal_cursor ();
	
	next_level = level_manager_get_next_level (app->lm, app->level);

	if (next_level) {
		app->level_no++;
       
		game_prepare_level (app, next_level);
	}
	else {
		/* player reached game end */
		game_finished (app);
	}
}

static void 
game_user_quits (AtomixApp *app)
{
	GtkWidget *dlg;
	gint response;

	g_return_if_fail (app != NULL);

	game_pause (app);

	dlg = gtk_message_dialog_new (GTK_WINDOW (app->mainwin),
				      GTK_DIALOG_MODAL,
				      GTK_MESSAGE_QUESTION,
				      GTK_BUTTONS_YES_NO,
				      "%s",
				      _("Do you want to finish the game?"));
	response = gtk_dialog_run (GTK_DIALOG (dlg));
	gtk_object_unref (GTK_OBJECT (dlg));
	
	if (response == GTK_RESPONSE_OK) {
		save_score (app);
		game_clean_up (app);
		app->state = GAME_NOT_RUNNING;
		board_view_message (BOARD_MSG_NEW_GAME);
		update_menu_item_state (app);
	}
	else
		game_continue (app);
}

static void 
game_finished (AtomixApp *app)
{
	GtkWidget* dlg;

	g_return_if_fail (app != NULL);

	dlg = gtk_message_dialog_new (GTK_WINDOW (app->mainwin),
				      GTK_DIALOG_MODAL,
				      GTK_MESSAGE_INFO,
				      GTK_BUTTONS_CLOSE,
				      "%s",
				      _("Congratulations! You have finished all Atomix levels."));
        gtk_dialog_run (GTK_DIALOG (dlg));
	gtk_object_unref (GTK_OBJECT (dlg));

	game_clean_up (app);
	app->state = GAME_NOT_RUNNING;
	board_view_message (BOARD_MSG_NEW_GAME);
	update_menu_item_state (app);				
}

static void 
game_clean_up (AtomixApp *app)
{
	g_return_if_fail (app != NULL);

	clear_player_info (app);

	if (app->level)
		g_object_unref (app->level);
	app->level = NULL;

	board_clear ();
}

static void 
game_pause (AtomixApp *app)
{
	g_return_if_fail (app != NULL);
	g_return_if_fail (app->state == GAME_RUNNING);
	
	board_hide ();
	board_view_message (BOARD_MSG_GAME_PAUSED);
	app->state = GAME_PAUSED;
	update_menu_item_state (app);
}


static void 
game_continue (AtomixApp *app)
{
	g_return_if_fail (app != NULL);
	g_return_if_fail (app->state == GAME_PAUSED);

	board_hide_message (BOARD_MSG_GAME_PAUSED);
	board_show ();
		
	app->state = GAME_RUNNING;
	update_menu_item_state (app);
}

static void 
update_player_info (AtomixApp *app)
{
	gchar *str_buffer;
    
	g_return_if_fail (app != NULL);
	
	/* set level number */
	str_buffer = g_new0 (gchar, 10);
	g_snprintf (str_buffer, 10, "%i", app->level_no);
	gtk_label_set_text (GTK_LABEL (app->lb_level), str_buffer);
	g_free (str_buffer);

	/* set levelname */
	gtk_label_set_text (GTK_LABEL (app->lb_name), 
			    level_get_name (app->level));

	/* set score and time*/
	gtk_label_set_text (GTK_LABEL (app->lb_score), 
			    "0.0");
}

static void 
clear_player_info(AtomixApp *app)
{
	/* level number */
	gtk_label_set_text(GTK_LABEL(app->lb_level), "");

	/* level name */
	gtk_label_set_text(GTK_LABEL(app->lb_name), "");
    
	/* level score */
	gtk_label_set_text(GTK_LABEL(app->lb_score), "");
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
		g_message ("Path: %s : %i", path, cmd_list[i].enabled);
		bonobo_ui_component_set_prop (app->ui_component, path, "sensitive",
					      cmd_list[i].enabled ? "1" : "0", NULL);
		g_free (path);
	}
}

static void 
save_score (AtomixApp *app)
{
#if 0
	if(score > 0.0)
	{
		gint highscore_pos;
		highscore_pos = gnome_score_log(score, NULL, TRUE);
		gnome_scores_display("Atomix", "atomix", NULL, highscore_pos);	    
	}
#endif
}

void create_user_config_dir(void)
{
	DIR* ds = NULL;
	const char* home_dir = g_get_home_dir (); 
	char* atomix_dir;
	char* themes_dir;
	char* levels_dir;

	atomix_dir = g_strjoin( "/", home_dir, ".atomix", NULL);
	themes_dir = g_strjoin( "/", atomix_dir, "themes", NULL);
	levels_dir = g_strjoin( "/", atomix_dir, "levels", NULL);

	/* first, check whether .atomix exists */    
	ds = opendir(atomix_dir);
	if(ds==NULL)
	{
		/* try to create one */
		if(mkdir(atomix_dir, 0755 )!=0)
		{
			g_print("An error occured creating your .atomix dir!\n");
			return;
		}
		g_print(".atomix created.\n");
	}
	else
	{
		// g_print(".atomix already exists.\n");
		closedir(ds);
	}

	/* check if .atomix/themes exists */
	ds = opendir(themes_dir);
	if(ds==NULL)
	{
		/* try to create one */
		if(mkdir(themes_dir, 0755)!=0)
		{
			g_print("An error occured creating your .atomix/themes dir!\n");
			return;
		}
		g_print(".atomix/themes created.\n");
	}
	else
	{
		// g_print(".atomix/themes already exists.\n");
		closedir(ds);
	}

	/* check if .atomix/levels exists */
	ds = opendir(levels_dir);
	if(ds==NULL)
	{
		/* try to create one */
		if(mkdir(levels_dir, 0755)!=0)
		{
			g_print("An error occured creating your .atomix/levels dir!\n");
			return;
		}
		g_print(".atomix/levels created.\n");
	}
	else
	{
		// g_print(".atomix/levels already exists.\n");
		closedir(ds);
	}
    
	g_free(atomix_dir);
	g_free(themes_dir);
	g_free(levels_dir);
}

static BonoboUIVerb verbs[] = {
	BONOBO_UI_VERB ("GameNew", verb_GameNew_cb),
	BONOBO_UI_VERB ("GameEnd", verb_GameEnd_cb),
	BONOBO_UI_VERB ("GameSkip", verb_GameSkip_cb),
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
	GtkWidget *scrollwin;
	GtkWidget *frame;

	scrollwin = gtk_scrolled_window_new (NULL, NULL);
	*canvas = gnome_canvas_new ();
	gtk_container_add (GTK_CONTAINER (scrollwin), GTK_WIDGET (*canvas));
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrollwin),
					GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	frame = gtk_frame_new (NULL);
	gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
	gtk_container_add (GTK_CONTAINER (frame), scrollwin);

	return frame;
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
	GtkWidget *label;

	/* create canvas widgets */
	pf = create_canvas_widget (&app->ca_matrix);
	goal = create_canvas_widget (&app->ca_goal);
	gtk_widget_set_size_request (GTK_WIDGET (goal), 200, 50);

	/* add playfield canvas to left side */
	hbox = gtk_hbox_new (FALSE, 6);
	gtk_container_set_border_width (GTK_CONTAINER (hbox), 6);
	gtk_box_pack_start (GTK_BOX (hbox), GTK_WIDGET (pf), TRUE, TRUE, 0);
	
	/* create right window side */
	vbox = gtk_vbox_new (FALSE, 6);
	gtk_box_pack_start (GTK_BOX (hbox), GTK_WIDGET (vbox), FALSE, TRUE, 0);

	g_message ("before frame");

	/* create statistics frame */
	frame = gtk_frame_new (_("Statistics"));
	table = gtk_table_new (3, 2, FALSE);
	gtk_table_set_row_spacings (GTK_TABLE (table), 4);
	gtk_table_set_col_spacings (GTK_TABLE (table), 4);
	gtk_container_set_border_width (GTK_CONTAINER (table), 6);
	
	label = gtk_label_new (_("Level:"));
	app->lb_level = gtk_label_new ("NO LEVEL NAME");
	gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (label),
				   0, 1, 0, 1);
	gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (app->lb_level),
				   1, 2, 0, 1);

	label = gtk_label_new (_("Name:"));
	app->lb_name = gtk_label_new ("");
	gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (label),
				   0, 1, 1, 2);
	gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (app->lb_name),
				   1, 2, 1, 2);
	
	label = gtk_label_new (_("Score:"));
	app->lb_score = gtk_label_new ("");
	gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (label),
				   0, 1, 2, 3);
	gtk_table_attach_defaults (GTK_TABLE (table), GTK_WIDGET (app->lb_score),
				   1, 2, 2, 3);
	gtk_container_add (GTK_CONTAINER (frame), GTK_WIDGET (table));

	/* add frame and goal canvas to left side */
	gtk_box_pack_start (GTK_BOX (vbox), GTK_WIDGET (frame), FALSE, FALSE, 0);
	gtk_box_pack_start (GTK_BOX (vbox), GTK_WIDGET (goal), TRUE, TRUE, 0);

	g_message ("gui fertig");
	
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
	gtk_signal_connect(GTK_OBJECT(app->mainwin), "delete_event",
			   GTK_SIGNAL_FUNC(on_app_destroy_event), app);

	app->ui_container = bonobo_ui_engine_get_ui_container (
		bonobo_window_get_ui_engine (BONOBO_WINDOW (app->mainwin)));

	app->ui_component = bonobo_ui_component_new ("atomix");
	bonobo_ui_component_set_container (app->ui_component,
					   BONOBO_OBJREF(app->ui_container),
					   NULL);

	/* find xml menu description */
	ui_file = gnome_program_locate_file (prog, GNOME_FILE_DOMAIN_DATADIR,
						"atomix-ui.xml", TRUE, NULL);
	g_print ("ui_file: %s\n", ui_file);
	if (!ui_file) {
		ui_file = g_build_filename (".", "atomix-ui.xml", NULL);
		g_print ("ui_file: %s\n", ui_file);
		if (!g_file_test (ui_file, G_FILE_TEST_EXISTS))
			g_error (_("XML file %s not found."), "atomix-ui.xml");
	}

	/* set menus */
	bonobo_ui_util_set_ui (app->ui_component,
			       "", ui_file,
			       "atomix");
	g_free (ui_file);

	bonobo_ui_component_add_verb_list_with_data (app->ui_component, verbs, app);

	/* create window contents */
	content = create_mainwin_content (app);
	gtk_widget_show (GTK_WIDGET (content));
	
	bonobo_window_set_contents (BONOBO_WINDOW (app->mainwin), content);

	return app;
}

int
main (int argc, char *argv[])
{
	GnomeProgram *prog;

	gnome_score_init (PACKAGE);

	bindtextdomain (PACKAGE, GNOMELOCALEDIR);
	textdomain (PACKAGE);

	prog = gnome_program_init (PACKAGE, VERSION, LIBGNOMEUI_MODULE, 
				   argc, argv, NULL);

	/* init preferences */
	/* preferences_init(); */

	/* make a few initalisations here */
	create_user_config_dir();

	app = create_gui (prog);

	game_init (app);

	gtk_widget_set_size_request (GTK_WIDGET (app->mainwin), 600, 400); 
	gtk_widget_show (app->mainwin);

	gtk_main ();
	return 0;
}


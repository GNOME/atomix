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

#define ATOMIX_GLADE_FILE "atomix2.glade"

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include "global.h"
#include "board.h"
#include "playfield.h"
#include "preferences.h"
#include "main.h"
#include "goal.h"
#include "level.h"
/* #include "gtk-time-limit.h" */
#include "callbacks.h"
#include "util.h"

/****************************
 *    global variables
 */

static GladeXML     *gui  = NULL;   /* xml gui representation    */
static AtomixApp    *app; 
static LevelData    *level_data;    /* all infos about a level   */
gint game_state;                    /* current state of the game */


/**
 *  dialog callbacks
 */
gint next_level_dlg_cb(gpointer data);
void save_score(gdouble score);
void game_clean_up(void);

void game_load_level(LevelData *data);
void update_menu_item_state (gint state);
void update_player_info(LevelData *data);
static void clear_player_info (AtomixApp *app);
void create_user_config_dir(void);
#if 0
GtkWidget* get_time_limit_widget(void);
#endif

void game_bonus_level_timeout(GtkWidget *widget, gpointer user_data);

/***************************
 *    functions
 */
GladeXML* get_gui ()
{
	return NULL;
}


AtomixApp* get_app ()
{
	return app;
}

Level* get_actual_level()
{
	return level_data->level;
}

Theme* get_actual_theme()
{
	return app->theme;
}

gint get_game_state()
{
	return game_state;
}

#if 0
GtkWidget* get_time_limit_widget(void)
{
	GtkWidget *alignment;
	alignment = glade_xml_get_widget (gui, "clock");
	return GTK_BIN(alignment)->child;
}
#endif 

void show_about_dlg (void) 
{
	GladeXML *dlg_xml = glade_xml_new ("../atomix2.glade", "dlg_about", NULL);
	gtk_object_unref (GTK_OBJECT (dlg_xml));
}


static void 
game_init (AtomixApp *app)
{
	GtkWidget* clock;

	/* init level data struct */
	level_data = g_malloc(sizeof(LevelData));
	level_data->no_level = 1;
	level_data->level = NULL;
	level_data->score = 0.0;
	level_data->theme = NULL;

	/* init the board */
	board_init (app);

	/* init goal */
	goal_init (app);

	/* init theme manager */
	app->tm = theme_manager_new ();
	theme_manager_init_themes (app->tm);

#if 0
	/* connect the timeout function */
	clock = get_time_limit_widget();
	gtk_signal_connect(GTK_OBJECT(clock), "timeout",
			   GTK_SIGNAL_FUNC(game_level_timeout),NULL);
#endif

	/* enable only usable menu items according to state */
	game_state = GAME_NOT_RUNNING;
	update_menu_item_state(game_state);

	/* clear the level info */
	clear_player_info (app);
}

void game_new()
{
	level_data->level = level_load_xml(level_get_first_level());

	if(level_data->level) 
	{
		board_hide_message(BOARD_MSG_NEW_GAME);

		/* init level data */
		level_data->no_level = 1;
		level_data->score = 0.0;

		/* update game state */
		game_state = GAME_RUNNING;
		update_menu_item_state(game_state);

		/* load the level */
		game_load_level(level_data);    
		
	}
	else
	{
		/* show level not found dialog */
		GladeXML *dlg_xml;
		dlg_xml = glade_xml_new ("../atomix2.glade", "dlg_level_not_found", NULL);
		set_appbar_temporary(_("Level not found."));
		gtk_object_unref (GTK_OBJECT (dlg_xml));
	}
}

void game_undo_move()
{
	if(board_undo_move())
	{
		/* xgettext:no-c-format */
		set_appbar_temporary(_("Undo last move. This costs you 5 % of your score."));
		level_data->score = level_data->score * 0.95;
		update_player_info(level_data);
	}
	else
	{
		set_appbar_temporary(_("No more undo levels."));
	}
}

void game_skip_level(void)
{
	if(game_state == GAME_RUNNING)
	{
		if(level_is_last_level(level_data->level))
		{
			set_appbar_temporary(_("This is the last level. You can't skip any further."));
		}
		else
		{
			gchar *next;
			GtkWidget *clock;

#if 0
			clock = get_time_limit_widget();
			gtk_time_limit_stop(GTK_TIME_LIMIT(clock));
#endif 
			
			next = g_strdup(level_data->level->next);    
			level_destroy(level_data->level);
			level_data->level = level_load_xml(next);
			level_data->no_level++;
			g_free(next);
			
			/* load level */
			game_load_level(level_data);
		}
	}
}

void game_load_level(LevelData *data)
{
	g_return_if_fail(data != NULL);
	g_return_if_fail(data->level != NULL);

	/* load theme if necessary */
	if(data->theme)
	{
		if (!g_strcasecmp(theme_get_name (app->theme), data->level->theme_name))
		{
			/* the theme changed for this level */
			g_object_unref (app->theme);
			app->theme = theme_manager_get_theme (app->tm, data->level->theme_name);
			data->theme = app->theme;
		}
	}    
	else
	{
		app->theme = theme_manager_get_theme (app->tm, data->level->theme_name);    
		data->theme = app->theme;
	}

	/* init the level */
	board_clear();
	board_init_level(data->level);

	update_player_info(data);

#if 0
	if((data->level->time > 0) &&  preferences_get()->score_time_enabled)
	{			
		gtk_time_limit_start(GTK_TIME_LIMIT(get_time_limit_widget()));
	}
#endif 
}


void game_reload_level()
{
	game_load_level(level_data);
}

void game_level_timeout(GtkWidget *widget, gpointer data)
{
	GtkWidget *dlg;
	GladeXML *dlg_xml;
	GtkWidget* clock;
	gint button_nr;

	/* stop the clock */
#if 0
	clock = get_time_limit_widget();
	gtk_time_limit_stop(GTK_TIME_LIMIT(clock));      
        gtk_clock_set_seconds(GTK_CLOCK(clock), 0);
#endif 

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
		game_load_level(level_data);
	}
	else
	{
		game_finished();
	}
}

void game_level_finished(void)
{
	GtkWidget* clock;
	gchar* next;
	gint secs;
	GtkWidget* dlg = NULL;
	NextLevelDlgInfo* dlg_info;

#if 0
	/* stop clock */
	clock = get_time_limit_widget();
	gtk_time_limit_stop(GTK_TIME_LIMIT(clock));
#endif

	/* show cursur */
	board_show_normal_cursor();

	/* retrieve next level */
	next = g_strdup(level_data->level->next);    
    
	if(!level_is_last_level(level_data->level))
	{
		/* prepare next level */

		if(preferences_get()->score_time_enabled)
		{
			GladeXML *dlg_xml;

			/* show statistics dialog */     
			secs = 0; /* gtk_time_limit_get_seconds(GTK_TIME_LIMIT(clock)); */
			dlg_xml = glade_xml_new ("../atomix2.glade", "dlg_next_level", NULL);
			dlg_info = g_malloc(sizeof(NextLevelDlgInfo));
			dlg_info->timer_id = -1;
			dlg_info->secs = secs;
			dlg_info->score = level_data->score;
			dlg_info->lb_secs = glade_xml_get_widget (dlg_xml, "time");
			dlg_info->lb_score = glade_xml_get_widget (dlg_xml, "score");
			dlg_info->timer_id = gtk_timeout_add(60, next_level_dlg_cb, 
							     dlg_info);
	    
			dlg = glade_xml_get_widget (dlg_xml, "dlg_next_level");
			gnome_dialog_run(GNOME_DIALOG(dlg));
			gtk_widget_destroy(GTK_WIDGET(dlg));
	    
			if(dlg_info->timer_id != -1)
			{
				gtk_timeout_remove(dlg_info->timer_id);
			}
			g_free(dlg_info);
    
			/* set new score */
			level_data->score = level_data->score + secs;
		}

		/* update level data */
		level_destroy(level_data->level);
		level_data->level = level_load_xml(next);
		level_data->no_level++;
       
		/* load level */
		game_load_level(level_data);
	}
	else
	{
		/* player reached game end */
		game_finished();
	}
	g_free(next);
}

void game_user_quits()
{
	GtkWidget *dlg;
	GladeXML *dlg_xml;
	gint button_nr;
	game_pause();
	
	dlg_xml = glade_xml_new ("../atomix2.glade", "dlg_quit_game", NULL);
	dlg = glade_xml_get_widget (dlg_xml, "dlg_quit_game");
	button_nr = gnome_dialog_run (GNOME_DIALOG(dlg));
	
	game_continue();
	if(button_nr == 0 /* yes */)
	{
#if 0
		GtkWidget *clock;
		/* stop the clock */
		clock = get_time_limit_widget();
		gtk_time_limit_stop(GTK_TIME_LIMIT(clock));
		gtk_clock_set_seconds(GTK_CLOCK(clock), 0);
#endif 

		save_score(level_data->score);
		game_clean_up();
		game_state = GAME_NOT_RUNNING;
		board_view_message(BOARD_MSG_NEW_GAME);
		update_menu_item_state(game_state);
	}
}

gint next_level_dlg_cb(gpointer data)
{
	NextLevelDlgInfo* dlg_info = (NextLevelDlgInfo*) data;
	gint minutes, secs;
	gchar* time_str = g_malloc(6*sizeof(gchar));
	gchar* score_str = g_malloc(10*sizeof(gchar));
    
	if(dlg_info->secs > 0)
	{
		dlg_info->secs--;
		dlg_info->score = dlg_info->score + 1.0;
	}
	else
	{
		gtk_timeout_remove(dlg_info->timer_id);
		dlg_info->timer_id = -1;
	}

	minutes = (dlg_info->secs)/60;
	secs = (dlg_info->secs)%60;

	if(secs < 10)
	{
		g_snprintf(time_str, 6, "%i:0%i", minutes, secs);
	}
	else
	{
		g_snprintf(time_str, 6, "%i:%i", minutes, secs);
	}
	g_snprintf(score_str, 10, "%.2f", dlg_info->score);
	
	gtk_label_set_text(GTK_LABEL(dlg_info->lb_secs), time_str);
	gtk_label_set_text(GTK_LABEL(dlg_info->lb_score), score_str);
    

	return TRUE;
}


void game_finished(void)
{
	GtkWidget* clock = NULL;
	gint secs;
	GtkWidget* dlg = NULL;
	NextLevelDlgInfo* dlg_info;
	gint button_nr = -1;

#if 0
	/* stop clock */
	clock = get_time_limit_widget();
#endif
	
	if(preferences_get()->score_time_enabled)
	{
		GladeXML *dlg_xml;

		/* show statistics dialog */     
		secs = 0; /* gtk_time_limit_get_seconds(GTK_TIME_LIMIT(clock)); */
		dlg_xml = glade_xml_new ("../atomix2.glade", "dlg_last_level", NULL);
		dlg = glade_xml_get_widget (dlg_xml, "dlg_last_level");
		dlg_info = g_malloc(sizeof(NextLevelDlgInfo));
		dlg_info->timer_id = -1;
		dlg_info->secs = secs;
		dlg_info->score = level_data->score;
		dlg_info->lb_secs = glade_xml_get_widget (dlg_xml, "time");
		dlg_info->lb_score = glade_xml_get_widget (dlg_xml, "score");       
		dlg_info->timer_id = gtk_timeout_add(60, next_level_dlg_cb, dlg_info);
	
		gtk_widget_show(GTK_WIDGET(dlg));
		button_nr = gnome_dialog_run(GNOME_DIALOG(dlg));
		gtk_widget_destroy(GTK_WIDGET(dlg));

		if(dlg_info->timer_id != -1)
		{
			gtk_timeout_remove(dlg_info->timer_id);
		}
		g_free(dlg_info);

		/* calculate final score */
		level_data->score = level_data->score + secs;
		save_score(level_data->score);
	}

	if(button_nr == 0)
	{
		/* player wants new game */
		game_new();
	}
	else
	{
		/* no new game */
		game_clean_up();
		game_state = GAME_NOT_RUNNING;
		board_view_message(BOARD_MSG_NEW_GAME);
		update_menu_item_state(game_state);				
	}
}

void game_clean_up(void)
{
	clear_player_info (app);

	if(level_data->level) level_destroy(level_data->level);
	level_data->level = NULL;

	board_clear();
}

void game_pause(void)
{
	if(game_state == GAME_RUNNING)
	{
#if 0
		if((level_data->level->time > 0) &&
		   preferences_get()->score_time_enabled)
		{
			GtkWidget *tl;
			tl = get_time_limit_widget();
			gtk_time_limit_stop(GTK_TIME_LIMIT(tl));
		}
#endif
	
		board_hide();
		board_view_message(BOARD_MSG_GAME_PAUSED);
		game_state = GAME_PAUSED;
		update_menu_item_state(game_state);
	}
}


void game_continue(void)
{
	if(game_state == GAME_PAUSED)
	{
		board_hide_message(BOARD_MSG_GAME_PAUSED);
		board_show();
	
#if 0
		if((level_data->level->time > 0) && 
		   preferences_get()->score_time_enabled)
		{
			GtkWidget *tl;
			tl = get_time_limit_widget();
			gtk_time_limit_start(GTK_TIME_LIMIT(tl));
		}
#endif

		game_state = GAME_RUNNING;
		update_menu_item_state(game_state);
	}
}

void update_player_info(LevelData *data)
{
	GtkWidget *lb_number;
	GtkWidget *lb_name;
	GtkWidget *lb_score;
	GtkWidget *clock;
	gchar *str_buffer;
    
	g_return_if_fail(data != NULL);
	g_return_if_fail(data->level != NULL);

	str_buffer = g_malloc(10*sizeof(str_buffer));
	
	/* set level number */
	lb_number = GTK_WIDGET(glade_xml_get_widget (gui, "lb_number"));
	g_snprintf(str_buffer, 10, "%i", data->no_level);
	gtk_label_set_text(GTK_LABEL(lb_number), str_buffer);

	/* set levelname */
	lb_name = GTK_WIDGET(glade_xml_get_widget (gui, "lb_level"));
	gtk_label_set_text(GTK_LABEL(lb_name), data->level->name);

	/* set score and time*/
	lb_score = GTK_WIDGET(glade_xml_get_widget (gui, "lb_score"));
#if 0
	clock = get_time_limit_widget();
	gtk_widget_show(clock);
#endif

	if(preferences_get()->score_time_enabled)
	{
		g_snprintf(str_buffer, 10, "%.2f", data->score);
		gtk_label_set_text(GTK_LABEL(lb_score), str_buffer);

#if 0
		gtk_clock_set_seconds(GTK_CLOCK(clock), data->level->time);
#endif
	}
	else
	{
		gtk_label_set_text(GTK_LABEL(lb_score), "");
#if 0
		gtk_clock_set_seconds(GTK_CLOCK(clock), 0);
#endif
	}

	g_free(str_buffer);
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
    
#if 0
	/* time */
	clock = get_time_limit_widget();
	gtk_widget_hide(clock);
	gtk_clock_set_seconds(GTK_CLOCK(clock), 0);
#endif
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

void update_menu_item_state (gint state)
{
	gchar *path;
	gint i;
	const CmdEnable *cmd_list = state_sensitivity [state];

	for (i = 0; cmd_list[i].cmd != NULL; i++) {
		path = g_strconcat ("/commands/", cmd_list[i].cmd, NULL);
		g_message ("Path: %s : %i", path, cmd_list[i].enabled);
		bonobo_ui_component_set_prop (app->ui_component, path, "sensitive",
					      cmd_list[i].enabled ? "1" : "0", NULL);
		g_free (path);
	}
}

void save_score(gdouble score)
{
	if(score > 0.0)
	{
		gint highscore_pos;
		highscore_pos = gnome_score_log(score, NULL, TRUE);
		gnome_scores_display("Atomix", "atomix", NULL, highscore_pos);	    
	}
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

	glade_gnome_init ();

	/* init preferences */
	preferences_init();

	/* make a few initalisations here */
	create_user_config_dir();
	level_create_hash_table();

	app = create_gui (prog);

	game_init (app);

	gtk_widget_set_size_request (GTK_WIDGET (app->mainwin), 600, 400); 
	gtk_widget_show (app->mainwin);

	gtk_main ();
	return 0;
}


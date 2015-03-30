/* Atomix -- a little puzzle game about atoms and molecules.
 * Copyright (C) 1999-2001 Jens Finke
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

#include "config.h"

#include <sys/types.h>
#include <dirent.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>

#include "board-gtk.h"
#include "playfield.h"
#include "main.h"
#include "goal.h"
#include "level.h"
#include "goal-view.h"
#include "clock.h"
#include "undo.h"

AtomixApp *app;

typedef enum
  {
    GAME_ACTION_NEW,
    GAME_ACTION_END,
    GAME_ACTION_PAUSE,
    GAME_ACTION_CONTINUE,
    GAME_ACTION_SKIP,
    GAME_ACTION_UNDO,
    GAME_ACTION_FINISHED,
    GAME_ACTION_RESTART,
  } GameAction;

static void controller_handle_action (GameAction action);
static void set_game_not_running_state (void);
static gboolean set_next_level (void);
static void setup_level (void);
static void level_cleanup_view (void);
static void atomix_exit (void);
static gboolean on_key_press_event (GObject *widget, GdkEventKey *event,
				    gpointer user_data);
static void game_init (void);
static void update_statistics (void);
static void view_congratulations (void);
static void calculate_score (void);

/* ===============================================================
      
             Menu callback  functions 

-------------------------------------------------------------- */
static void verb_GameNew_cb (GtkMenuItem * action, gpointer data)
{
  controller_handle_action (GAME_ACTION_NEW);
}

static void verb_GameEnd_cb (GtkMenuItem * action, gpointer data)
{
  controller_handle_action (GAME_ACTION_END);
}

static void verb_GameSkip_cb (GtkMenuItem * action, gpointer data)
{
  controller_handle_action (GAME_ACTION_SKIP);
}

static void verb_GameReset_cb (GtkMenuItem * action, gpointer data)
{
  controller_handle_action (GAME_ACTION_RESTART);
}

static void verb_GamePause_cb (GtkMenuItem * action, gpointer data)
{
  controller_handle_action (GAME_ACTION_PAUSE);
}

static void verb_GameContinue_cb (GtkMenuItem * action, gpointer data)
{
  controller_handle_action (GAME_ACTION_CONTINUE);
}

static void verb_GameUndo_cb (GtkMenuItem * action, gpointer data)
{
  controller_handle_action (GAME_ACTION_UNDO);
}

static void verb_GameExit_cb (GtkMenuItem * action, gpointer data)
{
  atomix_exit ();
}

static void verb_HelpAbout_cb (GtkMenuItem * action, gpointer data)
{
  GtkWidget *dlg;

  const char *authors[] =
    {
      "Robert Roth <robert.roth.off@gmail.com>",
      "Guilherme de S. Pastore <gpastore@gnome.org>",
      "Jens Finke <jens@triq.net>",
      NULL
    };

  const char *artists[] =
    {
      "Jakub Steiner <jimmac@ximian.com>",
      NULL
    };

  dlg = gtk_about_dialog_new ();
  g_signal_connect (dlg, "response", G_CALLBACK (gtk_widget_destroy), NULL);
  gtk_about_dialog_set_program_name (GTK_ABOUT_DIALOG(dlg), "Atomix");
  gtk_about_dialog_set_version (GTK_ABOUT_DIALOG(dlg), VERSION);
  gtk_about_dialog_set_comments (GTK_ABOUT_DIALOG(dlg), _("A puzzle game about atoms and molecules"));
  gtk_about_dialog_set_website (GTK_ABOUT_DIALOG(dlg), "http://wiki.gnome.org/Apps/Atomix");
  gtk_about_dialog_set_authors (GTK_ABOUT_DIALOG(dlg), authors);
  gtk_about_dialog_set_artists (GTK_ABOUT_DIALOG(dlg), artists);
  gtk_about_dialog_set_translator_credits (GTK_ABOUT_DIALOG(dlg), _("translator-credits"));
  gtk_window_set_transient_for (GTK_WINDOW (dlg), GTK_WINDOW (app->mainwin));
  gtk_widget_show (dlg);
}

static gboolean on_app_destroy_event (GtkWidget *widget, GdkEvent *event,
				      gpointer user_data)
{
  atomix_exit ();

  return TRUE;
}

/* ===============================================================
      
             Game steering functions

-------------------------------------------------------------- */

static void controller_handle_action (GameAction action)
{
  switch (app->state)
    {
    case GAME_STATE_NOT_RUNNING:
      if (action == GAME_ACTION_NEW)
	{
	  if (set_next_level ())
	    {
	      app->level_no = 1;
	      app->score = 0;
	      setup_level ();
	      app->state = GAME_STATE_RUNNING_UNMOVED;
	    }
	}
      break;

    case GAME_STATE_RUNNING_UNMOVED:
    case GAME_STATE_RUNNING:
      switch (action)
	{
	case GAME_ACTION_END:
	  level_cleanup_view ();
	  set_game_not_running_state ();
	  break;

	case GAME_ACTION_PAUSE:
	  clock_stop (CLOCK (app->clock));
	  board_gtk_hide ();
	  app->state = GAME_STATE_PAUSED;
	  break;

	case GAME_ACTION_SKIP:
	  level_cleanup_view ();

	  if (set_next_level ())
	    setup_level ();

	  else
	    set_game_not_running_state ();

	  break;

	case GAME_ACTION_FINISHED:
	  calculate_score ();
	  if (level_manager_is_last_level (app->lm, app->level))
	    {
	      view_congratulations ();
	      level_cleanup_view ();
	      set_game_not_running_state ();
	    }
	  else
	    {
	      level_cleanup_view ();
	      set_next_level ();
	      setup_level ();
	    }
	  break;

	case GAME_ACTION_RESTART:
	  g_assert (app->state != GAME_STATE_RUNNING_UNMOVED);

	  level_cleanup_view ();
	  setup_level ();
	  break;

	case GAME_ACTION_UNDO:
	  g_assert (app->state != GAME_STATE_RUNNING_UNMOVED);

	  board_gtk_undo_move ();
	  break;

	default:
	  break;
	}
      break;

    case GAME_STATE_PAUSED:
      if (action == GAME_ACTION_CONTINUE)
	{
	  clock_start (CLOCK(app->clock));
	  board_gtk_show ();
	  app->state = (undo_exists())?GAME_STATE_RUNNING:GAME_STATE_RUNNING_UNMOVED;
	}
      break;

    default:
      g_assert_not_reached ();
    }

  update_menu_item_state ();
  update_statistics ();
}

static void set_game_not_running_state (void)
{
  board_gtk_show_logo (TRUE);

  if (app->level)
    g_object_unref (app->level);

  if (app->goal)
    g_object_unref (app->goal);

  app->level = NULL;
  app->goal = NULL;
  app->score = 0;
  app->state = GAME_STATE_NOT_RUNNING;
}

static gboolean set_next_level (void)
{
  Level *next_level;
  PlayField *goal_pf;

  next_level = level_manager_get_next_level (app->lm, app->level);

  if (app->level)
    {
      g_object_unref (app->level);
    }
  app->level = NULL;

  if (app->goal)
    {
      g_object_unref (app->goal);
    }
  app->goal = NULL;

  if (next_level == NULL)
    {
      return FALSE;
    }

  app->level = next_level;
  app->level_no++;
  goal_pf = level_get_goal (app->level);
  app->goal = goal_new (goal_pf);

  g_object_unref (goal_pf);

  return TRUE;
}

static void setup_level (void)
{
  PlayField *env_pf;
  PlayField *sce_pf;

  g_return_if_fail (app != NULL);

  if (app->level == NULL)
    return;

  g_return_if_fail (app->goal != NULL);

  /* init board */
  env_pf = level_get_environment (app->level);
  sce_pf = level_get_scenario (app->level);
  board_gtk_show_logo (FALSE);
  board_gtk_init_level (env_pf, sce_pf, app->goal);

  /* init goal */
  goal_view_render (app->goal);

  /* init clock */
  clock_set_seconds (CLOCK(app->clock), 0);
  clock_start (CLOCK(app->clock));

  g_object_unref (env_pf);
  g_object_unref (sce_pf);
}

static void level_cleanup_view (void)
{
  board_gtk_clear ();
  goal_view_clear ();

  clock_stop (CLOCK(app->clock));
}

static void atomix_exit (void)
{
#ifdef DEBUG
  g_message ("Destroy application");
#endif

  g_return_if_fail (app != NULL);

  if (app->state != GAME_STATE_NOT_RUNNING)
    {
      set_game_not_running_state ();
    }

  board_gtk_destroy ();

  if (app->level)
    g_object_unref (app->level);

  if (app->lm)
    g_object_unref (app->lm);

  if (app->theme)
    g_object_unref (app->theme);

  if (app->tm)
    g_object_unref (app->tm);

  if (app->actions)
    g_hash_table_destroy (app->actions);

  /* quit application */
  gtk_widget_destroy (app->mainwin);

  gtk_main_quit ();
}

static gboolean on_key_press_event (GObject *widget, GdkEventKey *event,
				    gpointer user_data)
{
  if ((app->state == GAME_STATE_RUNNING) || (app->state == GAME_STATE_RUNNING_UNMOVED))
    {
      return board_gtk_handle_key_event (NULL, event, NULL);
    }

  return FALSE;
}

static void game_init ()
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
  clock_set_format (CLOCK(app->clock), "%M:%S");
  clock_set_seconds (CLOCK(app->clock), 0);

  /* init the board */
  board_gtk_init (app->theme, GTK_FIXED (app->fi_matrix));

  /* init goal */
  goal_view_init (app->theme, GTK_FIXED (app->fi_goal));

  /* update user visible information */
  app->state = GAME_STATE_NOT_RUNNING;
  update_menu_item_state ();
  update_statistics ();

  gtk_widget_grab_focus (GTK_WIDGET (app->fi_matrix));
}

void game_level_finished (void)
{
  controller_handle_action (GAME_ACTION_FINISHED);
}

static void calculate_score (void)
{
  gint seconds;

  seconds = time (NULL) - CLOCK(app->clock)->seconds;

  if (seconds > 300)
    return;

  if (app->score == 0)
    app->score = 300 - seconds;
  else
    app->score = app->score * (2 - (seconds / 300));
}

static void view_congratulations (void)
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

/* ===============================================================
      
             UI update functions

-------------------------------------------------------------- */
static void update_statistics (void)
{
  gchar *str_buffer;

  g_return_if_fail (app != NULL);

  if (app->state == GAME_STATE_NOT_RUNNING)
    {
      /* don't show anything */
      gtk_label_set_text (GTK_LABEL (app->lb_level), "");
      gtk_label_set_text (GTK_LABEL (app->lb_name), "");
      gtk_label_set_text (GTK_LABEL (app->lb_formula), "");
      gtk_label_set_text (GTK_LABEL (app->lb_score), "");
      gtk_widget_hide (GTK_WIDGET (app->clock));
    }
  else
    {
      /* set level number */
      str_buffer = g_new0 (gchar, 10);
      g_snprintf (str_buffer, 10, "%i", app->level_no);
      gtk_label_set_text (GTK_LABEL (app->lb_level), str_buffer);

      /* set levelname */
      gtk_label_set_text (GTK_LABEL (app->lb_name),
			  _(level_get_name (app->level)));

      /* set the formula of the compound */
      gtk_label_set_markup (GTK_LABEL (app->lb_formula),
			    level_get_formula (app->level));

      /* set score */
      g_snprintf (str_buffer, 10, "%i", app->score);
      gtk_label_set_text (GTK_LABEL (app->lb_score), str_buffer);

      /* show clock */
      gtk_widget_show (GTK_WIDGET (app->clock));

      g_free (str_buffer);
    }
}

typedef struct
{
  gchar *cmd;
  gboolean enabled;
} CmdEnable;

static const CmdEnable not_running[] =
  {
    { "GameNew",      TRUE  },
    { "GameEnd",      FALSE },
    { "GameSkip",     FALSE },
    { "GameReset",    FALSE },
    { "GameUndo",     FALSE },
    { "GamePause",    FALSE },
    { "GameContinue", FALSE },
    { NULL, FALSE }
  };

static const CmdEnable running_unmoved[] =
  {
    { "GameNew",      FALSE },
    { "GameEnd",      TRUE  },
    { "GameSkip",     TRUE  },
    { "GameReset",    FALSE },
    { "GameUndo",     FALSE },
    { "GamePause",    TRUE  },
    { "GameContinue", FALSE },
    { NULL, FALSE }
};

static const CmdEnable running[] =
  {
    { "GameNew",      FALSE },
    { "GameEnd",      TRUE  },
    { "GameSkip",     TRUE  },
    { "GameReset",    TRUE  },
    { "GameUndo",     TRUE  },
    { "GamePause",    TRUE  },
    { "GameContinue", FALSE },
    { NULL, FALSE }
};

static const CmdEnable paused[] =
  {
    { "GameNew",      FALSE },
    { "GameEnd",      FALSE },
    { "GameSkip",     FALSE },
    { "GameReset",    FALSE },
    { "GameUndo",     FALSE },
    { "GamePause",    FALSE },
    { "GameContinue", TRUE  },
    { NULL, FALSE }
};

static const CmdEnable *state_sensitivity[] =
  {
    not_running, running_unmoved, running, paused
  };

void update_menu_item_state (void)
{
  gint i;
  const CmdEnable *cmd_list = state_sensitivity[app->state];
  GtkWidget *widget;

  for (i = 0; cmd_list[i].cmd != NULL; i++)
    {
      widget = g_hash_table_lookup (app->actions, cmd_list[i].cmd);
      gtk_widget_set_sensitive (widget, cmd_list[i].enabled);
    }
}

/* ===============================================================
      
             GUI creation  functions 

-------------------------------------------------------------- */
static AtomixApp *create_gui (void)
{
  AtomixApp *app;
  gchar *ui_path;
  gchar *icon_path;
  GtkBuilder *builder;
  GtkWidget *stats_grid;
  GtkWidget *time_label;
  GtkWidget *menu_item;

  app = g_new0 (AtomixApp, 1);
  app->level = NULL;

  builder = gtk_builder_new ();
  
  ui_path = g_build_filename (PKGDATADIR, "ui", "interface.ui", NULL);
  gtk_builder_add_from_file (builder, ui_path, NULL);
  g_free (ui_path);

//  ui_path = g_build_filename (PKGDATADIR, "ui", "menus.ui", NULL);
//  gtk_builder_add_from_file (builder, ui_path, NULL);
//  g_free (ui_path);

  app->mainwin = GTK_WIDGET (gtk_builder_get_object (builder, "mainwin"));

  app->actions = g_hash_table_new (NULL, NULL);

  menu_item = GTK_WIDGET (gtk_builder_get_object (builder, "gameNew"));
  g_hash_table_insert (app->actions, "GameNew", menu_item);
  g_signal_connect (menu_item, "activate", (GCallback)verb_GameNew_cb, NULL);

  menu_item = GTK_WIDGET (gtk_builder_get_object (builder, "gameEnd"));
  g_hash_table_insert (app->actions, "GameEnd", menu_item);
  g_signal_connect (menu_item, "activate", (GCallback)verb_GameEnd_cb, NULL);

  menu_item = GTK_WIDGET (gtk_builder_get_object (builder, "gameSkip"));
  g_hash_table_insert (app->actions, "GameSkip", menu_item);
  g_signal_connect (menu_item, "activate", (GCallback)verb_GameSkip_cb, NULL);

  menu_item = GTK_WIDGET (gtk_builder_get_object (builder, "gameReset"));
  g_hash_table_insert (app->actions, "GameReset", menu_item);
  g_signal_connect (menu_item, "activate", (GCallback)verb_GameReset_cb, NULL);

  menu_item = GTK_WIDGET (gtk_builder_get_object (builder, "gameUndo"));
  g_hash_table_insert (app->actions, "GameUndo", menu_item);
  g_signal_connect (menu_item, "activate", (GCallback)verb_GameUndo_cb, NULL);

  menu_item = GTK_WIDGET (gtk_builder_get_object (builder, "gamePause"));
  g_hash_table_insert (app->actions, "GamePause", menu_item);
  g_signal_connect (menu_item, "activate", (GCallback)verb_GamePause_cb, NULL);

  menu_item = GTK_WIDGET (gtk_builder_get_object (builder, "gameContinue"));
  g_hash_table_insert (app->actions, "GameContinue", menu_item);
  g_signal_connect (menu_item, "activate", (GCallback)verb_GameContinue_cb, NULL);

  menu_item = GTK_WIDGET (gtk_builder_get_object (builder, "gameQuit"));
  g_signal_connect (menu_item, "activate", (GCallback)verb_GameExit_cb, NULL);

  menu_item = GTK_WIDGET (gtk_builder_get_object (builder, "gameAbout"));
  g_signal_connect (menu_item, "activate", (GCallback)verb_HelpAbout_cb, NULL);

  g_signal_connect (G_OBJECT (app->mainwin), "delete_event",
                    (GCallback) on_app_destroy_event, app);

  /* create canvas widgets */
  app->fi_matrix = GTK_WIDGET (gtk_builder_get_object (builder, "game_fixed"));
  app->fi_goal = GTK_WIDGET (gtk_builder_get_object (builder, "preview_fixed"));

  stats_grid = GTK_WIDGET (gtk_builder_get_object (builder, "stats_grid"));
  time_label = GTK_WIDGET (gtk_builder_get_object (builder, "time_label"));
  app->clock = clock_new ();
  gtk_grid_attach_next_to (GTK_GRID (stats_grid), app->clock,
                           time_label, GTK_POS_RIGHT, 1, 1);

  /* add playfield canvas to left side */
  g_signal_connect (G_OBJECT (app->mainwin), "key-press-event",
		    G_CALLBACK (on_key_press_event), app);

  app->lb_level = GTK_WIDGET (gtk_builder_get_object (builder, "level_value"));
  app->lb_name = GTK_WIDGET (gtk_builder_get_object (builder, "molecule_value"));
  app->lb_formula = GTK_WIDGET (gtk_builder_get_object (builder, "formula_value"));
  app->lb_score = GTK_WIDGET (gtk_builder_get_object (builder, "score_value"));

  g_object_unref (builder);

  icon_path = g_build_filename (DATADIR,
							    "pixmaps",
							    "atomix-icon.png",
							    NULL);
  gtk_window_set_default_icon_from_file (icon_path,
					 		   NULL);
  g_free (icon_path);

  gtk_widget_show_all (GTK_WIDGET (app->mainwin));

  return app;
}

int main (int argc, char *argv[])
{
  GOptionContext *context;
  gboolean retval;
  GError *error = NULL;

  context = g_option_context_new (NULL);
  g_option_context_set_translation_domain (context, GETTEXT_PACKAGE);
  g_option_context_add_group (context, gtk_get_option_group (TRUE));

  retval = g_option_context_parse (context, &argc, &argv, &error);
  g_option_context_free (context);
  if (!retval) {
    g_print ("%s", error->message);
    g_error_free (error);
    exit (1);
  }

  g_set_application_name (_("Atomix"));

  bindtextdomain (GETTEXT_PACKAGE, GNOMELOCALEDIR);
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
  textdomain (GETTEXT_PACKAGE);
  
  /* make a few initalisations here */
  app = create_gui ();

  game_init ();

  gtk_widget_set_size_request (GTK_WIDGET (app->mainwin), 678, 520);
  //gtk_window_set_resizable (GTK_WINDOW (app->mainwin), FALSE);
  gtk_widget_show (app->mainwin);

  gtk_main ();
  return 0;
}

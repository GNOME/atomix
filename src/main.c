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

#include "board-gtk.h"
#include "playfield.h"
#include "main.h"
#include "goal.h"
#include "level.h"
#include "goal-view.h"
#include "clock.h"
#include "undo.h"
#include <libgnome-games-support.h>

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
static void game_init (void);
static void update_statistics (void);
static void view_congratulations (void);
static void calculate_score (void);

/* ===============================================================
      
             Menu callback  functions 

-------------------------------------------------------------- */
static void verb_GameNew_cb (GSimpleAction *action, GVariant *variant, gpointer data)
{
  controller_handle_action (GAME_ACTION_NEW);
}

static void verb_GameEnd_cb (GSimpleAction *action, GVariant *variant, gpointer data)
{
  controller_handle_action (GAME_ACTION_END);
}

static void verb_GameSkip_cb (GSimpleAction *action, GVariant *variant, gpointer data)
{
  controller_handle_action (GAME_ACTION_SKIP);
}

static void verb_GameReset_cb (GSimpleAction *action, GVariant *variant, gpointer data)
{
  controller_handle_action (GAME_ACTION_RESTART);
}

static void verb_GamePause_cb (GSimpleAction *action, GVariant *variant, gpointer data)
{
  if (app->state != GAME_STATE_PAUSED) {
    controller_handle_action (GAME_ACTION_PAUSE);
  } else {
    controller_handle_action (GAME_ACTION_CONTINUE);
  }
}

static void verb_GameUndo_cb (GSimpleAction *action, GVariant *variant, gpointer data)
{
  controller_handle_action (GAME_ACTION_UNDO);
}

static void verb_GameExit_cb (GSimpleAction *action, GVariant *variant, gpointer data)
{
  atomix_exit ();
}

static void verb_HelpAbout_cb (GSimpleAction *action, GVariant *variant, gpointer data)
{
  const char *authors[] =
    {
      "Guilherme de S. Pastore <gpastore@gnome.org>",
      "Jens Finke <jens@triq.net>",
      "Robert Roth <robert.roth.off@gmail.com>",
      NULL
    };

  const char *artists[] =
    {
      "Jakub Steiner <jimmac@ximian.com>",
      NULL
    };


  gtk_show_about_dialog(GTK_WINDOW(app->mainwin),
                        "program-name", _("Atomix"),
                        "logo-icon-name", APP_ID,
                        "version", VERSION,
                        "comments", _("A puzzle game about atoms and molecules"),
                        "website", "https://wiki.gnome.org/Apps/Atomix",
                        "authors", authors,
                        "artists", artists,
                        "translator_credits", _("translator-credits"),
                        NULL);
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
      if (action == GAME_ACTION_NEW && set_next_level ())
	      {
	        app->level_no = 1;
	        app->score = 0;
	        setup_level ();
	        app->state = GAME_STATE_RUNNING_UNMOVED;
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

	      case GAME_ACTION_NEW:
	      case GAME_ACTION_CONTINUE:
	      default:
	        break;
	    }
      break;

    case GAME_STATE_PAUSED:
      if (action == GAME_ACTION_CONTINUE)
	      {
	        clock_resume (CLOCK(app->clock));
	        board_gtk_show ();
	        app->state = (undo_exists())?GAME_STATE_RUNNING:GAME_STATE_RUNNING_UNMOVED;
	      }
      break;
    default:
      g_assert_not_reached ();
  }

  update_menu_item_state ();
  update_statistics ();
  gtk_widget_grab_focus (GTK_WIDGET (app->fi_matrix));
}

static void
add_score_cb (GObject *source_object, GAsyncResult *res, gpointer user_data)
{
  GamesScoresContext *context = GAMES_SCORES_CONTEXT (source_object);
  GError *error = NULL;

  games_scores_context_add_score_finish (context, res, &error);
  if (error != NULL) {
    g_warning ("Failed to add score: %s", error->message);
    g_error_free (error);
  }
}


static void set_game_not_running_state (void)
{
  board_gtk_show_logo (TRUE);

  if (app->level)
    g_object_unref (app->level);

  if (app->goal)
    g_object_unref (app->goal);

  if (app->score > 0) {
    games_scores_context_add_score (app->high_scores,
                                    app->score,
                                    app->current_category,
                                    NULL,
                                    add_score_cb,
                                    NULL);
  }

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
  clock_reset (CLOCK(app->clock));
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
    set_game_not_running_state ();

  board_gtk_destroy ();

  if (app->level)
    g_object_unref (app->level);

  if (app->lm)
    g_object_unref (app->lm);

  if (app->theme)
    g_object_unref (app->theme);

  if (app->tm)
    g_object_unref (app->tm);

  /* quit application */
  gtk_widget_destroy (app->mainwin);

}

static void game_init (void)
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
  clock_reset (CLOCK(app->clock));

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
  gint seconds = clock_get_elapsed (CLOCK (app->clock));

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
      gtk_widget_set_visible (GTK_WIDGET (app->clock), FALSE);
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
      gtk_widget_set_visible (GTK_WIDGET (app->clock), TRUE);

      g_free (str_buffer);
    }
}

typedef struct
{
  const gchar *cmd;
  gboolean enabled;
} CmdEnable;


static const GActionEntry app_entries[] =
  {
    { "GameAbout", verb_HelpAbout_cb, NULL, NULL, NULL},
    { "GameQuit", verb_GameExit_cb, NULL, NULL, NULL}
  };

static const GActionEntry win_entries[] =
  {
    { "GameNew", verb_GameNew_cb, NULL, NULL, NULL},
    { "GameEnd", verb_GameEnd_cb, NULL, NULL, NULL},
    { "LevelSkip", verb_GameSkip_cb, NULL, NULL, NULL},
    { "LevelReset", verb_GameReset_cb, NULL, NULL, NULL},
    { "GameUndo", verb_GameUndo_cb, NULL, NULL, NULL},
    { "GamePause", verb_GamePause_cb, NULL, NULL, NULL},
  };

static const CmdEnable not_running[] =
  {
    { "GameNew",      TRUE  },
    { "GameEnd",      FALSE },
    { "LevelSkip",    FALSE },
    { "LevelReset",   FALSE },
    { "GameUndo",     FALSE },
    { "GamePause",    FALSE },
    { NULL, FALSE }
  };

static const CmdEnable running_unmoved[] =
  {
    { "GameNew",      FALSE },
    { "GameEnd",      TRUE  },
    { "LevelSkip",    TRUE  },
    { "LevelReset",   FALSE },
    { "GameUndo",     FALSE },
    { "GamePause",    TRUE  },
    { NULL, FALSE }
};

static const CmdEnable running[] =
  {
    { "GameNew",      FALSE },
    { "GameEnd",      TRUE  },
    { "LevelSkip",    TRUE  },
    { "LevelReset",   TRUE  },
    { "GameUndo",     TRUE  },
    { "GamePause",    TRUE  },
    { NULL, FALSE }
};

static const CmdEnable paused[] =
  {
    { "GameNew",      FALSE },
    { "GameEnd",      FALSE },
    { "LevelSkip",    FALSE },
    { "LevelReset",   FALSE },
    { "GameUndo",     FALSE },
    { "GamePause",    TRUE  },
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
  GAction *action;

  for (i = 0; cmd_list[i].cmd != NULL; i++)
    {
      action = g_action_map_lookup_action (G_ACTION_MAP (app->mainwin), cmd_list[i].cmd);
      g_simple_action_set_enabled (G_SIMPLE_ACTION (action), cmd_list[i].enabled);
    }
}

static GamesScoresCategory* get_scores_category(const gchar* category_key, void* user_data)
{
  return app->current_category;
}
/* ===============================================================
      
             GUI creation  functions 

-------------------------------------------------------------- */
static AtomixApp *create_gui (GApplication *app_instance)
{
  gchar *ui_path;
  GtkBuilder *builder;
  GtkWidget *stats_grid;
  GtkWidget *time_label;
  GtkWidget *headerbar;
  GtkWidget *primary_menu_button;

  app = g_new0 (AtomixApp, 1);
  app->level = NULL;
  app->app_instance = app_instance;

  builder = gtk_builder_new ();

  ui_path = g_build_filename (PKGDATADIR, "ui", "interface.ui", NULL);
  gtk_builder_add_from_file (builder, ui_path, NULL);
  g_free (ui_path);

  ui_path = g_build_filename (PKGDATADIR, "ui", "menu.ui", NULL);
  gtk_builder_add_from_file (builder, ui_path, NULL);
  g_free (ui_path);

  app->mainwin = GTK_WIDGET (gtk_builder_get_object (builder, "mainwin"));

  app->current_category = games_scores_category_new ("atomix", "Best scores");
  app->high_scores = games_scores_context_new("atomix", "Best scores",
                                                GTK_WINDOW (app->mainwin),
                                                get_scores_category,
                                                NULL, GAMES_SCORES_STYLE_POINTS_GREATER_IS_BETTER);

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

  app->lb_level = GTK_WIDGET (gtk_builder_get_object (builder, "level_value"));
  app->lb_name = GTK_WIDGET (gtk_builder_get_object (builder, "molecule_value"));
  app->lb_formula = GTK_WIDGET (gtk_builder_get_object (builder, "formula_value"));
  app->lb_score = GTK_WIDGET (gtk_builder_get_object (builder, "score_value"));

  gtk_window_present (GTK_WINDOW (app->mainwin));

  headerbar = GTK_WIDGET (gtk_builder_get_object(builder, "headerbar"));
  gtk_application_add_window (GTK_APPLICATION (app->app_instance), GTK_WINDOW (app->mainwin));

  const char *new_game_accels[] = {"<Primary>N", NULL};
  gtk_application_set_accels_for_action (GTK_APPLICATION (app->app_instance), "win.GameNew", new_game_accels);
  const char *pause_game_accels[] = {"<Primary>P", NULL};
  gtk_application_set_accels_for_action (GTK_APPLICATION (app->app_instance), "win.GamePause", pause_game_accels);
  const char *skip_level_accels[] = {"<Primary>S", NULL};
  gtk_application_set_accels_for_action (GTK_APPLICATION (app->app_instance), "win.LevelSkip", skip_level_accels);
  const char *undo_move_accels[] = {"<Primary>Z", NULL};
  gtk_application_set_accels_for_action (GTK_APPLICATION (app->app_instance), "win.GameUndo", undo_move_accels);

  gtk_window_set_titlebar (GTK_WINDOW (app->mainwin), headerbar);
  GMenu * menu = G_MENU (gtk_builder_get_object (builder, "primary-menu"));

  primary_menu_button = GTK_WIDGET (gtk_builder_get_object (builder, "primary-menu-button"));
  gtk_menu_button_set_menu_model (GTK_MENU_BUTTON (primary_menu_button), G_MENU_MODEL (menu));

  g_object_unref (builder);

  return app;
}

static void
app_activate (GApplication *app_instance, gpointer user_data)
{

  if (app == NULL) {
    /* make a few initalisations here */
    g_action_map_add_action_entries (G_ACTION_MAP (app_instance), app_entries, G_N_ELEMENTS (app_entries), app_instance);

    app = create_gui (app_instance);

    g_action_map_add_action_entries (G_ACTION_MAP (app->mainwin), win_entries, G_N_ELEMENTS (win_entries), app_instance);

    game_init ();
    gtk_widget_set_size_request (GTK_WIDGET (app->mainwin), 678, 520);

    //gtk_window_set_resizable (GTK_WINDOW (app->mainwin), FALSE);
    gtk_widget_set_visible (app->mainwin, TRUE);
  } else {
    gtk_window_present (GTK_WINDOW (app->mainwin));
  }
}

int main (int argc, char *argv[])
{
  GtkApplication *gtk_app;

  int status;

  g_set_application_name (_("Atomix"));
  g_set_prgname (APP_ID);
  gtk_window_set_default_icon_name (APP_ID);

  bindtextdomain (GETTEXT_PACKAGE, GNOMELOCALEDIR);
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
  textdomain (GETTEXT_PACKAGE);

  gtk_app = gtk_application_new (APP_ID, G_APPLICATION_DEFAULT_FLAGS);
  g_signal_connect (gtk_app, "activate", G_CALLBACK (app_activate), NULL);

  status = g_application_run (G_APPLICATION (gtk_app), argc, argv);
  g_object_unref (gtk_app);

  return status;
}

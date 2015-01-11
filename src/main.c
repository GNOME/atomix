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

#include "board.h"
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
static void verb_GameNew_cb (GtkAction * action, gpointer data)
{
  controller_handle_action (GAME_ACTION_NEW);
}

static void verb_GameEnd_cb (GtkAction * action, gpointer data)
{
  controller_handle_action (GAME_ACTION_END);
}

static void verb_GameSkip_cb (GtkAction * action, gpointer data)
{
  controller_handle_action (GAME_ACTION_SKIP);
}

static void verb_GameReset_cb (GtkAction * action, gpointer data)
{
  controller_handle_action (GAME_ACTION_RESTART);
}

static void verb_GamePause_cb (GtkAction * action, gpointer data)
{
  controller_handle_action (GAME_ACTION_PAUSE);
}

static void verb_GameContinue_cb (GtkAction * action, gpointer data)
{
  controller_handle_action (GAME_ACTION_CONTINUE);
}

static void verb_GameUndo_cb (GtkAction * action, gpointer data)
{
  controller_handle_action (GAME_ACTION_UNDO);
}

static void verb_GameExit_cb (GtkAction * action, gpointer data)
{
  atomix_exit ();
}

static void verb_EditPreferences_cb (GtkAction * action, gpointer data)
{
#if 0
  preferences_show_dialog ();
#endif
}

static void verb_HelpAbout_cb (GtkAction * action, gpointer data)
{
  GtkWidget *dlg;

  const char *authors[] =
    {
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
  gtk_about_dialog_set_website (GTK_ABOUT_DIALOG(dlg), "http://www.gnome.org/projects/atomix");
  gtk_about_dialog_set_authors (GTK_ABOUT_DIALOG(dlg), authors);
  gtk_about_dialog_set_artists (GTK_ABOUT_DIALOG(dlg), artists);
  gtk_about_dialog_set_translator_credits (GTK_ABOUT_DIALOG(dlg), _("translator-credits"));

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
	  board_hide ();
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

	  board_undo_move ();
	  break;

	default:
	  break;
	}
      break;

    case GAME_STATE_PAUSED:
      if (action == GAME_ACTION_CONTINUE)
	{
	  clock_start (CLOCK(app->clock));
	  board_show ();
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
  board_show_logo (TRUE);

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
  board_show_logo (FALSE);
  board_init_level (env_pf, sce_pf, app->goal);

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
  board_clear ();
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
  gtk_widget_destroy (app->mainwin);

  gtk_main_quit ();
}

static gboolean on_key_press_event (GObject *widget, GdkEventKey *event,
				    gpointer user_data)
{
  if ((app->state == GAME_STATE_RUNNING) || (app->state == GAME_STATE_RUNNING_UNMOVED))
    {
      board_handle_key_event (NULL, event, NULL);
    }

  return TRUE;
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
  board_init (app->theme, GNOME_CANVAS (app->ca_matrix));

  /* init goal */
  goal_view_init (app->theme, GTK_FIXED (app->fi_goal));

  /* update user visible information */
  app->state = GAME_STATE_NOT_RUNNING;
  update_menu_item_state ();
  update_statistics ();

  gtk_widget_grab_focus (GTK_WIDGET (app->ca_matrix));
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

#if 0
dlg = gtk_message_dialog_new (GTK_WINDOW (app->mainwin),
			      GTK_DIALOG_MODAL,
			      GTK_MESSAGE_ERROR,
			      GTK_BUTTONS_OK,
			      "%s", _("Couldn't find at least one level."));
gtk_dialog_run (GTK_DIALOG (dlg));
gtk_widget_destroy (GTK_WIDGET (dlg));


_("Do you want to finish the game?"));
#endif

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
  gchar *path;
  gint i;
  const CmdEnable *cmd_list = state_sensitivity[app->state];
  GtkWidget *widget;

  for (i = 0; cmd_list[i].cmd != NULL; i++)
    {
      path = g_strconcat ("/MainMenu/GameMenu/", cmd_list[i].cmd, NULL);
      widget = gtk_ui_manager_get_widget (app->ui_manager, path);
      gtk_widget_set_sensitive (widget, cmd_list[i].enabled);
      g_free (path);
    }
}

/* ===============================================================
      
             GUI creation  functions 

-------------------------------------------------------------- */
static GtkWidget *create_canvas_widget (GtkWidget **canvas)
{
  GtkWidget *frame;

  *canvas = gnome_canvas_new_aa ();

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_container_add (GTK_CONTAINER (frame), GTK_WIDGET (*canvas));

  return frame;
}

static GtkWidget *create_goal_widget (GtkWidget **fixed)
{
  GtkWidget *frame;

  *fixed = gtk_fixed_new ();

  frame = gtk_frame_new (NULL);
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_IN);
  gtk_container_add (GTK_CONTAINER (frame), GTK_WIDGET (*fixed));

  return frame;
}

static void add_statistics_table_entry (GtkWidget *table, gint row,
					gchar *label_str, gboolean is_clock,
					GtkWidget **return_widget)
{
  GtkWidget *label;
  GtkWidget *lb_align;
  GtkWidget *align;

  label = gtk_label_new (label_str);
  lb_align = gtk_alignment_new (1.0, 0.5, 0.0, 0.0);
  gtk_container_add (GTK_CONTAINER (lb_align), GTK_WIDGET (label));
  gtk_table_attach (GTK_TABLE (table), GTK_WIDGET (lb_align),
		    0, 1, row, row + 1, GTK_FILL, GTK_FILL, 0, 0);

  if (is_clock)
    *return_widget = clock_new ();
  else
    *return_widget = gtk_label_new ("NO CONTENT");

  align = gtk_alignment_new (0.0, 0.5, 0.0, 0.0);
  gtk_container_add (GTK_CONTAINER (align), GTK_WIDGET (*return_widget));
  gtk_table_attach (GTK_TABLE (table), GTK_WIDGET (align),
		    1, 2, row, row + 1, GTK_FILL, GTK_FILL, 6, 0);
}

static GtkWidget *create_mainwin_content (AtomixApp *app)
{
  GtkWidget *hbox;
  GtkWidget *vbox;
  GtkWidget *pf;
  GtkWidget *goal;
  GtkWidget *frame;
  GtkWidget *table;

  /* create canvas widgets */
  pf = create_canvas_widget (&app->ca_matrix);
  goal = create_goal_widget (&app->fi_goal);
  gtk_widget_set_size_request (GTK_WIDGET (goal), 180, 50);

  /* add playfield canvas to left side */
  hbox = gtk_hbox_new (FALSE, 6);
  gtk_container_set_border_width (GTK_CONTAINER (hbox), 6);
  gtk_box_pack_start (GTK_BOX (hbox), GTK_WIDGET (pf), TRUE, TRUE, 0);
  g_signal_connect (G_OBJECT (app->ca_matrix), "key_press_event",
		    G_CALLBACK (on_key_press_event), app);

  /* create right window side */
  vbox = gtk_vbox_new (FALSE, 6);
  gtk_box_pack_start (GTK_BOX (hbox), GTK_WIDGET (vbox), FALSE, TRUE, 0);

  /* create statistics frame */
  frame = gtk_frame_new (_("Statistics"));
  table = gtk_table_new (4, 2, FALSE);
  gtk_table_set_row_spacings (GTK_TABLE (table), 6);
  gtk_table_set_col_spacings (GTK_TABLE (table), 6);
  gtk_container_set_border_width (GTK_CONTAINER (table), 6);

  add_statistics_table_entry (table, 0, _("Level:"), FALSE, &app->lb_level);
  add_statistics_table_entry (table, 1, _("Molecule:"), FALSE, &app->lb_name);
  add_statistics_table_entry (table, 2, _("Formula:"), FALSE, &app->lb_formula);
  add_statistics_table_entry (table, 3, _("Score:"), FALSE, &app->lb_score);
  add_statistics_table_entry (table, 4, _("Time:"), TRUE, &app->clock);

  gtk_container_add (GTK_CONTAINER (frame), GTK_WIDGET (table));

  /* add frame and goal canvas to left side */
  gtk_box_pack_start (GTK_BOX (vbox), GTK_WIDGET (frame), FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), GTK_WIDGET (goal), TRUE, TRUE, 0);

  /* show all */
  gtk_widget_show_all (GTK_WIDGET (hbox));
  return hbox;
}

static AtomixApp *create_gui (void)
{
  AtomixApp *app;
  GtkAccelGroup *accel_group;
  GtkActionGroup *action_group;
  GtkWidget *vbox;
  GtkWidget *menubar;
  GtkWidget *content;

  static const GtkActionEntry actions[] = {
    {"GameMenu", NULL, N_("_Game")},
    {"HelpMenu", NULL, N_("_Help")},
    {"GameNew", NULL, N_("New Game"), NULL, NULL, G_CALLBACK (verb_GameNew_cb)},
    {"GameEnd", NULL, N_("End Game"), NULL, NULL, G_CALLBACK (verb_GameEnd_cb)},
    {"GameSkip", NULL, N_("Skip Level"), NULL, NULL, G_CALLBACK (verb_GameSkip_cb)},
    {"GameReset", NULL, N_("Reset Level"), NULL, NULL, G_CALLBACK (verb_GameReset_cb)},
    {"GameUndo", "gtk-undo", NULL, NULL, NULL, G_CALLBACK (verb_GameUndo_cb)},
    {"GamePause", NULL, N_("_Pause Game"), NULL, NULL, G_CALLBACK (verb_GamePause_cb)},
    {"GameContinue", NULL, N_("_Continue Game"), NULL, NULL, G_CALLBACK (verb_GameContinue_cb)},
    {"GameExit", "gtk-quit", NULL, NULL, NULL, G_CALLBACK (verb_GameExit_cb)},
    {"HelpAbout", NULL, N_("About"), NULL, NULL, G_CALLBACK (verb_HelpAbout_cb)}
  };

  const char ui_description[] =
    "<ui>"
    "  <menubar name='MainMenu'>"
    "    <menu action='GameMenu'>"
    "      <menuitem action='GameNew'/>"
    "      <menuitem action='GameEnd'/>"
    "      <separator/>"
    "      <menuitem action='GameSkip'/>"
    "      <menuitem action='GameReset'/>"
    "      <menuitem action='GameUndo'/>"
    "      <menuitem action='GamePause'/>"
    "      <menuitem action='GameContinue'/>"
    "      <separator/>"
    "      <menuitem action='GameExit'/>"
    "    </menu>"
    "    <menu action='HelpMenu'>"
    "      <menuitem action='HelpAbout'/>"
    "    </menu>"
    "  </menubar>"
    "</ui>";

  app = g_new0 (AtomixApp, 1);
  app->level = NULL;

  app->mainwin = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title (GTK_WINDOW (app->mainwin), _("Atomix"));

  g_signal_connect (G_OBJECT (app->mainwin), "delete_event",
		    (GCallback) on_app_destroy_event, app);

  app->ui_manager = gtk_ui_manager_new ();

  action_group = gtk_action_group_new ("MenuActions");
  gtk_action_group_set_translation_domain (action_group, GETTEXT_PACKAGE);
  gtk_action_group_add_actions (action_group, actions, G_N_ELEMENTS (actions), NULL);

  gtk_ui_manager_insert_action_group (app->ui_manager, action_group, 0);
  gtk_ui_manager_add_ui_from_string (app->ui_manager, ui_description, -1, NULL);
  accel_group = gtk_ui_manager_get_accel_group (app->ui_manager);
  gtk_window_add_accel_group (GTK_WINDOW (app->mainwin), accel_group);

  /* create window contents */
  menubar = gtk_ui_manager_get_widget (app->ui_manager, "/MainMenu");
  content = create_mainwin_content (app);

  gtk_window_set_default_icon_from_file (g_build_filename (DATADIR,
							   "pixmaps",
							   "atomix-icon.png",
							   NULL),
					 		   NULL);

  vbox = gtk_vbox_new (FALSE, 0);
  gtk_container_add (GTK_CONTAINER (app->mainwin), vbox);
  gtk_box_pack_start (GTK_BOX (vbox), menubar, FALSE, FALSE, 0);
  gtk_box_pack_start (GTK_BOX (vbox), content, TRUE, TRUE, 0);

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

  gtk_widget_set_size_request (GTK_WIDGET (app->mainwin), 660, 480);
  gtk_widget_show (app->mainwin);

  gtk_main ();
  return 0;
}

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

#include <gnome.h>
#include <glade/glade.h>

#include "callbacks.h"
#include "board.h"
#include "preferences.h"
#include "theme.h"
#include "level.h"
#include "main.h"
/* #include "gtk-time-limit.h" */ 

static void
atomix_exit (AtomixApp *app)
{
	g_message ("Destroy application");
	board_destroy();
	
	if(get_actual_level())
	{
		level_destroy(get_actual_level());
	}
	if(get_actual_theme())
	{
		theme_destroy(get_actual_theme());
	}
	preferences_destroy();
	level_destroy_hash_table();
	theme_destroy_hash_table();

	/* quit application */
	bonobo_object_unref (BONOBO_OBJECT (app->ui_component));
	gtk_widget_destroy (app->mainwin);

	gtk_main_quit();

}

void
verb_GameNew_cb (BonoboUIComponent *uic, gpointer user_data, const char *cname)
{
	game_new();
}

void
verb_GamePause_cb (BonoboUIComponent *uic, gpointer user_data, const char *cname)
{
	game_pause();

}

void
verb_GameContinue_cb (BonoboUIComponent *uic, gpointer user_data, const char *cname)
{
	game_continue();
}

void
verb_GameUndo_cb (BonoboUIComponent *uic, gpointer user_data, const char *cname)
{
	game_undo_move();
}


void
verb_GameScores_cb (BonoboUIComponent *uic, gpointer user_data, const char *cname)
{
	gnome_scores_display("Atomix", "atomix", NULL, 0);	
}


void
verb_GameExit_cb (BonoboUIComponent *uic, gpointer user_data, const char *cname)
{
	AtomixApp *app = (AtomixApp*) user_data;
	atomix_exit (app);
}


void
verb_EditPreferences_cb (BonoboUIComponent *uic, gpointer user_data, const char *cname)
{
	preferences_show_dialog();
}


void
verb_HelpAbout_cb (BonoboUIComponent *uic, gpointer user_data, const char *cname)
{
	show_about_dlg ();
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


/* This function ist used to remove all canvas items 
   (look at board_clear())*/
void
destroy_item(gpointer ptr, gpointer data)
{
	GtkObject* obj;

	obj = GTK_OBJECT (ptr);
	gtk_object_destroy(obj);
}


/* just to create my selfmade widget gtk_time_limit */
GtkWidget*
create_time_limit_widget (gchar *widget_name, gchar *string1, gchar *string2,
			  gint int1, gint int2)
{
	GtkWidget *alignment;
	GtkWidget *time_limit;
	
	alignment = gtk_alignment_new(0.0, 0.5, 0.0, 0.0);
	time_limit = /* gtk_time_limit_new(); */
		gtk_label_new ("NO TIME LIMIT");
	gtk_widget_show(time_limit);
	gtk_container_add(GTK_CONTAINER(alignment), time_limit);

	return alignment;
}


void
verb_GameSkip_cb (BonoboUIComponent *uic, gpointer user_data, const char *cname)
{
	game_skip_level();
}


#if 0
void
on_leveleditor_activate                (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
	static gchar *command[] = { "atomixed" }; 
	
	gnome_execute_async(NULL, 1, command);
}
#endif


gboolean
on_key_press_event           (GtkWidget       *widget,
			      GdkEventKey     *event,
			      gpointer         user_data)
{
	if(get_game_state() == GAME_RUNNING)
	{
		board_handle_key_event(event);
	}
	
	return TRUE;
}


void
verb_GameEnd_cb (BonoboUIComponent *uic, gpointer user_data, const char *cname)
{
	game_user_quits();
}


gboolean
on_delete_event                        (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data)
{
#if 0
	on_exit_activate(widget, 0);
#endif	

	return TRUE;
}

#if 0
/*
 * FIXME: Use new help API here.
 */
void
on_manual_activate                     (GtkMenuItem     *menuitem,
                                        gpointer         user_data)
{
	gchar *helpfile;

	helpfile = gnome_help_file_find_file("atomix", "atomix.html");
	
	if(helpfile != NULL)
	{
		gchar *url;
		
		url = g_strconcat("ghelp:", helpfile, NULL);
		
		gnome_help_goto(NULL, url);
		
		g_free(url);
		g_free(helpfile);
	}
	else
	{
		gnome_error_dialog(_("Couldn't find the Atomix manual!"));
	}
}


void
on_dlg_properties_help                 (GnomePropertyBox *gnomepropertybox,
                                        gint             pagenum,
                                        gpointer         user_data)
{
	static GnomeHelpMenuEntry help_entry = { "atomix", "preferences.html" };

	gnome_help_pbox_goto(NULL, 0, &help_entry);
}
#endif


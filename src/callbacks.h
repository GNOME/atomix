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
#include <gnome.h>
#include <bonobo.h>

void verb_GameNew_cb  (BonoboUIComponent *uic, gpointer user_data, const char *cname);
void verb_GameEnd_cb  (BonoboUIComponent *uic, gpointer user_data, const char *cname);

void verb_GameSkip_cb  (BonoboUIComponent *uic, gpointer user_data, const char *cname);
void verb_GameUndo_cb  (BonoboUIComponent *uic, gpointer user_data, const char *cname);
void verb_GamePause_cb  (BonoboUIComponent *uic, gpointer user_data, const char *cname);
void verb_GameContinue_cb  (BonoboUIComponent *uic, gpointer user_data, const char *cname);
void verb_GameScores_cb  (BonoboUIComponent *uic, gpointer user_data, const char *cname);

void verb_GameExit_cb  (BonoboUIComponent *uic, gpointer user_data, const char *cname);

void verb_EditPreferences_cb  (BonoboUIComponent *uic, gpointer user_data, const char *cname);

void verb_HelpAbout_cb  (BonoboUIComponent *uic, gpointer user_data, const char *cname);

gboolean on_app_destroy_event (GtkWidget       *widget,
			       GdkEvent        *event,
			       gpointer         user_data);

void free_imlib_image (GtkObject *object, gpointer data);

void destroy_item(gpointer ptr, gpointer data);


GtkWidget* gtk_stop_clock_new (gchar *widget_name, gchar *string1, gchar *string2,
			       gint int1, gint int2);

GtkWidget* create_time_limit_widget (gchar *widget_name, gchar *string1, 
				     gchar *string2,
				     gint int1, gint int2);

gboolean
on_key_press_event                     (GtkWidget       *widget,
                                        GdkEventKey     *event,
                                        gpointer         user_data);

gboolean
on_delete_event                        (GtkWidget       *widget,
                                        GdkEvent        *event,
                                        gpointer         user_data);


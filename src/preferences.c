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

#include "preferences.h"
#include "board.h"
#include "main.h"

Preferences *preferences;

#define GLADE_FILE "/home/jens/projekte/atomix/atomix.glade"
GladeXML *pref_xml;

void handle_apply (GtkWidget *propertybox, gint page_num, gpointer data);
void handle_mouse_ctrl_toggled (GtkToggleButton *togglebutton,
				gpointer data);
void handle_widget_changed (GtkWidget *widget, gpointer data);
void handle_global_checkbox_toggled (GtkToggleButton *togglebutton,
				     gpointer data);

void preferences_init (void)
{
  preferences = g_malloc (sizeof (Preferences));
  preferences->mouse_control =
    gnome_config_get_bool ("/atomix/Control/MouseControl=true");
  preferences->keyboard_control =
    gnome_config_get_bool ("/atomix/Control/KeyboardControl=false");
  preferences->hide_cursor =
    gnome_config_get_bool ("/atomix/Control/HideCursor=true");
  preferences->lazy_dragging =
    gnome_config_get_bool ("/atomix/Control/LazyDragging=true");
  preferences->mouse_sensitivity =
    gnome_config_get_int ("/atomix/Control/MouseSensitivity=8");
  preferences->score_time_enabled =
    gnome_config_get_bool ("/atomix/Global/EnableScoreAndTimeLimit=true");
}

void preferences_destroy (void)
{
  if (preferences)
    {
      g_free (preferences);
    }
}

void preferences_save (void)
{
  if (preferences)
    {
      gnome_config_set_bool ("/atomix/Global/EnableScoreAndTimeLimit",
			     preferences->score_time_enabled);
      gnome_config_set_bool ("/atomix/Control/MouseControl",
			     preferences->mouse_control);
      gnome_config_set_bool ("/atomix/Control/KeyboardControl",
			     preferences->keyboard_control);
      gnome_config_set_bool ("/atomix/Control/HideCursor",
			     preferences->hide_cursor);
      gnome_config_set_bool ("/atomix/Control/LazyDragging",
			     preferences->lazy_dragging);
      gnome_config_set_int ("/atomix/Control/MouseSensitivitiy",
			    preferences->mouse_sensitivity);
      gnome_config_sync ();
    }
}

Preferences *preferences_get (void)
{
  return preferences;
}

void preferences_show_dialog (void)
{
  GtkWidget *dlg;
  GtkWidget *radio_keyboard;
  GtkWidget *radio_mouse;
  GtkWidget *check_hide_cursor;
  GtkWidget *check_lazy_dragging;
  GtkWidget *check_score_time;
  GtkWidget *scroll_sensitivity;
  GtkAdjustment *adjustment;

  pref_xml = glade_xml_new (GLADE_FILE, "dlg_properties", NULL);
  dlg = glade_xml_get_widget (pref_xml, "dlg_properties");
  gtk_signal_connect (GTK_OBJECT (dlg), "apply",
		      GTK_SIGNAL_FUNC (handle_apply), NULL);

  /* get widget */
  radio_keyboard = glade_xml_get_widget (pref_xml, "radio_keyboard");
  radio_mouse = glade_xml_get_widget (pref_xml, "radio_mouse");
  check_hide_cursor = glade_xml_get_widget (pref_xml, "check_hide_cursor");
  check_lazy_dragging =
    glade_xml_get_widget (pref_xml, "check_lazy_dragging");
  check_score_time = glade_xml_get_widget (pref_xml, "check_score_time");
  scroll_sensitivity = glade_xml_get_widget (pref_xml, "scroll_sensitivity");

  /* set widget state */
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (radio_mouse),
				preferences->mouse_control);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (radio_keyboard),
				preferences->keyboard_control);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check_hide_cursor),
				preferences->hide_cursor);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check_lazy_dragging),
				preferences->lazy_dragging);
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (check_score_time),
				preferences->score_time_enabled);
  adjustment = gtk_range_get_adjustment (GTK_RANGE (scroll_sensitivity));
  gtk_adjustment_set_value (GTK_ADJUSTMENT (adjustment),
			    (preferences->mouse_sensitivity / 2));

  /* enable according to game state */
  if (get_game_state () != GAME_NOT_RUNNING)
    {
      GtkWidget *frame;
      frame = glade_xml_get_widget (pref_xml, "frame_constraints");
      gtk_widget_set_sensitive (GTK_WIDGET (frame), FALSE);
      gtk_widget_set_sensitive (GTK_WIDGET (check_score_time), FALSE);
    }

  /* if keyboard control selected, disable mouse properties */
  if (preferences->keyboard_control)
    {
      GtkWidget *frame = glade_xml_get_widget (pref_xml, "frame_mouse_props");
      gtk_widget_set_sensitive (frame, FALSE);
    }

  /* connect widgets with handlers */
  gtk_signal_connect (GTK_OBJECT (radio_mouse), "toggled",
		      GTK_SIGNAL_FUNC (handle_mouse_ctrl_toggled), dlg);
  gtk_signal_connect (GTK_OBJECT (radio_keyboard), "toggled",
		      GTK_SIGNAL_FUNC (handle_widget_changed), dlg);
  gtk_signal_connect (GTK_OBJECT (check_hide_cursor), "toggled",
		      GTK_SIGNAL_FUNC (handle_widget_changed), dlg);
  gtk_signal_connect (GTK_OBJECT (check_lazy_dragging), "toggled",
		      GTK_SIGNAL_FUNC (handle_widget_changed), dlg);
  gtk_signal_connect (GTK_OBJECT (check_score_time), "toggled",
		      GTK_SIGNAL_FUNC (handle_global_checkbox_toggled), dlg);
  gtk_signal_connect (GTK_OBJECT (adjustment), "value-changed",
		      GTK_SIGNAL_FUNC (handle_widget_changed), dlg);

  /* show dialog */
  gtk_widget_show (dlg);
}

void handle_apply (GtkWidget *propertybox, gint page_num, gpointer data)
{
  GtkWidget *radio_keyboard;
  GtkWidget *radio_mouse;
  GtkWidget *check_hide_cursor;
  GtkWidget *check_lazy_dragging;
  GtkWidget *check_score_time;
  GtkWidget *scroll_sensitivity;
  GtkAdjustment *adjustment;

  if (page_num == 0 /* general */ )
    {
      /* get widgets */
      check_score_time = glade_xml_get_widget (pref_xml, "check_score_time");

      /* set new values */
      preferences->score_time_enabled =
	gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (check_score_time));
    }
  else if (page_num == 1 /* control */ )
    {
      /* get widgets */
      radio_keyboard = glade_xml_get_widget (pref_xml, "radio_keyboard");
      radio_mouse = glade_xml_get_widget (pref_xml, "radio_mouse");
      check_hide_cursor =
	glade_xml_get_widget (pref_xml, "check_hide_cursor");
      check_lazy_dragging =
	glade_xml_get_widget (pref_xml, "check_lazy_dragging");
      scroll_sensitivity =
	glade_xml_get_widget (pref_xml, "scroll_sensitivity");
      adjustment = gtk_range_get_adjustment (GTK_RANGE (scroll_sensitivity));

      /* switch keyboard/mouse control if neccessary */
      if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (radio_mouse)))
	{
	  if (!preferences->mouse_control)
	    {
	      board_set_mouse_control ();
	    }
	}
      else
	{
	  if (!preferences->keyboard_control)
	    {
	      board_set_keyboard_control ();
	    }
	}

      /* set new values */
      preferences->mouse_control =
	gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (radio_mouse));
      preferences->keyboard_control =
	gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (radio_keyboard));
      preferences->hide_cursor =
	gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (check_hide_cursor));
      preferences->lazy_dragging =
	gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON
				      (check_lazy_dragging));
      preferences->mouse_sensitivity = (gint) (adjustment->value * 2);
    }
  else if (page_num == -1 /* all */ )
    {
      /* save preferences */
      preferences_save ();
    }
}

void handle_mouse_ctrl_toggled (GtkToggleButton *togglebutton, gpointer data)
{
  GtkWidget *frame;

  /* get widgets */
  frame = glade_xml_get_widget (pref_xml, "frame_mouse_props");

  /* set status of property box */
  gnome_property_box_changed (GNOME_PROPERTY_BOX (data));

  /* enable/disable mouse control properties */
  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (togglebutton)))
    {
      gtk_widget_set_sensitive (frame, TRUE);
    }
  else
    {
      gtk_widget_set_sensitive (frame, FALSE);
    }
}

void handle_widget_changed (GtkWidget *widget, gpointer data)
{
  gnome_property_box_changed (GNOME_PROPERTY_BOX (data));
}

void handle_global_checkbox_toggled (GtkToggleButton *togglebutton,
				     gpointer data)
{
  if (get_game_state () != GAME_NOT_RUNNING)
    {
      GtkWidget *frame;

      frame = glade_xml_get_widget (pref_xml, "frame_mouse_props");

      gtk_widget_set_sensitive (GTK_WIDGET (frame), FALSE);

      gtk_signal_disconnect_by_func (GTK_OBJECT (togglebutton),
				     GTK_SIGNAL_FUNC
				     (handle_global_checkbox_toggled), data);
      gtk_toggle_button_set_active (togglebutton,
				    !gtk_toggle_button_get_active
				    (togglebutton));
    }
  else
    {
      gnome_property_box_changed (GNOME_PROPERTY_BOX (data));
    }
}

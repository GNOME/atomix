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

#ifndef _ATOMIX_MAIN_H_
#define _ATOMIX_MAIN_H_

#include <gnome.h>
#include <bonobo.h>
#include <glade/glade.h>
#include "level.h"
#include "theme.h"

typedef struct _NextLevelDlgInfo NextLevelDlgInfo;

struct _NextLevelDlgInfo
{
	gint timer_id;
	GtkWidget* lb_secs;
	GtkWidget* lb_score;
	gint secs;
	gdouble score;
};

enum {
	GAME_NOT_RUNNING,
	GAME_RUNNING,
	GAME_PAUSED
};


struct _LevelData
{
	gint    no_level;  /* number of level */
	Level   *level;    /* actual level */
	gdouble score;     /* players score */
	Theme   *theme;    /* actual used theme */
};
typedef struct _LevelData LevelData;

typedef struct 
{
	GnomeProgram      *prog;
	GtkWidget         *mainwin;
	BonoboUIContainer *ui_container;
	BonoboUIComponent *ui_component;
	LevelData         *level;
	GtkWidget         *ca_matrix;
	GtkWidget         *ca_goal;
	GtkWidget         *lb_level;
	GtkWidget         *lb_name;
	GtkWidget         *lb_score;
} AtomixApp;


GladeXML*     get_gui(void);
Level*        get_actual_level(void);
Theme*        get_actual_theme(void);
gint          get_game_state(void);

void game_new(void);

void game_reload_level(void);
void game_level_timeout(GtkWidget *widget, gpointer data);
void game_level_finished(void);
void game_finished(void);

void game_pause(void);
void game_continue(void);
void game_undo_move(void);

void game_user_quits(void);

void game_skip_level(void);

void show_about_dlg (void);

#endif /* _ATOMIX_MAIN_H_ */

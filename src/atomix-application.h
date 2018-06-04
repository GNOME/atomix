/* Atomix -- a little puzzle game about atoms and molecules.
 * Copyright (C) 1999 Jens Finke
 * Copyright (C) 2005 Guilherme de S. Pastore
 * Copyright (C) 2015 Robert Roth
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

#ifndef _ATOMIX_APPLICATION_H_
#define _ATOMIX_APPLICATION_H_

#include <gtk/gtk.h>
#include <libgnome-games-support.h>
#include "theme-manager.h"
#include "level-manager.h"
#include "goal.h"
#include "atomix-application.h"

typedef enum
{
  GAME_STATE_NOT_RUNNING,
  GAME_STATE_RUNNING_UNMOVED,
  GAME_STATE_RUNNING,
  GAME_STATE_PAUSED
} GameState;

typedef struct
{
  GtkWidget *mainwin;
  GApplication *app_instance;

  GtkWidget *fi_matrix;
  GtkWidget *fi_goal;
  GtkWidget *lb_level;
  GtkWidget *lb_name;
  GtkWidget *lb_formula;
  GtkWidget *lb_score;
  GtkWidget *clock;

  LevelManager *lm;
  ThemeManager *tm;
  Theme *theme;

  GameState state;
  Level *level;
  Goal *goal;
  gint level_no;
  guint score;

  GamesScoresContext *high_scores;
  GamesScoresCategory *current_category;
} AtomixApp;

#endif /* _ATOMIX_APPLICATION_H_ */

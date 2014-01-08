/* Atomix -- a little puzzle game about atoms and molecules.
 * Copyright (C) 2001 Jens Finke
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

#ifndef _ATOMIX_LEVEL_PRIVATE_H_
#define _ATOMIX_LEVEL_PRIVATE_H_

#include "playfield.h"

struct _LevelPrivate
{
  gchar *name;			/* name of the level */
  gchar *formula;		/* formula of the compound */
  PlayField *environment;
  PlayField *scenario;		/* starting situation */
  PlayField *goal;		/* determines the end of the level */

  /* the following fields are only used by atomixed */
  gchar *file_name;		/* file name of the level */
  gboolean modified;		/* whether the level is modified */
};

Level *level_new (void);


#endif /* _ATOMIX_LEVEL_PRIVATE_H_ */

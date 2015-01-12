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
#include "main.h"
#include "math.h"
#include "canvas_helper.h"

void convert_to_playfield (Theme *theme, PlayField * playfield, gint x, gint y,
			   guint *row, guint *col)
{
  guint int_y, int_x;
  gint tile_width, tile_height;
  gint row_offset, col_offset;

  row_offset = BGR_FLOOR_ROWS / 2 - playfield_get_n_rows (playfield) / 2;
  col_offset = BGR_FLOOR_COLS / 2 - playfield_get_n_cols (playfield) / 2;
  theme_get_tile_size (theme, &tile_width, &tile_height);

  int_y = (guint) ceil (y);
  *row = (int_y / tile_height) - row_offset;

  int_x = (guint) ceil (x);
  *col = (int_x / tile_width) - col_offset;

}

void convert_to_canvas (Theme *theme, PlayField * playfield,guint row, guint col,
			gint *x, gint *y)
{
  gint tile_width, tile_height;
  gint row_offset, col_offset;

  row_offset = BGR_FLOOR_ROWS / 2 - playfield_get_n_rows (playfield) / 2;
  col_offset = BGR_FLOOR_COLS / 2 - playfield_get_n_cols (playfield) / 2;
  theme_get_tile_size (theme, &tile_width, &tile_height);

  *x = (col + col_offset) * tile_width;
  *y = (row + row_offset) * tile_height;
}

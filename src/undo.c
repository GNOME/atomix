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
#include "undo.h"

extern AtomixApp *app;
static GSList *undo_stack = NULL;

gboolean undo_exists (void)
{
  return ((undo_stack == NULL) ? FALSE : TRUE);
}

static void delete_move (UndoMove *move, gpointer data)
{
  g_free (move);
}

void undo_clear (void)
{
  if (undo_stack == NULL)
    return;

  g_slist_foreach (undo_stack, (GFunc) delete_move, NULL);
  g_slist_free (undo_stack);
  undo_stack = NULL;
}

void undo_push_move (gpointer item, gint src_row, gint src_col,
		     gint dest_row, gint dest_col)
{
  UndoMove *move = g_new0 (UndoMove, 1);

  move->item = item;
  move->src_row = src_row;
  move->src_col = src_col;
  move->dest_row = dest_row;
  move->dest_col = dest_col;

  undo_stack = g_slist_prepend (undo_stack, move);
}

UndoMove *undo_pop_move (void)
{
  UndoMove *move;

  if (undo_stack == NULL)
    return NULL;

  move = (UndoMove *) undo_stack->data;
  undo_stack = g_slist_delete_link (undo_stack, undo_stack);

  if (undo_stack == NULL)
    {
      app->state = GAME_STATE_RUNNING_UNMOVED;
      update_menu_item_state ();
    }

  return move;
}

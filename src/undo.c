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
#include "undo.h"

#define MAX_UNDO_STEPS 10

static UndoMove *move_array[MAX_UNDO_STEPS];
static gint head = -1;

void
undo_init()
{
	gint i;

	for(i = 0; i < MAX_UNDO_STEPS; i++)
	{
		move_array[i] = NULL;
	}
    
	head = -1;
}

void
undo_free_all_moves(void)
{
	gint i;

	for(i = 0; i < MAX_UNDO_STEPS; i++)
	{
		if(move_array[i])
		{
			g_free(move_array[i]);
			move_array[i] = NULL;
		}
	}
    
	head = -1;    
}


UndoMove*
undo_create_move(GnomeCanvasItem *item, gint src_row, gint src_col,
		 gint dest_row, gint dest_col)
{
	UndoMove *move = g_malloc(sizeof(UndoMove));

	move->item = item;
	move->src_row = src_row;
	move->src_col = src_col;
	move->dest_row = dest_row;
	move->dest_col = dest_col;

	return move;
}

void
undo_add_move(UndoMove *move)
{
	if(move != NULL)
	{
		head = (++head) % MAX_UNDO_STEPS;
		if(move_array[head])
		{
			g_free(move_array[head]);
		}
		move_array[head] = move;
	}
}

UndoMove*
undo_get_last_move(void)
{
	if(head >= 0)
	{
		UndoMove *move = move_array[head];
		move_array[head] = NULL;
		head--;
		if(head == -1) head += MAX_UNDO_STEPS;
	
		return move;
	}
	else
	{
		return NULL;
	}

	return NULL;
}

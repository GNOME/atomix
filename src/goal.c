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
#include "theme.h"
#include "goal.h"
#include "canvas_helper.h"

static GObjectClass *parent_class = NULL;

static void goal_class_init (GObjectClass * class);
static void goal_init (Goal * goal);
static void goal_finalize (GObject * object);

struct _GoalPrivate
{
  PlayField *pf;
  GSList *index;
};

void goal_print_offset (gpointer ptr, gpointer data);
GtkImage *create_small_item (gdouble x, gdouble y, Tile *tile);
static gboolean compare_playfield_with_goal (Goal *goal, PlayField *pf,
					     guint start_row,
					     guint start_col);

typedef struct
{
  Tile *tile;
  GSList *position;
} TileData;

typedef struct
{
  gint horiz;
  gint vert;
} PositionOffset;

GType goal_get_type (void)
{
  static GType object_type = 0;

  if (!object_type)
    {
      static const GTypeInfo object_info =
	{
	  sizeof (GoalClass),
	  (GBaseInitFunc) NULL,
	  (GBaseFinalizeFunc) NULL,
	  (GClassInitFunc) goal_class_init,
	  NULL,			/* clas_finalize */
	  NULL,			/* class_data */
	  sizeof (Goal),
	  0,			/* n_preallocs */
	  (GInstanceInitFunc) goal_init,
	};

      object_type = g_type_register_static (G_TYPE_OBJECT,
					    "Goal", &object_info, 0);
    }

  return object_type;
}

static void goal_class_init (GObjectClass *class)
{
  parent_class = g_type_class_ref (G_TYPE_OBJECT);
  class->finalize = goal_finalize;
}

static void goal_init (Goal *goal)
{
  GoalPrivate *priv;

  priv = g_new0 (GoalPrivate, 1);
  priv->pf = NULL;
  priv->index = NULL;

  goal->priv = priv;
}

static void goal_finalize (GObject *object)
{
  GSList *td_it;
  GSList *pos_it;
  TileData *td;
  Goal *goal = GOAL (object);

#ifdef DEBUG
  g_message ("Finalize Goal");
#endif

  if (goal->priv->pf)
    g_object_unref (goal->priv->pf);

  for (td_it = goal->priv->index; td_it != NULL; td_it = td_it->next)
    {
      td = (TileData *) td_it->data;
      g_object_unref (td->tile);

      for (pos_it = td->position; pos_it != NULL; pos_it = pos_it->next)
	g_free (pos_it->data);

      g_slist_free (td->position);
    }
  g_slist_free (goal->priv->index);

  g_free (goal->priv);
  goal->priv = NULL;
}

static int find_tile (gconstpointer p1, gconstpointer p2)
{
  TileData *td1;
  TileData *td2;

  td1 = (TileData *) p1;
  td2 = (TileData *) p2;

  g_return_val_if_fail (IS_TILE (td1->tile), 1);
  g_return_val_if_fail (IS_TILE (td2->tile), 1);

  if (tile_is_equal (td1->tile, td2->tile))
    return 0;
  else
    return 1;
}

Goal *goal_new (PlayField *pf)
{
  Goal *goal;
  guint row, col;
  Tile *tile;

  goal = GOAL (g_object_new (GOAL_TYPE, NULL));

  goal->priv->pf = g_object_ref (pf);

  /* initialise index */
  goal->priv->index = NULL;

  for (row = 0; row < playfield_get_n_rows (pf); row++)
    {
      for (col = 0; col < playfield_get_n_cols (pf); col++)
	{
	  tile = playfield_get_tile (pf, row, col);

	  if (tile && tile_get_tile_type (tile) == TILE_TYPE_ATOM)
	    {
	      TileData pattern;
	      TileData *td;
	      PositionOffset *po;
	      GSList *result = NULL;

	      po = g_new0 (PositionOffset, 1);
	      po->horiz = col;
	      po->vert = row;

	      pattern.tile = tile;
	      pattern.position = NULL;
	      result = g_slist_find_custom (goal->priv->index,
					    &pattern,
					    (GCompareFunc) find_tile);
	      if (result == NULL)
		{
		  td = g_new0 (TileData, 1);
		  td->tile = g_object_ref (tile);
		  td->position = NULL;
		  goal->priv->index = g_slist_append (goal->priv->index, td);
		}
	      else
		{
		  td = (TileData *) result->data;
		}

	      td->position = g_slist_append (td->position, po);
	    }

	  if (tile)
	    g_object_unref (tile);
	}
    }

  return goal;
}

PlayField *goal_get_playfield (Goal *goal)
{
  g_return_val_if_fail (IS_GOAL (goal), NULL);
  g_object_ref (goal->priv->pf);

  return goal->priv->pf;
}

gboolean goal_reached (Goal *goal, PlayField *pf,
		       guint row_anchor, guint col_anchor)
{
  GSList *result;
  GSList *it;
  gboolean comp_res = FALSE;
  Tile *tile;
  TileData *td;
  TileData pattern;
  PositionOffset *po;
  guint start_row;
  guint start_col;

  g_return_val_if_fail (IS_GOAL (goal), FALSE);
  g_return_val_if_fail (goal->priv->index != NULL, FALSE);
  g_return_val_if_fail (IS_PLAYFIELD (goal->priv->pf), FALSE);
  g_return_val_if_fail (IS_PLAYFIELD (pf), FALSE);

  tile = playfield_get_tile (pf, row_anchor, col_anchor);
  if (tile == NULL)
    return FALSE;

  pattern.tile = tile;
  pattern.position = NULL;
  result = g_slist_find_custom (goal->priv->index, &pattern,
				(GCompareFunc) find_tile);

  if (result == NULL)
    return FALSE;
  td = (TileData *) result->data;

  for (it = td->position; it != NULL && !comp_res; it = it->next)
    {
      po = (PositionOffset *) it->data;
      start_row = row_anchor - po->vert;
      start_col = col_anchor - po->horiz;

      if (start_row < playfield_get_n_rows (pf) &&
	  start_col < playfield_get_n_cols (pf))
	{
	  comp_res =
	    compare_playfield_with_goal (goal, pf, start_row, start_col);
	}
    }

  return comp_res;
}

static gboolean compare_playfield_with_goal (Goal *goal, PlayField *pf,
					     guint start_row, guint start_col)
{
  Tile *pf_tile;
  Tile *goal_tile;
  gint pf_row;
  gint pf_col;
  gint goal_row;
  gint goal_col;
  gint end_row;
  gint end_col;
  gboolean result;

  end_row =
    MIN (start_row + playfield_get_n_rows (goal->priv->pf),
	 playfield_get_n_rows (pf));
  end_col =
    MIN (start_col + playfield_get_n_cols (goal->priv->pf),
	 playfield_get_n_cols (pf));
  result = TRUE;

  for (pf_row = start_row, goal_row = 0; pf_row < end_row && result;
       pf_row++, goal_row++)
    {
      for (pf_col = start_col, goal_col = 0; pf_col < end_col && result;
	   pf_col++, goal_col++)
	{
	  goal_tile = playfield_get_tile (goal->priv->pf, goal_row, goal_col);
	  pf_tile = playfield_get_tile (pf, pf_row, pf_col);

	  if (goal_tile)
	    {
	      if (tile_get_tile_type (goal_tile) == TILE_TYPE_ATOM)
		{
		  if (!pf_tile)
		    result = FALSE;
		  else if (!tile_is_equal (goal_tile, pf_tile))
		    result = FALSE;
		}
	      g_object_unref (goal_tile);
	    }

	  if (pf_tile)
	    g_object_unref (pf_tile);
	}
    }

  return result;
}

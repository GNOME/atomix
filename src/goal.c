/* Atomix -- a little mind game about atoms and molecules.
 * Copyright (C) 1999-2001 Jens Finke
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

#include <libxml/parser.h>
#include <libxml/tree.h>

#include "main.h"
#include "theme.h"
#include "goal.h"
#include "canvas_helper.h"

static GObjectClass     *parent_class = NULL;

static void goal_class_init (GObjectClass *class);
static void goal_init (Goal *goal);
static void goal_finalize (GObject *object);

struct _GoalPrivate {
	PlayField*   pf;
	GHashTable*  index;
};

void goal_print_offset(gpointer ptr, gpointer data);
GnomeCanvasItem* create_small_item (GnomeCanvasGroup *group, gdouble x, gdouble y, Tile* tile);
static void destroy_hash_value (gpointer value);
static gboolean compare_playfield_with_goal (Goal *goal, PlayField *pf, 
					     guint start_row, guint start_col);


typedef struct 
{
	gint horiz;
	gint vert;
} TileOffset;


GType
goal_get_type (void)
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
			NULL,   /* clas_finalize */
			NULL,   /* class_data */
			sizeof(Goal),
			0,      /* n_preallocs */
			(GInstanceInitFunc) goal_init,
		};
		
		object_type = g_type_register_static (G_TYPE_OBJECT,
						      "Goal",
						      &object_info, 0);
	}
	
	return object_type;
}

static void 
goal_class_init (GObjectClass *class)
{
	parent_class = g_type_class_ref (G_TYPE_OBJECT);
	class->finalize = goal_finalize;
}

static void 
goal_init (Goal *goal)
{
	GoalPrivate *priv;

	priv = g_new0 (GoalPrivate, 1);
	priv->pf = NULL;
	priv->index = NULL;

	goal->priv = priv;
}

static void 
goal_finalize (GObject *object)
{
	Goal* goal = GOAL (object);
	
	g_message ("Finalize Goal");

	if (goal->priv->pf)
		g_object_unref (goal->priv->pf);

	g_hash_table_destroy ((GHashTable*) goal->priv->index);

	g_free (goal->priv);
	goal->priv = NULL;
}

Goal*
goal_new (PlayField *pf)
{
	Goal *goal;
	gint row,col;
	Tile *tile;

	goal = GOAL (g_object_new (GOAL_TYPE, NULL));

	g_object_ref (pf);
	goal->priv->pf = pf;
    
	/* initialise index */
	goal->priv->index = g_hash_table_new_full ((GHashFunc) g_direct_hash, 
						   (GCompareFunc) g_direct_equal,
						   NULL, (GDestroyNotify) destroy_hash_value);
	
	for(row = 0; row < playfield_get_n_rows (pf); row++)
	{
		for(col = 0; col < playfield_get_n_cols (pf); col++)
		{
			tile = playfield_get_tile(pf, row, col);

			if (tile &&
			    tile_get_tile_type (tile) == TILE_TYPE_ATOM)
			{
				gint tile_id;
				TileOffset *off;
				GSList *list = NULL;

				tile_id = 10; /* tile_get_hash_value (tile); */
				
				off = g_new0 (TileOffset, 1);
				off->horiz = col;
				off->vert = row;
				
				list = g_hash_table_lookup (goal->priv->index, 
							    GINT_TO_POINTER (tile_id));

				list = g_slist_append(list, off);
				g_hash_table_insert (goal->priv->index, GINT_TO_POINTER (tile_id),
						      (gpointer) list);
				
			}
			g_object_unref (tile);
		}
	}

	return goal;
}

PlayField*
goal_get_playfield (Goal *goal)
{
	g_return_val_if_fail (IS_GOAL (goal), NULL);
	g_object_ref (goal->priv->pf);
	return goal->priv->pf;
}


static void
destroy_hash_value (gpointer value)
{
	GSList *list = (GSList*) value;
	GSList *it;

	for (it = list; it != NULL; it = it->next)
		if (it->data) g_free (it->data);

	g_slist_free (list);
}


static void
print_hash_table (gpointer key, gpointer value, gpointer data)
{
	GSList *list = (GSList*) value;
	GSList *it;

	g_print ("HASH: %i ", GPOINTER_TO_INT (key));
	for (it = list; it != NULL; it = it->next) {
		TileOffset *off = (TileOffset*) it->data;
		g_print("{%x|%x} ",off->horiz, off->vert);
	}
}

void 
goal_print (Goal* goal)
{
	g_return_if_fail (IS_GOAL (goal));
	g_return_if_fail (IS_PLAYFIELD (goal->priv->pf));
	g_return_if_fail (goal->priv->index != NULL);

	g_print("GOAL:\n");
	playfield_print(goal->priv->pf);

	g_hash_table_foreach (goal->priv->index, (GHFunc) print_hash_table, NULL);

	g_print ("\n");
}

gboolean
goal_reached (Goal* goal, PlayField* pf, guint row_anchor, guint col_anchor)
{
	guint tile_id;
	GSList *list;
	GSList *it;
	gboolean result = FALSE;
	Tile *tile;

	g_return_val_if_fail (IS_GOAL (goal), FALSE);
	g_return_val_if_fail (goal->priv->index != NULL, FALSE);
	g_return_val_if_fail (IS_PLAYFIELD (goal->priv->pf), FALSE);
	g_return_val_if_fail (IS_PLAYFIELD (pf), FALSE);

	tile = playfield_get_tile (pf, row_anchor, col_anchor);
	if (tile == NULL) return FALSE;

	tile_id = 10; /* tile_get_hash_value (tile); */
	list = (GSList*) g_hash_table_lookup (goal->priv->index, GINT_TO_POINTER (tile_id));
    
	for (it = list; it != NULL && !result; it = it->next) {
		TileOffset *offset = (TileOffset*) it->data;
		gint start_row =  row_anchor - offset->vert;
		gint start_col =  col_anchor - offset->horiz;

		result = compare_playfield_with_goal (goal, pf, start_row, start_col);
	}

	return result;
}

static gboolean
compare_playfield_with_goal (Goal *goal, PlayField *pf, guint start_row, guint start_col) 
{
	gint pf_row;
	gint pf_col;
	gint goal_row;
	gint goal_col; 
	gint end_row;
	gint end_col;
	gboolean result;
	
	end_row = start_row + playfield_get_n_rows (goal->priv->pf);
	end_col = start_col + playfield_get_n_cols (goal->priv->pf);
	result = TRUE;
	
	for(pf_row = start_row, goal_row = 0; 
	    pf_row < end_row && result; 
	    pf_row++, goal_row++)
	{
		for(pf_col = start_col, goal_col = 0; 
		    pf_col < end_col && result; 
		    pf_col++, goal_col++)
		{
			Tile *pf_tile;
			Tile *goal_tile;

			goal_tile = playfield_get_tile(goal->priv->pf, goal_row, goal_col);
			pf_tile = playfield_get_tile (pf, pf_row , pf_col);

			if (goal_tile) {
				if (tile_get_tile_type (goal_tile) == TILE_TYPE_ATOM)
				{
					if (!pf_tile) 
						result = FALSE;
					else if (!tile_is_equal (goal_tile, pf_tile))
						result = FALSE;
				}
			}

			g_object_unref (goal_tile);
			g_object_unref (pf_tile);
		}
	}

	return result;
}

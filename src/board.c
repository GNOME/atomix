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

#include "board.h"
#include <gnome.h>
#include <math.h>
#include <unistd.h>
#include "goal.h"
#include "undo.h"
#include "main.h"
#include "playfield.h"
#include "tile.h"
#include "canvas_helper.h"

#define                  ANIM_TIMEOUT     8  /* time in milliseconds between 
						two atom movements */
typedef struct {
	gint              timeout_id;
	gint              counter;
	gint              dest_row;
	gint              dest_col;
	double            x_step;
	double            y_step;
} AnimData;

typedef struct {
	guint                row;
	guint                col;
	gboolean             selected;
	gint                 arrow_show_timeout;
	GnomeCanvasItem      *sel_item;
	GnomeCanvasItem      *selector;
	GnomeCanvasGroup     *arrows;
	GnomeCanvasItem      *arrow_left;
	GnomeCanvasItem      *arrow_right;
	GnomeCanvasItem      *arrow_top;
	GnomeCanvasItem      *arrow_bottom;
} SelectorData;

typedef struct  {
	GnomeCanvasGroup *messages;
	GnomeCanvasItem  *pause;
	GnomeCanvasItem  *game_over;
	GnomeCanvasItem  *new_game;
} MessageItems;

typedef struct {
	GnomeCanvasGroup *level;
	GnomeCanvasGroup *obstacles;
	GnomeCanvasGroup *moveables;
	GnomeCanvasGroup *floor;    
} LevelItems;

typedef enum 
{
	UP,
	DOWN,
	LEFT,
	RIGHT
} ItemDirection;

/*=================================================================
 
  Global variables
  
  ---------------------------------------------------------------*/
static Theme       *board_theme = NULL;
static GnomeCanvas *board_canvas = NULL;
static PlayField   *board_env = NULL;   /* the actual playfield */
static PlayField   *board_sce = NULL;   /* the actual playfield */
static Goal        *board_goal = NULL;    /* the goal of this level */


static AnimData         *anim_data;      /* holds the date for the atom 
					    animation */
static GSList           *board_canvas_items = NULL; /* a list of all used 
						       canvas items */
static MessageItems     *message_items;  /* references to the messages */
static LevelItems       *level_items;    /* references to the level groups */

static SelectorData     *selector_data;  /* data about the selector */

/*=================================================================
 
  Declaration of internal functions

  ---------------------------------------------------------------*/
GnomeCanvasItem* create_tile (double x, double y, Tile *tile,
			      GnomeCanvasGroup* group);

GnomeCanvasItem* create_message(GnomeCanvas *canvas, GnomeCanvasGroup* group, gchar* text);

void
board_render(void);

static void render_tile (Tile *tile, gint row, gint col);

gint item_event (GnomeCanvasItem *item, GdkEvent *event, gpointer data);

void move_item(GnomeCanvasItem* item, ItemDirection direc);

int move_item_anim (void *data);

static void selector_move_to (SelectorData *data, guint row, guint col);
static void selector_unselect (SelectorData *data);
static void selector_select (SelectorData *data, GnomeCanvasItem *item);
static SelectorData* selector_create (void);
static void selector_destroy (SelectorData *data);
static void selector_hide (SelectorData *data);
static void selector_arrows_hide (SelectorData *data);

/*=================================================================
 
  Board creation, initialisation and clean up

  ---------------------------------------------------------------*/

void 
board_init (Theme *theme, GnomeCanvas *canvas) 
{
	g_return_if_fail (IS_THEME (theme));
	g_return_if_fail (GNOME_IS_CANVAS (canvas));

	/* Animation Data Setup */
	anim_data = g_new0 (AnimData, 1);
	anim_data->timeout_id = -1;
	anim_data->counter = 0;
	anim_data->dest_row = 0;
	anim_data->dest_col = 0;
	anim_data->x_step = 0.0;
	anim_data->y_step = 0.0;    
		
	undo_clear ();

	/* Canvas setup */
	level_items = g_new0 (LevelItems, 1);

	/* create the canvas groups for the level
	 * (note: the first created group will be the first drawn group)
	 */	
	level_items->level     = create_group (canvas, NULL);
	level_items->obstacles = create_group (canvas, level_items->level);
	level_items->floor     = create_group (canvas, level_items->level);
	level_items->moveables = create_group (canvas, level_items->level);
	
	/* create canvas group and items for the messages */
	message_items = g_malloc(sizeof(MessageItems));
	message_items->messages = create_group (canvas, NULL);
	message_items->pause = create_message(canvas, message_items->messages,
					      _("Game Paused"));
	message_items->game_over = create_message(canvas, message_items->messages,
						  _("Game Over"));
	message_items->new_game = create_message(canvas, message_items->messages,
						 _("Atomix - Molecule Mind Game"));

	gtk_signal_connect (GTK_OBJECT (canvas), "key_press_event", 
			    (GtkSignalFunc) board_handle_key_event, NULL); 
	
	/* other initialistions */
	board_canvas = canvas;
	g_object_ref (theme);
	board_theme = theme;
	board_env = NULL;
	board_sce = NULL;
	board_goal = NULL;
	selector_data = selector_create ();

	set_background_color  (GTK_WIDGET (canvas), theme_get_background_color (theme));
	gnome_canvas_item_show(message_items->new_game);	
}

void
board_init_level (PlayField *env, PlayField *sce, Goal *goal)
{
	gint row, col;

	/* init item anim structure */
	anim_data->timeout_id = -1;
	anim_data->counter = 0;
	anim_data->dest_row = 0;
	anim_data->dest_col = 0;
	anim_data->x_step = 0.0;
	anim_data->y_step = 0.0;    
	
	/* reset undo of moves */
	undo_clear ();
	
	/* init board */
	board_env = g_object_ref (env);
	board_sce = playfield_copy (sce);
	
	/* init goal */
	board_goal = g_object_ref (goal);
	
	/* hide 'New Game' message */
	gnome_canvas_item_hide (message_items->new_game);

	row = playfield_get_n_rows (board_env)/2;
	col = playfield_get_n_cols (board_env)/2;
	selector_move_to (selector_data, row, col);
	selector_unselect (selector_data);
	
	/* render level */
	board_render ();
	board_show ();
}

void
board_destroy()
{
	if (board_env) g_object_unref (board_env);
	if (board_sce) g_object_unref (board_sce);
	if (anim_data) g_free(anim_data);
    
	undo_clear ();
	
	if(level_items)
	{
		if(level_items->level)
			gtk_object_destroy(GTK_OBJECT(level_items->level));
		
		g_free(level_items);
	}
	
	if(message_items)
	{
		if(message_items->messages)
			gtk_object_destroy(GTK_OBJECT(message_items->messages));
		
		g_free(message_items);
	}
	
	if(selector_data)
		selector_destroy (selector_data);

	if (board_theme)
		g_object_unref (board_theme);

	if (board_goal)
		g_object_unref (board_goal);
}

/*=================================================================
  
  Board functions

  ---------------------------------------------------------------*/

void
board_render ()
{
	gint row, col;
	gint tile_width, tile_height;
	gint ca_width, ca_height;
	gint x, y, width, height;
	Tile *tile;

	g_return_if_fail (board_theme != NULL);
		
	/* create canvas items */
	for(row = 0; row < playfield_get_n_rows (board_env); row++) 
	{
		for(col = 0; col < playfield_get_n_cols (board_env); col++)
		{
			tile = playfield_get_tile (board_sce, row, col);
			if (tile != NULL) {
				render_tile (tile, row, col);
				g_object_unref (tile);
			}

			tile = playfield_get_tile (board_env, row, col);
			if (tile != NULL) {
				render_tile (tile, row, col);
				if (tile_get_tile_type (tile) == TILE_TYPE_WALL)
					playfield_set_tile (board_sce, row, col, tile);

				g_object_unref (tile);
			}
		}
	}

	theme_get_tile_size (board_theme, &tile_width, &tile_height);

	/* center the whole thing */
	ca_width = GTK_WIDGET (board_canvas)->allocation.width;
	ca_height = GTK_WIDGET (board_canvas)->allocation.height;
	
	width = tile_width * playfield_get_n_cols (board_env);
	height = tile_height * playfield_get_n_rows (board_env);
	
	if (width > ca_width) {
		x = (width/2) - (ca_width/2);
		width = ca_width;
	}
	else
		x = 0;
	
	if (height > ca_height) {
		y = (height/2) - (ca_height/2);
		height = ca_height;
	}
	else
		y = 0;

	gnome_canvas_set_scroll_region (board_canvas, x, y,
					width + x, height + y);

	/* set background  color*/
	set_background_color (GTK_WIDGET (board_canvas), 
			      theme_get_background_color (board_theme));
}

static void
render_tile (Tile *tile, gint row, gint col)
{
	GnomeCanvasItem *item;
	TileType type;
	gdouble x,y;

	type = tile_get_tile_type (tile);
	switch(type)
	{
	case TILE_TYPE_ATOM:
		convert_to_canvas (board_theme, row, col, &x, &y);
		item = create_tile (x, y, tile, 
				    level_items->moveables);
		break;
		
	case TILE_TYPE_WALL:
		convert_to_canvas (board_theme, row, col, &x, &y);
		item = create_tile (x, y, tile, level_items->obstacles);
		break;
		
	case TILE_TYPE_FLOOR:
		convert_to_canvas (board_theme, row, col, &x, &y);
		item = create_tile (x, y, tile, level_items->floor);
		break;
		
	case TILE_TYPE_UNKNOWN:
	default:
	}
}

gboolean
board_undo_move ()
{
	UndoMove *move;
	gdouble x_src, y_src, x_dest, y_dest;
	gint animstep;

	g_return_val_if_fail (board_theme != NULL, FALSE);

	if (anim_data->timeout_id != -1) return FALSE;

	move = undo_pop_move ();
	if(move == NULL) return FALSE;

	playfield_swap_tiles(board_sce, 
			     move->src_row, 
			     move->src_col, 
			     move->dest_row, 
			     move->dest_col);
	if(selector_data->selected)
	{
		selector_hide (selector_data);
		selector_move_to (selector_data, move->src_row, 
				  move->src_col);
	}
	convert_to_canvas(board_theme, move->src_row, 
			  move->src_col, 
			  &x_src, &y_src);
	convert_to_canvas(board_theme, move->dest_row, 
			  move->dest_col, &x_dest, 
			  &y_dest);
	
	animstep = theme_get_animstep (board_theme);
	if(move->src_col == move->dest_col)
	{
		anim_data->counter = 
			(gint)(fabs(y_dest - y_src)/animstep);
		anim_data->x_step = 0;
		anim_data->y_step = animstep;
		if(move->src_row < move->dest_row )
		{
			anim_data->y_step = 
				-(anim_data->y_step);
		}
	}
	else
	{
		anim_data->counter = 
				(gint)(fabs(x_dest - x_src)/animstep);
		anim_data->x_step = animstep;
		anim_data->y_step =  0;    
		if(move->src_col < move->dest_col)
		{
			anim_data->x_step = 
				-(anim_data->x_step);
		}
	}
	
	anim_data->dest_row = move->src_row;
	anim_data->dest_col = move->src_col;
	selector_data->sel_item = move->item;
	
	anim_data->timeout_id = gtk_timeout_add (ANIM_TIMEOUT, 
						 move_item_anim,
						 anim_data);
	g_free(move);

	return TRUE;
}

void
board_clear()
{
	g_slist_foreach (board_canvas_items, (GFunc) gtk_object_destroy, NULL);
	g_slist_free(board_canvas_items);
	board_canvas_items = NULL;

	/* clear board */
	if(board_env) 
	{
		g_object_unref (board_env);
		board_env = NULL;
	}
	if(board_sce) 
	{
		g_object_unref (board_sce);
		board_sce = NULL;
	}
	if (board_goal) {
		g_object_unref (board_goal);
		board_goal = NULL;
	}

	selector_hide (selector_data);
}

void
board_print()
{
	g_print("Board:\n");
	playfield_print (board_env);
}

void board_view_message(gint msg_id)
{
	switch(msg_id)
	{
	case BOARD_MSG_NONE:
		gnome_canvas_item_hide(message_items->pause);
		gnome_canvas_item_hide(message_items->new_game);
		gnome_canvas_item_hide(message_items->game_over);
		break;
	case BOARD_MSG_GAME_PAUSED:
		gnome_canvas_item_show(message_items->pause);
		break;
	case BOARD_MSG_NEW_GAME:
		gnome_canvas_item_show(message_items->new_game);
		break;
	case BOARD_MSG_GAME_OVER:
		gnome_canvas_item_show(message_items->game_over);
		break;
	default:
	}
}

void board_hide_message(gint msg_id)
{
	switch(msg_id)
	{
	case BOARD_MSG_NONE:
		gnome_canvas_item_show(message_items->pause);
		gnome_canvas_item_show(message_items->new_game);
		gnome_canvas_item_show(message_items->game_over);
		break;
	case BOARD_MSG_GAME_PAUSED:
		gnome_canvas_item_hide(message_items->pause);
		break;
	case BOARD_MSG_NEW_GAME:
		gnome_canvas_item_hide(message_items->new_game);
		break;
	case BOARD_MSG_GAME_OVER:
		gnome_canvas_item_hide(message_items->game_over);
		break;
	default:
	}
}

void
board_hide(void)
{
	gnome_canvas_item_hide(GNOME_CANVAS_ITEM(level_items->moveables));
}

void
board_show(void)
{
	gnome_canvas_item_show(GNOME_CANVAS_ITEM(level_items->moveables));
}

/*=================================================================
  
  Board atom handling functions

  ---------------------------------------------------------------*/

static GnomeCanvasItem*
get_item_by_row_col (gint row, gint col)
{
	gdouble x, y;
	gint width, height;

	convert_to_canvas (board_theme, row, col, &x, &y);
	theme_get_tile_size (board_theme, &width, &height);
	
	x = x + (width/2);
	y = y + (height/2);

	return gnome_canvas_get_item_at (board_canvas, x, y);
}

void 
board_handle_key_event (GdkEventKey *event)
{
	GnomeCanvasItem *item;
	gint new_row, new_col;
	Tile *tile;

	g_return_if_fail(selector_data!=NULL);
	
	new_row = selector_data->row;
	new_col = selector_data->col;

        /* is currently an object moved? */
	if (anim_data->timeout_id != -1) return;

	switch(event->keyval)
	{
	case GDK_Return:
		if (selector_data->selected) {
			/* unselect item, show selector image */
			selector_unselect (selector_data);
		}
		else {
			item = get_item_by_row_col (selector_data->row, 
						    selector_data->col);
			if (item == NULL) break;

			tile = TILE (g_object_get_data (G_OBJECT (item), "tile"));
			if (tile == NULL) break;

			if (tile_get_tile_type (tile) == TILE_TYPE_ATOM)
				selector_select (selector_data, item);	
		}
		break;
			
	case GDK_Left:
		if (!selector_data->selected) {
			new_col--;
			if (new_col >= 0)
				selector_move_to (selector_data, new_row, new_col);
		}
		else {
			move_item (selector_data->sel_item, LEFT); /* selector will be
								      moved in this
								      function */
		}
		break;

	case GDK_Right:
		if (!selector_data->selected) {
			new_col++;
			if (new_col < playfield_get_n_cols (board_env))
				selector_move_to (selector_data, new_row, new_col);
		}
		else {
			move_item (selector_data->sel_item, RIGHT); /* selector will be
								       moved in this
								       function */
		}
		break;
		
	case GDK_Up:
		if(!selector_data->selected) {
			new_row--;
			if(new_row >= 0)
				selector_move_to (selector_data, new_row, new_col);
		}
		else {
			move_item(selector_data->sel_item, UP); /* selector will be moved
								   in this function */
		}
		break;
		
	case GDK_Down:
		if(!selector_data->selected) {
			new_row++;
			if(new_row < playfield_get_n_rows (board_env))
				selector_move_to (selector_data, new_row, new_col);
		}
		else {
			move_item (selector_data->sel_item, DOWN); /* selector will be
								      moved in this
								      function */
		}
		break;

	default:
		break;
	}
}


void
move_item (GnomeCanvasItem* item, ItemDirection direc)
{
	gdouble x1, y1, x2, y2;
	gdouble new_x1, new_y1;
	guint src_row, src_col, dest_row, dest_col, tmp_row, tmp_col;
	gint animstep;
	Tile *tile;
	gint tw, th;
	
	gnome_canvas_item_get_bounds (item, &x1, &y1, &x2, &y2);
	theme_get_tile_size (board_theme, &tw, &th);
	x1 = x1 - (((gint)x1) % tw); /* I don't have a clue why we must do
					this here. */
	convert_to_playfield (board_theme, x1, y1, &src_row, &src_col);

	/* find destination row/col */
	tmp_row = dest_row = src_row; 
	tmp_col = dest_col = src_col;
	while (TRUE)
	{
		switch (direc)
		{
		case UP:
			tmp_row = tmp_row - 1; break;
		case DOWN:
			tmp_row = tmp_row + 1; break;
		case LEFT:
			tmp_col = tmp_col - 1; break;
		case RIGHT:
			tmp_col = tmp_col + 1; break;
		}

		if (tmp_row < 0 || tmp_row >= playfield_get_n_rows (board_sce) ||
		    tmp_col < 0 || tmp_col >= playfield_get_n_cols (board_sce))
			break;
		
		tile = playfield_get_tile (board_sce, tmp_row, tmp_col);
		if (tile && (tile_get_tile_type (tile) == TILE_TYPE_ATOM || 
			     tile_get_tile_type (tile) == TILE_TYPE_WALL))
		{
			g_object_unref (tile);
			break;
		}

		dest_row = tmp_row; dest_col = tmp_col;
		if (tile) g_object_unref (tile);
	}
	
	/* move the item, if the new position is different */
	if(src_row != dest_row || src_col != dest_col)
	{
		undo_push_move (item, src_row, src_col, 
				dest_row, dest_col);
	
		convert_to_canvas (board_theme, dest_row, dest_col, &new_x1, &new_y1);
		playfield_swap_tiles (board_sce, src_row, src_col, dest_row, dest_col);

		selector_hide (selector_data);
		selector_move_to (selector_data, dest_row, dest_col);
		
		animstep = theme_get_animstep (board_theme);
		if(direc == UP || direc == DOWN)
		{
			anim_data->counter = (gint)(fabs(new_y1 - y1) / animstep);
			anim_data->x_step = 0;
			anim_data->y_step = (direc == DOWN) ? animstep : -animstep;
		}
		else
		{
			anim_data->counter = (gint)(fabs(new_x1 - x1) / animstep);
			anim_data->x_step = (direc == RIGHT) ? animstep : -animstep;
			anim_data->y_step =  0;    
		}
		
		anim_data->dest_row = dest_row;
		anim_data->dest_col = dest_col;

		anim_data->timeout_id = gtk_timeout_add(ANIM_TIMEOUT, move_item_anim, 
							anim_data);
	}
}

int
move_item_anim (void *data)
{
	AnimData* anim_data = (AnimData*)data;
	
	if(anim_data->counter > 0)
	{
		gnome_canvas_item_move (selector_data->sel_item,
					anim_data->x_step, anim_data->y_step);
		anim_data->counter--;
		return TRUE;
	}
	else
	{
		anim_data->timeout_id = -1;
		if(goal_reached (board_goal, board_sce, anim_data->dest_row, 
				 anim_data->dest_col))
		{
			game_level_finished (NULL);
		}
		else {
			if (selector_data->selected)
				selector_select (selector_data, selector_data->sel_item);
		}

		return FALSE;
	}
}

/*=================================================================
 
  Selector helper functions

  ---------------------------------------------------------------*/

static void 
selector_move_to (SelectorData *data, guint row, guint col)
{
	gdouble src_x, src_y;
	gdouble dest_x, dest_y;
	gint tile_width, tile_height;
	
	g_return_if_fail (data != NULL);

	if (data->arrow_show_timeout > -1)
		gtk_timeout_remove (data->arrow_show_timeout);
	data->arrow_show_timeout = -1;

	theme_get_tile_size (board_theme, &tile_width, &tile_height);
	convert_to_canvas (board_theme, data->row, data->col, &src_x, &src_y);
	convert_to_canvas (board_theme, row, col, &dest_x, &dest_y);

	gnome_canvas_item_move (data->selector, dest_x - src_x, dest_y - src_y);	
	gnome_canvas_item_move (GNOME_CANVAS_ITEM (data->arrows),
				dest_x - src_x, dest_y - src_y);

	data->row = row;
	data->col = col;
}

static void 
selector_unselect (SelectorData *data)
{
	g_return_if_fail (data != NULL);

	data->selected = FALSE;
	data->sel_item = NULL;
	gnome_canvas_item_show (data->selector);
	selector_arrows_hide (data);
}

static void
selector_arrows_hide (SelectorData *data)
{
	if (data->arrow_show_timeout > -1)
		gtk_timeout_remove (data->arrow_show_timeout);
	data->arrow_show_timeout = -1;
	gnome_canvas_item_hide (GNOME_CANVAS_ITEM (data->arrows));
}

static void
selector_hide (SelectorData *data)
{
	gnome_canvas_item_hide (data->selector);
	selector_arrows_hide (data);
}

static gboolean
show_arrow_group (SelectorData *data)
{
	gnome_canvas_item_show (GNOME_CANVAS_ITEM (data->arrows));
	data->arrow_show_timeout = -1;
	return FALSE;
}

static void
selector_arrows_show (SelectorData *data)
{
	gint r, c;
	Tile *tile;

	if (board_sce == NULL) {
		gnome_canvas_item_hide (GNOME_CANVAS_ITEM (data->arrows));
		return;
	}

	r = data->row - 1;
	c = data->col;
	if (r >= 0) {
		tile = playfield_get_tile (board_sce, r, c);
		if (tile == NULL)
			gnome_canvas_item_show (data->arrow_top);
		else {
			gnome_canvas_item_hide (data->arrow_top);
			g_object_unref (tile);
		}
	}

	r = data->row;
	c = data->col + 1;
	if (c < playfield_get_n_cols (board_sce)) {
		tile = playfield_get_tile (board_sce, r, c);
		if (tile == NULL)
			gnome_canvas_item_show (data->arrow_right);
		else {
			gnome_canvas_item_hide (data->arrow_right);
			g_object_unref (tile);
		}
	}

	r = data->row + 1;
	c = data->col;
	if (r < playfield_get_n_rows (board_sce)) {
		tile = playfield_get_tile (board_sce, r, c);
		if (tile == NULL)
			gnome_canvas_item_show (data->arrow_bottom);
		else {
			gnome_canvas_item_hide (data->arrow_bottom);
			g_object_unref (tile);
		}
	}

	r = data->row;
	c = data->col - 1;
	if (c >= 0) {
		tile = playfield_get_tile (board_sce, r, c);
		if (tile == NULL)
			gnome_canvas_item_show (data->arrow_left);
		else {
			gnome_canvas_item_hide (data->arrow_left);
			g_object_unref (tile);
		}
	}



	if (data->arrow_show_timeout > -1)
		gtk_timeout_remove (data->arrow_show_timeout);

	data->arrow_show_timeout = gtk_timeout_add (800, 
						    (GtkFunction) show_arrow_group,
						    data);
}

static void 
selector_select (SelectorData *data, GnomeCanvasItem *item)
{
	g_return_if_fail (data != NULL);

	data->selected = TRUE;
	data->sel_item = item;

	gnome_canvas_item_hide (data->selector);
	selector_arrows_show (data);
}

static void
selector_destroy (SelectorData *data)
{
	if (data->selector) 
		gtk_object_destroy (GTK_OBJECT (data->selector));
	
	if (data->arrows)
		gtk_object_destroy (GTK_OBJECT (data->arrows));

	g_free (data);
}

static SelectorData* 
selector_create (void)
{
	SelectorData *data;
	GdkPixbuf *pixbuf;
	GdkPixbuf *sel_arrows[4];
	gint tile_width, tile_height;
	
	data = g_new0 (SelectorData, 1);	

	pixbuf = theme_get_selector_image (board_theme);
	theme_get_selector_arrow_images (board_theme, &sel_arrows[0]);
	theme_get_tile_size (board_theme, &tile_width, &tile_height);

	g_return_val_if_fail(pixbuf != NULL, NULL);

	data->row = 0;
	data->col = 0;
	data->sel_item = NULL;
	data->selected = FALSE;
	data->arrow_show_timeout = -1;

	data->selector = gnome_canvas_item_new(gnome_canvas_root (board_canvas),
					       gnome_canvas_pixbuf_get_type(),
					       "pixbuf", pixbuf,
					       "x", 0.0,
					       "x_in_pixels", TRUE,
					       "y", 0.0,
					       "y_in_pixels", TRUE,
					       "width", (double) gdk_pixbuf_get_width (pixbuf),
					       "height", (double) gdk_pixbuf_get_height (pixbuf),
					       "anchor", GTK_ANCHOR_NW,
					       NULL);
	data->arrows = GNOME_CANVAS_GROUP (
		gnome_canvas_item_new (gnome_canvas_root (board_canvas),
				       gnome_canvas_group_get_type (), 
				       "x", (double) -tile_width,
				       "y", (double) -tile_height,
				       NULL));

	data->arrow_top = gnome_canvas_item_new (GNOME_CANVAS_GROUP (data->arrows),
						 gnome_canvas_pixbuf_get_type(),
						 "pixbuf", sel_arrows[0],
						 "x", (double) 1 * tile_width,
						 "x_in_pixels", TRUE,
						 "y", (double) 0 * tile_height,
						 "y_in_pixels", TRUE,
						 "anchor", GTK_ANCHOR_NW,
						 NULL);	
	data->arrow_right = gnome_canvas_item_new (GNOME_CANVAS_GROUP (data->arrows),
						   gnome_canvas_pixbuf_get_type(),
						   "pixbuf", sel_arrows[1],
						   "x", (double) 2 * tile_width,
						   "x_in_pixels", TRUE,
						   "y", (double) 1 * tile_height,
						   "y_in_pixels", TRUE,
						   "anchor", GTK_ANCHOR_NW,
						   NULL);	
	data->arrow_bottom = gnome_canvas_item_new (GNOME_CANVAS_GROUP (data->arrows),
						    gnome_canvas_pixbuf_get_type(),
						    "pixbuf", sel_arrows[2],
						    "x", (double) 1 * tile_width,
						    "x_in_pixels", TRUE,
						    "y", (double) 2 * tile_height,
						    "y_in_pixels", TRUE,
						    "anchor", GTK_ANCHOR_NW,
						    NULL);	
	data->arrow_left = gnome_canvas_item_new (GNOME_CANVAS_GROUP (data->arrows),
						  gnome_canvas_pixbuf_get_type(),
						  "pixbuf", sel_arrows[3],
						  "x", (double) 0 * tile_width,
						  "x_in_pixels", TRUE,
						  "y", (double) 1 * tile_height,
						  "y_in_pixels", TRUE,
						  "anchor", GTK_ANCHOR_NW,
						  NULL);	
	
	gnome_canvas_item_hide (GNOME_CANVAS_ITEM (data->selector));
	gnome_canvas_item_hide (GNOME_CANVAS_ITEM (data->arrows));

	return data;
}


/*=================================================================
 
  Internal creation functions
s
  ---------------------------------------------------------------*/

GnomeCanvasItem*
create_tile (double x, double y, Tile *tile, 
	     GnomeCanvasGroup* group)
{
	GdkPixbuf *pixbuf = NULL;
	GnomeCanvasItem *item = NULL;
	
	pixbuf = theme_get_tile_image (board_theme, tile);

	item = gnome_canvas_item_new(group,
				     gnome_canvas_pixbuf_get_type(),
				     "pixbuf", pixbuf,
				     "x", x,
				     "x_in_pixels", TRUE,
				     "y", y,
				     "y_in_pixels", TRUE,
				     "width", (double) gdk_pixbuf_get_width (pixbuf),
				     "height", (double) gdk_pixbuf_get_height (pixbuf),
				     "anchor", GTK_ANCHOR_NW,
				     NULL);
	g_object_set_data (G_OBJECT (item), "tile", tile);

        board_canvas_items = g_slist_prepend(board_canvas_items, item);

	return GNOME_CANVAS_ITEM (item);
}

GnomeCanvasItem*
create_message(GnomeCanvas *canvas, GnomeCanvasGroup* group, gchar* text)
{
	GnomeCanvasItem* item;
#if 0
	PangoFontDescription *font;
	gdouble x, y, x1, y1, x2, y2;

	g_return_val_if_fail (canvas != NULL, NULL);
	g_return_val_if_fail (GNOME_IS_CANVAS (canvas), NULL);
	g_return_val_if_fail (group != NULL, NULL);
	g_return_val_if_fail (GNOME_IS_CANVAS_GROUP (group), NULL);
	
	gnome_canvas_get_scroll_region(canvas, &x1, &y1, &x2, &y2);    
	x = x1 + (x2-x1)/2;
	y = y1 + (y2-y1)/2;    

	font = pango_font_description_new ();
	pango_font_description_set_family (font, "helvetica");
	pango_font_description_set_size (font, 14);
	pango_font_description_set_weight (font, PANGO_WEIGHT_BOLD);
	
	item = gnome_canvas_item_new(group,
				     gnome_canvas_text_get_type(),
				     "text", text,
				     "x", x,
				     "y", y,
				     "anchor", GTK_ANCHOR_CENTER,
				     "font_desc", font,
				     "justification", GTK_JUSTIFY_CENTER,
				     "fill_color", "black",
				     NULL);       
	
	gnome_canvas_item_hide(item);
#endif
	item = gnome_canvas_item_new(group,
				     gnome_canvas_re_get_type (),
				     "x1", 0.0,
				     "y1", 0.0,
				     "x2", 20.0,
				     "y2", 20.0,
				     "fill_color", "black",
				     NULL);       


	return item;
}


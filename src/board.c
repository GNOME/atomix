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

#define                  ANIM_TIMEOUT     7  /* time in milliseconds between 
						two atom movements */

typedef struct _AnimData      AnimData;
typedef struct _MessageItems  MessageItems;
typedef struct _LevelItems    LevelItems;
typedef struct _SelectorData  SelectorData;

struct _AnimData {
    gint              timeout_id;
    gint              counter;
    gint              dest_row;
    gint              dest_col;
    double            x_step;
    double            y_step;
};

struct _MessageItems
{
	GnomeCanvasGroup *messages;
	GnomeCanvasItem  *pause;
	GnomeCanvasItem  *game_over;
	GnomeCanvasItem  *new_game;
};

struct _LevelItems
{
	GnomeCanvasGroup *level;
	GnomeCanvasGroup *obstacles;
	GnomeCanvasGroup *moveables;
	GnomeCanvasGroup *selector;
	GnomeCanvasGroup *decor;    
};

typedef enum 
{
	UP,
	DOWN,
	LEFT,
	RIGHT
} ItemDirection;

enum {
	MOVEABLE_SELECTED,
	NOTHING_SELECTED
};

struct _SelectorData
{
	guint row;
	guint col;
	gint state;
	GnomeCanvasItem *item;
};

/*=================================================================
 
  Global variables
  
  ---------------------------------------------------------------*/
static Theme       *board_theme = NULL;
static GnomeCanvas *board_canvas = NULL;
static PlayField   *board_pf = NULL;   /* the actual playfield */
static Goal        *board_goal = NULL;    /* the goal of this level */


static GnomeCanvasItem  *selected_item;  /* which canvas item currently 
					    be moved */
static AnimData         *anim_data;      /* holds the date for the atom 
					    animation */
static GSList           *board_canvas_items = NULL; /* a list of all used 
						       canvas items */
static CanvasMap        *canvas_map=NULL;/* releation between row/col and CanvasItem */
static MessageItems     *message_items;  /* references to the messages */
static LevelItems       *level_items;    /* references to the level groups */
static gboolean         mouse_dragging;  /* whether the mouse is being 
					    dragged or not */
static SelectorData     *selector_data;  /* data about the selector */

static GdkCursor        *hidden_cursor;  /* stores the transparent cursor */

/*=================================================================
 
  Declaration of internal functions

  ---------------------------------------------------------------*/
GnomeCanvasItem* create_tile (double x, double y, Tile *tile,
			      GnomeCanvasGroup* group);

GnomeCanvasItem* create_message(GnomeCanvas *canvas, GnomeCanvasGroup* group, gchar* text);

void
create_hidden_cursor(void);

void
board_render(void);

gint
item_event (GnomeCanvasItem *item, GdkEvent *event, gpointer data);

void
move_item(GnomeCanvasItem* item, ItemDirection direc);

int
move_item_anim (void *data);

void
selector_set(SelectorData *data, guint row, guint col);

void
selector_hide(SelectorData *data);

void
selector_show(SelectorData *data);

SelectorData* selector_new(void);

/*=================================================================
 
  Board creation, initialisation and clean up

  ---------------------------------------------------------------*/

void 
board_init (Theme *theme, GnomeCanvas *canvas) 
{
	GdkColor color;

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
		
	/* init hidden cursor */
	create_hidden_cursor ();
	
	/* init undo */
	undo_init ();

	canvas_map = NULL;
	
	/* Canvas setup */
	level_items = g_new0 (LevelItems, 1);

	/* create the canvas groups for the level
	 * (note: the first created group will be the first drawn group)
	 */	
	level_items->level     = create_group (canvas, NULL);
	level_items->obstacles = create_group (canvas, level_items->level);
	level_items->decor     = create_group (canvas, level_items->level);/* not used yet*/
	level_items->moveables = create_group (canvas, level_items->level);
	level_items->selector  = create_group (canvas, level_items->level);
	
	/* create canvas group and items for the messages */
	message_items = g_malloc(sizeof(MessageItems));
	message_items->messages = create_group (canvas, NULL);
	message_items->pause = create_message(canvas, message_items->messages,
					      _("Game Paused"));
	message_items->game_over = create_message(canvas, message_items->messages,
						  _("Game Over"));
	message_items->new_game = create_message(canvas, message_items->messages,
						 _("Atomix - Molecule Mind Game"));
	
	/* other initialistions */
	board_canvas = canvas;
	g_object_ref (theme);
	board_theme = theme;
	board_pf = NULL;
	board_goal = NULL;
	selector_data = NULL;
	color.red = 60928;   /* this is the X11 color "cornsilk2" */
  	color.green = 59392;
	color.blue = 52480;
	set_background_color  (GTK_WIDGET (canvas), &color);
	gnome_canvas_item_show(message_items->new_game);	
}

void
board_init_level (PlayField *pf, Goal *goal)
{
	/* init item anim structure */
	anim_data->timeout_id = -1;
	anim_data->counter = 0;
	anim_data->dest_row = 0;
	anim_data->dest_col = 0;
	anim_data->x_step = 0.0;
	anim_data->y_step = 0.0;    
	
	selected_item = NULL;
	
	/* reset undo of moves */
	undo_free_all_moves ();
	
	/* init board */
	board_pf = playfield_copy (pf);
	
	/* init goal */
	board_goal = goal;
	
	/* hide 'New Game' message */
	gnome_canvas_item_hide (message_items->new_game);

	/* initalise canvas map */
	if(canvas_map != NULL) {
		canvas_map_destroy(canvas_map);
	}
	canvas_map = canvas_map_new();

	/* initialise control */
	selector_data = selector_new();

	board_set_keyboard_control();
	/* board_set_mouse_control(); */
	
	/* render level */
	board_render ();
}

void
board_destroy()
{
	if (board_pf) g_object_unref (board_pf);
	if (anim_data) g_free(anim_data);
    
	undo_free_all_moves();
	
	if(canvas_map)
	{
		canvas_map_destroy(canvas_map);
	}
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
	
	if(hidden_cursor)
	{
		gdk_cursor_destroy(hidden_cursor);
	}

	if(selector_data)
	{
		g_free(selector_data);
	}
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
	GnomeCanvasItem *item;
	gint row, col, width, height;
	gint tile_width, tile_height;
	gdouble x,y;
	Tile *tile;
	TileType type;

	g_return_if_fail (board_theme != NULL);
		
	/* create canvas items */
	for(row=0; row < playfield_get_n_rows (board_pf); row++) 
	{
		for(col=0; col < playfield_get_n_rows (board_pf); col++)
		{
			tile = playfield_get_tile (board_pf, row, col);
			if (tile != NULL) {
				type = tile_get_tile_type (tile);
				switch(type)
				{
				case TILE_TYPE_MOVEABLE:
					convert_to_canvas (board_theme, row, col, &x, &y);
					item = create_tile (x, y, tile, 
							    level_items->moveables);
					canvas_map_set_item (canvas_map, 
							     row, col, item);
					break;
					
				case TILE_TYPE_OBSTACLE:
					convert_to_canvas (board_theme, row, col, &x, &y);
					item = create_tile (x, y, tile, level_items->obstacles);
					break;
					
				case TILE_TYPE_UNKNOWN:
				default:
				}
			}
		}
	}
	
	theme_get_tile_size(board_theme, &tile_width, &tile_height);
	
	/* center the whole thing */
	width = tile_width * playfield_get_n_cols (board_pf);
	height = tile_height * playfield_get_n_rows (board_pf);
	set_canvas_dimensions (board_canvas, width, height);

	/* set background  color*/
	set_background_color (GTK_WIDGET (board_canvas), 
			      theme_get_background_color (board_theme));
}

gboolean
board_undo_move ()
{
	g_return_val_if_fail (board_theme != NULL, FALSE);

	if((anim_data->timeout_id == -1) &&
	   (mouse_dragging == FALSE))
	{
		UndoMove *move = undo_get_last_move();
		
		if(move != NULL)
		{
			gdouble x_src, y_src, x_dest, y_dest;
			gint animstep;
			
			playfield_swap_tiles(board_pf, 
					     move->src_row, 
					     move->src_col, 
					     move->dest_row, 
					     move->dest_col);
			canvas_map_move_item(canvas_map,
					     move->dest_row, 
					     move->dest_col, 
					     move->src_row, 
					     move->src_col);
			if(TRUE /* (preferences_get()->keyboard_control)*/ && 
			   (selector_data->state == MOVEABLE_SELECTED))
			{
				selector_set(selector_data, move->src_row, 
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
			selected_item = move->item;
			
			anim_data->timeout_id = gtk_timeout_add(ANIM_TIMEOUT, 
								move_item_anim,
 								anim_data);
			g_free(move);
		}     	
		else return FALSE;
	} else return FALSE;

	return TRUE;
}

void
board_clear()
{
	if(canvas_map)
	{
		canvas_map_destroy(canvas_map);
		canvas_map = NULL;
	}
	g_slist_foreach (board_canvas_items, (GFunc) gtk_object_destroy, NULL);
	g_slist_free(board_canvas_items);
	board_canvas_items = NULL;

	/* clear board */
	if(board_pf) 
	{
		g_object_unref (board_pf);
		board_pf = NULL;
	}

	if(selector_data)
	{
		g_free(selector_data);
		selector_data = NULL;
	}
}

void
board_print()
{
	g_print("Board:\n");
	playfield_print (board_pf);
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
	gnome_canvas_item_hide(GNOME_CANVAS_ITEM(level_items->level));
}

void
board_show(void)
{
	gnome_canvas_item_show(GNOME_CANVAS_ITEM(level_items->level));
}


void
board_show_normal_cursor(void)
{
	GdkCursor *cursor;
	
	if(selected_item != NULL)
	{
		gnome_canvas_item_ungrab(selected_item, GDK_CURRENT_TIME);
	}
	
	cursor = gdk_cursor_new(GDK_LEFT_PTR);
	gdk_window_set_cursor(gtk_widget_get_parent_window (GTK_WIDGET (board_canvas)),
			      cursor);
	gdk_cursor_destroy(cursor);
}

/*=================================================================
  
  Board atom handling functions

  ---------------------------------------------------------------*/

void board_set_mouse_control(void)
{
	gnome_canvas_item_hide(GNOME_CANVAS_ITEM(level_items->selector));
	gnome_canvas_item_show(selector_data->item);
	selected_item = NULL;
	selector_data->state = NOTHING_SELECTED;
}

void board_set_keyboard_control(void)
{
	gint row, col;
	gnome_canvas_item_show(selector_data->item);
	row = playfield_get_n_rows (board_pf)/2;
	col = playfield_get_n_cols (board_pf)/2;
	selector_set(selector_data, row, col);
	gnome_canvas_item_show(GNOME_CANVAS_ITEM(level_items->selector));
}

gint
item_event (GnomeCanvasItem *item, GdkEvent *event, gpointer data)
{
	GdkCursor *cursor;
	double x,y,diff_x, diff_y;
	
	static double selected_x, selected_y;
	static gint item_move_counter;

	if( TRUE /*!preferences_get()->mouse_control*/ ) return FALSE;
    
	switch (event->type) {
	case GDK_BUTTON_PRESS:		
                
                /* is currently an object moved? */
		if(anim_data->timeout_id!=-1) break; 
				

		if((item!=NULL) && (event->button.button==1)) 
		{
			selected_x = event->button.x;
			selected_y = event->button.y;
			selected_item = item;
			gnome_canvas_item_w2i (selected_item->parent, 
					       &selected_x,	    
					       &selected_y);
			
			/* change cursor */	    
			if(FALSE /* preferences_get()->hide_cursor */)
			{
				gnome_canvas_item_grab(item,
						       GDK_BUTTON_RELEASE_MASK | 
						       GDK_POINTER_MOTION_MASK,
						       hidden_cursor,
						       event->button.time);
			}
			else
			{
				gnome_canvas_item_grab(item,
						       GDK_BUTTON_RELEASE_MASK | 
						       GDK_POINTER_MOTION_MASK,
						       NULL,
						       event->button.time);
			}
			mouse_dragging = TRUE;
			item_move_counter = 0;
		}
		break;
		
    case GDK_MOTION_NOTIFY:
            /* Is currently an object moved? */
	    if(anim_data->timeout_id!=-1) break;

	    /* if no lazy dragging enabled, allow only 
	       one move per mouse click */
	    if( FALSE /* (!preferences_get()->lazy_dragging)*/ &&
	       (item_move_counter > 0))
	    {
		    break;
	    }
	    
	    /* in some rare cases it is possible 
	       that selected_item is NULL. */
	    if(selected_item == NULL) break;
	    
	    if (event->motion.state & GDK_BUTTON1_MASK) {
		    gint mouse_sensitivity;
		    
		    x = event->button.x;
		    y = event->button.y;
		    gnome_canvas_item_w2i (selected_item->parent, &x, &y);
		    
		    diff_x = x - selected_x;
		    diff_y = y - selected_y;
		    mouse_sensitivity = 10 /* preferences_get()->mouse_sensitivity */;
		    
		    if((fabs(diff_x) > mouse_sensitivity)|| 
		       (fabs(diff_y) > mouse_sensitivity))
		    {
			    /* ok we move the item */
			    selected_x = x;
			    selected_y = y;
			    item_move_counter++;
			    
			    if(fabs(diff_x) > fabs(diff_y))
			    {
				    /* move it horizontal */
				    if(diff_x < 0) 
				    {
					    move_item(selected_item, LEFT);
				    }
				    else 
				    {
					    move_item(selected_item, RIGHT);
				    }
			    }
			    else 
			    {
				    /* move it vertical */
				    if(diff_y < 0)
				    {
					    move_item(selected_item, UP);
				    }
				    else
				    {
					    move_item(selected_item, DOWN);
				    }
			    }
		    }
	    }
	    break;
	    
	case GDK_BUTTON_RELEASE:
		board_show_normal_cursor();
		if(anim_data->timeout_id==-1) selected_item = NULL;
		mouse_dragging = FALSE;
		break;
		
	case GDK_ENTER_NOTIFY:
		if(!mouse_dragging)
		{
			cursor = gdk_cursor_new(GDK_FLEUR);
			gdk_window_set_cursor(
				gtk_widget_get_parent_window(GTK_WIDGET (board_canvas)),
				cursor);
			gdk_cursor_destroy(cursor);
		}
		break;
		
	case GDK_LEAVE_NOTIFY:
		if(!mouse_dragging)
		{
			board_show_normal_cursor();
		}
		break;
		
	default:
	}
	
	return FALSE;    
}

void board_handle_key_event(GdkEventKey *event)
{
	GnomeCanvasItem *item;
	gint new_row, new_col;

	g_return_if_fail(selector_data!=NULL);
	
	if(FALSE /* !preferences_get()->keyboard_control*/ ) return;

	new_row = selector_data->row;
	new_col = selector_data->col;

        /* is currently an object moved? */
	if(anim_data->timeout_id!=-1) return;

	switch(event->keyval)
	{
	case GDK_Return:
		if(selector_data->state == MOVEABLE_SELECTED)
		{
			/* unselect item, show selector image */
			selector_data->state = NOTHING_SELECTED;
			selector_show(selector_data);
			selected_item = NULL;
		}
		else if(selector_data->state == NOTHING_SELECTED)
		{
			item = canvas_map_get_item(canvas_map, 
						   selector_data->row, 
						   selector_data->col);
			if(item!=NULL)
			{			
				/* select item, hide selector image */
				selected_item = item;
				selector_data->state = MOVEABLE_SELECTED;
				selector_hide(selector_data);
			}
			
		}
		break;
			
	case GDK_Left:
		if(selector_data->state == NOTHING_SELECTED)
		{
			new_col--;
			if(new_col >= 0)
			{
				selector_set(selector_data, new_row, new_col);
			}
		}
		else if(selector_data->state == MOVEABLE_SELECTED)
		{
			item = canvas_map_get_item(canvas_map, 
						   selector_data->row, 
						   selector_data->col);			
			move_item(selected_item, LEFT); /* selector will be
                                                           moved in this
                                                           function */
		}
		break;

	case GDK_Right:
		if(selector_data->state == NOTHING_SELECTED)
		{
			new_col++;
			if(new_col < playfield_get_n_cols (board_pf))
			{
				selector_set(selector_data, new_row, new_col);
			}
		}
		else if(selector_data->state == MOVEABLE_SELECTED)
		{
			item = canvas_map_get_item(canvas_map, 
						   selector_data->row, 
						   selector_data->col);			
			move_item(selected_item, RIGHT); /* selector will be
                                                            moved in this
                                                            function */
		}
		break;
		
	case GDK_Up:
		if(selector_data->state == NOTHING_SELECTED)
		{
			new_row--;
			if(new_row >= 0)
			{
				selector_set(selector_data, new_row, new_col);
			}
		}
		else if(selector_data->state == MOVEABLE_SELECTED)
		{
			item = canvas_map_get_item(canvas_map, 
						   selector_data->row, 
						   selector_data->col);			
			move_item(selected_item, UP); /* selector will be moved
                                                         in this function */
		}
		break;
		
	case GDK_Down:
		if(selector_data->state == NOTHING_SELECTED)
		{
			new_row++;
			if(new_row < playfield_get_n_rows (board_pf))
			{
				selector_set(selector_data, new_row, new_col);
			}
		}
		else if(selector_data->state == MOVEABLE_SELECTED)
		{
			item = canvas_map_get_item(canvas_map, 
						   selector_data->row, 
						   selector_data->col);			
			move_item(selected_item, DOWN); /* selector will be
                                                           moved in this
                                                           function */
		}
		break;

	default:
		break;
	}
}


void
move_item(GnomeCanvasItem* item, ItemDirection direc)
{
	gdouble x1, y1, x2, y2;
	gdouble new_x1, new_y1;
	guint src_row, src_col, dest_row, dest_col;
	gint animstep;
	TileType type;
	Tile *tile;
	UndoMove *move;
	
	gnome_canvas_item_get_bounds(item, &x1, &y1, &x2, &y2);
	convert_to_playfield(board_theme, x1, y1, &src_row, &src_col);
	
	dest_row = src_row; dest_col = src_col;
	do
	{
		switch(direc)
		{
		case UP:
			dest_row = dest_row - 1; break;
		case DOWN:
			dest_row = dest_row + 1; break;
		case LEFT:
			dest_col = dest_col - 1; break;
		case RIGHT:
			dest_col = dest_col + 1; break;
		}
		tile = playfield_get_tile(board_pf, dest_row, dest_col);
		type = tile_get_tile_type(tile);
	}
	while((tile != NULL) && (type==TILE_TYPE_UNKNOWN));
	switch(direc)
	{
	case UP:
		dest_row++; break;
	case DOWN:
		dest_row--; break;
	case LEFT:
		dest_col++; break;
	case RIGHT:
		dest_col--; break;
	}
	
	/* move the item, if the new position is different */
	if((src_row != dest_row) || (src_col != dest_col))
	{
		move = undo_create_move(item, src_row, src_col, 
					dest_row, dest_col);
		undo_add_move(move);

	
		convert_to_canvas(board_theme, dest_row, dest_col, &new_x1, &new_y1);
		playfield_swap_tiles(board_pf, src_row, src_col, dest_row, dest_col);
		canvas_map_move_item(canvas_map, src_row, src_col, dest_row, dest_col);
		if(TRUE /*preferences_get()->keyboard_control*/)
		{
			selector_set(selector_data, dest_row, dest_col);
		}
		
		animstep = theme_get_animstep (board_theme);
		if(direc == UP || direc == DOWN)
		{
			anim_data->counter = (gint)(fabs(new_y1 - y1) / animstep);
			anim_data->x_step = 0;
			anim_data->y_step = animstep;
			if(direc == UP)
			{
				anim_data->y_step = -(anim_data->y_step);
			}
		}
		else
		{
			anim_data->counter = (gint)(fabs(new_x1 - x1) / animstep);
			anim_data->x_step = animstep;
			anim_data->y_step =  0;    
			if(direc == LEFT)
			{
				anim_data->x_step = -(anim_data->x_step);
			}
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
		gnome_canvas_item_move(selected_item,
				       anim_data->x_step, anim_data->y_step);
		anim_data->counter--;
	}
	else
	{
		gtk_timeout_remove(anim_data->timeout_id);
		
		if(goal_reached(board_goal, board_pf, anim_data->dest_row, 
				anim_data->dest_col))
		{
			game_level_finished (NULL);
		}
		
		anim_data->timeout_id = -1;
	}
	return 1;
}

/*=================================================================
 
  Selector helper functions

  ---------------------------------------------------------------*/

void selector_set(SelectorData *data, guint row, guint col)
{
	gdouble src_x, src_y;
	gdouble dest_x, dest_y;
	
	if(data!=NULL)
	{
		convert_to_canvas(board_theme, data->row, data->col, &src_x, &src_y);
		convert_to_canvas(board_theme, row, col, &dest_x, &dest_y);
		
		gnome_canvas_item_move(data->item, dest_x - src_x, dest_y - src_y);
		data->row = row;
		data->col = col;
	}
}

void selector_hide(SelectorData *data)
{
	if(data!=NULL)
	{
		gnome_canvas_item_hide(data->item);
	}
}

void selector_show(SelectorData *data)
{
	if(data!=NULL)
	{
		gnome_canvas_item_show(data->item);
	}
}

SelectorData* selector_new()
{
	SelectorData *data;
	GdkPixbuf *pixbuf;
	gdouble x,y;
	
	data = g_malloc(sizeof(SelectorData));	

	pixbuf = theme_get_selector_image(board_theme);

	g_return_val_if_fail(pixbuf != NULL, NULL);

	data->row = playfield_get_n_rows (board_pf)/2;
	data->col = playfield_get_n_cols (board_pf)/2;

	convert_to_canvas(board_theme, data->row, data->col, &x, &y);

	data->item = gnome_canvas_item_new(level_items->selector,
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

	gnome_canvas_item_hide(data->item);

	data->state = NOTHING_SELECTED;

	return data;
}


/*=================================================================
 
  Internal creation functions

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

        board_canvas_items = g_slist_prepend(board_canvas_items, item);
	
	gtk_signal_connect (GTK_OBJECT (item), "event",
			    (GtkSignalFunc) item_event,
			    NULL);      

	return GNOME_CANVAS_ITEM (item);
}

GnomeCanvasItem*
create_message(GnomeCanvas *canvas, GnomeCanvasGroup* group, gchar* text)
{
	GnomeCanvasItem* item;
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
	return item;
}

void
create_hidden_cursor(void)
{
	GdkColor white = {0, 0xffff, 0xffff, 0xffff};
	GdkColor black = {0, 0x0000, 0x0000, 0x0000};
	GdkBitmap *bitmap;
	GdkBitmap *mask;
	gchar data[] = { 0 };
	
	bitmap = gdk_bitmap_create_from_data(NULL, data, 1, 1);
	mask = gdk_bitmap_create_from_data(NULL, data, 1, 1);
	
	hidden_cursor = gdk_cursor_new_from_pixmap(bitmap, mask, 
						   &white, &black, 1, 1);    
}


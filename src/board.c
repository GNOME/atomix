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

#include "board.h"
#include <gnome.h>
#include <math.h>
#include <unistd.h>
#include "goal.h"
#include "undo.h"
#include "callbacks.h"
#include "main.h"
#include "preferences.h"
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
static PlayField        *board = NULL;   /* the actual playfield */
static Goal             *goal = NULL;    /* the goal of this level */
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
GnomeCanvasItem*
create_obstacle (double x, double y, Tile *tile,
		 GnomeCanvasGroup* group);

GnomeCanvasItem*
create_moveable (double x, double y, Tile *tile,
		 GnomeCanvasGroup* group);

GnomeCanvasItem*
create_item (GdkPixbuf* img, double x, double y,
	     GnomeCanvasGroup* group);

GnomeCanvasItem*
create_message(GnomeCanvas *canvas, GnomeCanvasGroup* group, gchar* text);

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
board_init (AtomixApp *app) 
{
	GnomeCanvas *canvas;
	GdkColor color;

	/* Animation Data Setup */
	anim_data = g_malloc(sizeof(AnimData));
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
	level_items = g_malloc(sizeof(LevelItems));

	/* create the canvas groups for the level
	 * (note: the first created group will be the first drawn group)
	 */	
	canvas = GNOME_CANVAS (app->ca_matrix);
	level_items->level = create_group_ref(canvas, NULL);
	level_items->obstacles = create_group_ref(canvas, level_items->level);
	level_items->decor = create_group_ref(canvas, level_items->level);/* not used yet*/
	level_items->moveables = create_group_ref(canvas, level_items->level);
	level_items->selector = create_group_ref(canvas, level_items->level);
	
	/* create canvas group and items for the messages */
	message_items = g_malloc(sizeof(MessageItems));
	message_items->messages = create_group_ref(canvas, NULL);
	message_items->pause = create_message(canvas, message_items->messages,
					      _("Game Paused"));
	message_items->game_over = create_message(canvas, message_items->messages,
						  _("Game Over"));
	message_items->new_game = create_message(canvas, message_items->messages,
						 _("Atomix - Molecule Mind Game"));
	
	/* other initialistions */
	selector_data = NULL;
	color.red = 60928;   /* this is the X11 color "cornsilk2" */
  	color.green = 59392;
	color.blue = 52480;
	set_background_color_ref (GTK_WIDGET (canvas), &color);
	gnome_canvas_item_show(message_items->new_game);	
}

void
board_init_level (Level* l)
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
	undo_free_all_moves();
	
	/* init board */
	board = playfield_copy(l->playfield);
	
	/* init goal */
	goal = goal_new(l->goal);
	
	/* hide 'New Game' message */
	gnome_canvas_item_hide(message_items->new_game);

	/* initalise canvas map */
	if(canvas_map != NULL)
	{
		canvas_map_destroy(canvas_map);
	}
	canvas_map = canvas_map_new();

	/* initialise control */
	selector_data = selector_new();
	if(preferences_get()->keyboard_control)
	{
		board_set_keyboard_control();
	}
	else
	{
		board_set_mouse_control();
	}
	
	/* render level */
	board_render();
	goal_render(goal);
}

void
board_destroy()
{
	if(board) playfield_destroy(board);
	if(goal) goal_destroy(goal);
	if(anim_data) g_free(anim_data);
    
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
}

/*=================================================================
  
  Board functions

  ---------------------------------------------------------------*/

void
board_render(void)
{
	GnomeCanvas *canvas;
	GnomeCanvasItem *item;
	gint row,col,width,height;
	gint tile_width, tile_height;
	gdouble x,y;
	Tile *tile;
	TileType type;
	Theme *theme = get_actual_theme();
		
	/* create canvas items */
	for(row=0; row < board->n_rows; row++) 
	{
		for(col=0; col < board->n_cols; col++)
		{
			tile = playfield_get_tile(board, row, col);
			type = tile_get_type(tile);
			switch(type)
			{
			case TILE_MOVEABLE:
				convert_to_canvas(theme, row, col, &x, &y);
				item = create_moveable(x, y, tile, 
						       level_items->moveables);
				canvas_map_set_item(canvas_map, 
						    row, col, item);
				break;
				
			case TILE_OBSTACLE:
				convert_to_canvas(theme, row, col, &x, &y);
				item = create_obstacle(x, y, tile, 
						       level_items->obstacles);
				break;
				
			case TILE_NONE:
			default:
			}
		}
	}
	
	theme_get_tile_size(theme, &tile_width, &tile_height);
	
	/* center the whole thing */
	width = tile_width * board->n_cols;
	height = tile_height * board->n_rows;
	canvas = GNOME_CANVAS(glade_xml_get_widget (get_gui (), "ca_matrix"));
	set_canvas_dimensions_ref(canvas, width, height);

	/* set background  color*/
	set_background_color_ref(GTK_WIDGET(canvas), 
				 theme_get_background_color(theme));
}

gboolean
board_undo_move()
{
	if((anim_data->timeout_id == -1) &&
	   (mouse_dragging == FALSE))
	{
		UndoMove *move = undo_get_last_move();
		
		if(move != NULL)
		{
			Theme *theme = get_actual_theme();
			gdouble x_src, y_src, x_dest, y_dest;
			gint animstep;
			
			playfield_swap_tiles(board, 
					     move->src_row, 
					     move->src_col, 
					     move->dest_row, 
					     move->dest_col);
			canvas_map_move_item(canvas_map,
					     move->dest_row, 
					     move->dest_col, 
					     move->src_row, 
					     move->src_col);
			if((preferences_get()->keyboard_control) && 
			   (selector_data->state == MOVEABLE_SELECTED))
			{
				selector_set(selector_data, move->src_row, 
					     move->src_col);
			}
			convert_to_canvas(theme, move->src_row, 
					  move->src_col, 
					  &x_src, &y_src);
			convert_to_canvas(theme, move->dest_row, 
					  move->dest_col, &x_dest, 
					  &y_dest);
			
			animstep = theme->animstep;
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
	g_slist_foreach(board_canvas_items, destroy_item, NULL);
	g_slist_free(board_canvas_items);
	board_canvas_items = NULL;

	/* clear board */
	if(board) 
	{
		playfield_destroy(board);
		board = NULL;
	}

	/* clear goal */
	if(goal) 
	{
		goal_clear(goal);
		goal_destroy(goal);
		goal = NULL;
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
	playfield_print(board);
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
	GtkWidget *canvas;
	GdkCursor *cursor;
	
	if(selected_item != NULL)
	{
		gnome_canvas_item_ungrab(selected_item, GDK_CURRENT_TIME);
	}
	
	canvas = GTK_WIDGET(glade_xml_get_widget (get_gui (), "ca_matrix"));
	cursor = gdk_cursor_new(GDK_LEFT_PTR);
	gdk_window_set_cursor(gtk_widget_get_parent_window(canvas),
			      cursor);
	gdk_cursor_destroy(cursor);
}

/*=================================================================
  
  Board atom handling functions

  ---------------------------------------------------------------*/

void board_set_mouse_control(void)
{
	if(get_game_state() != GAME_NOT_RUNNING)
	{
		gnome_canvas_item_hide(GNOME_CANVAS_ITEM(level_items->selector));
		gnome_canvas_item_show(selector_data->item);
		selected_item = NULL;
		selector_data->state = NOTHING_SELECTED;
	}
}

void board_set_keyboard_control(void)
{
	if(get_game_state() != GAME_NOT_RUNNING)
	{
		gint row, col;
		gnome_canvas_item_show(selector_data->item);
		row = board->n_rows/2;
		col = board->n_cols/2;
		selector_set(selector_data, row, col);
		gnome_canvas_item_show(GNOME_CANVAS_ITEM(level_items->selector));
	}
}

gint
item_event (GnomeCanvasItem *item, GdkEvent *event, gpointer data)
{
	GtkWidget *canvas;
	GdkCursor *cursor;
	double x,y,diff_x, diff_y;
	
	static double selected_x, selected_y;
	static gint item_move_counter;

	if(!preferences_get()->mouse_control) return FALSE;
    
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
			if(preferences_get()->hide_cursor)
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
	    if((!preferences_get()->lazy_dragging) &&
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
		    mouse_sensitivity = preferences_get()->mouse_sensitivity;
		    
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
			canvas = GTK_WIDGET(glade_xml_get_widget (get_gui (),
							  "ca_matrix"));
			cursor = gdk_cursor_new(GDK_FLEUR);
			gdk_window_set_cursor(
				gtk_widget_get_parent_window(canvas),
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
	
	if(get_game_state() != GAME_RUNNING) return;
	if(!preferences_get()->keyboard_control) return;

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
			if(new_col < board->n_cols)
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
			if(new_row < board->n_rows)
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
	convert_to_playfield(get_actual_theme(), x1, y1, &src_row, &src_col);
	
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
		tile = playfield_get_tile(board, dest_row, dest_col);
		type = tile_get_type(tile);
	}
	while((tile != NULL) && (type==TILE_NONE));
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

	
		convert_to_canvas(get_actual_theme(), dest_row, dest_col, &new_x1, &new_y1);
		playfield_swap_tiles(board, src_row, src_col, dest_row, dest_col);
		canvas_map_move_item(canvas_map, src_row, src_col, dest_row, dest_col);
		if(preferences_get()->keyboard_control)
		{
			selector_set(selector_data, dest_row, dest_col);
		}
		
		animstep = get_actual_theme()->animstep;
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
		
		if(goal_reached(goal, board, anim_data->dest_row, 
				anim_data->dest_col))
		{
			game_level_finished();
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
	Theme *theme;
	
	if(data!=NULL)
	{
		theme = get_actual_theme();
		convert_to_canvas(theme, data->row, data->col, &src_x, &src_y);
		convert_to_canvas(theme, row, col, &dest_x, &dest_y);
		
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
	Theme *theme;
	GdkPixbuf *image;
	GnomeCanvas *canvas;
	gdouble x,y;
	
	data = g_malloc(sizeof(SelectorData));	
	theme = get_actual_theme();

	image = theme_get_selector_image(theme);

	g_return_val_if_fail(image != NULL, NULL);

	canvas = GNOME_CANVAS(glade_xml_get_widget (get_gui (), "ca_matrix"));

	data->row = board->n_rows/2;
	data->col = board->n_cols/2;

	convert_to_canvas(theme, data->row, data->col, &x, &y);
	data->item = create_item(image, x, y, level_items->selector); 
	gnome_canvas_item_hide(data->item);

	data->state = NOTHING_SELECTED;

	return data;
}


/*=================================================================
 
  Internal creation functions

  ---------------------------------------------------------------*/

GnomeCanvasItem*
create_obstacle (double x, double y, Tile *tile,
		 GnomeCanvasGroup* group)
{
	GdkPixbuf *img = NULL;
	GnomeCanvasItem *item = NULL;
	Theme* theme;
	
	theme = get_actual_theme();
	img = theme_get_tile_image(theme, tile);
	
	if(img) {
		item = create_item (img, x, y, group);
	} 
#ifdef DEBUG
	else {
		g_print("Wall image not found!\n");
	}
#endif    
	
	return item;
}

GnomeCanvasItem*
create_moveable (double x, double y, Tile *tile, 
		 GnomeCanvasGroup* group)
{
	GdkPixbuf *img = NULL;
	GdkPixbuf  *link_img = NULL;
	GnomeCanvasItem *item = NULL;
	GnomeCanvasGroup *moveable_group = NULL;
	Theme *theme;
	GSList *link_images;
	
	theme = get_actual_theme();
	img = theme_get_tile_image(theme, tile);
	link_images = theme_get_tile_link_images(theme, tile);
		
	if(img) 
	{
		gint i;

		/* create new group at (0,0) */
		moveable_group = create_group("ca_matrix", group);

		/* add connection and moveable images */
		if(link_images)
		{
			gint length = g_slist_length(link_images);
			for(i = 0; i < length; i++)
			{
				link_img = (GdkPixbuf*)g_slist_nth_data(link_images, i);
				item = create_item(link_img, 0.0, 0.0, moveable_group);
			}
		}
		item = create_item (img, 0.0, 0.0, moveable_group);

		/* move to the right location */
		gnome_canvas_item_move(GNOME_CANVAS_ITEM(moveable_group), x, y);
		
		gtk_signal_connect (GTK_OBJECT (moveable_group), "event",
				    (GtkSignalFunc) item_event,
				    NULL);      
	} 
#ifdef DEBUG
	else {
		g_print("Atom image not found!\n");
	}
#endif    
	g_slist_free(link_images);

	return GNOME_CANVAS_ITEM(moveable_group);
}

GnomeCanvasItem*
create_item (GdkPixbuf* img, double x, double y,
	     GnomeCanvasGroup* group)
{
	GnomeCanvasItem *item;
	
	item = gnome_canvas_item_new(group,
				     gnome_canvas_pixbuf_get_type(),
				     "pixbuf", img,
				     "x", x,
				     "x_in_pixels", TRUE,
				     "y", y,
				     "y_in_pixels", TRUE,
				     "width", (double) gdk_pixbuf_get_width (img),
				     "height", (double) gdk_pixbuf_get_height (img),
				     "anchor", GTK_ANCHOR_NW,
				     NULL);                              

        board_canvas_items = g_slist_prepend(board_canvas_items, item);
	return item;
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


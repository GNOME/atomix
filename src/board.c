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

#define                  ANIM_TIMEOUT     10	/* time in milliseconds between 
						   two atom movements */
typedef struct
{
  gint timeout_id;
  gint counter;
  gint dest_row;
  gint dest_col;
  double x_step;
  double y_step;
} AnimData;

typedef struct
{
  guint row;
  guint col;
  gboolean selected;
  gint arrow_show_timeout;
  gint mouse_steering;
  GnomeCanvasItem *sel_item;
  GnomeCanvasItem *selector;
  GnomeCanvasGroup *arrows;
  GnomeCanvasItem *arrow_left;
  GnomeCanvasItem *arrow_right;
  GnomeCanvasItem *arrow_top;
  GnomeCanvasItem *arrow_bottom;
} SelectorData;

typedef struct
{
  GnomeCanvasGroup *level;
  GnomeCanvasGroup *obstacles;
  GnomeCanvasGroup *moveables;
  GnomeCanvasGroup *floor;
  GnomeCanvasGroup *shadows;
  GnomeCanvasItem *logo;
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
extern AtomixApp *app;
static Theme *board_theme = NULL;
static GnomeCanvas *board_canvas = NULL;
static PlayField *board_env = NULL;	/* the actual playfield */
static PlayField *board_sce = NULL;	/* the actual playfield */
static PlayField *board_shadow = NULL;	/* the shadow positions */
static Goal *board_goal = NULL;	/* the goal of this level */

static AnimData *anim_data;	/* holds the date for the atom 
				   animation */
static GSList *board_canvas_items = NULL;	/* a list of all used 
						   canvas items */
static LevelItems *level_items;	/* references to the level groups */

static SelectorData *selector_data;	/* data about the selector */

/*=================================================================
 
  Declaration of internal functions

  ---------------------------------------------------------------*/
GnomeCanvasItem *create_tile (double x, double y, Tile * tile,
			      GnomeCanvasGroup * group);

void board_render (void);

static void render_tile (Tile *tile, gint row, gint col);

gint item_event (GnomeCanvasItem *item, GdkEvent *event, gpointer data);

void move_item (GnomeCanvasItem *item, ItemDirection direc);

int move_item_anim (void *data);

static void selector_move_to (SelectorData *data, guint row, guint col);
static void selector_unselect (SelectorData *data);
static void selector_select (SelectorData *data, GnomeCanvasItem *item);
static SelectorData *selector_create (void);
static void selector_hide (SelectorData *data);
static void selector_show (SelectorData *data);
static void selector_arrows_hide (SelectorData *data);

/*=================================================================
 
  Board creation, initialisation and clean up

  ---------------------------------------------------------------*/

#define BGR_FLOOR_ROWS 15
#define BGR_FLOOR_COLS 15

static void create_background_floor (void)
{
  int row, col;
  Tile *tile;
  GQuark quark;
  int tile_width, tile_height;
  double x, y;
  GdkPixbuf *pixbuf = NULL;
  GnomeCanvasItem *item;
  int ca_width, ca_height;
  int width, height;

  quark = g_quark_from_static_string ("floor");
  theme_get_tile_size (board_theme, &tile_width, &tile_height);

  tile = tile_new (TILE_TYPE_FLOOR);
  tile_set_base_id (tile, quark);
  pixbuf = theme_get_tile_image (board_theme, tile);
  g_object_unref (tile);

  for (row = 0; row < BGR_FLOOR_ROWS; row++)
    for (col = 0; col < BGR_FLOOR_COLS; col++)
      {
	x = col * tile_width;
	y = row * tile_height;

	item = gnome_canvas_item_new (level_items->floor,
				      gnome_canvas_pixbuf_get_type (),
				      "pixbuf", pixbuf, "x", x, "x_in_pixels",
				      TRUE, "y", y, "y_in_pixels", TRUE,
				      "width",
				      (double) gdk_pixbuf_get_width (pixbuf),
				      "height",
				      (double) gdk_pixbuf_get_height (pixbuf),
				      "anchor", GTK_ANCHOR_NW, NULL);
      }

  g_object_unref (pixbuf);

  /* center the whole thing */
  ca_width = GTK_WIDGET (board_canvas)->allocation.width;
  ca_height = GTK_WIDGET (board_canvas)->allocation.height;

  width = tile_width * BGR_FLOOR_COLS;
  height = tile_height * BGR_FLOOR_ROWS;

  if (width > ca_width)
    {
      x = (width / 2) - (ca_width / 2);
      width = ca_width;
    }
  else
    x = 0;

  if (height > ca_height)
    {
      y = (height / 2) - (ca_height / 2);
      height = ca_height;
    }
  else
    y = 0;

  gnome_canvas_set_scroll_region (board_canvas, x, y, width + x, height + y);

  /* set background  color */
  set_background_color (GTK_WIDGET (board_canvas),
			theme_get_background_color (board_theme));
}

static void create_logo (void)
{
  GdkPixbuf *pixbuf;
  double x1, y1, x2, y2;

  pixbuf = gdk_pixbuf_new_from_file (DATADIR "/atomix/atomix-logo.png", NULL);
  gnome_canvas_item_get_bounds (GNOME_CANVAS_ITEM (level_items->floor),
				&x1, &y1, &x2, &y2);

  level_items->logo = gnome_canvas_item_new (level_items->floor,
					     gnome_canvas_pixbuf_get_type (),
					     "pixbuf", pixbuf,
					     "x", (x2 - x1) / 2,
					     "x_in_pixels", TRUE,
					     "y", (y2 - y1) / 2,
					     "y_in_pixels", TRUE,
					     "width",
					     (double)
					     gdk_pixbuf_get_width (pixbuf),
					     "height",
					     (double)
					     gdk_pixbuf_get_height (pixbuf),
					     "anchor", GTK_ANCHOR_CENTER,
					     NULL);
  gnome_canvas_item_raise_to_top (GNOME_CANVAS_ITEM (level_items->logo));

  g_object_unref (pixbuf);
}

void board_init (Theme *theme, GnomeCanvas *canvas)
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
  level_items->floor = create_group (canvas, NULL);
  level_items->level = create_group (canvas, NULL);
  level_items->obstacles = create_group (canvas, level_items->level);
  level_items->moveables = create_group (canvas, level_items->level);
  level_items->shadows = create_group (canvas, level_items->level);

  g_signal_connect (G_OBJECT (canvas), "key_press_event",
		    G_CALLBACK (board_handle_key_event), NULL);

  /* other initialistions */
  board_canvas = canvas;
  g_object_ref (theme);
  board_theme = theme;
  board_env = NULL;
  board_sce = NULL;
  board_goal = NULL;

  create_background_floor ();
  create_logo ();
  selector_data = selector_create ();
}

void board_init_level (PlayField *base_env, PlayField *sce, Goal *goal)
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
  board_env = playfield_generate_environment (base_env, board_theme);
  board_sce = playfield_copy (sce);
  board_shadow = playfield_generate_shadow (base_env);

  /* init goal */
  board_goal = g_object_ref (goal);

  row = playfield_get_n_rows (board_env) / 2;
  col = playfield_get_n_cols (board_env) / 2;
  selector_move_to (selector_data, row, col);
  selector_unselect (selector_data);

  /* render level */
  board_render ();
  board_show ();
}

void board_destroy ()
{
  if (board_env)
    g_object_unref (board_env);
  if (board_sce)
    g_object_unref (board_sce);
  if (anim_data)
    g_free (anim_data);

  undo_clear ();

  if (level_items)
    {
      if (level_items->level)
	gtk_object_destroy (GTK_OBJECT (level_items->level));

      g_free (level_items);
    }

  if (selector_data)
    g_free (selector_data);

  if (board_theme)
    g_object_unref (board_theme);

  if (board_goal)
    g_object_unref (board_goal);
}

/*=================================================================
  
  Board functions

  ---------------------------------------------------------------*/

void board_render ()
{
  gint row, col;
  gint tile_width, tile_height;
  Tile *tile;
  double x_offset, y_offset;
  double dst[6];

  g_return_if_fail (board_theme != NULL);

  /* render one row more than the actual environment because
     of the shadow, which is one row/col larger */

  for (row = 0; row <= playfield_get_n_rows (board_env); row++)
    {
      for (col = 0; col <= playfield_get_n_cols (board_env); col++)
	{
	  if (row < playfield_get_n_rows (board_env) &&
	      col < playfield_get_n_cols (board_env))
	    {
	      tile = playfield_get_tile (board_sce, row, col);
	      if (tile != NULL)
		{
		  render_tile (tile, row, col);
		  g_object_unref (tile);
		}

	      tile = playfield_get_tile (board_env, row, col);

	      if (tile != NULL)
		{
		  render_tile (tile, row, col);
		  if (tile_get_tile_type (tile) == TILE_TYPE_WALL)
		    playfield_set_tile (board_sce, row, col, tile);

		  g_object_unref (tile);
		}
	    }

	  tile = playfield_get_tile (board_shadow, row, col);
	  if (tile != NULL)
	    {
	      render_tile (tile, row, col);
	      g_object_unref (tile);
	    }
	}
    }

  theme_get_tile_size (board_theme, &tile_width, &tile_height);

  x_offset =
    tile_width * (BGR_FLOOR_COLS / 2 - playfield_get_n_cols (board_env) / 2);
  y_offset =
    tile_height * (BGR_FLOOR_ROWS / 2 - playfield_get_n_rows (board_env) / 2);

  art_affine_translate (dst, x_offset, y_offset);

  gnome_canvas_item_affine_absolute (GNOME_CANVAS_ITEM (level_items->level),
				     dst);
}

static void render_tile (Tile *tile, gint row, gint col)
{
  GnomeCanvasItem *item;
  GnomeCanvasGroup *group;
  TileType type;
  gdouble x, y;

  type = tile_get_tile_type (tile);
  switch (type)
    {
    case TILE_TYPE_ATOM:
      group = level_items->moveables;
      break;

    case TILE_TYPE_WALL:
      group = level_items->obstacles;
      break;

    case TILE_TYPE_FLOOR:
      group = NULL;		/* level_items->floor; */
      break;

    case TILE_TYPE_SHADOW:
      group = level_items->shadows;
      break;

    case TILE_TYPE_UNKNOWN:
    default:
      group = NULL;
    }

  if (group != NULL)
    {
      convert_to_canvas (board_theme, row, col, &x, &y);
      item = create_tile (x, y, tile, group);
    }
}

gboolean board_undo_move ()
{
  UndoMove *move;
  gdouble x_src, y_src, x_dest, y_dest;
  gint animstep;

  g_return_val_if_fail (board_theme != NULL, FALSE);

  if (anim_data->timeout_id != -1)
    return FALSE;

  move = undo_pop_move ();
  if (move == NULL)
    return FALSE;

  playfield_swap_tiles (board_sce,
			move->src_row,
			move->src_col, move->dest_row, move->dest_col);

  if (selector_data->selected)
    {
      selector_hide (selector_data);
      selector_move_to (selector_data, move->src_row, move->src_col);
    }
  convert_to_canvas (board_theme, move->src_row,
		     move->src_col, &x_src, &y_src);
  convert_to_canvas (board_theme, move->dest_row,
		     move->dest_col, &x_dest, &y_dest);

  animstep = theme_get_animstep (board_theme);
  if (move->src_col == move->dest_col)
    {
      anim_data->counter = (gint) (fabs (y_dest - y_src) / animstep);
      anim_data->x_step = 0;
      anim_data->y_step = animstep;
      if (move->src_row < move->dest_row)
	{
	  anim_data->y_step = -(anim_data->y_step);
	}
    }
  else
    {
      anim_data->counter = (gint) (fabs (x_dest - x_src) / animstep);
      anim_data->x_step = animstep;
      anim_data->y_step = 0;
      if (move->src_col < move->dest_col)
	{
	  anim_data->x_step = -(anim_data->x_step);
	}
    }

  anim_data->dest_row = move->src_row;
  anim_data->dest_col = move->src_col;
  selector_data->sel_item = move->item;

  anim_data->timeout_id = gtk_timeout_add (ANIM_TIMEOUT,
					   move_item_anim, anim_data);
  g_free (move);

  return TRUE;
}

void board_clear ()
{
  g_slist_foreach (board_canvas_items, (GFunc) gtk_object_destroy, NULL);
  g_slist_free (board_canvas_items);
  board_canvas_items = NULL;

  /* clear board */
  if (board_env)
    {
      g_object_unref (board_env);
      board_env = NULL;
    }
  if (board_sce)
    {
      g_object_unref (board_sce);
      board_sce = NULL;
    }
  if (board_goal)
    {
      g_object_unref (board_goal);
      board_goal = NULL;
    }
  if (board_shadow)
    {
      g_object_unref (board_shadow);
      board_shadow = NULL;
    }

  selector_hide (selector_data);
}

void board_print ()
{
  g_print ("Board:\n");
  playfield_print (board_env);
}

void board_hide (void)
{
  gnome_canvas_item_hide (GNOME_CANVAS_ITEM (level_items->moveables));
  gnome_canvas_item_hide (GNOME_CANVAS_ITEM (selector_data->arrows));
}

void board_show (void)
{
  gnome_canvas_item_show (GNOME_CANVAS_ITEM (level_items->moveables));
  if (undo_exists())
    gnome_canvas_item_show (GNOME_CANVAS_ITEM (selector_data->arrows));
}

void board_show_logo (gboolean visible)
{
  if (visible)
    gnome_canvas_item_show (GNOME_CANVAS_ITEM (level_items->logo));

  else
    gnome_canvas_item_hide (GNOME_CANVAS_ITEM (level_items->logo));
}

/*=================================================================
  
  Board atom handling functions

  ---------------------------------------------------------------*/

static GnomeCanvasItem *get_item_by_row_col (gint row, gint col)
{
  gint width, height;
  ArtPoint item_point;
  ArtPoint world_point;
  double affine[6];

  convert_to_canvas (board_theme, row, col, &item_point.x, &item_point.y);
  theme_get_tile_size (board_theme, &width, &height);

  item_point.x = item_point.x + (width / 2);
  item_point.y = item_point.y + (height / 2);

  gnome_canvas_item_i2w_affine (GNOME_CANVAS_ITEM (level_items->level),
				affine);
  art_affine_point (&world_point, &item_point, affine);

  return gnome_canvas_get_item_at (board_canvas, world_point.x,
				   world_point.y);
}

static void get_row_col_by_item (GnomeCanvasItem *item, guint *row, guint *col)
{
  gdouble x1, y1, x2, y2;

  g_return_if_fail (GNOME_IS_CANVAS_ITEM (item));

  gnome_canvas_item_get_bounds (item, &x1, &y1, &x2, &y2);

  convert_to_playfield (board_theme, x1, y1, row, col);
}

static gboolean board_handle_arrow_event (GnomeCanvasItem *item,
					  GdkEvent *event, gpointer direction)
{
  /* is currently an object moved? */
  if (anim_data->timeout_id != -1)
    return FALSE;

  if (event->type == GDK_BUTTON_PRESS && selector_data->selected)
    {
      selector_data->mouse_steering = TRUE;
      move_item (selector_data->sel_item, GPOINTER_TO_INT (direction));

      return TRUE;
    }

  return FALSE;
}

static gboolean board_handle_item_event (GnomeCanvasItem *item,
					 GdkEvent *event, gpointer data)
{
  gboolean just_unselect;
  guint new_row, new_col;

  /* is currently an object moved? */
  if (anim_data->timeout_id != -1)
    return FALSE;

  if (event->type == GDK_BUTTON_PRESS)
    {
      selector_data->mouse_steering = TRUE;
      just_unselect = (item == selector_data->sel_item);

      if (selector_data->selected)
	{
	  /* unselect item, show selector image */
	  selector_unselect (selector_data);
	}

      if (!just_unselect)
	{
	  get_row_col_by_item (item, &new_row, &new_col);

	  selector_move_to (selector_data, new_row, new_col);
	  selector_select (selector_data, item);
	}

      return TRUE;
    }
  return FALSE;
}

void board_handle_key_event (GObject *canvas, GdkEventKey *event,
			     gpointer data)
{
  GnomeCanvasItem *item;
  gint new_row, new_col;
  Tile *tile;

  g_return_if_fail (selector_data != NULL);

  new_row = selector_data->row;
  new_col = selector_data->col;

  /* is currently an object moved? */
  if (anim_data->timeout_id != -1)
    return;

  switch (event->keyval)
    {
    case GDK_Return:
      selector_data->mouse_steering = FALSE;
      if (selector_data->selected)
	{
	  /* unselect item, show selector image */
	  selector_unselect (selector_data);
	}
      else
	{
	  item = get_item_by_row_col (selector_data->row, selector_data->col);
	  if (item == NULL)
	    break;
	  if (g_object_get_data (G_OBJECT (item), "tile") == NULL)
	    break;

	  tile = TILE (g_object_get_data (G_OBJECT (item), "tile"));

	  if (tile_get_tile_type (tile) == TILE_TYPE_ATOM)
	    selector_select (selector_data, item);
	}
      break;

    case GDK_Left:
      selector_data->mouse_steering = FALSE;
      if (!selector_data->selected)
	{
	  new_col--;
	  if (new_col >= 0)
	    {
	      selector_show (selector_data);
	      selector_move_to (selector_data, new_row, new_col);
	    }
	}
      else
	{
	  move_item (selector_data->sel_item, LEFT);	/* selector will be
							   moved in this
							   function */
	}
      break;

    case GDK_Right:
      selector_data->mouse_steering = FALSE;
      if (!selector_data->selected)
	{
	  new_col++;
	  if (new_col < playfield_get_n_cols (board_env))
	    {
	      selector_show (selector_data);
	      selector_move_to (selector_data, new_row, new_col);
	    }
	}
      else
	{
	  move_item (selector_data->sel_item, RIGHT);	/* selector will be
							   moved in this
							   function */
	}
      break;

    case GDK_Up:
      selector_data->mouse_steering = FALSE;
      if (!selector_data->selected)
	{
	  new_row--;
	  if (new_row >= 0)
	    {
	      selector_show (selector_data);
	      selector_move_to (selector_data, new_row, new_col);
	    }
	}
      else
	{
	  move_item (selector_data->sel_item, UP);	/* selector will be moved
							   in this function */
	}
      break;

    case GDK_Down:
      selector_data->mouse_steering = FALSE;
      if (!selector_data->selected)
	{
	  new_row++;
	  if (new_row < playfield_get_n_rows (board_env))
	    {
	      selector_show (selector_data);
	      selector_move_to (selector_data, new_row, new_col);
	    }
	}
      else
	{
	  move_item (selector_data->sel_item, DOWN);	/* selector will be
							   moved in this
							   function */
	}
      break;

    default:
      break;
    }
}

void move_item (GnomeCanvasItem *item, ItemDirection direc)
{
  gdouble x1, y1, x2, y2;
  gdouble new_x1, new_y1;
  guint src_row, src_col, dest_row, dest_col, tmp_row, tmp_col;
  gint animstep;
  Tile *tile;
  gint tw, th;

  gnome_canvas_item_get_bounds (item, &x1, &y1, &x2, &y2);
  theme_get_tile_size (board_theme, &tw, &th);
  x1 = x1 - (((gint) x1) % tw);	/* I don't have a clue why we must do
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
	  tmp_row = tmp_row - 1;
	  break;

	case DOWN:
	  tmp_row = tmp_row + 1;
	  break;

	case LEFT:
	  tmp_col = tmp_col - 1;
	  break;

	case RIGHT:
	  tmp_col = tmp_col + 1;
	  break;
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

      dest_row = tmp_row;
      dest_col = tmp_col;
      if (tile)
	g_object_unref (tile);
    }

  /* move the item, if the new position is different */
  if (src_row != dest_row || src_col != dest_col)
    {
      if (!undo_exists())
	{
	  app->state = GAME_STATE_RUNNING;
	  update_menu_item_state (app);
	}

      undo_push_move (item, src_row, src_col, dest_row, dest_col);

      convert_to_canvas (board_theme, dest_row, dest_col, &new_x1, &new_y1);
      playfield_swap_tiles (board_sce, src_row, src_col, dest_row, dest_col);

      selector_hide (selector_data);
      selector_move_to (selector_data, dest_row, dest_col);

      animstep = theme_get_animstep (board_theme);
      if (direc == UP || direc == DOWN)
	{
	  anim_data->counter = (gint) (fabs (new_y1 - y1) / animstep);
	  anim_data->x_step = 0;
	  anim_data->y_step = (direc == DOWN) ? animstep : -animstep;
	}
      else
	{
	  anim_data->counter = (gint) (fabs (new_x1 - x1) / animstep);
	  anim_data->x_step = (direc == RIGHT) ? animstep : -animstep;
	  anim_data->y_step = 0;
	}

      anim_data->dest_row = dest_row;
      anim_data->dest_col = dest_col;

      anim_data->timeout_id = gtk_timeout_add (ANIM_TIMEOUT, move_item_anim,
					       anim_data);
    }
}

int move_item_anim (void *data)
{
  AnimData *anim_data = (AnimData *) data;

  if (anim_data->counter > 0)
    {
      gnome_canvas_item_move (selector_data->sel_item,
			      anim_data->x_step, anim_data->y_step);
      anim_data->counter--;

      return TRUE;
    }

  else
    {
      anim_data->timeout_id = -1;

      if (goal_reached (board_goal, board_sce, anim_data->dest_row,
			anim_data->dest_col))
	{
	  game_level_finished (NULL);
	}

      else
	{
	  if (selector_data->selected)
	    selector_select (selector_data, selector_data->sel_item);
	}

      return FALSE;
    }
}

/*=================================================================
 
  Selector helper functions

  ---------------------------------------------------------------*/

static void selector_move_to (SelectorData *data, guint row, guint col)
{
  ArtPoint dest;
  ArtPoint dest_world;
  gint tile_width, tile_height;
  double affine[6];
  double translate[6];

  g_return_if_fail (data != NULL);

  if (data->arrow_show_timeout > -1)
    gtk_timeout_remove (data->arrow_show_timeout);

  data->arrow_show_timeout = -1;

  theme_get_tile_size (board_theme, &tile_width, &tile_height);

  convert_to_canvas (board_theme, row, col, &dest.x, &dest.y);
  art_affine_point (&dest_world, &dest, affine);

  art_affine_translate (translate, dest.x, dest.y);
  gnome_canvas_item_affine_absolute (data->selector, translate);
  art_affine_translate (translate, dest.x - tile_width, dest.y - tile_height);
  gnome_canvas_item_affine_absolute (GNOME_CANVAS_ITEM (data->arrows),
				     translate);

  data->row = row;
  data->col = col;
}

static void selector_unselect (SelectorData *data)
{
  g_return_if_fail (data != NULL);

  data->selected = FALSE;
  data->sel_item = NULL;
  if (!data->mouse_steering)
    {
      gnome_canvas_item_show (data->selector);
    }
  selector_arrows_hide (data);
}

static void selector_arrows_hide (SelectorData *data)
{
  if (data->arrow_show_timeout > -1)
    gtk_timeout_remove (data->arrow_show_timeout);
  data->arrow_show_timeout = -1;
  gnome_canvas_item_hide (GNOME_CANVAS_ITEM (data->arrows));
}

static void selector_hide (SelectorData *data)
{
  gnome_canvas_item_hide (data->selector);
  selector_arrows_hide (data);
}

static void selector_show (SelectorData *data)
{
  gnome_canvas_item_show (data->selector);
}

static gboolean show_arrow_group (SelectorData *data)
{
  gnome_canvas_item_show (GNOME_CANVAS_ITEM (data->arrows));
  data->arrow_show_timeout = -1;

  return FALSE;
}

static void selector_arrows_show (SelectorData *data)
{
  gint r, c;
  Tile *tile;

  if (board_sce == NULL)
    {
      gnome_canvas_item_hide (GNOME_CANVAS_ITEM (data->arrows));
      return;
    }

  r = data->row - 1;
  c = data->col;
  if (r >= 0)
    {
      tile = playfield_get_tile (board_sce, r, c);

      if (tile == NULL)
	gnome_canvas_item_show (data->arrow_top);

      else
	{
	  gnome_canvas_item_hide (data->arrow_top);
	  g_object_unref (tile);
	}
    }

  r = data->row;
  c = data->col + 1;

  if (c < playfield_get_n_cols (board_sce))
    {
      tile = playfield_get_tile (board_sce, r, c);

      if (tile == NULL)
	gnome_canvas_item_show (data->arrow_right);
      else
	{
	  gnome_canvas_item_hide (data->arrow_right);
	  g_object_unref (tile);
	}
    }

  r = data->row + 1;
  c = data->col;

  if (r < playfield_get_n_rows (board_sce))
    {
      tile = playfield_get_tile (board_sce, r, c);

      if (tile == NULL)
	gnome_canvas_item_show (data->arrow_bottom);
      else
	{
	  gnome_canvas_item_hide (data->arrow_bottom);
	  g_object_unref (tile);
	}
    }

  r = data->row;
  c = data->col - 1;

  if (c >= 0)
    {
      tile = playfield_get_tile (board_sce, r, c);

      if (tile == NULL)
	gnome_canvas_item_show (data->arrow_left);

      else
	{
	  gnome_canvas_item_hide (data->arrow_left);
	  g_object_unref (tile);
	}
    }

  if (data->mouse_steering)
    {
      show_arrow_group (data);
    }

  else
    {
      if (data->arrow_show_timeout > -1)
	gtk_timeout_remove (data->arrow_show_timeout);

      data->arrow_show_timeout =
	gtk_timeout_add (800, (GtkFunction) show_arrow_group, data);
    }
}

static void selector_select (SelectorData *data, GnomeCanvasItem *item)
{
  g_return_if_fail (data != NULL);

  data->selected = TRUE;
  data->sel_item = item;

  gnome_canvas_item_hide (data->selector);
  selector_arrows_show (data);
}

static SelectorData *selector_create (void)
{
  SelectorData *data;
  GdkPixbuf *pixbuf;
  GdkPixbuf *sel_arrows[4];
  gint tile_width, tile_height;

  data = g_new0 (SelectorData, 1);

  pixbuf = theme_get_selector_image (board_theme);
  theme_get_selector_arrow_images (board_theme, &sel_arrows[0]);
  theme_get_tile_size (board_theme, &tile_width, &tile_height);

  g_return_val_if_fail (pixbuf != NULL, NULL);

  data->row = 0;
  data->col = 0;
  data->sel_item = NULL;
  data->selected = FALSE;
  data->arrow_show_timeout = -1;
  data->mouse_steering = FALSE;

  data->selector = gnome_canvas_item_new (level_items->level,
					  gnome_canvas_pixbuf_get_type (),
					  "pixbuf", pixbuf,
					  "x", 0.0,
					  "x_in_pixels", TRUE,
					  "y", 0.0,
					  "y_in_pixels", TRUE,
					  "width",
					  (double)
					  gdk_pixbuf_get_width (pixbuf),
					  "height",
					  (double)
					  gdk_pixbuf_get_height (pixbuf),
					  "anchor", GTK_ANCHOR_NW, NULL);
  data->arrows =
    GNOME_CANVAS_GROUP (gnome_canvas_item_new
			(level_items->level, gnome_canvas_group_get_type (),
			 "x", (double) -tile_width, "y",
			 (double) -tile_height, NULL));

  data->arrow_top = gnome_canvas_item_new (GNOME_CANVAS_GROUP (data->arrows),
					   gnome_canvas_pixbuf_get_type (),
					   "pixbuf", sel_arrows[0],
					   "x", (double) 1 * tile_width,
					   "x_in_pixels", TRUE,
					   "y", (double) 0 * tile_height,
					   "y_in_pixels", TRUE,
					   "anchor", GTK_ANCHOR_NW, NULL);
  g_signal_connect (G_OBJECT (data->arrow_top), "event",
		    G_CALLBACK (board_handle_arrow_event),
		    GINT_TO_POINTER (UP));

  data->arrow_right =
    gnome_canvas_item_new (GNOME_CANVAS_GROUP (data->arrows),
			   gnome_canvas_pixbuf_get_type (), "pixbuf",
			   sel_arrows[1], "x", (double) 2 * tile_width,
			   "x_in_pixels", TRUE, "y", (double) 1 * tile_height,
			   "y_in_pixels", TRUE, "anchor", GTK_ANCHOR_NW,
			   NULL);
  g_signal_connect (G_OBJECT (data->arrow_right), "event",
		    G_CALLBACK (board_handle_arrow_event),
		    GINT_TO_POINTER (RIGHT));

  data->arrow_bottom =
    gnome_canvas_item_new (GNOME_CANVAS_GROUP (data->arrows),
			   gnome_canvas_pixbuf_get_type (), "pixbuf",
			   sel_arrows[2], "x", (double) 1 * tile_width,
			   "x_in_pixels", TRUE, "y", (double) 2 * tile_height,
			   "y_in_pixels", TRUE, "anchor", GTK_ANCHOR_NW,
			   NULL);

  g_signal_connect (G_OBJECT (data->arrow_bottom), "event",
		    G_CALLBACK (board_handle_arrow_event),
		    GINT_TO_POINTER (DOWN));

  data->arrow_left = gnome_canvas_item_new (GNOME_CANVAS_GROUP (data->arrows),
					    gnome_canvas_pixbuf_get_type (),
					    "pixbuf", sel_arrows[3],
					    "x", (double) 0 * tile_width,
					    "x_in_pixels", TRUE,
					    "y", (double) 1 * tile_height,
					    "y_in_pixels", TRUE,
					    "anchor", GTK_ANCHOR_NW, NULL);
  g_signal_connect (G_OBJECT (data->arrow_left), "event",
		    G_CALLBACK (board_handle_arrow_event),
		    GINT_TO_POINTER (LEFT));

  gnome_canvas_item_hide (GNOME_CANVAS_ITEM (data->selector));
  gnome_canvas_item_hide (GNOME_CANVAS_ITEM (data->arrows));

  return data;
}

/*=================================================================
 
  Internal creation functions

  ---------------------------------------------------------------*/

GnomeCanvasItem *create_tile (double x, double y,
			      Tile *tile, GnomeCanvasGroup *group)
{
  GdkPixbuf *pixbuf = NULL;
  GnomeCanvasItem *item = NULL;

  pixbuf = theme_get_tile_image (board_theme, tile);

  item = gnome_canvas_item_new (group,
				gnome_canvas_pixbuf_get_type (),
				"pixbuf", pixbuf,
				"x", x,
				"x_in_pixels", TRUE,
				"y", y,
				"y_in_pixels", TRUE,
				"width",
				(double) gdk_pixbuf_get_width (pixbuf),
				"height",
				(double) gdk_pixbuf_get_height (pixbuf),
				"anchor", GTK_ANCHOR_NW, NULL);

  g_object_set_data (G_OBJECT (item), "tile", tile);

  if (tile_get_tile_type (tile) == TILE_TYPE_ATOM)
    {
      g_signal_connect (G_OBJECT (item), "event",
			G_CALLBACK (board_handle_item_event), NULL);
    }

  board_canvas_items = g_slist_prepend (board_canvas_items, item);

  return GNOME_CANVAS_ITEM (item);
}

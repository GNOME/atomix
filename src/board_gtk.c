/* Atomix -- a little puzzle game about atoms and molecules.
 * Copyright (C) 1999 Jens Finke
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

#include "board_gtk.h"

#define BGR_FLOOR_ROWS 15
#define BGR_FLOOR_COLS 15

#define ANIM_TIMEOUT     10	/* time in milliseconds between 
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

typedef enum
{
  UP,
  DOWN,
  LEFT,
  RIGHT
} ItemDirection;

typedef struct
{
  GSList *level;
  GSList *obstacles;
  GSList *moveables;
  GSList *shadows;
} LevelItems;


/* Static declarations, to be removed */
static GtkFixed *board_canvas = NULL;
static Theme *board_theme = NULL;
static PlayField *board_env = NULL;	/* the actual playfield */
static PlayField *board_sce = NULL;	/* the actual playfield */
static PlayField *board_shadow = NULL;	/* the shadow positions */
static AnimData *anim_data;	/* holds the date for the atom 
                               animation */
static GSList *board_canvas_items = NULL;	/* a list of all used  */
static Goal *board_goal = NULL;	/* the goal of this level */
static LevelItems *level_items;	/* references to the level groups */


/* Forward declarations of internal functions */
void board_gtk_render (void);
static void render_tile (Tile *tile, gint row, gint col);
GtkImage* create_tile (double x, double y, Tile *tile);


/* Function implementations */
static void create_background_floor (void)
{
  int row, col;
  Tile *tile;
  GQuark quark;
  int tile_width, tile_height;
  double x, y;
  GdkPixbuf *pixbuf = NULL;
  GtkWidget *item;
  int ca_width, ca_height;
  int width, height;
  GtkAllocation allocation;

  quark = g_quark_from_static_string ("floor");
  theme_get_tile_size (board_theme, &tile_width, &tile_height);

  tile = tile_new (TILE_TYPE_FLOOR);
  tile_set_base_id (tile, quark);
  pixbuf = theme_get_tile_image (board_theme, tile);
  g_object_unref (tile);

  for (row = 0; row < BGR_FLOOR_ROWS; row++)
    for (col = 0; col < BGR_FLOOR_COLS; col++) {
      x = col * tile_width;
      y = row * tile_height;

      item = gtk_image_new_from_pixbuf (pixbuf);
      gtk_fixed_put (GTK_FIXED (board_canvas), item, x, y);
    }

  g_object_unref (pixbuf);

  /* center the whole thing */
  gtk_widget_get_allocation (GTK_WIDGET (board_canvas), &allocation);
  ca_width = allocation.width;
  ca_height = allocation.height;

  width = tile_width * BGR_FLOOR_COLS;
  height = tile_height * BGR_FLOOR_ROWS;

  if (width > ca_width) {
    x = (width / 2) - (ca_width / 2);
    width = ca_width;
  } else
    x = 0;

  if (height > ca_height) {
    y = (height / 2) - (ca_height / 2);
    height = ca_height;
  } else
    y = 0;
}

void board_gtk_init (Theme * theme, gpointer canvas)
{
  board_theme = theme;
  board_canvas = canvas;
  g_object_ref (theme);
  board_theme = theme;
  board_env = NULL;
  board_sce = NULL;
  board_goal = NULL;

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

  g_signal_connect (G_OBJECT (canvas), "key_press_event",
                    G_CALLBACK (board_gtk_handle_key_event), NULL);

  create_background_floor ();
  gtk_widget_show_all (GTK_WIDGET(board_canvas));
}

void board_gtk_render () {
  gint row, col;
  Tile *tile;

  g_return_if_fail (board_theme != NULL);

  /* render one row more than the actual environment because
     of the shadow, which is one row/col larger */

  for (row = 0; row <= playfield_get_n_rows (board_env); row++) {
    for (col = 0; col <= playfield_get_n_cols (board_env); col++) {
      if (row < playfield_get_n_rows (board_env) &&
          col < playfield_get_n_cols (board_env)) {
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

      tile = playfield_get_tile (board_shadow, row, col);
      if (tile != NULL) {
        render_tile (tile, row, col);
        g_object_unref (tile);
      }
    }
  }
}

static void render_tile (Tile *tile, gint row, gint col) {
  gboolean create = FALSE;
  TileType type;
  gint row_offset, col_offset;
  gdouble x, y;

  type = tile_get_tile_type (tile);
  switch (type) {
    case TILE_TYPE_ATOM:
      create = TRUE;
      break;

    case TILE_TYPE_WALL:
      create = TRUE;
      break;

    case TILE_TYPE_SHADOW:
      create = TRUE;
      break;

    case TILE_TYPE_UNKNOWN:
    case TILE_TYPE_FLOOR:
    default:
      break;
  }


  if (create) {
    row_offset = BGR_FLOOR_ROWS / 2 - playfield_get_n_rows (board_env) / 2;
    col_offset = BGR_FLOOR_COLS / 2 - playfield_get_n_cols (board_env) / 2;
    convert_to_canvas (board_theme, row + row_offset, col + col_offset, &x, &y);
    create_tile (x, y, tile);
  }
}

GtkImage* create_tile (double x, double y,
                       Tile *tile)
{
  GdkPixbuf *pixbuf = NULL;
  GtkWidget *item = NULL;
  TileType type;

  type = tile_get_tile_type (tile);
  pixbuf = theme_get_tile_image (board_theme, tile);

  item = gtk_image_new_from_pixbuf (pixbuf);
  gtk_widget_show (item);
  gtk_fixed_put (GTK_FIXED (board_canvas), item, x, y);

  g_object_set_data (G_OBJECT (item), "tile", tile);

// TODO handle button click
//  if (tile_get_tile_type (tile) == TILE_TYPE_ATOM)
//    {
//      g_signal_connect (G_OBJECT (item), "event",
//			G_CALLBACK (board_handle_item_event), NULL);
//    }

  board_canvas_items = g_slist_prepend (board_canvas_items, item);
  switch (type)
    {
    case TILE_TYPE_ATOM:
      level_items->moveables =  g_slist_prepend (level_items->moveables, item);
      break;

    case TILE_TYPE_WALL:
      level_items->obstacles = g_slist_prepend (level_items->obstacles, item);
      break;

    case TILE_TYPE_SHADOW:
      level_items->shadows = g_slist_prepend (level_items->shadows, item);
      break;

    case TILE_TYPE_UNKNOWN:
    case TILE_TYPE_FLOOR:
    default:
      break;
    }

  return GTK_IMAGE (item);
}

static void remove_items (GSList **list)
{
  GSList *to_free = NULL;

  while (*list != NULL) {
    to_free = *list;
    gtk_container_remove (GTK_CONTAINER (board_canvas), GTK_WIDGET ((*list)->data));
    *list = g_slist_next (*list);
    g_slist_free_1 (to_free);
  }
  
}

static void level_clear (void)
{
  remove_items (&(level_items->moveables));
  remove_items (&(level_items->obstacles));
  remove_items (&(level_items->shadows));
}

void board_gtk_init_level (PlayField * base_env, PlayField * sce, Goal * goal)
{
  /* init item anim structure */
  anim_data->timeout_id = -1;
  anim_data->counter = 0;
  anim_data->dest_row = 0;
  anim_data->dest_col = 0;
  anim_data->x_step = 0.0;
  anim_data->y_step = 0.0;

  /* reset undo of moves */
  undo_clear ();

  level_clear ();

  /* init board */
  board_env = playfield_generate_environment (base_env, board_theme);
  board_sce = playfield_copy (sce);
  board_shadow = playfield_generate_shadow (base_env);

  /* init goal */
  board_goal = g_object_ref (goal);

  /* render level */
  board_gtk_render ();
  board_gtk_show ();
}

void board_gtk_destroy (void)
{
}

void board_gtk_clear (void)
{
}

void board_gtk_print (void)
{
}

void board_gtk_hide (void)
{
}

void board_gtk_show (void)
{
}

gboolean board_gtk_undo_move (void)
{
    return FALSE;
}

void board_gtk_show_logo (gboolean visible)
{
}

void board_gtk_handle_key_event (GObject * canvas, GdkEventKey * event,
                                 gpointer data)
{
}


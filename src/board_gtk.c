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

typedef struct
{
  guint row;
  guint col;
  gboolean selected;
  gint arrow_show_timeout;
  gint mouse_steering;
  GtkWidget *sel_item;
  GtkWidget *selector;
  GSList *arrows;
  GtkWidget *arrow_left;
  GtkWidget *arrow_right;
  GtkWidget *arrow_top;
  GtkWidget *arrow_bottom;
} SelectorData;

typedef enum
{
  UP,
  DOWN,
  LEFT,
  RIGHT
} ItemDirection;

typedef struct
{
  GSList *moveables;
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
static SelectorData *selector_data;	/* data about the selector */


/* Forward declarations of internal functions */
void board_gtk_render (void);
static void render_tile (Tile *tile, gint row, gint col);
GtkWidget* create_tile (double x, double y, Tile *tile);

static void selector_move_to (SelectorData *data, guint row, guint col);
static void selector_unselect (SelectorData *data);
static void selector_select (SelectorData *data, GtkWidget *item);
static SelectorData *selector_create (void);
static void selector_hide (SelectorData *data);
static void selector_show (SelectorData *data);
static void selector_arrows_hide (SelectorData *data);


/* Function implementations */

static void get_row_col_by_item (GtkWidget *item, guint *row, guint *col)
{
  gint x, y;
  gint row_offset, col_offset;

  g_return_if_fail (GTK_IS_WIDGET (item));

  gtk_container_child_get (GTK_CONTAINER (board_canvas), item, "x", &x, "y", &y, NULL);

  row_offset = BGR_FLOOR_ROWS / 2 - playfield_get_n_rows (board_env) / 2;
  col_offset = BGR_FLOOR_COLS / 2 - playfield_get_n_cols (board_env) / 2;

  convert_to_playfield (board_theme, x, y, row, col);

  *row = *row - row_offset;
  *col = *col - col_offset; 
}

static gboolean board_handle_arrow_event (GtkWidget *item,
					  GdkEvent *event, gpointer direction)
{
  /* is currently an object moved? */
  if (anim_data->timeout_id != -1)
    return FALSE;

  if (event->type == GDK_BUTTON_PRESS && selector_data->selected)
    {
      selector_data->mouse_steering = TRUE;
//      move_item (selector_data->sel_item, GPOINTER_TO_INT (direction));

      return TRUE;
    }

  return FALSE;
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

  data->selector = gtk_image_new_from_pixbuf (pixbuf);

  data->arrow_top = gtk_image_new_from_pixbuf (sel_arrows[0]);

  g_signal_connect (G_OBJECT (data->arrow_top), "event",
                    G_CALLBACK (board_handle_arrow_event),
                    GINT_TO_POINTER (UP));

  data->arrow_right = gtk_image_new_from_pixbuf (sel_arrows[1]);

  g_signal_connect (G_OBJECT (data->arrow_right), "event",
                    G_CALLBACK (board_handle_arrow_event),
                    GINT_TO_POINTER (RIGHT));

  data->arrow_bottom = gtk_image_new_from_pixbuf (sel_arrows[2]);

  g_signal_connect (G_OBJECT (data->arrow_bottom), "event",
                    G_CALLBACK (board_handle_arrow_event),
                    GINT_TO_POINTER (DOWN));

  data->arrow_left = gtk_image_new_from_pixbuf (sel_arrows[3]);

  g_signal_connect (G_OBJECT (data->arrow_left), "event",
                    G_CALLBACK (board_handle_arrow_event),
                    GINT_TO_POINTER (LEFT));
  gtk_fixed_put (GTK_FIXED (board_canvas), data->selector, 0, 0);
  gtk_fixed_put (GTK_FIXED (board_canvas), data->arrow_left, 0, 0);
  gtk_fixed_put (GTK_FIXED (board_canvas), data->arrow_right, 0, 0);
  gtk_fixed_put (GTK_FIXED (board_canvas), data->arrow_top, 0, 0);
  gtk_fixed_put (GTK_FIXED (board_canvas), data->arrow_bottom, 0, 0);

  return data;
}

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
  selector_data = selector_create ();
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
  gint x, y;

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

static gboolean show_arrow_group (SelectorData *data)
{
  g_slist_foreach (data->arrows, (GFunc)gtk_widget_show, NULL);
  data->arrow_show_timeout = -1;

  return FALSE;
}


static gboolean board_handle_item_event (GtkWidget *item,
                                         GdkEventButton *event, gpointer data) {
  gboolean just_unselect;
  guint new_row, new_col;

  printf ("Item event\n");
  /* is currently an object moved? */
  if (anim_data->timeout_id != -1)
    return FALSE;

  if (event->type == GDK_BUTTON_PRESS) {
    printf ("Button press\n");
    selector_data->mouse_steering = TRUE;
    just_unselect = (item == selector_data->sel_item);

    if (selector_data->selected)
      /* unselect item, show selector image */
      selector_unselect (selector_data);

    if (!just_unselect) {
      get_row_col_by_item (item, &new_row, &new_col);
      selector_move_to (selector_data, new_row, new_col);
      selector_select (selector_data, item);
    }

    return FALSE;
  }

  return FALSE;
}

GtkWidget* create_tile (double x, double y,
                       Tile *tile)
{
  GdkPixbuf *pixbuf = NULL;
  GtkWidget *item = NULL;
  GtkWidget *event_box = NULL;
  pixbuf = theme_get_tile_image (board_theme, tile);

  item = gtk_image_new_from_pixbuf (pixbuf);

// TODO handle button click
  if (tile_get_tile_type (tile) == TILE_TYPE_ATOM) {
    event_box = gtk_event_box_new ();
    gtk_container_add (GTK_CONTAINER (event_box), item);
    gtk_widget_show (item);
    gtk_widget_set_events (event_box, GDK_BUTTON_PRESS_MASK);
    item = event_box;
    g_signal_connect (G_OBJECT (item), "button-press-event",
                      G_CALLBACK (board_handle_item_event), NULL);
  }

  gtk_widget_show (item);
  gtk_fixed_put (GTK_FIXED (board_canvas), item, x, y);

  g_object_set_data (G_OBJECT (item), "tile", tile);

  board_canvas_items = g_slist_prepend (board_canvas_items, item);
  return item;
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

void board_gtk_init_level (PlayField * base_env, PlayField * sce, Goal * goal)
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

  board_gtk_clear ();

  /* init board */
  board_env = playfield_generate_environment (base_env, board_theme);
  board_sce = playfield_copy (sce);
  board_shadow = playfield_generate_shadow (base_env);

  /* init goal */
  board_goal = g_object_ref (goal);

  /* init selector */
  row = playfield_get_n_rows (board_env) / 2;
  col = playfield_get_n_cols (board_env) / 2;
  selector_hide (selector_data);
  selector_move_to (selector_data, row, col);
  selector_unselect (selector_data);

  /* render level */
  board_gtk_render ();
  board_gtk_show ();
}

void board_gtk_destroy (void)
{
  g_slist_free_full (board_canvas_items, (GDestroyNotify)gtk_widget_destroy);

  if (board_env)
    g_object_unref (board_env);
  if (board_sce)
    g_object_unref (board_sce);
  if (anim_data)
    g_free (anim_data);

  undo_clear ();

  if (board_theme)
    g_object_unref (board_theme);

  if (board_goal)
    g_object_unref (board_goal);

}

void board_gtk_clear (void)
{
   remove_items (&(board_canvas_items));
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


static void selector_move_to (SelectorData *data, guint row, guint col)
{
  int tile_width, tile_height;
  int x, y;

  int row_offset = BGR_FLOOR_ROWS / 2 - playfield_get_n_rows (board_env) / 2;
  int col_offset = BGR_FLOOR_COLS / 2 - playfield_get_n_cols (board_env) / 2;

  g_return_if_fail (data != NULL);

  if (data->arrow_show_timeout > -1)
    g_source_remove (data->arrow_show_timeout);

  data->arrow_show_timeout = -1;

  theme_get_tile_size (board_theme, &tile_width, &tile_height);

  convert_to_canvas (board_theme, row+row_offset, col+col_offset, &x, &y);

  gtk_fixed_move (GTK_FIXED (board_canvas), data->selector, x, y);

  gtk_fixed_move (GTK_FIXED (board_canvas), data->selector, x, y);
  gtk_fixed_move (GTK_FIXED (board_canvas), data->arrow_left, x - tile_width, y);
  gtk_fixed_move (GTK_FIXED (board_canvas), data->arrow_right, x + tile_width, y);
  gtk_fixed_move (GTK_FIXED (board_canvas), data->arrow_top, x, y - tile_width);
  gtk_fixed_move (GTK_FIXED (board_canvas), data->arrow_bottom, x, y + tile_width);

  data->row = row;
  data->col = col;
}

static void selector_arrows_show (SelectorData *data)
{
  gint r, c;
  Tile *tile;

  if (board_sce == NULL)
    {
      selector_arrows_hide (data);
      return;
    }

  r = data->row - 1;
  c = data->col;
  if (r >= 0)
    {
      tile = playfield_get_tile (board_sce, r, c);

      if (tile == NULL)
	gtk_widget_show (data->arrow_top);

      else
	{
	  gtk_widget_hide (data->arrow_top);
	  g_object_unref (tile);
	}
    }

  r = data->row;
  c = data->col + 1;

  if (c < playfield_get_n_cols (board_sce))
    {
      tile = playfield_get_tile (board_sce, r, c);

      if (tile == NULL)
	gtk_widget_show (data->arrow_right);
      else
	{
	  gtk_widget_hide (data->arrow_right);
	  g_object_unref (tile);
	}
    }

  r = data->row + 1;
  c = data->col;

  if (r < playfield_get_n_rows (board_sce))
    {
      tile = playfield_get_tile (board_sce, r, c);

      if (tile == NULL)
	gtk_widget_show (data->arrow_bottom);
      else
	{
	  gtk_widget_hide (data->arrow_bottom);
	  g_object_unref (tile);
	}
    }

  r = data->row;
  c = data->col - 1;

  if (c >= 0)
    {
      tile = playfield_get_tile (board_sce, r, c);

      if (tile == NULL)
	gtk_widget_show (data->arrow_left);

      else
	{
	  gtk_widget_hide (data->arrow_left);
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
	g_source_remove (data->arrow_show_timeout);

      data->arrow_show_timeout =
	g_timeout_add (800, (GSourceFunc) show_arrow_group, data);
    }
}

static void selector_select (SelectorData *data, GtkWidget *item)
{
  g_return_if_fail (data != NULL);

  data->selected = TRUE;
  data->sel_item = item;

  gtk_widget_hide (data->selector);
  selector_arrows_show (data);
}

static void selector_unselect (SelectorData *data)
{
  g_return_if_fail (data != NULL);

  data->selected = FALSE;
  data->sel_item = NULL;

  if (!data->mouse_steering)
    selector_show (data);

  selector_arrows_hide (data);
}

static void selector_arrows_hide (SelectorData *data)
{
  if (data->arrow_show_timeout > -1)
    g_source_remove (data->arrow_show_timeout);
  data->arrow_show_timeout = -1;
  
  g_slist_foreach (data->arrows, (GFunc)gtk_widget_hide, NULL);
}

static void selector_hide (SelectorData *data)
{
  gtk_widget_hide (data->selector);
  selector_arrows_hide (data);
}

static void selector_show (SelectorData *data)
{
  gtk_widget_show (data->selector);
}


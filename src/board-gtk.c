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

#include "board-gtk.h"
#include "main.h"

#define ANIM_TIMEOUT     8     /* time in milliseconds between 
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

typedef struct
{
  GSList *moveables;
  GtkWidget *logo;
} LevelItems;

typedef enum
{
  UP,
  DOWN,
  LEFT,
  RIGHT
} ItemDirection;


// FIXME get rid of static variables
/* Static declarations, to be removed */
extern AtomixApp *app;
static GtkFixed *board_canvas = NULL;
static Theme *board_theme = NULL;
static PlayField *board_env = NULL;	/* the actual playfield */
static PlayField *board_sce = NULL;	/* the actual playfield */
static PlayField *board_shadow = NULL;	/* the shadow positions */
static AnimData *anim_data;	/* holds the date for the atom 
                               animation */
static LevelItems *level_items;
static GSList *board_canvas_items = NULL;	/* a list of all used  */
static Goal *board_goal = NULL;	/* the goal of this level */
static SelectorData *selector_data;	/* data about the selector */
static GtkEventController *key_controller = NULL; /* keyboard interaction handler */


/* Forward declarations of internal functions */
static gboolean on_key_press_event (GtkEventController *self, guint keyval, guint keycode,
                                    GdkModifierType *state, gpointer user_data);
void board_gtk_render (void);
static void render_tile (Tile *tile, gint row, gint col);
GtkWidget* create_tile (double x, double y, Tile *tile);
void move_item (GtkWidget *item, ItemDirection direc);
int move_item_anim (void *data);
static GtkWidget *get_item_by_row_col (guint row, guint col);

static void selector_move_to (SelectorData *data, guint row, guint col);
static void selector_unselect (SelectorData *data);
static void selector_select (SelectorData *data, GtkWidget *item);
static SelectorData *selector_create (void);
static void selector_hide (SelectorData *data);
static void selector_show (SelectorData *data);
static void selector_arrows_hide (SelectorData *data);


/* Function implementations */

static GtkWidget *get_item_by_row_col (guint row, guint col)
{
  gint width, height;
  gint item_point_x, item_point_y;
  guint item_row, item_col;
  GSList *list_item = level_items->moveables;

  theme_get_tile_size (board_theme, &width, &height);

  while (list_item != NULL) {
    gtk_container_child_get (GTK_CONTAINER (board_canvas), list_item->data, "x", &item_point_x, "y", &item_point_y, NULL);

    convert_to_playfield (board_theme, board_env, item_point_x, item_point_y, &item_row, &item_col);

    if (item_col == col && item_row == row)
      return GTK_WIDGET (list_item->data);

    list_item = g_slist_next (list_item);
  }

  return NULL;
}

static void get_row_col_by_item (GtkWidget *item, guint *row, guint *col)
{
  gint x, y;

  g_return_if_fail (GTK_IS_WIDGET (item));

  gtk_container_child_get (GTK_CONTAINER (board_canvas), item, "x", &x, "y", &y, NULL);
  convert_to_playfield (board_theme, board_env, x, y, row, col);

}

void move_item (GtkWidget *item, ItemDirection direc)
{
  gint x1, y1;
  gint new_x1, new_y1;
  guint src_row, src_col, dest_row, dest_col, tmp_row, tmp_col;
  gint animstep;
  Tile *tile;
  gint tw, th;

  gtk_container_child_get (GTK_CONTAINER (board_canvas), item, "x", &x1, "y", &y1, NULL);
  theme_get_tile_size (board_theme, &tw, &th);
  convert_to_playfield (board_theme, board_env, x1, y1, &src_row, &src_col);

  /* find destination row/col */
  tmp_row = dest_row = src_row;
  tmp_col = dest_col = src_col;

  while (TRUE) {
    switch (direc) {
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
      default:
	break;
    }

    if (tmp_row >= playfield_get_n_rows (board_sce) ||
        tmp_col >= playfield_get_n_cols (board_sce))
      break;

    tile = playfield_get_tile (board_sce, tmp_row, tmp_col);
    if (tile && (tile_get_tile_type (tile) == TILE_TYPE_ATOM ||
        tile_get_tile_type (tile) == TILE_TYPE_WALL)) {
      g_object_unref (tile);
      break;
    }

    dest_row = tmp_row;
    dest_col = tmp_col;
    if (tile)
      g_object_unref (tile);
  }

  /* move the item, if the new position is different */
  if (src_row != dest_row || src_col != dest_col) {
    if (!undo_exists()) {
      app->state = GAME_STATE_RUNNING;
      update_menu_item_state ();
    }

    undo_push_move (item, src_row, src_col, dest_row, dest_col);

    convert_to_canvas (board_theme, board_env, dest_row, dest_col, &new_x1, &new_y1);
    playfield_swap_tiles (board_sce, src_row, src_col, dest_row, dest_col);

    selector_hide (selector_data);

    animstep = theme_get_animstep (board_theme);
    if (direc == UP || direc == DOWN) {
      anim_data->counter = (gint) (fabs (new_y1 - y1) / animstep);
      anim_data->x_step = 0;
      anim_data->y_step = (direc == DOWN) ? animstep : -animstep;
    } else {
      anim_data->counter = (gint) (fabs (new_x1 - x1) / animstep);
      anim_data->x_step = (direc == RIGHT) ? animstep : -animstep;
      anim_data->y_step = 0;
    }

    anim_data->dest_row = dest_row;
    anim_data->dest_col = dest_col;

    anim_data->timeout_id = g_timeout_add (ANIM_TIMEOUT, move_item_anim,
                                           anim_data);
  }
}

int move_item_anim (void *data) {
  //AnimData *anim_data = (AnimData *) data;
  gint x, y;

  if (anim_data->counter > 0) {
    gtk_container_child_get (GTK_CONTAINER (board_canvas), 
                             selector_data->sel_item, "x", &x, "y", &y, NULL);
    gtk_fixed_move (GTK_FIXED (board_canvas), selector_data->sel_item,
                    x + anim_data->x_step, y + anim_data->y_step);
    anim_data->counter--;

    return TRUE;
  }
  // else
  anim_data->timeout_id = -1;
  selector_move_to (selector_data, anim_data->dest_row, anim_data->dest_col);

  if (goal_reached (board_goal, board_sce, anim_data->dest_row,
      anim_data->dest_col)){
    game_level_finished ();
  } else if (selector_data->selected)
      selector_select (selector_data, selector_data->sel_item);
  return FALSE;
}

static gboolean board_handle_arrow_event (GtkWidget *item,
                                          GdkEventButton *event,
                                          gpointer direction)
{
  gtk_widget_grab_focus (GTK_WIDGET (board_canvas));
  /* is currently an object moved? */
  if (anim_data->timeout_id != -1)
    return FALSE;
  if (event->type == GDK_BUTTON_PRESS && selector_data->selected) {
    selector_data->mouse_steering = TRUE;
    move_item (selector_data->sel_item, GPOINTER_TO_INT (direction));
    return TRUE;
  }

  return FALSE;
}

static GtkWidget* create_arrow (SelectorData *data, GdkPixbuf *pixbuf, ItemDirection direction) {
  GtkWidget *image;
  GtkWidget *arrow = gtk_event_box_new ();

  image = gtk_image_new_from_pixbuf (pixbuf);
  gtk_widget_set_visible (image, TRUE);
  gtk_container_add (GTK_CONTAINER (arrow), image);
  gtk_widget_set_events (arrow, GDK_BUTTON_PRESS_MASK);

  g_signal_connect (G_OBJECT (arrow), "button-press-event",
                    G_CALLBACK (board_handle_arrow_event),
                    GINT_TO_POINTER (direction));

  data->arrows = g_slist_prepend (data->arrows, arrow);
  gtk_fixed_put (GTK_FIXED (board_canvas), arrow, 0, 0);
  g_object_unref (pixbuf);
  return arrow;
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
  gtk_fixed_put (GTK_FIXED (board_canvas), data->selector, 0, 0);
  g_object_unref (pixbuf);

  data->arrow_top = create_arrow (data, sel_arrows[0], UP);
  data->arrow_right = create_arrow (data, sel_arrows[1], RIGHT);
  data->arrow_bottom = create_arrow (data, sel_arrows[2], DOWN);
  data->arrow_left = create_arrow (data, sel_arrows[3], LEFT);

  return data;
}

static void create_logo (void)
{
  GdkPixbuf *pixbuf;
  int tile_width, tile_height;
  GtkWidget *logo_image;
  GtkWidget *tips_label;
  GtkCssProvider *provider;
  GtkStyleContext *context;

  theme_get_tile_size (board_theme, &tile_width, &tile_height);
  pixbuf = gdk_pixbuf_new_from_file (DATADIR "/atomix/atomix-logo.png", NULL);

  level_items->logo = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);

  context = gtk_widget_get_style_context (level_items->logo);
  provider = gtk_css_provider_new ();
  gtk_css_provider_load_from_data (provider, "* {  background-color: rgba(230, 230, 230, 0.6); }", -1, NULL);
  gtk_style_context_add_provider (context, GTK_STYLE_PROVIDER (provider), GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
  logo_image = gtk_image_new_from_pixbuf (pixbuf);
  gtk_widget_set_valign (logo_image, GTK_ALIGN_END);
  gtk_box_pack_start (GTK_BOX (level_items->logo), logo_image, TRUE, TRUE, 12);

  tips_label = gtk_label_new (_("Guide the atoms through the maze to form molecules. "
                                "Click, or use the arrow keys and Enter, to select an atom and move it. "
                                "Be careful, though: an atom keeps moving until it hits a wall."));
  gtk_box_pack_start (GTK_BOX (level_items->logo), tips_label, TRUE, TRUE, 12);
  gtk_widget_set_valign (tips_label, GTK_ALIGN_START);
  gtk_widget_show_all (level_items->logo);

  gtk_fixed_put (GTK_FIXED (board_canvas), level_items->logo,
                 0, 0);
  gtk_widget_set_size_request (level_items->logo, BGR_FLOOR_COLS * tile_width, BGR_FLOOR_ROWS * tile_height);
  gtk_widget_set_size_request (GTK_WIDGET (tips_label), BGR_FLOOR_COLS * tile_width, -1);
  gtk_label_set_justify (GTK_LABEL (tips_label), GTK_JUSTIFY_CENTER);
  gtk_label_set_line_wrap (GTK_LABEL (tips_label), TRUE);
  
  g_object_unref (pixbuf);
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
  level_items = g_new0 (LevelItems, 1);

  create_background_floor ();
  create_logo ();
  gtk_widget_show_all (GTK_WIDGET(board_canvas));

    /* add playfield canvas to left side */
  key_controller = gtk_event_controller_key_new (GTK_WIDGET (board_canvas));
  g_signal_connect (GTK_EVENT_CONTROLLER(key_controller), "key-pressed",
                    G_CALLBACK (on_key_press_event), app);

  selector_data = selector_create ();
}

void board_gtk_render (void) {
  guint row, col;
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
  TileType type;
  gint x, y;

  type = tile_get_tile_type (tile);
  switch (type) {
    case TILE_TYPE_ATOM:
    case TILE_TYPE_WALL:
    case TILE_TYPE_SHADOW:
      convert_to_canvas (board_theme, board_env, row, col, &x, &y);
      create_tile (x, y, tile);
      break;

    case TILE_TYPE_UNKNOWN:
    case TILE_TYPE_FLOOR:
    case TILE_TYPE_NONE:
    case TILE_TYPE_LAST:
    default:
      break;
  }
}

static void show_sensitive (GtkWidget *widget)
{
  gtk_widget_set_visible (widget, gtk_widget_is_sensitive (widget));
}

static gboolean show_arrow_group (SelectorData *data)
{
  g_slist_foreach (data->arrows, (GFunc)show_sensitive, NULL);
  data->arrow_show_timeout = -1;

  return FALSE;
}


static gboolean board_handle_item_event (GtkWidget *item,
                                         GdkEventButton *event, gpointer data) {
  gboolean just_unselect;
  guint new_row, new_col;

  gtk_widget_grab_focus (GTK_WIDGET (board_canvas));
  /* is currently an object moved? */
  if (anim_data->timeout_id != -1)
    return FALSE;

  if (event->type == GDK_BUTTON_PRESS) {
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

  if (tile_get_tile_type (tile) == TILE_TYPE_ATOM) {
    event_box = gtk_event_box_new ();
    gtk_container_add (GTK_CONTAINER (event_box), item);
    gtk_widget_set_visible (item, TRUE);
    gtk_widget_set_events (event_box, GDK_BUTTON_PRESS_MASK);
    item = event_box;
    g_signal_connect (G_OBJECT (item), "button-press-event",
                      G_CALLBACK (board_handle_item_event), NULL);
    level_items->moveables = g_slist_prepend (level_items->moveables, item);
  }

  gtk_widget_set_visible (item, TRUE);
  gtk_fixed_put (GTK_FIXED (board_canvas), item, x, y);

  g_object_set_data (G_OBJECT (item), "tile", tile);

  board_canvas_items = g_slist_prepend (board_canvas_items, item);
  return item;
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
  selector_move_to (selector_data, row, col);
  selector_unselect (selector_data);
  selector_show (selector_data);
  selector_arrows_hide (selector_data);
  /* render level */
  board_gtk_render ();
  board_gtk_show ();
}

void board_gtk_destroy (void)
{
  if (board_env)
    g_object_unref (board_env);
  if (board_sce)
    g_object_unref (board_sce);
  if (anim_data)
    g_free (anim_data);
  if (level_items)
    g_free (level_items);
  undo_clear ();

  if (selector_data) {
    g_slist_free_full (selector_data->arrows, (GDestroyNotify)gtk_widget_destroy);
    g_free (selector_data);
  }

  if (board_theme)
    g_object_unref (board_theme);

  if (board_goal)
    g_object_unref (board_goal);

}

void board_gtk_clear (void)
{
  g_slist_foreach (board_canvas_items, (GFunc) gtk_widget_destroy, NULL);
  g_slist_free (board_canvas_items);
  board_canvas_items = NULL;

  g_slist_free (level_items->moveables);
  level_items->moveables = NULL;

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

void board_gtk_print (void)
{
  g_print ("Board:\n");
  playfield_print (board_env);
}

void board_gtk_hide (void)
{
  g_slist_foreach (level_items->moveables, (GFunc)gtk_widget_hide, NULL);
  g_slist_foreach (selector_data->arrows, (GFunc)gtk_widget_hide, NULL);
}

void board_gtk_show (void)
{
  g_slist_foreach (level_items->moveables, (GFunc)gtk_widget_show, NULL);
  if (undo_exists ())
    g_slist_foreach (selector_data->arrows, (GFunc)gtk_widget_show, NULL);
}

gboolean board_gtk_undo_move (void)
{
  UndoMove *move;
  gint x_src, y_src, x_dest, y_dest;
  gint animstep;

  g_return_val_if_fail (board_theme != NULL, FALSE);

  if (anim_data->timeout_id != -1)
    return FALSE;

  move = undo_pop_move ();
  if (move == NULL)
    return FALSE;

  playfield_swap_tiles (board_sce,
                        move->src_row, move->src_col, 
                        move->dest_row, move->dest_col);

  if (selector_data->selected) {
    selector_hide (selector_data);
    selector_move_to (selector_data, move->src_row, move->src_col);
  }

  convert_to_canvas (board_theme, board_env, move->src_row,
             move->src_col, &x_src, &y_src);
  convert_to_canvas (board_theme, board_env, move->dest_row,
             move->dest_col, &x_dest, &y_dest);

  animstep = theme_get_animstep (board_theme);
  if (move->src_col == move->dest_col) {
    anim_data->counter = (gint) (fabs (y_dest - y_src) / animstep);
    anim_data->x_step = 0;
    anim_data->y_step = animstep;
    if (move->src_row < move->dest_row)
      anim_data->y_step = -(anim_data->y_step);
  } else {
    anim_data->counter = (gint) (fabs (x_dest - x_src) / animstep);
    anim_data->x_step = animstep;
    anim_data->y_step = 0;
    if (move->src_col < move->dest_col)
      anim_data->x_step = -(anim_data->x_step);
  }

  anim_data->dest_row = move->src_row;
  anim_data->dest_col = move->src_col;
  selector_data->sel_item = move->item;

  anim_data->timeout_id = g_timeout_add (ANIM_TIMEOUT,
                                         move_item_anim, anim_data);
  g_free (move);

  return TRUE;
}

void board_gtk_show_logo (gboolean visible)
{
  gtk_widget_set_visible (level_items->logo, visible);
}

gboolean board_gtk_handle_key_event (GObject * canvas, guint keyval,
                                     gpointer data)
{
  GtkWidget *item;
  guint new_row, new_col;
  Tile *tile;

  g_return_val_if_fail (selector_data != NULL, FALSE);

  new_row = selector_data->row;
  new_col = selector_data->col;

  /* is currently an object moved? */
  if (anim_data->timeout_id != -1)
    return FALSE;

  switch (keyval) {
    case GDK_KEY_space:
    case GDK_KEY_Return:
      selector_data->mouse_steering = FALSE;
      if (selector_data->selected)
        /* unselect item, show selector image */
        selector_unselect (selector_data);
      else {
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
    case GDK_KEY_Escape:
      if (selector_data->selected)
        /* unselect item, show selector image */
        selector_unselect (selector_data);
      break;
    case GDK_KEY_Left:
      selector_data->mouse_steering = FALSE;
      if (!selector_data->selected) {
        new_col--;
        if (new_col < playfield_get_n_cols (board_env)) {
          selector_show (selector_data);
          selector_move_to (selector_data, new_row, new_col);
        }
      } else
        move_item (selector_data->sel_item, LEFT); /* selector will be
                                                    moved in this
                                                    function */
      break;

    case GDK_KEY_Right:
      selector_data->mouse_steering = FALSE;
      if (!selector_data->selected) {
        new_col++;
        if (new_col < playfield_get_n_cols (board_env)) {
          selector_show (selector_data);
          selector_move_to (selector_data, new_row, new_col);
        }
      } else
        move_item (selector_data->sel_item, RIGHT); /* selector will be
                                                    moved in this
                                                    function */
      break;

    case GDK_KEY_Up:
      selector_data->mouse_steering = FALSE;
      if (!selector_data->selected) {
        new_row--;
        if (new_row < playfield_get_n_rows (board_env)) {
          selector_show (selector_data);
          selector_move_to (selector_data, new_row, new_col);
        }
      } else
        move_item (selector_data->sel_item, UP);  /* selector will be moved
                                                  in this function */
      break;

    case GDK_KEY_Down:
      selector_data->mouse_steering = FALSE;
      if (!selector_data->selected) {
        new_row++;
        if (new_row < playfield_get_n_rows (board_env)) {
          selector_show (selector_data);
          selector_move_to (selector_data, new_row, new_col);
        }
      } else
        move_item (selector_data->sel_item, DOWN);  /* selector will be
                                                    moved in this function */
      break;

    default:
      break;
    }

  return FALSE;
}

static gboolean on_key_press_event (GtkEventController *self, guint keyval, guint keycode,
                                    GdkModifierType *state, gpointer user_data)
{
  if ((app->state == GAME_STATE_RUNNING) || (app->state == GAME_STATE_RUNNING_UNMOVED))
    return board_gtk_handle_key_event (NULL, keyval, NULL);

  return FALSE;
}

static void selector_move_to (SelectorData *data, guint row, guint col)
{
  int tile_width, tile_height;
  int x, y;

  g_return_if_fail (data != NULL);

  if (data->arrow_show_timeout > -1)
    g_source_remove (data->arrow_show_timeout);

  data->arrow_show_timeout = -1;

  theme_get_tile_size (board_theme, &tile_width, &tile_height);

  convert_to_canvas (board_theme, board_env, row, col, &x, &y);

  g_object_ref (data->selector);
  gtk_container_remove (GTK_CONTAINER (board_canvas), data->selector);
  gtk_fixed_put (GTK_FIXED (board_canvas), data->selector, x, y);
  g_object_unref (data->selector);

  gtk_fixed_move (GTK_FIXED (board_canvas), data->arrow_left, x - tile_width, y);
  gtk_fixed_move (GTK_FIXED (board_canvas), data->arrow_right, x + tile_width, y);
  gtk_fixed_move (GTK_FIXED (board_canvas), data->arrow_top, x, y - tile_width);
  gtk_fixed_move (GTK_FIXED (board_canvas), data->arrow_bottom, x, y + tile_width);

  data->row = row;
  data->col = col;
}

static void check_for_arrow (gint r, gint c, GtkWidget *arrow) {
  Tile *tile;

  tile = playfield_get_tile (board_sce, r, c);

  gtk_widget_set_sensitive (arrow, tile == NULL);

  if (tile != NULL)
    g_object_unref (tile);
}

static void selector_arrows_show (SelectorData *data)
{
  guint r, c;

  if (board_sce == NULL)
    {
      selector_arrows_hide (data);
      return;
    }

  r = data->row - 1;
  c = data->col;
//  if (r >= 0)
    check_for_arrow (r, c, data->arrow_top);

  r = data->row;
  c = data->col + 1;

  if (c < playfield_get_n_cols (board_sce))
    check_for_arrow (r, c, data->arrow_right);

  r = data->row + 1;
  c = data->col;

  if (r < playfield_get_n_rows (board_sce))
    check_for_arrow (r, c, data->arrow_bottom);

  r = data->row;
  c = data->col - 1;

//  if (c >= 0)
    check_for_arrow (r, c, data->arrow_left);

  if (data->mouse_steering)
    show_arrow_group (data);
  else {
    if (data->arrow_show_timeout > -1)
      g_source_remove (data->arrow_show_timeout);

    data->arrow_show_timeout = 
        g_timeout_add (2000, (GSourceFunc) show_arrow_group, data);
  }
}

static void selector_select (SelectorData *data, GtkWidget *item)
{
  gint x, y;

  g_return_if_fail (data != NULL);
  gtk_container_child_get (GTK_CONTAINER(board_canvas), item, "x", &x, "y", &y, NULL);

  data->selected = TRUE;
  data->sel_item = item;

  gtk_widget_set_visible (data->selector, FALSE);
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
  gtk_widget_set_visible (data->selector, FALSE);
  selector_arrows_hide (data);
}

static void selector_show (SelectorData *data)
{
  gtk_widget_set_visible (data->selector, TRUE);
}


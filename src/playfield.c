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
#include "playfield.h"

Tile* get_tile(PlayField *pf, gint row, gint col);
void set_tile(PlayField *pf, gint row, gint col, Tile *tile);
static void read_tile (PlayField *pf, guint row, guint col, xmlNodePtr node);

static GObjectClass     *parent_class = NULL;

static void playfield_class_init (GObjectClass *class);
static void playfield_init (PlayField *pf);
static void playfield_finalize (GObject *object);

struct _PlayFieldPrivate {
	guint n_rows;
	guint n_cols;
	Tile **matrix;
};

GType
playfield_get_type (void)
{
  static GType object_type = 0;

  if (!object_type)
    {
      static const GTypeInfo object_info =
      {
	sizeof (PlayFieldClass),
	(GBaseInitFunc) NULL,
	(GBaseFinalizeFunc) NULL,
	(GClassInitFunc) playfield_class_init,
	NULL,   /* clas_finalize */
	NULL,   /* class_data */
	sizeof(PlayField),
	0,      /* n_preallocs */
	(GInstanceInitFunc) playfield_init,
      };

      object_type = g_type_register_static (G_TYPE_OBJECT,
					    "PlayField",
					    &object_info, 0);
    }

  return object_type;
}

static void 
playfield_class_init (GObjectClass *class)
{
	parent_class = g_type_class_ref (G_TYPE_OBJECT);
	class->finalize = playfield_finalize;
}

static void 
playfield_init (PlayField *pf)
{
	PlayFieldPrivate *priv;

	priv = g_new0 (PlayFieldPrivate, 1);
	priv->n_rows = 0;
	priv->n_cols = 0;
	priv->matrix = NULL;
	
	pf->priv = priv;
}

static void 
playfield_finalize (GObject *object)
{
	gint row, col;
	PlayField* pf = PLAYFIELD (object);

	g_message ("Finalize Playfield.");

	for (row = 0; row < pf->priv->n_rows; row++) 
		for (col = 0; col < pf->priv->n_cols; col++) {
			Tile *tile = get_tile (pf, row, col);		
			if (tile)
				g_object_unref (tile);
		}
	g_free (pf->priv->matrix);

	g_free (pf->priv);
	pf->priv = NULL;
}

PlayField*
playfield_new (void)
{
	PlayField* pf;

	pf = PLAYFIELD (g_object_new (PLAYFIELD_TYPE, NULL));
	return pf;
}


Tile* 
get_tile (PlayField *pf, gint row, gint col)
{
	PlayFieldPrivate *priv;

	g_return_val_if_fail (IS_PLAYFIELD (pf), NULL);

	priv = pf->priv;
	
	return priv->matrix [row * priv->n_cols + col];
}

void
set_tile (PlayField *pf, gint row, gint col, Tile *new_tile)
{
	Tile *tile;
	PlayFieldPrivate *priv;

	g_return_if_fail (IS_PLAYFIELD (pf));

	priv = pf->priv;

	tile = get_tile (pf, row, col);
	if (tile)
		g_object_unref (tile);

	priv->matrix [row * priv->n_cols + col] = new_tile;
}

guint 
playfield_get_n_rows (PlayField *pf)
{
	g_return_val_if_fail (IS_PLAYFIELD (pf), 0);
	return pf->priv->n_rows;
}

guint 
playfield_get_n_cols (PlayField *pf)
{
	g_return_val_if_fail (IS_PLAYFIELD (pf), 0);
	return pf->priv->n_cols;
}

void
playfield_add_row (PlayField* pf)
{
	Tile **new_matrix;
	int n_rows, n_cols;

	g_return_if_fail (IS_PLAYFIELD (pf));

	n_rows = pf->priv->n_rows;
	n_cols = pf->priv->n_cols;

	new_matrix = g_malloc0 ((n_rows+1) * n_cols * sizeof (Tile*));
	if (new_matrix == NULL) return;

	memcpy (new_matrix, pf->priv->matrix, n_rows * n_cols * sizeof (Tile*));

	g_free (pf->priv->matrix);
	pf->priv->matrix = new_matrix;
	pf->priv->n_rows++;
}



void
playfield_add_column(PlayField* pf)
{
	Tile **new_matrix;
	int n_rows, n_cols;

	g_return_if_fail (IS_PLAYFIELD (pf));

	n_rows = pf->priv->n_rows;
	n_cols = pf->priv->n_cols;

	new_matrix = g_malloc0 (n_rows * (n_cols+1) * sizeof (Tile*));
	if (new_matrix == NULL) return;

	memcpy (new_matrix, pf->priv->matrix, n_rows * n_cols * sizeof (Tile*));

	g_free (pf->priv->matrix);
	pf->priv->matrix = new_matrix;
	pf->priv->n_cols++;
}

void 
playfield_set_matrix_size (PlayField *pf, guint n_rows, guint n_cols)
{
	gint row = 0;
	gint col = 0;
	guint old_n_rows;
	guint old_n_cols;
	Tile **new_matrix;

	g_return_if_fail (IS_PLAYFIELD (pf));

	old_n_rows = pf->priv->n_rows;
	old_n_cols = pf->priv->n_cols;

	if (n_rows == 0 || n_cols == 0) return;
	if (old_n_rows == n_rows && old_n_cols == n_cols) return;

	if (old_n_rows > n_rows || old_n_cols > n_cols) {
		// free the left over tiles;
		for (row = 0; row < old_n_rows; row++) {
			for (col = 0; col < old_n_cols; col++) {
				Tile *tile = get_tile (pf, row, col);
				if (row >= n_rows && tile)
					g_object_unref (tile);
				else if (col >= n_cols && tile)
					g_object_unref (tile);
			}
		}
	}

	new_matrix = g_malloc0 (n_rows * n_cols * sizeof (Tile*));
	memcpy (new_matrix, pf->priv->matrix, old_n_rows * old_n_cols * sizeof (Tile*));
	
	g_free (pf->priv->matrix);
	pf->priv->matrix = new_matrix;
	pf->priv->n_rows = n_rows;
	pf->priv->n_cols = n_cols;
}

PlayField*
playfield_copy (PlayField* pf)
{
	PlayField* pf_copy = NULL;
	gint row, col;
	gint matrix_size;
	
	g_return_val_if_fail (IS_PLAYFIELD (pf), NULL);
	
	pf_copy = playfield_new ();
	g_assert (IS_PLAYFIELD (pf_copy));
	
	matrix_size = pf->priv->n_rows * pf->priv->n_cols * sizeof (Tile*);
	pf_copy->priv->matrix = g_malloc0 (matrix_size);
	pf_copy->priv->n_rows = pf->priv->n_rows;
	pf_copy->priv->n_cols = pf->priv->n_cols;

	for (row = 0; row < pf->priv->n_rows; row++) 
		for (col = 0; col < pf->priv->n_cols; col++)
			playfield_set_tile (pf_copy, row, col, get_tile (pf, row, col));
    
	return pf_copy;
}

void
playfield_swap_tiles (PlayField* pf, guint src_row, guint src_col,
		      guint dest_row, guint dest_col)
{
	Tile* src_tile;
	Tile* dest_tile;
	gint n_rows, n_cols;
	
	g_return_if_fail (IS_PLAYFIELD (pf));

	n_rows = pf->priv->n_rows;
	n_cols = pf->priv->n_cols;

	g_return_if_fail (src_row < n_rows && dest_row < n_cols &&
			  src_col < n_cols && dest_col < n_cols);

	src_tile = get_tile (pf, src_row, src_col);
	if (src_tile) g_object_ref (src_tile);
	
	dest_tile = get_tile (pf, dest_row, dest_col);
	if (dest_tile) g_object_ref (dest_tile);

	playfield_set_tile (pf, src_row, src_col, dest_tile);
	playfield_set_tile (pf, dest_row, dest_col, src_tile);

	if (src_tile) g_object_unref (src_tile);
	if (dest_tile) g_object_unref (dest_tile);
}


Tile*
playfield_clear_tile(PlayField *pf, guint row, guint col)
{
	Tile *tile;

	g_return_val_if_fail (IS_PLAYFIELD (pf), NULL);
	g_return_val_if_fail (row < pf->priv->n_rows && col < pf->priv->n_cols, NULL);

	tile = get_tile (pf, row, col);
	if (tile) {
		g_object_ref (tile);
		set_tile (pf, row, col, NULL);
	}

	return tile;
}

void
playfield_clear (PlayField *pf)
{
	gint row, col;

	g_return_if_fail (IS_PLAYFIELD (pf));
	
	for(row = 0; row < pf->priv->n_rows; row++)
		for (col = 0; col < pf->priv->n_cols; col++)
			set_tile (pf, row, col, NULL);
}

PlayField*
playfield_strip (PlayField *pf)
{
	PlayField *stripped_pf;
	gint row, col;
	gint n_rows, n_cols;
	gint max_row = 0;
	gint min_row = 10000;
	gint max_col = 0; 
	gint min_col = 10000;

	g_return_val_if_fail (IS_PLAYFIELD (pf), NULL);

	/* determine the really used playfield size */
	for(row = 0; row < pf->priv->n_rows; row++) {
		for(col = 0; col < pf->priv->n_cols; col++) {
			Tile *tile = get_tile (pf, row, col);
			if (tile != NULL) {
				max_row = MAX(max_row, row);
				min_row = MIN(min_row, row);
				max_col = MAX(max_col, col);
				min_col = MIN(min_col, col);
			}
		}
	}
	
	/* number of columns and rows in the stripped playfield */
	n_rows = max_row - min_row + 1;
	n_cols = max_col - min_col + 1;
	
	if (pf->priv->n_rows == n_rows && pf->priv->n_cols == n_cols) 
		return g_object_ref (pf);

	stripped_pf = playfield_new ();
	playfield_set_matrix_size (stripped_pf, n_rows, n_cols);
	
	for(row = min_row;  row <= max_row; row++) 
		for (col = min_col; col <= max_col; col++) {
			Tile *tile;
			tile = get_tile (pf, row, col);

			playfield_set_tile (stripped_pf, 
					    row - min_row,
					    col - min_col,
					    tile);
		}
	
	return stripped_pf;
}

Tile*
playfield_get_tile(PlayField *pf, guint row, guint col)
{
	Tile *tile;

	g_return_val_if_fail (IS_PLAYFIELD (pf), NULL);
	g_return_val_if_fail (row < pf->priv->n_rows && col < pf->priv->n_cols, NULL);
	
	tile = get_tile (pf, row, col);
	if (tile) g_object_ref (tile);
	
	return tile;
}

void
playfield_set_tile(PlayField* pf, guint row, guint col, Tile *tile)
{
	g_return_if_fail (IS_PLAYFIELD (pf));
	g_return_if_fail (row < pf->priv->n_rows && col < pf->priv->n_cols);

	if (tile) g_object_ref (tile);
	
	set_tile (pf, row, col, tile);
}

void
playfield_print(PlayField* pf)
{
	PlayFieldPrivate *priv;
	int row, col;
	Tile *tile;

	g_return_if_fail (IS_PLAYFIELD (pf));

	priv = pf->priv;

	g_print ("N_ROWS: %d\n",priv->n_rows);
	g_print ("N_COLS: %d\n",priv->n_cols);
	
	for(row = 0; row < priv->n_rows; row++) 
	{
		for(col = 0; col < priv->n_cols; col++) {
			tile = get_tile(pf, row, col); 
			if(tile)
				tile_print (tile);		
			else
				g_print("NULL ");
		}
		g_print("\n");
	}
	g_print("\n");
}

PlayField* 
playfield_new_from_xml (xmlNodePtr node)
{
	xmlNodePtr child_node;
	PlayField *pf;
	gint row, col;
	gint n_rows, n_cols;
	gchar *prop_value;
	gchar *content;

	g_return_val_if_fail (node != NULL, NULL);

	pf = playfield_new ();
    	row = 0; col = 0;
	n_rows = 0; n_cols = 0;
		
	/* reading non empty tiles */
	for (; node != NULL; node = node->next)
	{
		if (!g_strcasecmp (node->name, "n_rows")) {
			content = xmlNodeGetContent (node);
			n_rows = atoi (content);
		}
		else if (!g_strcasecmp (node->name, "n_columns")) {
			content = xmlNodeGetContent (node);
			n_cols = atoi (content);
			playfield_set_matrix_size (pf, n_rows, n_cols);
		}
		else if (!g_strcasecmp (node->name, "row")) {
			prop_value = xmlGetProp (node, "no");
			row = atoi (prop_value);
			for (child_node = node->xmlChildrenNode;
			     child_node != NULL; child_node = child_node->next)
			{
				if (!g_strcasecmp (child_node->name, "col")) {
					prop_value = xmlGetProp (child_node, "no");
					col = atoi (prop_value);
					read_tile (pf, row, col, child_node->xmlChildrenNode);
				}
			}
		}
		else if (!g_strcasecmp (node->name, "text")) {
		}
		else {
			g_warning("Skipping unexpected Tag %s.", 
				  node->name);
		}
	}

	return pf;
}

static void
read_tile (PlayField *pf, guint row, guint col, xmlNodePtr node)
{
	Tile *tile;

	for (; node != NULL; node = node->next) {
		if (!g_strcasecmp (node->name, "tile")) {
			tile = tile_new_from_xml (node);
			playfield_set_tile (pf, row, col, tile);
			g_object_unref (tile);
		}
	}
}


#if 0
/*
 * Creates the XML representation of an PlayField.
 * pf = Playfield to save
 * pf_node = reference to the <PLAYFIELD> Tag node
 */
void playfield_save_xml(PlayField *pf, xmlNodePtr pf_node)
{
	PlayFieldPrivate *priv;
	gint row, col;
	xmlAttrPtr attr;
	xmlNodePtr tile_node;
	gint max_row = 0;
	gint min_row = 10000;
	gint max_col = 0; 
	gint min_col = 10000;
	gint n_rows, n_cols;

	g_return_if_fail (IS_PLAYFIELD (pf));
	priv = pf->priv;
	
	/* determine the really used playfield size */
	for(row = 0; row < priv->n_rows; row++)
	{
		for(col = 0; col < priv->n_cols; col++)
		{
			Tile *tile = playfield_get_tile(pf, row, col);
			if(tile_get_tile_type (tile) != TILE_TYPE_UNKNOWN)
			{
				max_row = MAX(max_row, row);
				min_row = MIN(min_row, row);
				max_col = MAX(max_col, col);
				min_col = MIN(min_col, col);
			}
		}
	}

	/* number of columns and rows */
	n_rows = max_row - min_row + 1;
	n_cols = max_col - min_col + 1;
	
	if((n_rows > 0) && (n_cols > 0))
	{
		gchar *str_buffer; 
		gint length;
		
		str_buffer = g_malloc(5*sizeof(gchar));
    
		length = g_snprintf(str_buffer, 5, "%i", n_rows);
		attr = xmlSetProp(pf_node, "rows", g_strdup(str_buffer));
		length = g_snprintf(str_buffer, 5, "%i", n_cols);
		attr = xmlSetProp(pf_node, "cols", g_strdup(str_buffer));

		/* add every non empty tile */
		for(row = 0; row < priv->n_rows; row++)
		{	
			for(col = 0; col < priv->n_cols; col++)
			{
				Tile *tile = playfield_get_tile(pf, row, col);
				if (tile_get_tile_type (tile) != TILE_TYPE_UNKNOWN)
				{
					tile_node = xmlNewChild(pf_node, NULL, "TILE", NULL);
					length = g_snprintf(str_buffer, 5, "%i", row - min_row);
					attr = xmlSetProp(tile_node, "row",g_strdup(str_buffer));
					length = g_snprintf(str_buffer, 5, "%i", col - min_col);
					attr = xmlSetProp(tile_node, "col",g_strdup(str_buffer));
					
					tile_save_xml(tile, tile_node);			
				}
			}
		}
		g_free(str_buffer);		         
	}
	else
	{
		gchar *str_buffer; 
		gint length;
		
		str_buffer = g_malloc(5*sizeof(gchar));
    
		length = g_snprintf(str_buffer, 5, "%i", priv->n_rows);
		attr = xmlSetProp(pf_node, "rows", g_strdup(str_buffer));
		length = g_snprintf(str_buffer, 5, "%i", priv->n_cols);
		attr = xmlSetProp(pf_node, "cols", g_strdup(str_buffer));		

		g_free(str_buffer);		         
	}
}


#endif

/*=================================================================

  Functions for generating playfields from basic level descriptions.

 -----------------------------------------------------------------*/

typedef struct {
	GQuark id;
	gchar *string;
} TranslationItem;

static TranslationItem wall_map [] = {
	{  0, "wall-single" },
	{  1, "wall-vertical-bottom-end" },
	{  2, "wall-single" },
	{  3, "wall-bottom-left" },
	{  4, "wall-vertical-top-end" },
	{  5, "wall-vertical" },
	{  6, "wall-top-left" },
	{  7, "wall-vertical-rightx" },
	{  8, "wall-single" },
	{  9, "wall-bottom-right" },
	{ 10, "wall-horizontal" },
	{ 11, "wall-horizontal-topx" },
	{ 12, "wall-top-right" },
	{ 13, "wall-vertical-leftx" },
	{ 14, "wall-horizontal-bottomx" },
	{ 15, "wall-single"},
	{ 0, NULL }
};

static void 
setup_translation_map (TranslationItem array[])
{
	gint i = 0;
	for (; array[i].string != NULL; i++) {
		array[i].id = g_quark_from_string (array[i].string);
	}
}

enum {
	ENV_TOP,
	ENV_RIGHT,
	ENV_BOTTOM,
	ENV_LEFT,
	ENV_TOP_RIGHT,
	ENV_BOTTOM_RIGHT,
	ENV_BOTTOM_LEFT,
	ENV_TOP_LEFT,
	ENV_LAST
};

typedef struct {
	int row;
	int col;
} offset;

static const offset env_offset[8] = { 
	{ -1, 0},  /* ENV_TOP */
	{ 0, 1 },  /* ENV_RIGHT */
	{ 1, 0 },  /* ENV_BOTTOM */
	{ 0, -1 }, /* ENV_LEFT */
	{ -1, 1},  /* ENV_TOP_RIGHT */
	{ 1, 1},   /* ENV_BOTTOM_RIGHT */
	{ 1, -1},  /* ENV_BOTTOM_LEFT */
	{ -1, -1}  /* ENV_TOP_LEFT */
};

static TileType
get_env_tile_type (PlayField *pf, gint row, gint col)
{
	Tile *tile;
	TileType type = TILE_TYPE_NONE;
	
	if (row < 0) return type;
	if (col < 0) return type;
	if (row >= playfield_get_n_rows (pf)) return type;
	if (col >= playfield_get_n_cols (pf)) return type;
	
	tile = get_tile (pf, row, col);
	if (tile) {
		type = tile_get_tile_type (tile);
	}
	else
		type = TILE_TYPE_FLOOR;

	return type;
}

static void
create_tile_env (PlayField *pf, gint row, gint col, int tile_env[])
{
	gint i;
	TileType type;
	
	for (i = ENV_TOP; i < ENV_LAST; i++) {
		type = get_env_tile_type (pf, row + env_offset[i].row, 
					  col + env_offset[i].col);
		tile_env[i] = (int) type;
	}
}

static Tile*
convert_wall_tiles (Tile *tile, int tile_env[])
{
	Tile *new_tile = NULL;
	gint wall_id = 0; 
	gint i;

	if (tile == NULL)
		new_tile = tile_new (TILE_TYPE_FLOOR);
	else 
		new_tile = tile_copy (tile);

	if (tile_get_tile_type (new_tile) != TILE_TYPE_WALL) return new_tile;
	
	for (i = ENV_LEFT; i >= ENV_TOP; i--) {
		if (tile_env[i] == TILE_TYPE_WALL)
			wall_id = wall_id + 1;

		if (i != ENV_TOP)
			wall_id = wall_id << 1;
	}
	tile_set_base_id (new_tile, wall_map[wall_id].id);

	return new_tile;
}


PlayField*
playfield_generate_environment (PlayField *pf, Theme *theme)
{
	PlayField *env_pf;
	PlayFieldPrivate *priv;
	gint row, col;
	gint tile_env[8];
	Tile *env_tile;
	Tile *tile = NULL;
	gint n_decor_tiles;
	gint max_try;
	gint min_wall[2];
	gint max_wall[2];

	g_return_val_if_fail (IS_PLAYFIELD (pf), NULL);

	priv = pf->priv;

	if (wall_map[0].id == 0)
		setup_translation_map (wall_map);

	env_pf = playfield_new ();
	playfield_set_matrix_size (env_pf, 
				   playfield_get_n_rows (pf),
				   playfield_get_n_cols (pf));
	
	min_wall[0] = 10000;
	min_wall[1] = 10000;
	max_wall[0] = 0;
	max_wall[1] = 0;

	/* determine the really used playfield size */
	for (row = 0; row < priv->n_rows; row++) {
		for (col = 0; col < priv->n_cols; col++) {
			create_tile_env (pf, row, col, tile_env);
			tile = get_tile (pf, row, col);

			env_tile = convert_wall_tiles (tile, tile_env);

			playfield_set_tile (env_pf, row, col, env_tile);

			if (env_tile) {
				if (tile_get_tile_type (env_tile) == TILE_TYPE_WALL) {
					min_wall [0] = MIN (min_wall[0], row);
					min_wall [1] = MIN (min_wall[1], col);
					max_wall [0] = MAX (max_wall[0], row);
					max_wall [1] = MAX (max_wall[1], col);
				}
				g_object_unref (env_tile);
			}
		}
	}

	/* apply decoration to some of the tiles */
	n_decor_tiles = (max_wall[0] - min_wall[0]) * (max_wall[1] - min_wall[1]) * 0.03;
	while (n_decor_tiles--) {
		max_try = 4; /* maximum number of tries */
		do {
			row = g_random_int_range (min_wall[0], max_wall[0]);
			col = g_random_int_range (min_wall[1], max_wall[1]);
			tile = get_tile (env_pf, row, col);
		} while (!theme_apply_decoration (theme, tile) && max_try--);
	}
	
	return env_pf;
}

static GQuark shadow_id[6] = { 0, 0, 0, 0, 0, 0 };

static Tile*
convert_shadow_tiles (Tile *tile, int tile_env[])
{
	Tile *new_tile = NULL;
	gint base_id = 0; 
	TileType type;

	if (tile != NULL) {
		type = tile_get_tile_type (tile);
		if (type != TILE_TYPE_FLOOR) return NULL;
	}

	if (tile_env[ENV_LEFT]     == TILE_TYPE_WALL && 
	    tile_env[ENV_TOP]      == TILE_TYPE_WALL)
		base_id = 6; /* top-left */
	
	else if (tile_env[ENV_TOP_LEFT] != TILE_TYPE_WALL &&
		 tile_env[ENV_LEFT]     != TILE_TYPE_WALL && 
		 tile_env[ENV_TOP]      == TILE_TYPE_WALL)
		base_id = 2;
	
	else if (tile_env[ENV_TOP_LEFT] != TILE_TYPE_WALL &&
		 tile_env[ENV_LEFT]     == TILE_TYPE_WALL && 
		 tile_env[ENV_TOP]      != TILE_TYPE_WALL)
		base_id = 5;
	
	else if (tile_env[ENV_TOP_LEFT] == TILE_TYPE_WALL &&
		 tile_env[ENV_LEFT]     != TILE_TYPE_WALL && 
		 tile_env[ENV_TOP]      != TILE_TYPE_WALL)
		base_id = 4;
	
	else if (tile_env[ENV_TOP_LEFT] == TILE_TYPE_WALL &&
		 tile_env[ENV_LEFT]     == TILE_TYPE_WALL && 
		 tile_env[ENV_TOP]      != TILE_TYPE_WALL)
		base_id = 3;
	
	else if (tile_env[ENV_TOP_LEFT] == TILE_TYPE_WALL &&
		 tile_env[ENV_LEFT]     != TILE_TYPE_WALL && 
		 tile_env[ENV_TOP]      == TILE_TYPE_WALL)
		base_id = 1;
	
	if (base_id) {
		new_tile = tile_new (TILE_TYPE_SHADOW);
		tile_set_base_id (new_tile, shadow_id[base_id-1]);
	}

	return new_tile;
}

PlayField*
playfield_generate_shadow (PlayField *pf)
{
	PlayField *env_pf;
	PlayFieldPrivate *priv;
	gint row, col;
	gint tile_env[8];
	Tile *env_tile;
	Tile *tile;

	g_return_val_if_fail (IS_PLAYFIELD (pf), NULL);

	priv = pf->priv;

	if (!shadow_id[0]) {
		shadow_id[0] = g_quark_from_static_string ("shadow-top");
		shadow_id[1] = g_quark_from_static_string ("shadow-top-left");
		shadow_id[2] = g_quark_from_static_string ("shadow-left");
		shadow_id[3] = g_quark_from_static_string ("shadow-bottom-right");
		shadow_id[4] = g_quark_from_static_string ("shadow-left-top");
		shadow_id[5] = g_quark_from_static_string ("shadow-top-left-both");
	}

	env_pf = playfield_new ();
	playfield_set_matrix_size (env_pf, 
				   playfield_get_n_rows (pf),
				   playfield_get_n_cols (pf));

	/* determine the really used playfield size */
	for (row = 0; row < priv->n_rows; row++) {
		for (col = 0; col < priv->n_cols; col++) {
			create_tile_env (pf, row, col, tile_env);
			tile = get_tile (pf, row, col);

			env_tile = convert_shadow_tiles (tile, tile_env);

			playfield_set_tile (env_pf, row, col, env_tile);

			if (env_tile)
				g_object_unref (env_tile);
		}
	}
	
	return env_pf;
}

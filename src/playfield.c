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
#include "playfield.h"

Tile* playfield_get_tile_nc(PlayField *pf, gint row, gint col);
void playfield_set_tile_nc(PlayField *pf, gint row, gint col, Tile *tile);

PlayField*
playfield_new(void)
{
	PlayField* pf;

	pf = g_malloc(sizeof(PlayField));

	if(pf) 
	{
		/* initialise matrix */
		pf->n_rows = 0;
		pf->n_cols = 0;

		pf->matrix = g_ptr_array_new();
	}
#ifdef DEBUG
	else {
		g_print("Not enough memory for Matrix allocation!");
	}
#endif

	return pf;
}
gboolean
playfield_add_row(PlayField* pf)
{
	GPtrArray *new_row = g_ptr_array_new();

	if(pf->n_cols > 0) 
	{
		  int col;
		  
		  /* add appropriate numbers of cols to the new row */
		  for(col = 0; col < pf->n_cols; col++)
		  {
			  g_ptr_array_add(new_row, tile_new());  
		  }
	}
	
	/* add the new row */
        g_ptr_array_add(pf->matrix, new_row);
	
	pf->n_rows++;
	
	return TRUE;
}

gboolean
playfield_add_column(PlayField* pf)
{
	if(pf->n_rows > 0)
	{
		int row;
		GPtrArray *row_array;
		/* add in every existing row a column */
		for(row = 0; row < pf->n_rows; row++)
		{
			row_array = (GPtrArray*)g_ptr_array_index(pf->matrix,
								  row);
			g_ptr_array_add(row_array, tile_new());
		}
	}
	pf->n_cols++;

	return TRUE;
}

void 
playfield_set_matrix_size(PlayField *pf, guint n_rows, guint n_cols)
{
	gint row = 0;
	gint col = 0;
	guint old_n_rows = pf->n_rows;
	guint old_n_cols = pf->n_cols;

	if((old_n_rows == n_rows) && (old_n_cols == n_cols)) return;

	// handle columns
	if(old_n_cols < n_cols)
	{
		// add columns
		for(col = 0; col < (n_cols - old_n_cols); col++)
		{
			playfield_add_column(pf);
		}		
	}
	else if(old_n_cols > n_cols)
	{
		// remove columns
		for(row = 0; row < old_n_rows; row++)
		{
			GPtrArray *row_array = (GPtrArray*) g_ptr_array_index(pf->matrix,
									     row);
			for(col = n_cols; col < old_n_cols; col++)
			{
				Tile *tile = playfield_get_tile_nc(pf, row, col);
				tile_destroy(tile);
			}
			g_ptr_array_set_size(row_array, n_cols);
		}
	}	
	pf->n_cols = n_cols;

	
	// handle rows
	if(old_n_rows < n_rows) 
	{
		// add rows
		for(row = 0; row < (n_rows - old_n_rows); row++)
		{
			playfield_add_row(pf);
		}		
	}
	else if(old_n_rows > n_rows)
	{
		// remove rows
		for(row = n_rows; col < old_n_rows; row++)
		{
			GPtrArray *row_array = (GPtrArray*) g_ptr_array_index(pf->matrix,
									      row);
			for(col = 0; col < old_n_cols; col++)
			{
				Tile *tile = playfield_get_tile_nc(pf, row, col);
				tile_destroy(tile);
			}
			g_ptr_array_free(row_array, FALSE);
		}
		g_ptr_array_set_size(pf->matrix, n_rows);
	}
	pf->n_rows = n_rows;
}

void
playfield_destroy(PlayField* pf)
{
	int row, col;

	for(row = 0; row < pf->n_rows; row++) 
	{
		GPtrArray *array_row = (GPtrArray*) g_ptr_array_index(pf->matrix, 
								      row);
		for(col = 0; col < pf->n_cols; col++)
		{
			tile_destroy((Tile*)g_ptr_array_index(array_row, col));
		}
		g_ptr_array_free(array_row, FALSE);
		
	}
	g_ptr_array_free(pf->matrix, FALSE);

	g_free(pf);
}

PlayField*
playfield_copy(PlayField* pf)
{
	PlayField* pf_copy = NULL;
	guint row,col;

	if(pf) 
	{
		pf_copy = playfield_new();
		g_assert(pf_copy!=NULL);
		for(row=0; row < pf->n_rows; row++) 
		{
			playfield_add_row(pf_copy);
			for(col=0; col < pf->n_cols; col++)
			{
				if(row==0)
				{
					playfield_add_column(pf_copy);
				}
				playfield_set_tile(pf_copy, row, col, 
						   tile_copy(playfield_get_tile(pf, row, col)));

			}
		}    
	}
#ifdef DEBUG
	else
	{
		g_print("pf equals NULL.\n");
	}
#endif
    
	return pf_copy;
}

void
playfield_swap_tiles(PlayField* pf, guint src_row, guint src_col,
		     guint dest_row, guint dest_col)
{
	Tile* src_tile;
	src_tile = tile_copy(playfield_get_tile(pf, src_row, src_col));
	
	playfield_set_tile_nc(pf, src_row, src_col, 
			      playfield_get_tile(pf, dest_row, dest_col));
	playfield_set_tile(pf, dest_row, dest_col, src_tile);
}


Tile*
playfield_clear_tile(PlayField *pf, guint row, guint col)
{
	Tile *tile;

	if(pf!=NULL && 
	   (row < pf->n_rows) && (row >= 0 )&&
	   (col < pf->n_cols) && (col >= 0)) 
	{	
		tile = tile_copy(playfield_get_tile(pf, row, col));
		playfield_set_tile(pf, row, col, tile_new());
	
		return tile;
	}
#ifdef DEBUG
	else {
		g_print("playfield_clear_tile: Array out of bounds or pointer equals NULL!\n");
	}
#endif    
    
	return NULL;
}

void
playfield_clear(PlayField *pf)
{
	gint row, col;

	g_return_if_fail(pf!=NULL);
	
	for(row = 0; row < pf->n_rows; row++)
	{
		for(col = 0; col < pf->n_cols; col++)
		{
			Tile *tile = playfield_clear_tile(pf, row, col);
			tile_destroy(tile);
		}
	}
       
}

PlayField*
playfield_strip(PlayField *pf)
{
	PlayField *stripped_pf;
	gint row, col;
	gint n_rows, n_cols;
	gint max_row = 0;
	gint min_row = 10000;
	gint max_col = 0; 
	gint min_col = 10000;

	/* determine the really used playfield size */
	for(row = 0; row < pf->n_rows; row++)
	{
		for(col = 0; col < pf->n_cols; col++)
		{
			Tile *tile = playfield_get_tile(pf, row, col);
			if(tile_get_unique_id(tile) != 0)
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
	
	if(((n_rows > 0) && (n_cols > 0)) && 
	   ((pf->n_rows != n_rows) || (pf->n_cols != n_cols)))
	{
		stripped_pf = playfield_new();
		playfield_set_matrix_size(stripped_pf, n_rows, n_cols);

		for(row = 0; row < n_rows; row++)
		{
			for(col = 0; col < n_cols; col++)
			{
				Tile *tile;
				tile = playfield_get_tile(pf, row + min_row, 
							  col + min_col);
				if(tile_get_unique_id(tile) != 0)
				{
					playfield_set_tile(stripped_pf,
							   row, col,
							   tile_copy(tile));
				}
			}
		}
	}
	else
	{
		stripped_pf = playfield_copy(pf);
	}
	
	return stripped_pf;
}

Tile*
playfield_get_tile(PlayField *pf, guint row, guint col)
{
	if(pf!=NULL && 
	   (row < pf->n_rows) && (row >= 0 )&&
	   (col < pf->n_cols) && (col >= 0)) 
	{
		return playfield_get_tile_nc(pf, row, col);
	}

#ifdef DEBUG
	else {
		g_print("playfield_get_tile() Array out of bounds or pointer equals NULL!\n");
	}
#endif
    
	return NULL;
}

void
playfield_set_tile(PlayField* pf, guint row, guint col, Tile *tile)
{
	if(pf!=NULL && 
	   (row < pf->n_rows) && (row >= 0 )&&
	   (col < pf->n_cols) && (col >= 0)) 
	{
		playfield_set_tile_nc(pf, row, col, tile);
		tile_destroy(tile);
	}

#ifdef DEBUG
	else {
		g_print("playfield_set_tile: Array out of bounds or pointer equals NULL!\n");
	}
#endif
}

void
playfield_print(PlayField* pf)
{
	int row, col;
	Tile *tile;

	if(pf)
	{
		g_print("N_ROWS: %d\n",pf->n_rows);
		g_print("N_COLS: %d\n",pf->n_cols);
	
		for(row = 0; row < pf->n_rows; row++) 
		{
			for(col = 0; col < pf->n_cols; col++)
			{
				tile = playfield_get_tile(pf, row, col); 
				if(tile)
				{
					g_print("%d|%d|%d ",
						tile_get_type(tile),
						tile_get_image_id(tile),
						0 /*tile_get_connection_id(tile)*/);
					
				}
				else
				{
					g_print("NULL ");
				}
			}
			g_print("\n");
		}
		g_print("\n");
	}
	else
	{
		g_print("Can´t print playfield because of NULL pointer.\n");
	}
}


PlayField* 
playfield_load_xml(xmlNodePtr pf_node, gint revision)
{
	xmlNodePtr row_node;
	xmlNodePtr tile_node;
	PlayField *pf;
	gint row, col;

	pf = playfield_new();
    
	/* read the matrix */
	row = col = 0;

	if(revision < 2)
	{
		/* reading old format */
		row_node = pf_node->xmlChildrenNode;
		while(row_node != NULL)
		{
			if(g_strcasecmp(row_node->name, "ROW")==0)
			{
				playfield_add_row(pf);
				tile_node = row_node->xmlChildrenNode;
				col = 0;
	    
				while(tile_node != NULL)
				{	    
					if(g_strcasecmp(tile_node->name, "TILE")==0)
					{
						Tile *tile = tile_load_xml(tile_node, 
									   revision);
						if(row==0)
						{
							playfield_add_column(pf);
						}
					
						playfield_set_tile(pf, row, col, tile);
						col++;
					}
					else
					{
						g_print("Unexpected Tag (<%s>), ignoring.\n", 
							tile_node->name);
					}
		
					tile_node = tile_node->next;
				}
				row++;
			}
			else
			{
				g_print("Unexpected Tag (<%s>), ignoring.\n", 
					row_node->name);
			}

			row_node = row_node->next;
		}
	}
	else
	{
		/* reading new format */
		gchar *prop_value;
		gint row, col;
		gint n_rows, n_cols;
		Tile *tile;

		/* reading number of columns and rows */
		prop_value = xmlGetProp(pf_node, "rows");
		n_rows = atoi(prop_value);
		prop_value = xmlGetProp(pf_node, "cols");
		n_cols = atoi(prop_value);
		
		playfield_set_matrix_size(pf, n_rows, n_cols);

		/* reading non empty tiles */
		tile_node = pf_node->xmlChildrenNode;
		while(tile_node)
		{
			if(g_strcasecmp(tile_node->name, "TILE")==0)
			{
				prop_value = xmlGetProp(tile_node, "row");
				row = atoi(prop_value);
				prop_value = xmlGetProp(tile_node, "col");
				col = atoi(prop_value);
				
				tile = tile_load_xml(tile_node, revision);
				playfield_set_tile(pf, row, col, tile);
			}
			else
			{
				g_print("playfield.: Unexpected Tag (<%s>), ignoring.\n", 
					tile_node->name);
			}
			
			tile_node = tile_node->next;
		}

	}

	return pf;
}

/*
 * Creates the XML representation of an PlayField.
 * pf = Playfield to save
 * pf_node = reference to the <PLAYFIELD> Tag node
 */
void playfield_save_xml(PlayField *pf, xmlNodePtr pf_node)
{
	gint row, col;
	xmlAttrPtr attr;
	xmlNodePtr tile_node;
	gint max_row = 0;
	gint min_row = 10000;
	gint max_col = 0; 
	gint min_col = 10000;
	gint n_rows, n_cols;

	/* determine the really used playfield size */
	for(row = 0; row < pf->n_rows; row++)
	{
		for(col = 0; col < pf->n_cols; col++)
		{
			Tile *tile = playfield_get_tile(pf, row, col);
			if(tile_get_unique_id(tile) != 0)
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
		for(row = 0; row < pf->n_rows; row++)
		{	
			for(col = 0; col < pf->n_cols; col++)
			{
				Tile *tile = playfield_get_tile(pf, row, col);
				if(tile_get_unique_id(tile) != 0)
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
    
		length = g_snprintf(str_buffer, 5, "%i", pf->n_rows);
		attr = xmlSetProp(pf_node, "rows", g_strdup(str_buffer));
		length = g_snprintf(str_buffer, 5, "%i", pf->n_cols);
		attr = xmlSetProp(pf_node, "cols", g_strdup(str_buffer));		

		g_free(str_buffer);		         
	}
}

/* playfield_get_tile_no_check */
Tile* 
playfield_get_tile_nc(PlayField *pf, gint row, gint col)
{
	GPtrArray *array_row = (GPtrArray*) g_ptr_array_index(pf->matrix, row);
	return (Tile*) g_ptr_array_index(array_row, col);
}

/* playfield_set_tile_no_check */
void
playfield_set_tile_nc(PlayField *pf, gint row, gint col, Tile *value_tile)
{
	GPtrArray *array_row = (GPtrArray*) g_ptr_array_index(pf->matrix, row);
	Tile *array_tile = g_ptr_array_index(array_row, col);
	tile_set_values(array_tile, value_tile);
}

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
#ifndef _ATOMIX_CANVAS_HELPER_H_
#define _ATOMIX_CANVAS_HELPER_H_

#include <gnome.h>
#include "theme.h"


/* 
 * CanvasMap is only a wrapper around a GHashTable. It's possible
 * that the implementation change, so I don't want the hashtable stuff
 * in other files. 
 */
typedef struct _CanvasMap  CanvasMap;
struct _CanvasMap
{
	GHashTable *table;
};


void set_background_color (GtkWidget *canvas, GdkColor *color);

void convert_to_playfield(Theme *theme, 
			  gdouble x, gdouble y, 
			  guint *row, guint *col);


void convert_to_canvas(Theme *theme, 
		       guint row, guint col, 
		       gdouble *x, gdouble *y);

void set_canvas_dimensions (GnomeCanvas *canvas, gint width, gint height);

GnomeCanvasGroup* create_group (GnomeCanvas *canvas,
				GnomeCanvasGroup *parent);

void free_imlib_image (GtkObject *object, gpointer data);

/*=======================================================
            Canvas Map functions 
---------------------------------------------------------*/
CanvasMap* canvas_map_new(void);

void canvas_map_destroy(CanvasMap *map);

void canvas_map_set_item(CanvasMap *map, guint row, guint col, GnomeCanvasItem *item);

GnomeCanvasItem* canvas_map_get_item(CanvasMap *map, guint row, guint col);

void canvas_map_move_item(CanvasMap *map, guint src_row, guint src_col, 
			  guint dest_row, guint dest_col);

void canvas_map_remove_item(CanvasMap *map, guint row, guint col);

#endif /* _ATOMIX_CANVAS_HELPER_H_ */



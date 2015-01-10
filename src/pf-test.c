#include <glib-object.h>
#include "playfield.h"

int main (void)
{
  PlayField *pf;
  PlayField *spf;
  PlayField *cpf;
  Tile *tile;

  pf = playfield_new ();

  playfield_set_matrix_size (pf, 10, 7);

  tile = tile_new (TILE_TYPE_ATOM);

  playfield_set_tile (pf, 0, 0, tile);
  playfield_set_tile (pf, 1, 0, tile);
  playfield_set_tile (pf, 2, 0, tile);
  playfield_set_tile (pf, 6, 3, tile);
  g_object_unref (tile);

  playfield_print (pf);

  /* test copy */
  g_message ("==== Copy Tests ====");
  cpf = playfield_copy (pf);
  playfield_print (cpf);

  tile = tile_new (TILE_TYPE_WALL);
  playfield_set_tile (pf, 3, 4, tile);
  g_object_unref (tile);

  playfield_print (pf);
  playfield_print (cpf);

  g_object_unref (cpf);

  /* test swap */
  g_message ("==== Swap Tests ====");
  playfield_swap_tiles (pf, 2, 0, 0, 2);
  playfield_print (pf);

  /* test strip */
  spf = playfield_strip (pf);
  playfield_print (spf);
  g_object_unref (spf);

  g_object_unref (pf);

  exit (0);
}

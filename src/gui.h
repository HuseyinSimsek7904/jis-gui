/*
This file is part of jis-gui.

jis-gui is free software: you can redistribute it and/or modify it under the
terms of the GNU General Public License as published by the Free Software
Foundation, either version 3 of the License, or (at your option) any later
version.

jis-gui is distributed in the hope that it will be useful, but WITHOUT ANY
WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A
PARTICULAR PURPOSE. See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License along with
jis-gui. If not, see <https://www.gnu.org/licenses/>.
*/

#ifndef GUI_H
#define GUI_H

#include "jis_process.h"

#include <raylib.h>

extern const char *GUI_TITLE;

extern const int GRID_SQUARE_SIZE;
extern const float GRID_CIRCLE_RATIO;

extern const Color BACKGROUND_COLOR;
extern const Color GRID_WHITE_COLOR;
extern const Color GRID_BLACK_COLOR;
extern const Color GRID_HELD_COLOR;

extern const Color CIRCLE_TO_COLOR;
extern const Color CIRCLE_CAPTURE_COLOR;

extern const Color LAST_MOVE_FROM_COLOR;
extern const Color LAST_MOVE_TO_COLOR;

extern const Rectangle HISTORY_RECT;
extern const Rectangle BOARD_RECT;

extern const int WINDOW_WIDTH;
extern const int WINDOW_HEIGHT;

extern const int MOVE_ANIM_FRAMES;

typedef struct {
  Texture2D grid_texture;
  Texture2D circle_texture;

  Texture2D white_pawn_texture;
  Texture2D white_knight_texture;
  Texture2D black_pawn_texture;
  Texture2D black_knight_texture;
} assets;

Vector2 pos_to_window_vec(int pos);
Vector2 pos_to_window_vec_center(int pos);
Rectangle pos_to_window_rect(int pos);

int window_vec_to_id(Vector2 vec);

float quad_interpolate(float x);
Rectangle anim_linint(Rectangle start, Rectangle end, float t);

move find_move_for_position(move available_moves[4], int position);

Texture2D load_texture_rel(const char *root_path, const char *rel_path);

void gui_init();

void gui_load_assets(assets *assets);
void gui_unload_assets(assets *assets);

void gui_make_move(jis_process *process, assets *gui_assets, char *board,
                   bool *board_turn, int *board_status, char *move_string,
                   move *last_move);

#endif

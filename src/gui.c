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

#include "gui.h"
#include "position.h"

#include <libgen.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const char *GUI_TITLE = "JazzInSea - Cez GUI";

const int GRID_SQUARE_SIZE = 100;
const float GRID_CIRCLE_RATIO = 0.3;

const Color BACKGROUND_COLOR = (Color){0x20, 0x20, 0x20, 0xff};
const Color GRID_WHITE_COLOR = (Color){0xf0, 0xf0, 0xf0, 0xff};
const Color GRID_BLACK_COLOR = (Color){0xa0, 0xa0, 0xa0, 0xff};
const Color GRID_HELD_COLOR = (Color){0xcc, 0xaa, 0x22, 0x80};

const Color CIRCLE_TO_COLOR = (Color){0x55, 0x55, 0x55, 0x80};
const Color CIRCLE_CAPTURE_COLOR = (Color){0xff, 0x00, 0x00, 0x80};

const Color LAST_MOVE_FROM_COLOR = (Color){0x66, 0xff, 0xff, 0x80};
const Color LAST_MOVE_TO_COLOR = (Color){0x66, 0xff, 0xff, 0x60};

const Rectangle HISTORY_RECT = (Rectangle){900, 80, 200, 200};
const Rectangle BOARD_RECT =
    (Rectangle){50, 50, 8 * GRID_SQUARE_SIZE, 8 * GRID_SQUARE_SIZE};

const int WINDOW_WIDTH = 1150;
const int WINDOW_HEIGHT = 900;

const int MOVE_ANIM_FRAMES = 20;

Vector2 pos_to_window_vec(int pos) {
  return (Vector2){to_col(pos) * GRID_SQUARE_SIZE + BOARD_RECT.x,
                   (7 - to_row(pos)) * GRID_SQUARE_SIZE + BOARD_RECT.y};
}

Vector2 pos_to_window_vec_center(int pos) {
  return (Vector2){(to_col(pos) - 0.5) * GRID_SQUARE_SIZE + BOARD_RECT.x +
                       GRID_SQUARE_SIZE,
                   (7.5 - to_row(pos)) * GRID_SQUARE_SIZE + BOARD_RECT.y};
}

Rectangle pos_to_window_rect(int pos) {
  return (Rectangle){to_col(pos) * GRID_SQUARE_SIZE + BOARD_RECT.x,
                     (7 - to_row(pos)) * GRID_SQUARE_SIZE + BOARD_RECT.y,
                     GRID_SQUARE_SIZE, GRID_SQUARE_SIZE};
}

int window_vec_to_id(Vector2 vec) {
  return ((int)((vec.x - BOARD_RECT.x) / GRID_SQUARE_SIZE) +
          (7 - (int)((vec.y - BOARD_RECT.y) / GRID_SQUARE_SIZE)) * 8);
}

float quad_interpolate(float x) { return 2 * x - x * x; }

Rectangle anim_linint(Rectangle start, Rectangle end, float t) {
  return (Rectangle){(end.x - start.x) * t + start.x,
                     (end.y - start.y) * t + start.y, start.width,
                     start.height};
}

move find_move_for_position(move available_moves[4], int position) {
  // Iterate through all available moves and check if the 'to' or
  // 'capture' positions matches.
  for (int i = 0; i < 4; i++) {
    move move = available_moves[i];

    if (move.to == position || move.capture == position)
      return move;
  }

  return (move){.from = POSITION_INV};
}

Texture2D load_texture_rel(const char *root_path, const char *rel_path) {
  char path[strlen(root_path) + strlen(rel_path)];
  strcpy(path, root_path);
  strcat(path, rel_path);
  return LoadTexture(path);
}

void gui_init() {
  InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, GUI_TITLE);

  SetTargetFPS(90);
}

void gui_load_assets(assets *assets) {
  // Create the board grid texture.
  {
    Color *pixel_data =
        malloc(64 * GRID_SQUARE_SIZE * GRID_SQUARE_SIZE * sizeof(Color));

    if (!pixel_data) {
      fprintf(stderr, "error: malloc failed\n");
      perror("malloc");
      exit(1);
    }

    // I do not believe that's the best way to do this.
    // Rows are flipped since the topmost row is the 0th row.
    for (int row = 0; row < 8 * GRID_SQUARE_SIZE; row++) {
      for (int col = 0; col < 8 * GRID_SQUARE_SIZE; col++) {
        pixel_data[row * 8 * GRID_SQUARE_SIZE + col] =
            (row / GRID_SQUARE_SIZE + col / GRID_SQUARE_SIZE) % 2
                ? GRID_BLACK_COLOR
                : GRID_WHITE_COLOR;
      }
    }

    Image grid_image = {pixel_data, 8 * GRID_SQUARE_SIZE, 8 * GRID_SQUARE_SIZE,
                        1, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8};
    assets->grid_texture = LoadTextureFromImage(grid_image);
    UnloadImage(grid_image);
  }

  // Create a circle texture which can be used to indicate the move target
  // squares.
  {
    Image circle_image =
        GenImageColor(GRID_SQUARE_SIZE * 2, GRID_SQUARE_SIZE * 2, BLANK);

    ImageDrawCircle(&circle_image, GRID_SQUARE_SIZE, GRID_SQUARE_SIZE,
                    GRID_SQUARE_SIZE * GRID_CIRCLE_RATIO, WHITE);

    assets->circle_texture = LoadTextureFromImage(circle_image);

    GenTextureMipmaps(&assets->circle_texture);
    SetTextureFilter(assets->circle_texture, TEXTURE_FILTER_TRILINEAR);

    UnloadImage(circle_image);
  }

  // Get the path of the binary to figure out the path of the share directory.
  // This is not very reliable sadly.
  const char *bin_path = getenv("_");
  char root_path[strlen(bin_path) + 16];
  strcpy(root_path, bin_path);
  dirname(root_path);
  strcat(root_path, "/../");

  // Load the piece textures.
  assets->white_pawn_texture =
      load_texture_rel(root_path, "share/jis-gui/wP.png");
  assets->white_knight_texture =
      load_texture_rel(root_path, "share/jis-gui/wN.png");
  assets->black_pawn_texture =
      load_texture_rel(root_path, "share/jis-gui/bP.png");
  assets->black_knight_texture =
      load_texture_rel(root_path, "share/jis-gui/bN.png");

  GenTextureMipmaps(&assets->white_pawn_texture);
  GenTextureMipmaps(&assets->white_knight_texture);
  GenTextureMipmaps(&assets->black_pawn_texture);
  GenTextureMipmaps(&assets->black_knight_texture);

  SetTextureFilter(assets->white_pawn_texture, TEXTURE_FILTER_TRILINEAR);
  SetTextureFilter(assets->white_knight_texture, TEXTURE_FILTER_TRILINEAR);
  SetTextureFilter(assets->black_pawn_texture, TEXTURE_FILTER_TRILINEAR);
  SetTextureFilter(assets->black_knight_texture, TEXTURE_FILTER_TRILINEAR);
}

void gui_unload_assets(assets *assets) {
  UnloadTexture(assets->grid_texture);
  UnloadTexture(assets->circle_texture);

  UnloadTexture(assets->white_pawn_texture);
  UnloadTexture(assets->white_knight_texture);
  UnloadTexture(assets->black_pawn_texture);
  UnloadTexture(assets->black_knight_texture);
}

void gui_make_move(jis_process *process, assets *gui_assets, char *board,
                   bool *board_turn, int *board_status, char *move_string,
                   move *last_move) {
  jis_make_move(process, board, board_turn, board_status, move_string);
  *last_move = jis_desc_move(process, move_string);
}

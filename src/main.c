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

#include <raylib.h>

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

const int SQUARE_SIZE = 100;
const Color GRID_WHITE_COLOR = (Color){0xf0, 0xf0, 0xf0, 0xff};
const Color GRID_BLACK_COLOR = (Color){0xa0, 0xa0, 0xa0, 0xff};
const Color GRID_HELD_COLOR = (Color){0xcc, 0xaa, 0x22, 0x80};

const Vector2 BOARD_VECTOR = (Vector2){50, 50};

// A position out of the bounds [0, 63] is an invalid position.
static inline int to_position(int row, int col) { return (row << 3) + col; }

static inline bool is_valid(int position) {
  return position >= 0 && position < 64;
}
static inline int to_row(int position) { return position >> 3; }
static inline int to_col(int position) { return position & 7; }

#define POSITION_INV -1

Vector2 pos_to_window_vec(int pos) {
  return (Vector2){to_col(pos) * SQUARE_SIZE + BOARD_VECTOR.x,
                   (7 - to_row(pos)) * SQUARE_SIZE + BOARD_VECTOR.y};
}

Rectangle pos_to_window_rect(int pos) {
  return (Rectangle){to_col(pos) * SQUARE_SIZE + BOARD_VECTOR.x,
                     (7 - to_row(pos)) * SQUARE_SIZE + BOARD_VECTOR.y,
                     SQUARE_SIZE, SQUARE_SIZE};
}

int window_vec_to_id(Vector2 vec) {
  return ((int)((vec.x - BOARD_VECTOR.x) / SQUARE_SIZE) +
          (7 - (int)((vec.y - BOARD_VECTOR.y) / SQUARE_SIZE)) * 8);
}

int main(int argc, char *argv[]) {
  InitWindow(900, 900, "JazzInSea - Cez GUI");

  SetTargetFPS(60);

  // Create the board grid texture.
  Texture2D grid_texture;
  {
    Color *pixel_data = malloc(64 * SQUARE_SIZE * SQUARE_SIZE * sizeof(Color));

    // I do not believe that's the best way to do this.
    // Rows are flipped since the topmost row is the 0th row.
    for (int row = 0; row < 8 * SQUARE_SIZE; row++) {
      for (int col = 0; col < 8 * SQUARE_SIZE; col++) {
        pixel_data[row * 8 * SQUARE_SIZE + col] =
            (row / SQUARE_SIZE + col / SQUARE_SIZE) % 2 ? GRID_BLACK_COLOR
                                                        : GRID_WHITE_COLOR;
      }
    }

    Image grid_image = {pixel_data, 8 * SQUARE_SIZE, 8 * SQUARE_SIZE, 1,
                        PIXELFORMAT_UNCOMPRESSED_R8G8B8A8};
    grid_texture = LoadTextureFromImage(grid_image);
    UnloadImage(grid_image);
  }

  // Load the piece textures.
  Texture2D white_pawn_texture = LoadTexture("assets/wP.png");
  Texture2D white_knight_texture = LoadTexture("assets/wN.png");
  Texture2D black_pawn_texture = LoadTexture("assets/bP.png");
  Texture2D black_knight_texture = LoadTexture("assets/bN.png");

  SetTextureFilter(white_pawn_texture, TEXTURE_FILTER_TRILINEAR);
  SetTextureFilter(white_knight_texture, TEXTURE_FILTER_TRILINEAR);
  SetTextureFilter(black_pawn_texture, TEXTURE_FILTER_TRILINEAR);
  SetTextureFilter(black_knight_texture, TEXTURE_FILTER_TRILINEAR);

  char board[64] = {"np    PN"
                    "pp    PP"
                    "        "
                    "        "
                    "        "
                    "        "
                    "PP    pp"
                    "NP    pn"};

  int held_piece = POSITION_INV;

  while (!WindowShouldClose()) {
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
      held_piece = window_vec_to_id(GetMousePosition());
    } else if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
      held_piece = POSITION_INV;
    }

    BeginDrawing();
    DrawTextureRec(grid_texture,
                   (Rectangle){0, 0, grid_texture.width, grid_texture.height},
                   BOARD_VECTOR, WHITE);

    if (held_piece >= 0) {
      DrawRectangleRec(pos_to_window_rect(held_piece), GRID_HELD_COLOR);
    }

    // Rows are inverted, unlike how jazz-in-sea represents them.
    for (int position = 0; position < 64; position++) {
      Texture2D *texture;
      switch (board[position]) {
      case 'P':
        texture = &white_pawn_texture;
        break;
      case 'N':
        texture = &white_knight_texture;
        break;
      case 'p':
        texture = &black_pawn_texture;
        break;
      case 'n':
        texture = &black_knight_texture;
        break;
      default:
        continue;
      }

      Rectangle rect;
      if (position == held_piece) {
        int x = GetMouseX() - SQUARE_SIZE / 2;
        int y = GetMouseY() - SQUARE_SIZE / 2;
        rect = (Rectangle){x, y, SQUARE_SIZE, SQUARE_SIZE};
      } else {
        rect = pos_to_window_rect(position);
      }

      // Upsize the texture to the square size and draw it.
      DrawTexturePro(*texture,
                     (Rectangle){0, 0, texture->width, texture->height}, rect,
                     (Vector2){0, 0}, 0, WHITE);
    }

    EndDrawing();
  }

  // Deinitialize resources.
  UnloadTexture(grid_texture);
  UnloadTexture(white_pawn_texture);
  UnloadTexture(white_knight_texture);
  UnloadTexture(black_pawn_texture);
  UnloadTexture(black_knight_texture);
  CloseWindow();
}

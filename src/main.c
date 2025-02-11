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

int main(int argc, char *argv[]) {
  InitWindow(800, 800, "JazzInSea - Cez GUI");

  SetTargetFPS(60);

  // Create the board grid texture.
  Texture2D grid_texture;
  {
    Color *pixel_data = malloc(64 * SQUARE_SIZE * SQUARE_SIZE * sizeof(Color));

    for (int row = 0; row < 8 * SQUARE_SIZE; row++) {
      for (int col = 0; col < 8 * SQUARE_SIZE; col++) {
        pixel_data[row * 8 * SQUARE_SIZE + col] =
            (row / SQUARE_SIZE + col / SQUARE_SIZE) % 2 ? GRID_WHITE_COLOR
                                                        : GRID_BLACK_COLOR;
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

  char board[8][8] = {
      "np    PN", "pp    PP", "        ", "        ",
      "        ", "        ", "PP    pp", "NP    pn",
  };

  while (!WindowShouldClose()) {
    BeginDrawing();
    DrawTextureRec(grid_texture,
                   (Rectangle){0, 0, grid_texture.width, grid_texture.height},
                   (Vector2){0, 0}, WHITE);

    for (int row = 0; row < 8; row++) {
      for (int col = 0; col < 8; col++) {
        Texture2D *texture;
        switch (board[row][col]) {
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

        // Upsize the texture to the square size and draw it.
        DrawTexturePro(*texture,
                       (Rectangle){0, 0, texture->width, texture->height},
                       (Rectangle){col * SQUARE_SIZE, row * SQUARE_SIZE,
                                   SQUARE_SIZE, SQUARE_SIZE},
                       (Vector2){0, 0}, 0, WHITE);
      }
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

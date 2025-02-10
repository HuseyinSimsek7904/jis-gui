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

  while (!WindowShouldClose()) {
    BeginDrawing();
    DrawTextureRec(grid_texture,
                   (Rectangle){0, 0, grid_texture.width, grid_texture.height},
                   (Vector2){0, 0}, WHITE);
    EndDrawing();
  }

  // Deinitialize resources.
  UnloadTexture(grid_texture);
  CloseWindow();
}

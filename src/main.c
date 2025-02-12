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

#include "fen.h"
#include "position.h"

#include <raylib.h>

#include <assert.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

const int GRID_SQUARE_SIZE = 100;
const int GRID_CIRCLE_RADIUS = 20;

const Color BACKGROUND_COLOR = (Color){0x20, 0x20, 0x20, 0xff};
const Color GRID_WHITE_COLOR = (Color){0xf0, 0xf0, 0xf0, 0xff};
const Color GRID_BLACK_COLOR = (Color){0xa0, 0xa0, 0xa0, 0xff};
const Color GRID_HELD_COLOR = (Color){0xcc, 0xaa, 0x22, 0x80};
const Color GRID_TO_COLOR = (Color){0x55, 0x55, 0x55, 0x80};
const Color GRID_CAPTURE_COLOR = (Color){0xff, 0x00, 0xff, 0x80};

const Rectangle BOARD_RECT =
    (Rectangle){50, 50, 8 * GRID_SQUARE_SIZE, 8 * GRID_SQUARE_SIZE};

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

const char *JIS_EXECUTABLE = "jazzinsea";

void ask_jis(int child_stdin, int child_stdout, char *buffer,
             size_t buffer_size, const char *format, ...) {
  va_list args;
  va_start(args, format);

  vdprintf(child_stdin, format, args);

  int length = read(child_stdout, buffer, buffer_size);
  if (length < 0) {
    fprintf(stderr, "error: reading from %s failed\n", JIS_EXECUTABLE);
    perror("read");
    exit(1);
  }
}

typedef struct {
  int from;
  int to;
  int capture;
} move;

int main(int argc, char *argv[]) {
  InitWindow(900, 900, "JazzInSea - Cez GUI");

  SetTargetFPS(60);

  // Create the board grid texture.
  Texture2D grid_texture;
  {
    Color *pixel_data =
        malloc(64 * GRID_SQUARE_SIZE * GRID_SQUARE_SIZE * sizeof(Color));

    if (!pixel_data) {
      fprintf(stderr, "error: malloc failed\n");
      perror("malloc");
      return 1;
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
    grid_texture = LoadTextureFromImage(grid_image);
    UnloadImage(grid_image);
  }

  // Load the piece textures.
  Texture2D white_pawn_texture = LoadTexture("assets/wP.png");
  Texture2D white_knight_texture = LoadTexture("assets/wN.png");
  Texture2D black_pawn_texture = LoadTexture("assets/bP.png");
  Texture2D black_knight_texture = LoadTexture("assets/bN.png");

  GenTextureMipmaps(&white_pawn_texture);
  GenTextureMipmaps(&white_knight_texture);
  GenTextureMipmaps(&black_pawn_texture);
  GenTextureMipmaps(&black_knight_texture);

  SetTextureFilter(white_pawn_texture, TEXTURE_FILTER_TRILINEAR);
  SetTextureFilter(white_knight_texture, TEXTURE_FILTER_TRILINEAR);
  SetTextureFilter(black_pawn_texture, TEXTURE_FILTER_TRILINEAR);
  SetTextureFilter(black_knight_texture, TEXTURE_FILTER_TRILINEAR);

  // Index 0 will be used for reading, and 1 will be used for writing.
  int child_stdin_pipe[2];
  int child_stdout_pipe[2];

  if (pipe(child_stdin_pipe) < 0 || pipe(child_stdout_pipe) < 0) {
    fprintf(stderr, "error: pipe failed\n");
    perror("pipe");
    return 1;
  }

  int pid = fork();

  if (pid < 0) {
    fprintf(stderr, "error: fork failed\n");
    perror("fork");
    return 1;
  }

  if (pid == 0) {
    // The child process, close unused ends of pipe.
    close(child_stdin_pipe[1]);
    close(child_stdout_pipe[0]);

    if (dup2(child_stdin_pipe[0], fileno(stdin)) < 0 ||
        dup2(child_stdout_pipe[1], fileno(stdout)) < 0) {
      fprintf(stderr, "error: dup2 on child failed\n");
      perror("dup2");
      exit(1);
    }

    // Execute the jazzinsea executable
    execlp(JIS_EXECUTABLE, JIS_EXECUTABLE, (char *)NULL);
    fprintf(stderr, "error: execl failed\n");
    perror("execl");
    exit(1);
  }

  // The parent process, close unused ends of pipe.
  close(child_stdin_pipe[0]);
  close(child_stdout_pipe[1]);

  int child_stdin = child_stdin_pipe[1];
  int child_stdout = child_stdout_pipe[0];

  char board[64];
  bool board_turn;
  if (!load_fen("np4PN/pp4PP/8/8/8/8/PP4pp/NP4pn w", board, &board_turn)) {
    fprintf(stderr, "error: could not load board FEN from string\n");
    return 1;
  }

  int held_piece = POSITION_INV;

  move available_moves[4];

  while (!WindowShouldClose()) {
    Vector2 mouse_vec = GetMousePosition();

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
      if (CheckCollisionPointRec(mouse_vec, BOARD_RECT)) {
        held_piece = POSITION_INV;
        int pressed_position = window_vec_to_id(mouse_vec);

        // Clear all moves.
        for (int i = 0; i < 4; i++) {
          available_moves[i].from = POSITION_INV;
        }

        if (board[pressed_position] != ' ') {
          held_piece = pressed_position;

          char position_str[3];
          get_position_str(held_piece, position_str);

          char buffer[256];
          ask_jis(child_stdin, child_stdout, buffer, sizeof(buffer),
                  "allmoves %s\n", position_str);

          // Add available moves.
          int i = 0;
          char *old = buffer + 2;
          while (*old != '}') {
            char *new = strchr(old, ' ');
            if (!new) {
              fprintf(stderr, "error: invalid moves list from jazzinsea\n");
              return 1;
            }
            *new = '\0';

            // Ask jazzinsea to describe the move.
            // This will fill the buffer with 'from', 'to' and 'capture'
            // position strings seperated by spaces.
            char buffer[256];
            ask_jis(child_stdin, child_stdout, buffer, sizeof(buffer),
                    "descmove %s\n", old);

            // Convert into a list of strings separated by \0.
            char *second_word = strchr(buffer, ' ');
            if (!second_word) {
              fprintf(stderr, "error: invalid description of move\n");
              return 1;
            }
            *(second_word++) = '\0';

            char *third_word = strchr(second_word + 1, ' ');
            if (!second_word) {
              fprintf(stderr, "error: invalid description of move\n");
              return 1;
            }
            *(third_word++) = '\0';

            // There can not be more than 4 moves.
            assert(i < 4);

            // Every move must originate from the held piece/
            int from_position = str_to_position(buffer);
            assert(from_position == held_piece);

            available_moves[i++] =
                (move){.from = from_position,
                       .to = str_to_position(second_word),
                       .capture = str_to_position(third_word)};

            old = new + 1;
          }
        }
      }
    } else if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
      held_piece = POSITION_INV;
    }

    BeginDrawing();
    ClearBackground(BACKGROUND_COLOR);

    DrawTextureRec(grid_texture,
                   (Rectangle){0, 0, grid_texture.width, grid_texture.height},
                   (Vector2){BOARD_RECT.x, BOARD_RECT.y}, WHITE);

    // Draw indicators to available squares.
    for (int i = 0; i < 4; i++) {
      move move = available_moves[i];

      if (!is_valid(move.from))
        continue;

      DrawCircleV(pos_to_window_vec_center(move.to), GRID_CIRCLE_RADIUS,
                  GRID_TO_COLOR);
    }

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
        int x = (int)(mouse_vec.x) - GRID_SQUARE_SIZE / 2;
        int y = (int)(mouse_vec.y) - GRID_SQUARE_SIZE / 2;
        rect = (Rectangle){x, y, GRID_SQUARE_SIZE, GRID_SQUARE_SIZE};
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

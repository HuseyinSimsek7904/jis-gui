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
#include "jis_process.h"
#include "position.h"

#include <raylib.h>

#include <assert.h>
#include <poll.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/poll.h>
#include <unistd.h>

const int GRID_SQUARE_SIZE = 100;
const float GRID_CIRCLE_RATIO = 0.3;

const Color BACKGROUND_COLOR = (Color){0x20, 0x20, 0x20, 0xff};
const Color GRID_WHITE_COLOR = (Color){0xf0, 0xf0, 0xf0, 0xff};
const Color GRID_BLACK_COLOR = (Color){0xa0, 0xa0, 0xa0, 0xff};
const Color GRID_HELD_COLOR = (Color){0xcc, 0xaa, 0x22, 0x80};
const Color GRID_TO_COLOR = (Color){0x55, 0x55, 0x55, 0x80};
const Color GRID_CAPTURE_COLOR = (Color){0xff, 0x00, 0x00, 0x80};

const Rectangle HISTORY_RECT = (Rectangle){900, 50, 200, 200};
const Rectangle BOARD_RECT =
    (Rectangle){50, 50, 8 * GRID_SQUARE_SIZE, 8 * GRID_SQUARE_SIZE};
const int WINDOW_WIDTH = 1150;
const int WINDOW_HEIGHT = 900;

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

int main(int argc, char *argv[]) {
  InitWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "JazzInSea - Cez GUI");

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

  // Create a circle texture which can be used to indicate the move target
  // squares.
  Texture2D circle_texture;
  {
    Image circle_image =
        GenImageColor(GRID_SQUARE_SIZE * 2, GRID_SQUARE_SIZE * 2, BLANK);

    ImageDrawCircle(&circle_image, GRID_SQUARE_SIZE, GRID_SQUARE_SIZE,
                    GRID_SQUARE_SIZE * GRID_CIRCLE_RATIO, WHITE);

    circle_texture = LoadTextureFromImage(circle_image);

    GenTextureMipmaps(&circle_texture);
    SetTextureFilter(circle_texture, TEXTURE_FILTER_TRILINEAR);

    UnloadImage(circle_image);
  }

  // Try to create a JazzInSea process.
  jis_process process = {.child_executable = JIS_EXECUTABLE};
  if (!jis_create_proc(&process)) {
    return 1;
  }

  // Copy the board position from the process.
  char board[64];
  bool board_turn;
  jis_copy_position(process, board, &board_turn);

  // The user interface states.
  int selected_piece = POSITION_INV;
  move available_moves[4] = {
      {POSITION_INV},
      {POSITION_INV},
      {POSITION_INV},
      {POSITION_INV},
  };

  enum { GUI, AI } players[2] = {AI, GUI};
  bool asked_for_move = false;

  while (!WindowShouldClose()) {
    Vector2 mouse_vec = GetMousePosition();

    if (players[board_turn] == AI) {
      if (!asked_for_move) {
        // Ask the AI for a move.
        jis_start_eval_r(process);
        asked_for_move = true;

      } else {
        // Check if AI returned a move.
        int result = jis_poll(process);

        if (result < 0)
          return 1;

        if (result > 0) {
          asked_for_move = false;

          // Child returned, make the generated move.
          char move_string[8];
          if (jis_read(process, move_string, sizeof(move_string)) < 0) {
            return 1;
          }

          jis_make_move(process, board, &board_turn, move_string);

          // If there is a selected piece, generated moves for it.
          if (is_valid(selected_piece)) {
            jis_ask_avail_moves(process, selected_piece, available_moves);
          }
        }
      }
    }

    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
      if (CheckCollisionPointRec(mouse_vec, BOARD_RECT)) {
        int pressed_position = window_vec_to_id(mouse_vec);

        move made_move =
            find_move_for_position(available_moves, pressed_position);

        // After searching available moves, clear all moves.
        for (int i = 0; i < 4; i++) {
          available_moves[i].from = POSITION_INV;
        }

        selected_piece = pressed_position;

        if (is_valid(made_move.from)) {
          // Make move on board and tell jazzinsea to update its board as well.
          jis_make_move(process, board, &board_turn, made_move.string);
          selected_piece = POSITION_INV;

        } else if (players[board_turn] == GUI &&
                   board[pressed_position] != ' ') {

          char position_str[3];
          get_position_str(selected_piece, position_str);

          char buffer[256];
          jis_ask(process, buffer, sizeof(buffer), "allmoves %s\n",
                  position_str);

          // Add available moves.
          int i = 0;
          char *current_move_string = buffer + 2;
          while (*current_move_string != '}') {
            char *new = strchr(current_move_string, ' ');
            if (!new) {
              fprintf(stderr, "error: invalid moves list from jazzinsea\n");
              return 1;
            }
            *new = '\0';

            // There can not be more than 4 moves.
            assert(i < 4);

            // Ask jazzinsea to describe the move.
            // Every move must originate from the held piece.
            move result = jis_desc_move(process, current_move_string);
            assert(result.from == selected_piece);
            available_moves[i++] = result;

            current_move_string = new + 1;
          }
        }
      }
    } else if (IsMouseButtonReleased(MOUSE_BUTTON_LEFT)) {
      if (board[selected_piece] == ' ') {
        selected_piece = POSITION_INV;

      } else {
        int pressed_position = window_vec_to_id(mouse_vec);
        move made_move =
            find_move_for_position(available_moves, pressed_position);

        if (is_valid(made_move.from)) {
          selected_piece = POSITION_INV;
          for (int i = 0; i < 4; i++) {
            available_moves[i].from = POSITION_INV;
          }

          // Make move on board and tell jazzinsea to update its board as well.
          jis_make_move(process, board, &board_turn, made_move.string);
        }
      }
    }
    BeginDrawing();
    ClearBackground(BACKGROUND_COLOR);

    DrawTextureRec(grid_texture,
                   (Rectangle){0, 0, grid_texture.width, grid_texture.height},
                   (Vector2){BOARD_RECT.x, BOARD_RECT.y}, WHITE);

    if (selected_piece >= 0) {
      DrawRectangleRec(pos_to_window_rect(selected_piece), GRID_HELD_COLOR);
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
      if (position == selected_piece && IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
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

    // Draw indicators to available squares.
    for (int i = 0; i < 4; i++) {
      move move = available_moves[i];

      if (!is_valid(move.from))
        continue;

      DrawTexturePro(
          circle_texture,
          (Rectangle){0, 0, circle_texture.width, circle_texture.height},
          pos_to_window_rect(move.to), (Vector2){0, 0}, 0, GRID_TO_COLOR);

      if (is_valid(move.capture))
        DrawTexturePro(
            circle_texture,
            (Rectangle){0, 0, circle_texture.width, circle_texture.height},
            pos_to_window_rect(move.capture), (Vector2){0, 0}, 0,
            GRID_CAPTURE_COLOR);
    }

    EndDrawing();
  }

  // Deinitialize resources.
  UnloadTexture(grid_texture);
  UnloadTexture(white_pawn_texture);
  UnloadTexture(white_knight_texture);
  UnloadTexture(black_pawn_texture);
  UnloadTexture(black_knight_texture);
  UnloadTexture(circle_texture);
  CloseWindow();
}

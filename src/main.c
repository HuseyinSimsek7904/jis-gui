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
#include "gui.h"
#include "jis_process.h"
#include "position.h"

#include <raylib.h>

#include <assert.h>
#include <libgen.h>
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

const char *JIS_EXECUTABLE = "jazzinsea";

int main(int argc, char *argv[]) {
  gui_init();

  // Load the assets.
  assets gui_assets;
  gui_load_assets(&gui_assets);

  // Try to create a JazzInSea process.
  jis_process process = {.child_executable = JIS_EXECUTABLE};
  if (!jis_create_proc(&process)) {
    return 1;
  }

  // Copy the board position from the process.
  char board[64];
  bool board_turn;
  int board_status;
  jis_copy_position(&process, board, &board_turn, &board_status);

  // The user interface states.
  int selected_piece = POSITION_INV;
  move available_moves[4] = {
      {POSITION_INV},
      {POSITION_INV},
      {POSITION_INV},
      {POSITION_INV},
  };
  move last_move = {POSITION_INV};

  enum { GUI, AI } players[2] = {AI, GUI};
  bool asked_for_move = false;

  uint anim_counter = MOVE_ANIM_FRAMES;

  while (!WindowShouldClose()) {
    Vector2 mouse_vec = GetMousePosition();

    if (players[board_turn] == AI) {
      if (!asked_for_move) {
        // Ask the AI for a move.
        jis_start_eval_r(&process);
        asked_for_move = true;

      } else {
        // Check if AI returned a move.
        int result = jis_poll(&process);

        if (result < 0)
          return 1;

        if (result > 0) {
          asked_for_move = false;

          // Child returned, make the generated move.
          char move_string[8];
          if (jis_read(&process, move_string, sizeof(move_string)) < 0) {
            return 1;
          }

          gui_make_move(&process, &gui_assets, board, &board_turn,
                        &board_status, move_string, &last_move);
          anim_counter = 0;

          // If there is a selected piece, generated moves for it.
          if (is_valid(selected_piece)) {
            jis_ask_avail_moves(&process, selected_piece, available_moves);
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
          gui_make_move(&process, &gui_assets, board, &board_turn,
                        &board_status, made_move.string, &last_move);
          anim_counter = 0;
          selected_piece = POSITION_INV;

        } else if (players[board_turn] == GUI &&
                   board[pressed_position] != ' ') {

          char position_str[3];
          get_position_str(selected_piece, position_str);

          char buffer[256];
          jis_ask(&process, buffer, sizeof(buffer), "allmoves %s\n",
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
            move result = jis_desc_move(&process, current_move_string);
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

          gui_make_move(&process, &gui_assets, board, &board_turn,
                        &board_status, made_move.string, &last_move);
          anim_counter = MOVE_ANIM_FRAMES;
        }
      }
    }

    // Board graphics
    BeginDrawing();
    ClearBackground(BACKGROUND_COLOR);

    DrawTextureRec(gui_assets.grid_texture,
                   (Rectangle){0, 0, gui_assets.grid_texture.width,
                               gui_assets.grid_texture.height},
                   (Vector2){BOARD_RECT.x, BOARD_RECT.y}, WHITE);

    if (is_valid(selected_piece) && selected_piece != last_move.to) {
      DrawRectangleRec(pos_to_window_rect(selected_piece), GRID_HELD_COLOR);
    }

    if (is_valid(last_move.from)) {
      DrawRectangleRec(pos_to_window_rect(last_move.from),
                       LAST_MOVE_FROM_COLOR);
      DrawRectangleRec(pos_to_window_rect(last_move.to), LAST_MOVE_TO_COLOR);
    }

    // Rows are inverted, unlike how jazz-in-sea represents them.
    for (int position = 0; position < 64; position++) {
      Texture2D *texture;
      switch (board[position]) {
      case 'P':
        texture = &gui_assets.white_pawn_texture;
        break;
      case 'N':
        texture = &gui_assets.white_knight_texture;
        break;
      case 'p':
        texture = &gui_assets.black_pawn_texture;
        break;
      case 'n':
        texture = &gui_assets.black_knight_texture;
        break;
      default:
        continue;
      }

      Rectangle rect;
      if (last_move.to == position && anim_counter < MOVE_ANIM_FRAMES) {

        Rectangle start_rect = pos_to_window_rect(last_move.from);
        Rectangle end_rect = pos_to_window_rect(last_move.to);

        rect = anim_linint(
            start_rect, end_rect,
            quad_interpolate((float)anim_counter / MOVE_ANIM_FRAMES));

      } else if (position == selected_piece &&
                 IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
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

      DrawTexturePro(gui_assets.circle_texture,
                     (Rectangle){0, 0, gui_assets.circle_texture.width,
                                 gui_assets.circle_texture.height},
                     pos_to_window_rect(move.to), (Vector2){0, 0}, 0,
                     CIRCLE_TO_COLOR);

      if (is_valid(move.capture))
        DrawTexturePro(gui_assets.circle_texture,
                       (Rectangle){0, 0, gui_assets.circle_texture.width,
                                   gui_assets.circle_texture.height},
                       pos_to_window_rect(move.capture), (Vector2){0, 0}, 0,
                       CIRCLE_CAPTURE_COLOR);
    }

    // Draw status text.
    const char *status_text = "<invalid>";
    switch (board_status >> 4) {
    case 0:
      if (board_turn)
        status_text = "White to play";
      else
        status_text = "Black to play";
      break;
    case 1:
      status_text = "Draw";
      break;
    case 2:
      status_text = "White wins";
      break;
    case 3:
      status_text = "Black wins";
      break;
    }
    DrawText(status_text, 900, 50, 30, WHITE);

    EndDrawing();

    if (anim_counter < MOVE_ANIM_FRAMES) {
      anim_counter++;
    }
  }

  // Make sure the jis process is no more.
  jis_kill_proc(&process);

  // Unload the assets.
  gui_unload_assets(&gui_assets);

  CloseWindow();
}

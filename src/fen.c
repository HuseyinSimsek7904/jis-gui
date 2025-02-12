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

#include <stdbool.h>

// Copy-pasted and slightly modified from JazzInSea source code.
bool load_fen(const char *fen, char *board, bool *turn) {
  int row = 0, col = 0;

  for (; *fen != ' '; fen++) {
    switch (*fen) {
    case '/':
      if (col != 8 || row >= 8)
        return false;

      // Skip to next row.
      row++;
      col = 0;
      break;

    case '1' ... '8':;
      // Add spaces.
      // Technically, 8/62/8/... is not a valid sentence as there should not be
      // digits right next to each other. However checking this seems
      // unnecessary.
      int spaces = *fen - '0';
      if (col + spaces > 8)
        return false;
      while (spaces--)
        board[to_position(row, col++)] = ' ';
      break;

    case 'P':
    case 'N':
    case 'p':
    case 'n':
      if (row > 7 || col > 7)
        return false;

      // Add the corresponding piece.
      board[to_position(row, col++)] = *fen;
      break;

    default:
      return false;
    }
  }

  // Check if we reached the end of the board.
  if (row != 7 || col != 8)
    return false;

  // Get the current player information.
  fen++;
  switch (*fen++) {
  case 'w':
    *turn = true;
    break;
  case 'b':
    *turn = false;
    break;
  default:
    return false;
  }

  // Check if we reached the end of the string.
  if (*fen != '\0')
    return false;

  return true;
}

// Copy-pasted and slightly modified from JazzInSea source code.
char *get_fen_string(char *fen, char *board, bool turn) {
  for (int position = 0; position < 64; position++) {
    char piece = board[position];

    if (piece == ' ') {
      if (*(fen - 1) >= '1' && *(fen - 1) <= '8') {
        (*(fen - 1))++;
      } else {
        *fen++ = '1';
      }
    } else {
      *fen++ = piece;
    }

    if (position % 8 == 7 && position / 8 < 7)
      *fen++ = '/';
  }

  *fen++ = ' ';
  *fen++ = turn ? 'w' : 'b';

  *fen = '\0';
  return fen;
}

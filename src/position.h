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

#ifndef POSITION_H
#define POSITION_H

#include <stdbool.h>

// An integer value out of the bounds [0, 63] is an invalid position.
static inline int to_position(int row, int col) { return (row << 3) + col; }

static inline bool is_valid(int position) {
  return position >= 0 && position < 64;
}
static inline int to_row(int position) { return position >> 3; }
static inline int to_col(int position) { return position & 7; }

#define POSITION_INV -1

void get_position_str(int position, char buffer[3]);

int str_to_position(char *buffer);

#endif

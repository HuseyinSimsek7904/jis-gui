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

#ifndef FEN_H
#define FEN_H

#include <stdbool.h>

// Load a board configuration from a FEN string.
bool load_fen(const char *fen, char *board, bool *turn);

// Generate a FEN string from a board configuration.
char *get_fen_string(char *fen, char *board, bool turn);

#endif

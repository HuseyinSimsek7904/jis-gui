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

#ifndef JIS_API_H
#define JIS_API_H

#include <stdbool.h>
#include <stddef.h>

typedef struct {
  const char *child_executable;
  int child_stdin;
  int child_stdout;
} jis_process;

typedef struct {
  int from;
  int to;
  int capture;
  char string[5];
} move;

// Create a subprocess by executing the process.child_executable.
bool jis_create_proc(jis_process *process);

// Block and read from the process stdout until some data is available.
int jis_read(jis_process process, char *buffer, size_t buffer_size);

// Send formatted commands to process stdin and collect the stdout of process.
int jis_ask(jis_process process, char *buffer, size_t buffer_size,
            const char *format, ...);

// Ask process to describe move and return.
move jis_desc_move(jis_process process, char *string);

// Ask process for the board position and copy it to GUI board.
bool jis_copy_position(jis_process process, char *board, bool *board_turn,
                       int *board_status);

// Ask process to make a move and copy the new board position.
bool jis_make_move(jis_process process, char *board, bool *board_turn,
                   int *board_status, char *move_string, move *last_move);

// Start a random evaluation on the process.
void jis_start_eval_r(jis_process process);

// Check if any data is available on the stdout of process.
int jis_poll(jis_process process);

// Ask the process the available moves originating from position.
bool jis_ask_avail_moves(jis_process process, int from_position,
                         move available_moves[4]);

#endif

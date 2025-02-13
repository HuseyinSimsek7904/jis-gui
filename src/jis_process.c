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

#include "jis_process.h"
#include "fen.h"
#include "position.h"

#include <assert.h>
#include <poll.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

bool jis_create_proc(jis_process *process) {
  // Index 0 will be used for reading, and 1 will be used for writing.
  int child_stdin_pipe[2];
  int child_stdout_pipe[2];

  if (pipe(child_stdin_pipe) < 0 || pipe(child_stdout_pipe) < 0) {
    fprintf(stderr, "error: pipe failed\n");
    perror("pipe");
    return false;
  }

  int pid = fork();

  if (pid < 0) {
    fprintf(stderr, "error: fork failed\n");
    perror("fork");
    return false;
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
    execlp(process->child_executable, process->child_executable, "-d%",
           (char *)NULL);
    fprintf(stderr, "error: execl failed\n");
    perror("execl");
    exit(1);
  }

  // The parent process, close unused ends of pipe.
  close(child_stdin_pipe[0]);
  close(child_stdout_pipe[1]);

  process->child_stdin = child_stdin_pipe[1];
  process->child_stdout = child_stdout_pipe[0];

  return true;
}

int jis_read(jis_process process, char *buffer, size_t buffer_size) {
  int length = read(process.child_stdout, buffer, buffer_size - 1);
  if (length < 0) {
    fprintf(stderr, "error: reading from %s failed\n",
            process.child_executable);
    perror("read");
    return length;
  }
  buffer[length] = '\0';
  return length;
}

int jis_ask(jis_process process, char *buffer, size_t buffer_size,
            const char *format, ...) {
  va_list args;
  va_start(args, format);

  vdprintf(process.child_stdin, format, args);
  return jis_read(process, buffer, buffer_size);
}

move jis_desc_move(jis_process process, char *string) {
  // Ask jazzinsea to describe a move.
  // This will fill the buffer with 'from', 'to' and 'capture'
  // position strings seperated by spaces.
  char first_word[256];
  jis_ask(process, first_word, sizeof(first_word), "descmove %s\n", string);

  // Convert into a list of strings separated by \0.
  char *second_word = strchr(first_word, ' ');
  if (!second_word) {
    fprintf(stderr, "error: invalid description of move\n");
    return (move){.from = POSITION_INV};
  }
  *(second_word++) = '\0';

  char *third_word = strchr(second_word + 1, ' ');
  if (!second_word) {
    fprintf(stderr, "error: invalid description of move\n");
    return (move){.from = POSITION_INV};
  }
  *(third_word++) = '\0';

  // Create the move object.
  move result = (move){.from = str_to_position(first_word),
                       .to = str_to_position(second_word),
                       .capture = str_to_position(third_word)};
  strcpy(result.string, string);

  return result;
}

bool jis_copy_position(jis_process process, char *board, bool *board_turn,
                       int *board_status) {
  char buffer[256];
  int length = jis_ask(process, buffer, sizeof(buffer), "savefen\n");
  if (length < 0)
    return false;

  load_fen(buffer, board, board_turn);
  length = jis_ask(process, buffer, sizeof(buffer), "status -i\n");
  if (length < 0)
    return false;

  *board_status = strtol(buffer, NULL, 10);
  return true;
}

bool jis_make_move(jis_process process, char *board, bool *board_turn,
                   int *board_status, char *move_string, move *last_move) {
  dprintf(process.child_stdin, "makemove %s\n", move_string);
  *last_move = jis_desc_move(process, move_string);
  return jis_copy_position(process, board, board_turn, board_status);
}

void jis_start_eval_r(jis_process process) {
  dprintf(process.child_stdin, "evaluate -r\n");
}

int jis_poll(jis_process process) {
  struct pollfd pollfd = {process.child_stdout, POLLIN};
  int result = poll(&pollfd, 1, 0);

  if (result < 0) {
    fprintf(stderr, "error: poll failed\n");
    perror("poll");
  }
  return result;
}

bool jis_ask_avail_moves(jis_process process, int from_position,
                         move available_moves[4]) {
  char position_str[3];
  get_position_str(from_position, position_str);

  char buffer[256];
  jis_ask(process, buffer, sizeof(buffer), "allmoves %s\n", position_str);

  // Add available moves.
  int i = 0;
  char *current_move_string = buffer + 2;
  while (*current_move_string != '}') {
    char *new = strchr(current_move_string, ' ');
    if (!new) {
      fprintf(stderr, "error: invalid moves list from %s\n",
              process.child_executable);
      return false;
    }
    *new = '\0';

    // There can not be more than 4 moves.
    assert(i < 4);

    // Ask jazzinsea to describe the move.
    // Every move must originate from the from_position.
    move result = jis_desc_move(process, current_move_string);
    assert(result.from == from_position);
    available_moves[i++] = result;

    current_move_string = new + 1;
  }

  return true;
}

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

void print_error() {
  char error_message[30] = "An error has occurred\n";
  write(STDERR_FILENO, error_message, strlen(error_message));
}

int main(int argc, char *argv[]) {
  char *line = NULL;
  size_t len = 0;
  ssize_t nread;

  // Initial path
  char *search_paths[128];
  search_paths[0] = strdup("/bin");
  int num_paths = 1;

  // Interactive mode if no arguments
  int interactive = (argc == 1);

  FILE *input_stream = stdin;

  if (argc > 1) {
    if (argc > 2) {
      print_error();
      exit(1);
    }
    input_stream = fopen(argv[1], "r");
    if (input_stream == NULL) {
      print_error();
      exit(1);
    }
    interactive = 0;
  }

  while (1) {
    if (interactive) {
      printf("wish> ");
      fflush(stdout);
    }

    nread = getline(&line, &len, input_stream);
    if (nread == -1) {
      // EOF reached
      break;
    }

    // Remove newline character at the end
    if (line[nread - 1] == '\n') {
      line[nread - 1] = '\0';
    }

    char *parallel_cmd_str = line;
    char *parallel_token;

    pid_t pids[128];
    int num_pids = 0;

    while ((parallel_token = strsep(&parallel_cmd_str, "&")) != NULL) {
      if (strlen(parallel_token) == 0) {
        continue;
      }

      char *cmd_str = parallel_token;
      char *redir_file = NULL;

      // Count '>' first
      int gt_count = 0;
      for (int j = 0; cmd_str[j] != '\0'; j++) {
        if (cmd_str[j] == '>')
          gt_count++;
      }

      if (gt_count > 1) {
        print_error();
        continue;
      }

      if (gt_count == 1) {
        char *left = strsep(&cmd_str, ">");
        char *right = cmd_str; // right of >

        char *r_token;
        char *r_args[10];
        int r_argc = 0;
        while ((r_token = strsep(&right, " \t")) != NULL) {
          if (strlen(r_token) > 0) {
            if (r_argc < 10)
              r_args[r_argc++] = r_token;
          }
        }
        if (r_argc != 1 || left == NULL) {
          print_error();
          continue;
        }
        redir_file = r_args[0];
        cmd_str = left;
      }

      char *token;
      char *args[128];
      int i = 0;

      while ((token = strsep(&cmd_str, " \t")) != NULL) {
        if (strlen(token) == 0) {
          continue;
        }
        args[i++] = token;
      }
      args[i] = NULL;

      if (i == 0) {
        if (redir_file != NULL) {
          print_error();
        }
        continue;
      }

      // Built-in command: exit
      if (strcmp(args[0], "exit") == 0) {
        if (args[1] != NULL) {
          print_error();
        } else {
          exit(0);
        }
        continue;
      }

      // Built-in command: cd
      if (strcmp(args[0], "cd") == 0) {
        if (args[1] == NULL || args[2] != NULL) {
          print_error();
        } else {
          if (chdir(args[1]) != 0) {
            print_error();
          }
        }
        continue;
      }

      // Built-in command: path
      if (strcmp(args[0], "path") == 0) {
        for (int p = 0; p < num_paths; p++) {
          free(search_paths[p]);
        }
        num_paths = 0;

        int p_idx = 1;
        while (args[p_idx] != NULL && num_paths < 128) {
          search_paths[num_paths++] = strdup(args[p_idx]);
          p_idx++;
        }
        continue;
      }

      // Find executable in path
      char exec_path[256];
      int found = 0;

      for (int p = 0; p < num_paths; p++) {
        snprintf(exec_path, sizeof(exec_path), "%s/%s", search_paths[p],
                 args[0]);
        if (access(exec_path, X_OK) == 0) {
          found = 1;
          break;
        }
      }

      if (!found) {
        print_error();
        continue;
      }

      // Create a subprocess to handle the command
      pid_t pid = fork();

      if (pid < 0) {
        print_error();
      } else if (pid == 0) {
        // Child process
        if (redir_file != NULL) {
          int fd = open(redir_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
          if (fd < 0) {
            print_error();
            exit(1);
          }
          dup2(fd, STDOUT_FILENO);
          dup2(fd, STDERR_FILENO);
          close(fd);
        }
        execv(exec_path, args);
        // If execv returns, it must have failed
        print_error();
        exit(1);
      } else {
        // Parent process stores pid
        pids[num_pids++] = pid;
      }
    }

    // Parent waits for all children from parallel commands
    for (int p = 0; p < num_pids; p++) {
      waitpid(pids[p], NULL, 0);
    }
  }

  free(line);
  if (!interactive) {
    fclose(input_stream);
  }
  return 0;
}

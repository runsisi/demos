//
// Created by runsisi on 10/6/23.
//

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <spawn.h>
#include <sys/wait.h>
#include <errno.h>

// modified from posixspawn
// https://github.com/AlexanderOMara/posixspawn

extern char **environ;

struct argdata {
  char *path;
  bool wait;
  char **args;
};

void usage(FILE *out) {
  fprintf(
      out,
      "Usage: vfork [options...] [--] [args...]\n"
      "\n"
      "options:\n"
      "  -h         Show this help message.\n"
      "  -p <path>  Set the executable path separate from argv[0].\n"
      "  -w         Wait for child process to complete before returning.\n"
      "\n"
      "args:\n"
      "  The remaining arguments are passed to the child process.\n"
      "\n"
  );
}

struct argdata parse_args(int argc, char **argv) {
  struct argdata args;
  // Check for the minimum 1 argument.
  if (argc <= 1) {
    usage(stderr);
    exit(EXIT_FAILURE);
  }
  // Initialize the return data.
  args.path = NULL;
  args.wait = 0;
  args.args = NULL;
  // Parse arguments.
  for (int opt; (opt = getopt(argc, argv, "hvf:p:w")) != -1;) {
    switch (opt) {
      case 'h': {
        usage(stdout);
        exit(EXIT_SUCCESS);
        break;
      }
      case 'p': {
        args.path = optarg;
        break;
      }
      case 'w': {
        args.wait = 1;
        break;
      }
      default: {
        exit(EXIT_FAILURE);
      }
    }
  }
  // Compute remaining arguments, requiring at least one more.
  int args_count = argc - optind;
  if (args_count <= 0 && !args.path) {
    fprintf(stderr, "%s: no executable specified\n", argv[0]);
    exit(EXIT_FAILURE);
  }
  // Initialize memory for the arguments array, plus a null terminator.
  args.args = malloc((args_count + 1) * sizeof(char *));
  // Loop over remaining arguments, inserting them.
  for (int i = optind; i < argc; i++) {
    args.args[i - optind] = argv[i];
  }
  // Add the null terminator.
  args.args[args_count] = NULL;
  return args;
}

int main(int argc, char **argv) {
  struct argdata args = parse_args(argc, argv);
  pid_t pid = vfork();
  if (pid == -1) {
    printf("ERROR: vfork: %s\n", strerror(errno));
    exit(EXIT_FAILURE);
  } else if (pid > 0) {
    // parent
  } else {
    // child
    execve(args.path ? args.path : args.args[0], args.args, environ);
    exit(EXIT_FAILURE); // exec never returns
  }
  printf("PID: %i\n", pid);
  if (args.wait) {
    int status;
    if (waitpid(pid, &status, 0) != -1) {
      printf("EXIT: %i\n", status);
    }
    else {
      perror("ERROR: waitpid");
    }
  }
  return EXIT_SUCCESS;
}

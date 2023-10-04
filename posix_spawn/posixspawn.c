/* 
 * Copyright (c) 2015-2022 Alexander O'Mara
 * 
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#define POSIXSPAWN_VERSION "2.0.0"

#define _GNU_SOURCE

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

#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE 1
#endif

extern char **environ;

struct constant {
	char *name;
	short flag;
};

struct argdata {
	short flags;
	char *path;
	bool wait;
	char **args;
};

struct constant poxis_spawn_items[] = {
	{ .name = "POSIX_SPAWN_RESETIDS"              , .flag = POSIX_SPAWN_RESETIDS               },
	{ .name = "POSIX_SPAWN_SETPGROUP"             , .flag = POSIX_SPAWN_SETPGROUP              },
	{ .name = "POSIX_SPAWN_SETSIGDEF"             , .flag = POSIX_SPAWN_SETSIGDEF              },
	{ .name = "POSIX_SPAWN_SETSIGMASK"            , .flag = POSIX_SPAWN_SETSIGMASK             },
  { .name = "POSIX_SPAWN_SETSCHEDPARAM"         , .flag = POSIX_SPAWN_SETSCHEDPARAM          },
  { .name = "POSIX_SPAWN_SETSCHEDULER"          , .flag = POSIX_SPAWN_SETSCHEDULER           },
  // Since glibc 2.24, this flag has no effect.
  { .name = "POSIX_SPAWN_USEVFORK"              , .flag = POSIX_SPAWN_USEVFORK               },
	{ .name = "POSIX_SPAWN_SETSID"                , .flag = POSIX_SPAWN_SETSID                 },

};

void usage(FILE *out) {
	fprintf(
		out,
		"posixspawn -- The power of posix_spawn in your shell.\n"
		"Version %s\n"
		"Copyright (c) 2015-2022 Alexander O'Mara\n"
		"Licensed under MPL 2.0 <http://mozilla.org/MPL/2.0/>\n"
		"\n"
		"Usage: posixspawn [options...] [--] [args...]\n"
		"\n"
		"options:\n"
		"  -h         Show this help message.\n"
		"  -v         Show version.\n"
		"  -f <flags> Pipe-delimited list of flags (see flags section below).\n"
		"  -p <path>  Set the executable path separate from argv[0].\n"
		"  -w         Wait for child process to complete before returning.\n"
		"\n"
		"args:\n"
		"  The remaining arguments are passed to the child process.\n"
		"\n"
		"flags:\n"
		"  The flags arguments is a pipe-delimited list of constants or integers.\n"
		"  A flag can be a string constant, a base-16 string, or a base-10 string.\n"
		"  Example argument:\n"
		"    \"EXAMPLE_CONSTANT|0xF0|16\"\n"
		"  String constants:\n",
		POSIXSPAWN_VERSION
	);
	size_t constant_length = sizeof(poxis_spawn_items) / sizeof(struct constant);
	for (size_t i = 0; i < constant_length; i++) {
		fprintf(
			out,
			"    0x%04x  %s\n",
			poxis_spawn_items[i].flag,
			poxis_spawn_items[i].name
		);
	}
	fprintf(out, "\n");
}

void version() {
	printf("%s\n", POSIXSPAWN_VERSION);
}

short parse_constant(char *s, struct constant *c, size_t constant_size) {
	size_t constant_length = constant_size / sizeof(struct constant);
	size_t s_s = 0;
	size_t s_e = 0;
	short r = 0;
	size_t s_length = strlen(s);
	for (size_t i = 0; i < s_length; i++) {
		bool last = i == s_length - 1;
		if (last || s[i] == '|') {
			// Set the endpoint for the current segment.
			s_e = last ? i + 1 : i;
			// Calculate the length of this segment.
			size_t s_l = s_e - s_s;
			// If this segment has length, look for a match.
			if (s_l) {
				short flag = 0;
				// Loop through the constants looking for a match.
				for (size_t j = 0; j < constant_length; j++) {
					// If a match, break the loop and set the value.
					if (s_l == strlen(c[j].name) && !strncmp(&s[s_s], c[j].name, s_l)) {
						flag = c[j].flag;
						break;
					}
				}
				// If no flags identified, maybe hex or integer.
				if (!flag) {
					// Maybe hex.
					if (s_l > 2 && s[s_s] == '0' && (s[s_s + 1] == 'x' || s[s_s + 1] == 'X')) {
						flag = (short)strtol(&s[s_s], NULL, 0);
					}
					// If still zero, maye just integer.
					if (!flag) {
						flag = atoi(&s[s_s]);
					}
				}
				// Update the return value with the new bits.
				r |= flag;
			}
			// Update segment start position.
			s_s = i + 1;
		}
	}
	return r;
}

struct argdata parse_args(int argc, char **argv) {
	struct argdata args;
	// Check for the minimum 1 argument.
	if (argc <= 1) {
		usage(stderr);
		exit(EXIT_FAILURE);
	}
	// Initialize the return data.
	args.flags = 0;
	args.path = NULL;
	args.wait = FALSE;
	args.args = NULL;
	// Parse arguments.
	for (int opt; (opt = getopt(argc, argv, "hvf:p:w")) != -1;) {
		switch (opt) {
			case 'h': {
				usage(stdout);
				exit(EXIT_SUCCESS);
				break;
			}
			case 'v': {
				version();
				exit(EXIT_SUCCESS);
				break;
			}
			case 'f': {
				args.flags = parse_constant(optarg, poxis_spawn_items, sizeof(poxis_spawn_items));
				break;
			}
			case 'p': {
				args.path = optarg;
				break;
			}
			case 'w': {
				args.wait = TRUE;
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
	pid_t pid;
	posix_spawnattr_t attr;
	posix_spawnattr_init(&attr);
	posix_spawnattr_setflags(&attr, args.flags);
  int status;
  // reuse this flag
	if (!(args.flags & POSIX_SPAWN_USEVFORK)) {
    status = posix_spawn(&pid, args.path ? args.path : args.args[0], NULL, &attr, args.args, environ);
    if (status) {
      printf("ERROR: posix_spawn: %s\n", strerror(status));
      exit(EXIT_FAILURE);
    }
  } else {
    pid = vfork();
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
  }
	printf("PID: %i\n", pid);
	if (args.wait) {
		if (waitpid(pid, &status, 0) != -1) {
			printf("EXIT: %i\n", status);
		}
		else {
			perror("ERROR: waitpid");
		}
	}
	return EXIT_SUCCESS;
}

//
// Created by runsisi on 10/4/23.
//

#include <stdio.h>

int main(int argc, const char *argv[]) {
  fprintf(stderr, "argc: %d\n", argc);
  fprintf(stderr, "argv[0]: %s\n", argv[0]);
  fprintf(stderr, "argv[1]: %s\n", argv[1]);

  return 0;
}
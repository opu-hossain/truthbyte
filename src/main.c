#include "commands.h"
#include <getopt.h>
#include <stdio.h>
#include <string.h>

int main(int argc, char *argv[]) {
  if (argc < 2) {
    cmd_help(0, NULL);
    return 1;
  }

  for (int i = 0; commands[i].name != NULL; i++) {
    if (strcmp(argv[1], commands[i].name) == 0) {
      return commands[i].handler(argc - 1, argv + 1);
    }
  }

  printf("Unknown command: %s\n", argv[1]);
  cmd_help(0, NULL);
  return 1;
}

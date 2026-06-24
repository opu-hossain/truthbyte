#ifndef COMMANDS_H
#define COMMANDS_H

typedef int (*command_handler_t)(int argc, char *argv[]);

typedef struct {
  const char *name;
  command_handler_t handler;
  const char *help;
} Command;

// Command function declarations
int cmd_hash(int argc, char *argv[]);
int cmd_verify(int argc, char *argv[]);
int cmd_help(int argc, char *argv[]);
int cmd_version(int argc, char *argv[]);
int cmd_scan(int argc, char *argv[]);

// External command table
extern Command commands[];

#endif // !COMMANDS_H

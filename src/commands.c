#include "commands.h"
#include "hash.h"
#include <getopt.h>
#include <stdio.h>
#include <string.h>

Command commands[] = {{"hash", cmd_hash, "Calculate hash of a file"},
                      {"verify", cmd_verify, "Verify file hash"},
                      {"help", cmd_help, "Show this help"},
                      {"version", cmd_version, "Shows version"},
                      {NULL, NULL, NULL}};

int cmd_hash(int argc, char *argv[]) {
  static struct option long_options[] = {

      {"sha256", no_argument, 0, 256}, {"sha512", no_argument, 0, 512},
      {"md5", no_argument, 0, 5},      {"sha1", no_argument, 0, 1},
      {"help", no_argument, 0, 'h'},   {0, 0, 0, 0}};

  HashAlgorithm algo = HASH_SHA256;
  const char *fp = NULL;
  int opt;

  while ((opt = getopt_long(argc, argv, "", long_options, NULL)) != -1) {
    switch (opt) {
    case 256:
      algo = HASH_SHA256;
      break;
    case 512:
      algo = HASH_SHA512;
      break;
    case 5:
      algo = HASH_MD5;
      break;

    case 1:
      algo = HASH_SHA1;
      break;
    case 'h':
      printf("Usage: truthbyte hash [--sha256|--sha512|--md5|--sha1] <file>\n");
      printf("  --sha256    Use SHA256 (default)\n");
      printf("  --sha512    Use SHA512\n");
      printf("  --md5       Use MD5\n");
      printf("  --sha1      Use SHA1\n");
      printf("  --help      Show this help\n");
      return 0;
    }
  }

  if (optind >= argc) {
    printf("Error: No filename specified\n");
    printf("Usage: truthbyte hash [--algorithm] <filename>\n");
    return 1;
  }

  fp = argv[optind];

  // Compute hash
  unsigned char hash[MAX_HASH_LEN];
  unsigned int hash_len;

  if (compute_file_hash(fp, algo, hash, &hash_len) == 0) {
    printf("File: %s\n", fp);
    print_hash(hash, hash_len, algo);
    return 0;
  } else {
    printf("ERROR: Failed to compute hash for %s\n", fp);
    return 1;
  }
}

int cmd_verify(int argc, char *argv[]) {
  printf("Verify command - to be implemented\n");
  printf("Usage: truthbyte verify <filename> --hash <expected_hash>\n");
  return 0;
}

int cmd_help(int argc, char *argv[]) {
  printf("TruthByte - File Hashing Utility\n\n");
  printf("Usage: truthbyte <command> [options] [arguments]\n\n");
  printf("Commands:\n");
  for (int i = 0; commands[i].name != NULL; i++) {
    printf("  %-10s %s\n", commands[i].name, commands[i].help);
  }
  printf("\nFor help on a specific command: truthbyte <command> --help\n");
  return 0;
}

int cmd_version(int argc, char *argv[]) {
  // todo
  printf("TruthByte v2.0.0\n");
  return 0;
}

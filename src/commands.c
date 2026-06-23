#include "../include/truthbyte/commands.h"
#include "../include/truthbyte/hash.h"
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
  static struct option long_options[] = {{"sha256", no_argument, 0, 256},
                                         {"sha512", no_argument, 0, 512},
                                         {"md5", no_argument, 0, 5},
                                         {"sha1", no_argument, 0, 1},
                                         {"hash", required_argument, 0, 'H'},
                                         {"help", no_argument, 0, 'h'},
                                         {0, 0, 0, 0}};

  HashAlgorithm algo = HASH_SHA256;
  const char *filename = NULL;
  const char *expected_hash = NULL;
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
    case 'H':
      expected_hash = optarg;
      break;
    case 'h':
      printf("Usage: truthbyte verify [OPTIONS] <filename>\n");
      printf("Options:\n");
      printf("  --sha256    Use SHA256 (default)\n");
      printf("  --sha512    Use SHA512\n");
      printf("  --md5       Use MD5\n");
      printf("  --sha1      Use SHA1\n");
      printf("  --hash      Expected hash value to verify against\n");
      printf("  --help      Show this help\n");
      printf("\nExample:\n");
      printf("  truthbyte verify --sha256 --hash abc123... file.txt\n");
      return 0;
    default:
      return 1;
    }
  }

  if (optind >= argc) {
    printf("Error: No file name specified\n");
    printf("Usage: truthbyte verify [--algorithm] --hash <hash> <filename>\n");
    return 1;
  }

  filename = argv[optind];

  if (!expected_hash) {
    printf("Error: No Hash provided. Use --hash <hash_value>\n");
    return 1;
  }

  int result = verify_file_hash(filename, algo, expected_hash);

  const char *algo_name;
  switch (algo) {
  case HASH_SHA256:
    algo_name = "SHA256";
    break;
  case HASH_SHA512:
    algo_name = "SHA512";
    break;
  case HASH_MD5:
    algo_name = "MD5";
    break;
  case HASH_SHA1:
    algo_name = "SHA1";
    break;
  default:
    algo_name = "SHA256";
    break;
  }

  switch (result) {
  case 0:
    printf("✓ File '%s' matches the %s hash\n", filename, algo_name);
    return 0;
    break;
  case 1:
    printf("✗ File '%s' does NOT match the %s hash\n", filename, algo_name);
    printf("  Expected: %s\n", expected_hash);
    return 1;
  case -1:
    printf("Error: Could not compute hash for file '%s'\n", filename);
    return 1;
  case -2:
    printf("Error: Invalid hash format. Hash must be hexadecimal.\n");
    return 1;
  default:
    printf("Error: Unknown verification error\n");
    return 1;
  }
}

int cmd_help(int argc, char *argv[]) {
  (void)argc;
  (void)argv;

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
  (void)argc;
  (void)argv;

  // todo
  printf("TruthByte v2.0.0\n");
  return 0;
}

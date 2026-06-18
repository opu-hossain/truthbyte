#include "hash.h"
#include <ctype.h>
#include <openssl/evp.h>
#include <stdio.h>
#include <string.h>

#define BUFFER_SIZE 32768

HashAlgorithm parse_hash_algorithm(const char *algo_str) {
  if (strcmp(algo_str, "sha256") == 0) {
    return HASH_SHA256;
  } else if (strcmp(algo_str, "sha512") == 0) {
    return HASH_SHA512;
  } else if (strcmp(algo_str, "md5") == 0) {
    return HASH_MD5;
  } else if (strcmp(algo_str, "sha1") == 0) {
    return HASH_SHA1;
  }

  return HASH_SHA256;
}

const EVP_MD *get_evp_md(HashAlgorithm algo) {
  switch (algo) {
  case HASH_SHA256:
    return EVP_sha256();
    break;
  case HASH_SHA512:
    return EVP_sha512();
    break;
  case HASH_MD5:
    return EVP_md5();
    break;
  case HASH_SHA1:
    return EVP_sha1();
    break;
  default:
    return EVP_sha256();
  }
}

int compute_file_hash(const char *filename, HashAlgorithm algo,
                      unsigned char *md_value, unsigned int *md_len) {
  FILE *fp = fopen(filename, "rb");
  if (!fp) {
    printf("Error: Failed to open file!\n");
    return 1;
  }

  const EVP_MD *md_type = get_evp_md(algo);
  EVP_MD_CTX *mdctx = EVP_MD_CTX_new();
  if (!mdctx) {
    fclose(fp);
    return 1;
  }

  if (EVP_DigestInit_ex(mdctx, md_type, NULL) != 1) {
    EVP_MD_CTX_free(mdctx);
    fclose(fp);
    return 1;
  }

  unsigned char buffer[BUFFER_SIZE];
  size_t bytes_read;
  while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, fp)) > 0) {
    if (EVP_DigestUpdate(mdctx, buffer, bytes_read) != 1) {
      EVP_MD_CTX_free(mdctx);
      fclose(fp);
      return 1;
    }
  }

  if (EVP_DigestFinal_ex(mdctx, md_value, md_len) != 1) {
    EVP_MD_CTX_free(mdctx);
    fclose(fp);
    return 1;
  }

  EVP_MD_CTX_free(mdctx);
  fclose(fp);
  return 0;
}

void print_hash(const unsigned char *hash, unsigned int len,
                HashAlgorithm algo) {
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
  }

  printf("%s: ", algo_name);
  for (unsigned int i = 0; i < len; i++) {
    printf("%02x", hash[i]);
  }

  printf("\n");
}

int hex_to_bin(const char *hex_str, unsigned char *bin, unsigned int *bin_len) {
  size_t hex_len = strlen(hex_str);

  if (hex_len % 2 != 0) {
    return 1;
  }

  *bin_len = hex_len / 2;

  for (size_t i = 0; i < hex_len; i++) {
    if (!isxdigit(hex_str[i])) {
      return 1;
    }
  }

  for (size_t i = 0; i < *bin_len; i++) {
    sscanf(hex_str + (2 * i), "%02hhx", &bin[i]);
  }

  return 0;
}

int verify_file_hash(const char *filename, HashAlgorithm algo,
                     const char *expected_hash) {
  unsigned char computed_hash[MAX_HASH_LEN];
  unsigned int computed_len;

  if (compute_file_hash(filename, algo, computed_hash, &computed_len) != 0) {
    return -1;
  }

  unsigned char expected_bin[MAX_HASH_LEN];
  unsigned int expected_len;

  if (hex_to_bin(expected_hash, expected_bin, &expected_len) != 0) {
    return 1;
  }

  if (computed_len != expected_len) {
    return 1;
  }

  int result = 0;
  for (unsigned int i = 0; i < computed_len; i++) {
    result |= (computed_hash[i] ^ expected_bin[i]);
  }

  return (result == 0) ? 0 : 1;
}

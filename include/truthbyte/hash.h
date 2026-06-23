#ifndef HASH_H
#define HASH_H

#include <openssl/evp.h>

#define BUFFER_SIZE 32768
#define MAX_HASH_LEN 64

typedef enum {
  HASH_SHA256,
  HASH_SHA512,
  HASH_MD5,
  HASH_SHA1,
} HashAlgorithm;

// Convert string to enum
HashAlgorithm parse_hash_algorithm(const char *alog_str);

// Get EVP_MD from enum
const EVP_MD *get_evp_md(HashAlgorithm algo);

// Core hash function
int compute_file_hash(const char *filename, HashAlgorithm algo,
                      unsigned char *md_value, unsigned int *md_len);

// Convert hex string to binary (for hash verification)
int hex_to_bin(const char *hex_str, unsigned char *bin, unsigned int *bin_len);

// Compare outputed hash with expected hash string
int verify_file_hash(const char *filename, HashAlgorithm algo,
                     const char *expected_hash);

// Convert binary to hex string (for display);
int bin_to_hex(const unsigned char *hash, unsigned int len, char *output,
               size_t output_size);

// Helper function to print hash
void print_hash(const unsigned char *hash, unsigned int len,
                HashAlgorithm algo);

#endif

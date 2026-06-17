#ifndef CRYPTO_H
#define CRYPTO_H

#include "types.h"

int sha256_hex(const char *input, char *output_hex, size_t output_size);
int hash_meets_difficulty(const char *hash_hex, int difficulty);
void generate_keypair(Account *account);
int sign_transaction(Transaction *tx, const char *private_key_pem);
int verify_transaction_signature(const Transaction *tx, const char *public_key_hex);
void serialize_transaction_for_hash(const Transaction *tx, char *buffer, size_t buffer_size);

#endif

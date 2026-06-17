#include "crypto.h"
#include <openssl/ec.h>
#include <openssl/ecdsa.h>
#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/sha.h>
#include <stdio.h>
#include <string.h>

void serialize_transaction_for_hash(const Transaction *tx, char *buffer, size_t buffer_size) {
    snprintf(buffer, buffer_size,
             "%s|%s|%s|%.8f|%s|%ld|%llu",
             tx->transaction_id,
             tx->sender_address,
             tx->receiver_address,
             tx->amount,
             tx_type_to_string(tx->transaction_type),
             (long)tx->timestamp,
             (unsigned long long)tx->sender_nonce);
}

int sha256_hex(const char *input, char *output_hex, size_t output_size) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    if (!input || !output_hex || output_size < MAX_HASH_LEN) {
        return -1;
    }
    SHA256((const unsigned char *)input, strlen(input), hash);
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        snprintf(output_hex + (i * 2), 3, "%02x", hash[i]);
    }
    output_hex[64] = '\0';
    return 0;
}

int hash_meets_difficulty(const char *hash_hex, int difficulty) {
    if (!hash_hex || difficulty < 1) {
        return 0;
    }
    for (int i = 0; i < difficulty; i++) {
        if (hash_hex[i] != '0') {
            return 0;
        }
    }
    return 1;
}

static EC_KEY *create_ec_key(void) {
    EC_KEY *key = EC_KEY_new_by_curve_name(NID_secp256k1);
    if (!key) {
        return NULL;
    }
    if (EC_KEY_generate_key(key) != 1) {
        EC_KEY_free(key);
        return NULL;
    }
    return key;
}

void generate_keypair(Account *account) {
    EC_KEY *key = create_ec_key();
    if (!key) {
        return;
    }

    BIO *bio = BIO_new(BIO_s_mem());
    if (bio) {
        PEM_write_bio_ECPrivateKey(bio, key, NULL, NULL, 0, NULL, NULL);
        BUF_MEM *mem = NULL;
        BIO_get_mem_ptr(bio, &mem);
        if (mem && mem->length < sizeof(account->private_key_pem)) {
            memcpy(account->private_key_pem, mem->data, mem->length);
            account->private_key_pem[mem->length] = '\0';
        }
        BIO_free(bio);
    }

    const EC_POINT *pub = EC_KEY_get0_public_key(key);
    const EC_GROUP *group = EC_KEY_get0_group(key);
    char *pub_hex_ptr = EC_POINT_point2hex(group, pub, POINT_CONVERSION_COMPRESSED, NULL);
    if (pub_hex_ptr) {
        strncpy(account->public_key_hex, pub_hex_ptr, MAX_ADDR_LEN - 1);
        strncpy(account->address, pub_hex_ptr, MAX_ADDR_LEN - 1);
        OPENSSL_free(pub_hex_ptr);
    }

    EC_KEY_free(key);
}

static EC_KEY *load_private_key(const char *pem) {
    BIO *bio = BIO_new_mem_buf(pem, (int)strlen(pem));
    if (!bio) {
        return NULL;
    }
    EC_KEY *key = PEM_read_bio_ECPrivateKey(bio, NULL, NULL, NULL);
    BIO_free(bio);
    return key;
}

static EC_KEY *load_public_key_from_hex(const char *hex) {
    EC_KEY *key = EC_KEY_new_by_curve_name(NID_secp256k1);
    const EC_GROUP *group = EC_KEY_get0_group(key);
    EC_POINT *point = EC_POINT_hex2point(group, hex, NULL, NULL);
    if (!point) {
        EC_KEY_free(key);
        return NULL;
    }
    EC_KEY_set_public_key(key, point);
    EC_POINT_free(point);
    return key;
}

int sign_transaction(Transaction *tx, const char *private_key_pem) {
    char payload[MAX_LINE_LEN];
    char digest_hex[MAX_HASH_LEN];
    unsigned char digest[SHA256_DIGEST_LENGTH];

    serialize_transaction_for_hash(tx, payload, sizeof(payload));
    SHA256((const unsigned char *)payload, strlen(payload), digest);

    EC_KEY *key = load_private_key(private_key_pem);
    if (!key) {
        return -1;
    }

    ECDSA_SIG *sig = ECDSA_do_sign(digest, SHA256_DIGEST_LENGTH, key);
    EC_KEY_free(key);
    if (!sig) {
        return -1;
    }

    const BIGNUM *r = NULL;
    const BIGNUM *s = NULL;
    ECDSA_SIG_get0(sig, &r, &s);
    char *r_hex = BN_bn2hex(r);
    char *s_hex = BN_bn2hex(s);
    snprintf(tx->digital_signature, MAX_SIG_LEN, "%s:%s", r_hex, s_hex);
    OPENSSL_free(r_hex);
    OPENSSL_free(s_hex);
    ECDSA_SIG_free(sig);
    (void)digest_hex;
    return 0;
}

int verify_transaction_signature(const Transaction *tx, const char *public_key_hex) {
    if (!tx->digital_signature[0]) {
        return 0;
    }

    /* System-generated transactions use SYSTEM signature marker */
    if (strncmp(tx->digital_signature, "SYSTEM:", 7) == 0) {
        return 1;
    }

    char payload[MAX_LINE_LEN];
    unsigned char digest[SHA256_DIGEST_LENGTH];
    serialize_transaction_for_hash(tx, payload, sizeof(payload));
    SHA256((const unsigned char *)payload, strlen(payload), digest);

    char sig_copy[MAX_SIG_LEN];
    strncpy(sig_copy, tx->digital_signature, MAX_SIG_LEN - 1);
    char *colon = strchr(sig_copy, ':');
    if (!colon) {
        return 0;
    }
    *colon = '\0';
    char *r_hex = sig_copy;
    char *s_hex = colon + 1;

    BIGNUM *r = NULL;
    BIGNUM *s = NULL;
    BN_hex2bn(&r, r_hex);
    BN_hex2bn(&s, s_hex);
    ECDSA_SIG *sig = ECDSA_SIG_new();
    ECDSA_SIG_set0(sig, r, s);

    EC_KEY *key = load_public_key_from_hex(public_key_hex);
    if (!key) {
        ECDSA_SIG_free(sig);
        return 0;
    }

    int valid = ECDSA_do_verify(digest, SHA256_DIGEST_LENGTH, sig, key);
    ECDSA_SIG_free(sig);
    EC_KEY_free(key);
    return valid == 1;
}

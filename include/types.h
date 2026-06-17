#ifndef TYPES_H
#define TYPES_H

#include <stdint.h>
#include <time.h>

#define MAX_HASH_LEN 65
#define MAX_SIG_LEN 512
#define MAX_ADDR_LEN 128
#define MAX_ID_LEN 64
#define MAX_NAME_LEN 64
#define MAX_PLAN_LEN 32
#define MAX_BLOCKS 512
#define MAX_TX_PER_BLOCK 50
#define MAX_MEMPOOL 500
#define MAX_UTXOS 5000
#define MAX_ACCOUNTS 500
#define MAX_MEMBERS 200
#define MAX_PROVIDERS 100
#define MAX_POLICIES 500
#define MAX_CLAIMS 1000
#define MAX_HISTORY 5000
#define MAX_POOL_MINERS 16
#define MAX_CMD_LEN 512
#define MAX_LINE_LEN 1024
#define PERSISTENCE_FILE "chain_state.dat"

#define REINSURANCE_RATE 0.05
#define REINSURANCE_THRESHOLD 1000.0
#define RETARGET_INTERVAL 10
#define MIN_RETARGET_TIME 30
#define MAX_RETARGET_TIME 90
#define POLICY_DURATION_SEC (365L * 24 * 3600)
#define BLOCK_REWARD_DEFAULT 50.0
#define DIFFICULTY_DEFAULT 2
#define GENESIS_DIFFICULTY 2

#define INSURANCE_POOL_ADDR "INSURANCE_POOL"
#define REINSURANCE_POOL_ADDR "REINSURANCE_POOL"
#define MINER_POOL_ADDR "MINER_WALLET"
#define SYSTEM_ADDR "SYSTEM"

typedef enum {
    TX_POLICY_ENROLLMENT,
    TX_PREMIUM_PAYMENT,
    TX_REINSURANCE_CONTRIBUTION,
    TX_POLICY_RENEWAL,
    TX_SERVICE_REQUEST,
    TX_PREAUTH_REQUEST,
    TX_PREAUTH_APPROVE,
    TX_CLAIM_SUBMISSION,
    TX_CLAIM_APPROVAL,
    TX_CLAIM_REJECTION,
    TX_CLAIM_SETTLEMENT,
    TX_TOKEN_TRANSFER,
    TX_MINING_REWARD,
    TX_POOL_REWARD,
    TX_ACCOUNT_TRANSFER
} TransactionType;

typedef enum {
    MEMPOOL_PENDING,
    MEMPOOL_CONFIRMED,
    MEMPOOL_SUSPICIOUS
} MempoolStatus;

typedef enum {
    POLICY_ACTIVE,
    POLICY_EXPIRED,
    POLICY_RENEWED
} PolicyStatus;

typedef struct {
    char transaction_id[MAX_ID_LEN];
    char sender_address[MAX_ADDR_LEN];
    char receiver_address[MAX_ADDR_LEN];
    double amount;
    TransactionType transaction_type;
    time_t timestamp;
    uint64_t sender_nonce;
    char digital_signature[MAX_SIG_LEN];
} Transaction;

typedef struct {
    char transaction_id[MAX_ID_LEN];
    char sender[MAX_ADDR_LEN];
    char receiver[MAX_ADDR_LEN];
    double amount;
    TransactionType transaction_type;
    double fee;
    MempoolStatus status;
    Transaction tx;
} MempoolEntry;

typedef struct {
    int block_id;
    time_t timestamp;
    int transaction_count;
    char previous_hash[MAX_HASH_LEN];
    char merkle_root[MAX_HASH_LEN];
    uint64_t nonce;
    char miner_id[MAX_ADDR_LEN];
    int difficulty;
    char block_hash[MAX_HASH_LEN];
    Transaction transactions[MAX_TX_PER_BLOCK];
} Block;

typedef struct {
    char token_name[MAX_NAME_LEN];
    char token_symbol[8];
    double total_supply;
} Token;

typedef struct {
    char utxo_id[MAX_ID_LEN];
    char owner_address[MAX_ADDR_LEN];
    double amount;
    int spent;
    char origin_tx_id[MAX_ID_LEN];
} UTXO;

typedef struct {
    char address[MAX_ADDR_LEN];
    double balance;
    uint64_t nonce;
    char private_key_pem[512];
    char public_key_hex[MAX_ADDR_LEN];
} Account;

typedef struct {
    char member_id[MAX_ID_LEN];
    char name[MAX_NAME_LEN];
    char wallet_address[MAX_ADDR_LEN];
    int registered;
} Member;

typedef struct {
    char provider_id[MAX_ID_LEN];
    char name[MAX_NAME_LEN];
    char wallet_address[MAX_ADDR_LEN];
    int registered;
    double total_claims;
    int claim_count;
} Provider;

typedef struct {
    char policy_id[MAX_ID_LEN];
    char member_id[MAX_ID_LEN];
    char coverage_plan[MAX_PLAN_LEN];
    time_t enrollment_date;
    time_t expiry_date;
    PolicyStatus status;
} Policy;

typedef struct {
    char claim_id[MAX_ID_LEN];
    char policy_id[MAX_ID_LEN];
    char provider_id[MAX_ID_LEN];
    char member_id[MAX_ID_LEN];
    double amount;
    time_t submitted_at;
    int approved;
    int settled;
    int rejected;
} Claim;

typedef struct {
    int current_difficulty;
    double block_reward;
    int last_retarget_block;
    int chain_verified;
    int block_count;
    Block blocks[MAX_BLOCKS];
    MempoolEntry mempool[MAX_MEMPOOL];
    int mempool_count;
    UTXO utxos[MAX_UTXOS];
    int utxo_count;
    Account accounts[MAX_ACCOUNTS];
    int account_count;
    Member members[MAX_MEMBERS];
    int member_count;
    Provider providers[MAX_PROVIDERS];
    int provider_count;
    Policy policies[MAX_POLICIES];
    int policy_count;
    Claim claims[MAX_CLAIMS];
    int claim_count;
    Token token;
    char last_block_hash[MAX_HASH_LEN];
    int tx_counter;
    int claim_counter;
} ChainState;

const char *tx_type_to_string(TransactionType type);
TransactionType string_to_tx_type(const char *s);
const char *mempool_status_to_string(MempoolStatus status);
const char *policy_status_to_string(PolicyStatus status);

#endif

#include "types.h"
#include <stdio.h>
#include <string.h>

const char *tx_type_to_string(TransactionType type) {
    switch (type) {
        case TX_POLICY_ENROLLMENT: return "POLICY_ENROLLMENT";
        case TX_PREMIUM_PAYMENT: return "PREMIUM_PAYMENT";
        case TX_REINSURANCE_CONTRIBUTION: return "REINSURANCE_CONTRIBUTION";
        case TX_POLICY_RENEWAL: return "POLICY_RENEWAL";
        case TX_SERVICE_REQUEST: return "SERVICE_REQUEST";
        case TX_PREAUTH_REQUEST: return "PREAUTH_REQUEST";
        case TX_PREAUTH_APPROVE: return "PREAUTH_APPROVE";
        case TX_CLAIM_SUBMISSION: return "CLAIM_SUBMISSION";
        case TX_CLAIM_APPROVAL: return "CLAIM_APPROVAL";
        case TX_CLAIM_REJECTION: return "CLAIM_REJECTION";
        case TX_CLAIM_SETTLEMENT: return "CLAIM_SETTLEMENT";
        case TX_TOKEN_TRANSFER: return "TOKEN_TRANSFER";
        case TX_MINING_REWARD: return "MINING_REWARD";
        case TX_POOL_REWARD: return "POOL_REWARD";
        case TX_ACCOUNT_TRANSFER: return "ACCOUNT_TRANSFER";
        default: return "UNKNOWN";
    }
}

TransactionType string_to_tx_type(const char *s) {
    if (!s) return TX_TOKEN_TRANSFER;
    if (strcmp(s, "POLICY_ENROLLMENT") == 0) return TX_POLICY_ENROLLMENT;
    if (strcmp(s, "PREMIUM_PAYMENT") == 0) return TX_PREMIUM_PAYMENT;
    if (strcmp(s, "REINSURANCE_CONTRIBUTION") == 0) return TX_REINSURANCE_CONTRIBUTION;
    if (strcmp(s, "POLICY_RENEWAL") == 0) return TX_POLICY_RENEWAL;
    if (strcmp(s, "SERVICE_REQUEST") == 0) return TX_SERVICE_REQUEST;
    if (strcmp(s, "PREAUTH_REQUEST") == 0) return TX_PREAUTH_REQUEST;
    if (strcmp(s, "PREAUTH_APPROVE") == 0) return TX_PREAUTH_APPROVE;
    if (strcmp(s, "CLAIM_SUBMISSION") == 0) return TX_CLAIM_SUBMISSION;
    if (strcmp(s, "CLAIM_APPROVAL") == 0) return TX_CLAIM_APPROVAL;
    if (strcmp(s, "CLAIM_REJECTION") == 0) return TX_CLAIM_REJECTION;
    if (strcmp(s, "CLAIM_SETTLEMENT") == 0) return TX_CLAIM_SETTLEMENT;
    if (strcmp(s, "TOKEN_TRANSFER") == 0) return TX_TOKEN_TRANSFER;
    if (strcmp(s, "MINING_REWARD") == 0) return TX_MINING_REWARD;
    if (strcmp(s, "POOL_REWARD") == 0) return TX_POOL_REWARD;
    if (strcmp(s, "ACCOUNT_TRANSFER") == 0) return TX_ACCOUNT_TRANSFER;
    return TX_TOKEN_TRANSFER;
}

const char *mempool_status_to_string(MempoolStatus status) {
    switch (status) {
        case MEMPOOL_PENDING: return "PENDING";
        case MEMPOOL_CONFIRMED: return "CONFIRMED";
        case MEMPOOL_SUSPICIOUS: return "SUSPICIOUS";
        default: return "UNKNOWN";
    }
}

const char *policy_status_to_string(PolicyStatus status) {
    switch (status) {
        case POLICY_ACTIVE: return "ACTIVE";
        case POLICY_EXPIRED: return "EXPIRED";
        case POLICY_RENEWED: return "RENEWED";
        default: return "UNKNOWN";
    }
}

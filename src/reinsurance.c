#include "reinsurance.h"
#include "policy.h"
#include "account.h"
#include "mempool.h"
#include "transaction.h"
#include "utxo.h"
#include <stdio.h>
#include <string.h>

double reinsurance_balance(const ChainState *state) {
    const Account *acc = find_account((ChainState *)state, REINSURANCE_POOL_ADDR);
    return acc ? acc->balance : 0.0;
}

int pay_premium_with_contribution(ChainState *state, const char *member_addr,
                                  double amount, double fee) {
    if (amount <= 0) {
        printf("Invalid premium amount.\n");
        return -1;
    }

    Account *member = find_account(state, member_addr);
    if (!member) {
        printf("Member wallet not found.\n");
        return -1;
    }
    if (member->balance < amount) {
        printf("Insufficient balance for premium payment (need %.2f AHT).\n", amount);
        return -1;
    }

    double insurance_part = amount * (1.0 - REINSURANCE_RATE);
    double contribution = amount * REINSURANCE_RATE;

    Transaction premium;
    create_and_sign_transaction(state, &premium, member_addr, INSURANCE_POOL_ADDR,
                                insurance_part, TX_PREMIUM_PAYMENT, fee);
    if (mempool_add(state, &premium, fee) != 0) {
        return -1;
    }

    Transaction reins;
    create_and_sign_transaction(state, &reins, member_addr, REINSURANCE_POOL_ADDR,
                                contribution, TX_REINSURANCE_CONTRIBUTION, fee * 0.5);
    if (mempool_add(state, &reins, fee * 0.5) != 0) {
        return -1;
    }

    utxo_create(state, INSURANCE_POOL_ADDR, insurance_part, premium.transaction_id);

    printf("Premium payment %.2f AHT queued (Insurance: %.2f, Reinsurance: %.2f).\n",
           amount, insurance_part, contribution);
    return 0;
}

int settle_claim_with_reinsurance(ChainState *state, const char *claim_id) {
    Claim *claim = find_claim(state, claim_id);
    if (!claim) {
        printf("Claim not found: %s\n", claim_id);
        return -1;
    }
    if (!claim->approved || claim->settled || claim->rejected) {
        printf("Claim must be approved and not yet settled.\n");
        return -1;
    }

    Provider *prov = find_provider(state, claim->provider_id);
    if (!prov) {
        return -1;
    }

    double amount = claim->amount;
    char provider_addr[MAX_ADDR_LEN];
    strncpy(provider_addr, prov->wallet_address, MAX_ADDR_LEN - 1);

    if (amount <= REINSURANCE_THRESHOLD) {
        Transaction tx;
        create_and_sign_transaction(state, &tx, INSURANCE_POOL_ADDR, provider_addr,
                                    amount, TX_CLAIM_SETTLEMENT, 2.0);
        if (mempool_add(state, &tx, 2.0) != 0) {
            printf("Settlement failed: could not queue transaction.\n");
            return -1;
        }
        printf("Settlement of %.2f AHT from Insurance Pool queued.\n", amount);
    } else {
        double insurance_part = REINSURANCE_THRESHOLD;
        double reins_part = amount - REINSURANCE_THRESHOLD;
        Account *reins_acc = find_account(state, REINSURANCE_POOL_ADDR);

        Transaction tx1;
        create_and_sign_transaction(state, &tx1, INSURANCE_POOL_ADDR, provider_addr,
                                    insurance_part, TX_CLAIM_SETTLEMENT, 3.0);
        if (mempool_add(state, &tx1, 3.0) != 0) {
            printf("Settlement failed: could not queue insurance pool transaction.\n");
            return -1;
        }

        if (reins_acc && reins_acc->balance >= reins_part) {
            Transaction tx2;
            create_and_sign_transaction(state, &tx2, REINSURANCE_POOL_ADDR, provider_addr,
                                        reins_part, TX_CLAIM_SETTLEMENT, 3.0);
            if (mempool_add(state, &tx2, 3.0) != 0) {
                printf("Settlement failed: could not queue reinsurance transaction.\n");
                return -1;
            }
            printf("High-value settlement split: %.2f AHT (Insurance) + %.2f AHT (Reinsurance).\n",
                   insurance_part, reins_part);
        } else {
            double available = reins_acc ? reins_acc->balance : 0.0;
            if (available > 0) {
                Transaction tx2;
                create_and_sign_transaction(state, &tx2, REINSURANCE_POOL_ADDR, provider_addr,
                                            available, TX_CLAIM_SETTLEMENT, 3.0);
                if (mempool_add(state, &tx2, 3.0) != 0) {
                    printf("Settlement failed: could not queue partial reinsurance transaction.\n");
                    return -1;
                }
            }
            printf("WARNING: Reinsurance pool insufficient. Partial settlement flagged for manual review.\n");
            printf("Insurance: %.2f AHT, Reinsurance: %.2f AHT (requested %.2f).\n",
                   insurance_part, available, reins_part);
        }
    }

    claim->settled = 1;
    return 0;
}

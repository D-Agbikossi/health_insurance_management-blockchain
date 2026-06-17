#include "cli.h"
#include "policy.h"
#include "account.h"
#include "crypto.h"
#include "mempool.h"
#include "mining.h"
#include "blockchain.h"
#include "persistence.h"
#include "reinsurance.h"
#include "fraud.h"
#include "utxo.h"
#include "transaction.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int require_verified(ChainState *state) {
    if (!state->chain_verified) {
        printf("Chain not verified. Run 'blockchain_verify' first.\n");
        return 0;
    }
    return 1;
}

static void print_help(void) {
    printf("\n=== ALU Health Insurance Blockchain CLI ===\n\n");
    printf("Membership:     register_member, view_member, wallet_balance\n");
    printf("Policy:         enroll_policy, view_policy, renew_policy, policy_status\n");
    printf("Token:          token_transfer, token_balance\n");
    printf("Insurance:      pay_premium, service_request, preauth_request, preauth_approve\n");
    printf("                submit_claim, approve_claim, reject_claim, settle_claim\n");
    printf("                reinsurance_balance\n");
    printf("Blockchain:     create_transaction, mempool_view, mine_solo, mine_pool\n");
    printf("                blockchain_view, blockchain_verify, chain_save, chain_load\n");
    printf("                difficulty_status\n");
    printf("UTXO:           utxo_view, utxo_validate\n");
    printf("Account:        account_balance, account_transfer, account_nonce\n");
    printf("Fraud/Audit:    fraud_review, approve_suspicious, reject_suspicious\n");
    printf("                transaction_history, provider_history, premium_history, claim_history\n");
    printf("Other:          help, quit\n\n");
}

static void cmd_register_member(ChainState *state, char *args) {
    char member_id[MAX_ID_LEN], name[MAX_NAME_LEN];
    if (sscanf(args, "%63s %63s", member_id, name) < 2) {
        printf("Usage: register_member <member_id> <name>\n");
        return;
    }
    register_member(state, member_id, name);
}

static void cmd_view_member(ChainState *state, char *args) {
    char member_id[MAX_ID_LEN];
    if (sscanf(args, "%63s", member_id) < 1) {
        printf("Usage: view_member <member_id>\n");
        return;
    }
    Member *m = find_member(state, member_id);
    if (!m) {
        printf("Member not found.\n");
        return;
    }
    printf("Member ID: %s\nName: %s\nWallet: %s\n", m->member_id, m->name, m->wallet_address);
    account_view_balance(state, m->wallet_address);
}

static void cmd_wallet_balance(ChainState *state, char *args) {
    char member_id[MAX_ID_LEN];
    if (sscanf(args, "%63s", member_id) < 1) {
        printf("Usage: wallet_balance <member_id>\n");
        return;
    }
    Member *m = find_member(state, member_id);
    if (!m) { printf("Member not found.\n"); return; }
    account_view_balance(state, m->wallet_address);
}

static void cmd_enroll_policy(ChainState *state, char *args) {
    if (!require_verified(state)) return;
    char member_id[MAX_ID_LEN], policy_id[MAX_ID_LEN], plan[MAX_PLAN_LEN];
    if (sscanf(args, "%63s %63s %31s", member_id, policy_id, plan) < 3) {
        printf("Usage: enroll_policy <member_id> <policy_id> <coverage_plan>\n");
        return;
    }
    enroll_policy(state, member_id, policy_id, plan);
}

static void cmd_view_policy(ChainState *state, char *args) {
    char policy_id[MAX_ID_LEN];
    if (sscanf(args, "%63s", policy_id) < 1) {
        printf("Usage: view_policy <policy_id>\n");
        return;
    }
    Policy *p = find_policy(state, policy_id);
    if (!p) { printf("Policy not found.\n"); return; }
    printf("Policy ID:     %s\nMember ID:     %s\nPlan:          %s\n",
           p->policy_id, p->member_id, p->coverage_plan);
    printf("Enrolled:      %s", ctime(&p->enrollment_date));
    printf("Expiry:        %s", ctime(&p->expiry_date));
    printf("Status:        %s\n", policy_status_to_string(p->status));
}

static void cmd_renew_policy(ChainState *state, char *args) {
    if (!require_verified(state)) return;
    char policy_id[MAX_ID_LEN];
    if (sscanf(args, "%63s", policy_id) < 1) {
        printf("Usage: renew_policy <policy_id>\n");
        return;
    }
    renew_policy(state, policy_id);
}

static void cmd_policy_status(ChainState *state, char *args) {
    char policy_id[MAX_ID_LEN];
    if (sscanf(args, "%63s", policy_id) < 1) {
        printf("Usage: policy_status <policy_id>\n");
        return;
    }
    Policy *p = find_policy(state, policy_id);
    if (!p) { printf("Policy not found.\n"); return; }
    if (time(NULL) > p->expiry_date) p->status = POLICY_EXPIRED;
    printf("Policy %s status: %s\n", policy_id, policy_status_to_string(p->status));
}

static void cmd_token_transfer(ChainState *state, char *args) {
    if (!require_verified(state)) return;
    char from[MAX_ADDR_LEN], to[MAX_ADDR_LEN];
    double amount;
    if (sscanf(args, "%127s %127s %lf", from, to, &amount) < 3) {
        printf("Usage: token_transfer <from_addr> <to_addr> <amount>\n");
        return;
    }
    Transaction tx;
    Account *sender = find_account(state, from);
    if (!sender) { printf("Sender not found.\n"); return; }
    create_and_sign_transaction(state, &tx, from, to, amount, TX_TOKEN_TRANSFER, amount * 0.01);
    mempool_add(state, &tx, amount * 0.01);
}

static void cmd_token_balance(ChainState *state, char *args) {
    char addr[MAX_ADDR_LEN];
    if (sscanf(args, "%127s", addr) < 1) {
        printf("Usage: token_balance <address>\n");
        return;
    }
    account_view_balance(state, addr);
}

static void cmd_pay_premium(ChainState *state, char *args) {
    if (!require_verified(state)) return;
    char member_id[MAX_ID_LEN];
    double amount, fee = 1.0;
    int n = sscanf(args, "%63s %lf %lf", member_id, &amount, &fee);
    if (n < 2) {
        printf("Usage: pay_premium <member_id> <amount> [fee]\n");
        return;
    }
    Member *m = find_member(state, member_id);
    if (!m) { printf("Member not found.\n"); return; }
    pay_premium_with_contribution(state, m->wallet_address, amount, fee);
}

static void cmd_register_provider(ChainState *state, char *provider_id, char *name) {
    if (find_provider(state, provider_id)) return;
    if (state->provider_count >= MAX_PROVIDERS) return;
    Provider *p = &state->providers[state->provider_count++];
    strncpy(p->provider_id, provider_id, MAX_ID_LEN - 1);
    strncpy(p->name, name, MAX_NAME_LEN - 1);
    Account *wallet = &state->accounts[state->account_count];
    memset(wallet, 0, sizeof(*wallet));
    generate_keypair(wallet);
    state->account_count++;
    strncpy(p->wallet_address, wallet->address, MAX_ADDR_LEN - 1);
    wallet->balance = 500.0;
    printf("Provider registered: %s (%s) Wallet: %s\n", provider_id, name, p->wallet_address);
}

static void cmd_service_request(ChainState *state, char *args) {
    if (!require_verified(state)) return;
    char member_id[MAX_ID_LEN], policy_id[MAX_ID_LEN], desc[MAX_NAME_LEN];
    if (sscanf(args, "%63s %63s %63s", member_id, policy_id, desc) < 3) {
        printf("Usage: service_request <member_id> <policy_id> <description>\n");
        return;
    }
    if (!check_policy_valid_for_claim(state, policy_id)) return;
    Member *m = find_member(state, member_id);
    if (!m) return;
    Transaction tx;
    create_and_sign_transaction(state, &tx, m->wallet_address, INSURANCE_POOL_ADDR,
                                  0.0, TX_SERVICE_REQUEST, 0.5);
    mempool_add(state, &tx, 0.5);
    printf("Service request recorded.\n");
}

static void cmd_preauth_request(ChainState *state, char *args) {
    if (!require_verified(state)) return;
    char provider_id[MAX_ID_LEN], policy_id[MAX_ID_LEN];
    double amount;
    if (sscanf(args, "%63s %63s %lf", provider_id, policy_id, &amount) < 3) {
        printf("Usage: preauth_request <provider_id> <policy_id> <amount>\n");
        return;
    }
    Provider *p = find_provider(state, provider_id);
    if (!p) { printf("Provider not found.\n"); return; }
    if (!check_policy_valid_for_claim(state, policy_id)) return;
    Transaction tx;
    create_and_sign_transaction(state, &tx, p->wallet_address, INSURANCE_POOL_ADDR,
                                  amount, TX_PREAUTH_REQUEST, 1.0);
    mempool_add(state, &tx, 1.0);
}

static void cmd_preauth_approve(ChainState *state, char *args) {
    if (!require_verified(state)) return;
    char policy_id[MAX_ID_LEN];
    double amount;
    if (sscanf(args, "%63s %lf", policy_id, &amount) < 2) {
        printf("Usage: preauth_approve <policy_id> <amount>\n");
        return;
    }
    Transaction tx;
    create_and_sign_transaction(state, &tx, INSURANCE_POOL_ADDR, SYSTEM_ADDR,
                                  amount, TX_PREAUTH_APPROVE, 0.5);
    mempool_add(state, &tx, 0.5);
    printf("Pre-authorization approved.\n");
}

static void cmd_submit_claim(ChainState *state, char *args) {
    if (!require_verified(state)) return;
    char provider_id[MAX_ID_LEN], policy_id[MAX_ID_LEN], claim_id[MAX_ID_LEN];
    double amount;
    if (sscanf(args, "%63s %63s %63s %lf", provider_id, policy_id, claim_id, &amount) < 4) {
        printf("Usage: submit_claim <provider_id> <policy_id> <claim_id> <amount>\n");
        return;
    }
    if (!check_policy_valid_for_claim(state, policy_id)) return;
    Provider *prov = find_provider(state, provider_id);
    if (!prov) { printf("Provider not found.\n"); return; }
    if (find_claim(state, claim_id)) { printf("Claim ID already exists.\n"); return; }

    Policy *pol = find_policy(state, policy_id);
    Transaction tx;
    create_and_sign_transaction(state, &tx, prov->wallet_address, INSURANCE_POOL_ADDR,
                                  amount, TX_CLAIM_SUBMISSION, 2.0);

    int suspicious = fraud_check(state, &tx, pol->member_id, provider_id, amount);
    if (mempool_add(state, &tx, 2.0) == 0 && suspicious) {
        MempoolEntry *e = mempool_find(state, tx.transaction_id);
        if (e) e->status = MEMPOOL_SUSPICIOUS;
    }

    if (state->claim_count < MAX_CLAIMS) {
        Claim *c = &state->claims[state->claim_count++];
        strncpy(c->claim_id, claim_id, MAX_ID_LEN - 1);
        strncpy(c->policy_id, policy_id, MAX_ID_LEN - 1);
        strncpy(c->provider_id, provider_id, MAX_ID_LEN - 1);
        strncpy(c->member_id, pol->member_id, MAX_ID_LEN - 1);
        c->amount = amount;
        c->submitted_at = time(NULL);
        c->approved = 0;
        c->settled = 0;
        c->rejected = 0;
    }
    prov->total_claims += amount;
    prov->claim_count++;
    printf("Claim %s submitted for %.2f AHT.\n", claim_id, amount);
}

static void cmd_approve_claim(ChainState *state, char *args) {
    if (!require_verified(state)) return;
    char claim_id[MAX_ID_LEN];
    if (sscanf(args, "%63s", claim_id) < 1) {
        printf("Usage: approve_claim <claim_id>\n");
        return;
    }
    Claim *c = find_claim(state, claim_id);
    if (!c) { printf("Claim not found.\n"); return; }
    c->approved = 1;
    Transaction tx;
    create_and_sign_transaction(state, &tx, INSURANCE_POOL_ADDR, SYSTEM_ADDR,
                                  c->amount, TX_CLAIM_APPROVAL, 1.0);
    mempool_add(state, &tx, 1.0);
    printf("Claim %s approved.\n", claim_id);
}

static void cmd_reject_claim(ChainState *state, char *args) {
    if (!require_verified(state)) return;
    char claim_id[MAX_ID_LEN];
    if (sscanf(args, "%63s", claim_id) < 1) {
        printf("Usage: reject_claim <claim_id>\n");
        return;
    }
    Claim *c = find_claim(state, claim_id);
    if (!c) { printf("Claim not found.\n"); return; }
    c->rejected = 1;
    Transaction tx;
    create_and_sign_transaction(state, &tx, INSURANCE_POOL_ADDR, SYSTEM_ADDR,
                                  0.0, TX_CLAIM_REJECTION, 0.5);
    mempool_add(state, &tx, 0.5);
    printf("Claim %s rejected.\n", claim_id);
}

static void cmd_settle_claim(ChainState *state, char *args) {
    if (!require_verified(state)) return;
    char claim_id[MAX_ID_LEN];
    if (sscanf(args, "%63s", claim_id) < 1) {
        printf("Usage: settle_claim <claim_id>\n");
        return;
    }
    settle_claim_with_reinsurance(state, claim_id);
}

static void cmd_create_transaction(ChainState *state, char *args) {
    if (!require_verified(state)) return;
    char sender[MAX_ADDR_LEN], receiver[MAX_ADDR_LEN], type_str[32];
    double amount, fee = 1.0;
    if (sscanf(args, "%127s %127s %lf %31s %lf", sender, receiver, &amount, type_str, &fee) < 4) {
        printf("Usage: create_transaction <sender> <receiver> <amount> <type> [fee]\n");
        return;
    }
    Transaction tx;
    TransactionType type = string_to_tx_type(type_str);
    Account *s = find_account(state, sender);
    if (!s) { printf("Sender not found.\n"); return; }
    create_and_sign_transaction(state, &tx, sender, receiver, amount, type, fee);
    mempool_add(state, &tx, fee);
}

static void cmd_mine_pool(ChainState *state, char *args) {
    if (!require_verified(state)) return;
    PoolMiner miners[MAX_POOL_MINERS];
    int count = 0;
    char *token = strtok(args, " ");
    while (token && count < MAX_POOL_MINERS) {
        strncpy(miners[count].miner_id, token, MAX_ADDR_LEN - 1);
        miners[count].hashes_attempted = 0;
        get_or_create_account(state, token);
        count++;
        token = strtok(NULL, " ");
    }
    if (count < 2) {
        printf("Usage: mine_pool <miner1> <miner2> [miner3...]\n");
        return;
    }
    mine_pool(state, miners, count, MAX_TX_PER_BLOCK);
}

static void cmd_utxo_validate(ChainState *state, char *args) {
    char utxo_id[MAX_ID_LEN];
    if (sscanf(args, "%63s", utxo_id) < 1) {
        printf("Usage: utxo_validate <utxo_id>\n");
        return;
    }
    if (utxo_validate(state, utxo_id)) {
        printf("UTXO %s is valid (unspent).\n", utxo_id);
    } else {
        printf("UTXO %s is invalid or already spent.\n", utxo_id);
    }
}

static void cmd_account_nonce(ChainState *state, char *args) {
    char addr[MAX_ADDR_LEN];
    if (sscanf(args, "%127s", addr) < 1) {
        printf("Usage: account_nonce <address>\n");
        return;
    }
    Account *a = find_account(state, addr);
    if (!a) { printf("Account not found.\n"); return; }
    printf("Account %s nonce: %llu\n", addr, (unsigned long long)a->nonce);
}

static void print_tx_history(ChainState *state, TransactionType filter, const char *label) {
    printf("\n=== %s ===\n", label);
    int found = 0;
    for (int b = 0; b < state->block_count; b++) {
        for (int t = 0; t < state->blocks[b].transaction_count; t++) {
            Transaction *tx = &state->blocks[b].transactions[t];
            if (tx->transaction_type == filter) {
                found = 1;
                printf("[%s] Block %d: %s -> %s %.2f AHT\n",
                       tx->transaction_id, b, tx->sender_address,
                       tx->receiver_address, tx->amount);
            }
        }
    }
    if (!found) printf("No records found.\n");
}

static void cmd_provider_history(ChainState *state, char *args) {
    char provider_id[MAX_ID_LEN];
    if (sscanf(args, "%63s", provider_id) < 1) {
        printf("Usage: provider_history <provider_id>\n");
        return;
    }
    Provider *p = find_provider(state, provider_id);
    if (!p) { printf("Provider not found.\n"); return; }
    printf("Provider %s (%s): %d claims, total %.2f AHT, avg %.2f AHT\n",
           p->provider_id, p->name, p->claim_count, p->total_claims,
           p->claim_count > 0 ? p->total_claims / p->claim_count : 0.0);
}

static void cmd_claim_history(ChainState *state, char *args) {
    (void)args;
    printf("\n=== Claim History ===\n");
    for (int i = 0; i < state->claim_count; i++) {
        Claim *c = &state->claims[i];
        printf("%s | Policy: %s | Provider: %s | %.2f AHT | %s%s%s\n",
               c->claim_id, c->policy_id, c->provider_id, c->amount,
               c->approved ? "APPROVED " : "",
               c->rejected ? "REJECTED " : "",
               c->settled ? "SETTLED" : "PENDING");
    }
}

void run_cli(ChainState *state) {
    char line[MAX_LINE_LEN];
    print_help();

    while (1) {
        printf("aht> ");
        fflush(stdout);
        if (!fgets(line, sizeof(line), stdin)) break;

        line[strcspn(line, "\n")] = 0;
        if (line[0] == '\0') continue;

        char cmd[64] = {0};
        char *args = line;
        sscanf(line, "%63s", cmd);
        char *space = strchr(line, ' ');
        args = space ? space + 1 : "";

        if (strcmp(cmd, "quit") == 0 || strcmp(cmd, "exit") == 0) {
            chain_save(state, PERSISTENCE_FILE);
            break;
        } else if (strcmp(cmd, "help") == 0) {
            print_help();
        } else if (strcmp(cmd, "register_member") == 0) {
            cmd_register_member(state, args);
        } else if (strcmp(cmd, "register_provider") == 0) {
            char pid[MAX_ID_LEN], pname[MAX_NAME_LEN];
            if (sscanf(args, "%63s %63s", pid, pname) >= 2)
                cmd_register_provider(state, pid, pname);
            else printf("Usage: register_provider <provider_id> <name>\n");
        } else if (strcmp(cmd, "view_member") == 0) {
            cmd_view_member(state, args);
        } else if (strcmp(cmd, "wallet_balance") == 0) {
            cmd_wallet_balance(state, args);
        } else if (strcmp(cmd, "enroll_policy") == 0) {
            cmd_enroll_policy(state, args);
        } else if (strcmp(cmd, "view_policy") == 0) {
            cmd_view_policy(state, args);
        } else if (strcmp(cmd, "renew_policy") == 0) {
            cmd_renew_policy(state, args);
        } else if (strcmp(cmd, "policy_status") == 0) {
            cmd_policy_status(state, args);
        } else if (strcmp(cmd, "token_transfer") == 0) {
            cmd_token_transfer(state, args);
        } else if (strcmp(cmd, "token_balance") == 0) {
            cmd_token_balance(state, args);
        } else if (strcmp(cmd, "pay_premium") == 0) {
            cmd_pay_premium(state, args);
        } else if (strcmp(cmd, "service_request") == 0) {
            cmd_service_request(state, args);
        } else if (strcmp(cmd, "preauth_request") == 0) {
            cmd_preauth_request(state, args);
        } else if (strcmp(cmd, "preauth_approve") == 0) {
            cmd_preauth_approve(state, args);
        } else if (strcmp(cmd, "submit_claim") == 0) {
            cmd_submit_claim(state, args);
        } else if (strcmp(cmd, "approve_claim") == 0) {
            cmd_approve_claim(state, args);
        } else if (strcmp(cmd, "reject_claim") == 0) {
            cmd_reject_claim(state, args);
        } else if (strcmp(cmd, "settle_claim") == 0) {
            cmd_settle_claim(state, args);
        } else if (strcmp(cmd, "reinsurance_balance") == 0) {
            printf("Reinsurance Pool Balance: %.2f AHT\n", reinsurance_balance(state));
        } else if (strcmp(cmd, "create_transaction") == 0) {
            cmd_create_transaction(state, args);
        } else if (strcmp(cmd, "mempool_view") == 0) {
            mempool_view(state);
        } else if (strcmp(cmd, "mine_solo") == 0) {
            if (!require_verified(state)) continue;
            char miner[MAX_ADDR_LEN];
            int max_tx = MAX_TX_PER_BLOCK;
            if (sscanf(args, "%127s %d", miner, &max_tx) >= 1)
                mine_solo(state, miner, max_tx);
            else
                mine_solo(state, MINER_POOL_ADDR, MAX_TX_PER_BLOCK);
        } else if (strcmp(cmd, "mine_pool") == 0) {
            cmd_mine_pool(state, args);
        } else if (strcmp(cmd, "blockchain_view") == 0) {
            blockchain_view(state);
        } else if (strcmp(cmd, "blockchain_verify") == 0) {
            blockchain_verify(state);
        } else if (strcmp(cmd, "chain_save") == 0) {
            char file[256] = PERSISTENCE_FILE;
            sscanf(args, "%255s", file);
            chain_save(state, file);
        } else if (strcmp(cmd, "chain_load") == 0) {
            char file[256] = PERSISTENCE_FILE;
            sscanf(args, "%255s", file);
            chain_load(state, file);
        } else if (strcmp(cmd, "difficulty_status") == 0) {
            difficulty_status(state);
        } else if (strcmp(cmd, "utxo_view") == 0) {
            utxo_view(state);
        } else if (strcmp(cmd, "utxo_validate") == 0) {
            cmd_utxo_validate(state, args);
        } else if (strcmp(cmd, "account_balance") == 0) {
            cmd_token_balance(state, args);
        } else if (strcmp(cmd, "account_transfer") == 0) {
            if (!require_verified(state)) continue;
            char from[MAX_ADDR_LEN], to[MAX_ADDR_LEN];
            double amount;
            if (sscanf(args, "%127s %127s %lf", from, to, &amount) >= 3)
                account_transfer(state, from, to, amount, NULL);
            else printf("Usage: account_transfer <from> <to> <amount>\n");
        } else if (strcmp(cmd, "account_nonce") == 0) {
            cmd_account_nonce(state, args);
        } else if (strcmp(cmd, "fraud_review") == 0) {
            fraud_review(state);
        } else if (strcmp(cmd, "approve_suspicious") == 0) {
            char tx_id[MAX_ID_LEN];
            if (sscanf(args, "%63s", tx_id) >= 1) approve_suspicious(state, tx_id);
            else printf("Usage: approve_suspicious <tx_id>\n");
        } else if (strcmp(cmd, "reject_suspicious") == 0) {
            char tx_id[MAX_ID_LEN];
            if (sscanf(args, "%63s", tx_id) >= 1) reject_suspicious(state, tx_id);
            else printf("Usage: reject_suspicious <tx_id>\n");
        } else if (strcmp(cmd, "transaction_history") == 0) {
            print_tx_history(state, TX_TOKEN_TRANSFER, "Transaction History");
        } else if (strcmp(cmd, "premium_history") == 0) {
            print_tx_history(state, TX_PREMIUM_PAYMENT, "Premium History");
        } else if (strcmp(cmd, "provider_history") == 0) {
            cmd_provider_history(state, args);
        } else if (strcmp(cmd, "claim_history") == 0) {
            cmd_claim_history(state, args);
        } else {
            printf("Unknown command: %s (type 'help' for commands)\n", cmd);
        }
    }
}

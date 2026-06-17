#include "cli.h"
#include "blockchain.h"
#include "persistence.h"
#include <stdio.h>
#include <stdlib.h>

int main(void) {
    ChainState *state = calloc(1, sizeof(ChainState));
    if (!state) {
        fprintf(stderr, "Failed to allocate chain state.\n");
        return 1;
    }

    if (chain_load(state, PERSISTENCE_FILE) != 0) {
        printf("No saved chain found. Initializing genesis blockchain...\n");
        chain_init(state);
        chain_save(state, PERSISTENCE_FILE);
    }

    printf("\n");
    printf("====================================================\n");
    printf("  ALU Health Insurance Blockchain Management System  \n");
    printf("  Native Token: AHT (ALU Health Token)               \n");
    printf("====================================================\n");
    printf("Loaded %d block(s). Difficulty: %d | Reward: %.2f AHT\n",
           state->block_count, state->current_difficulty, state->block_reward);

    run_cli(state);

    chain_save(state, PERSISTENCE_FILE);
    free(state);
    printf("Goodbye.\n");
    return 0;
}

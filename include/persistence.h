#ifndef PERSISTENCE_H
#define PERSISTENCE_H

#include "types.h"

int chain_save(const ChainState *state, const char *filename);
int chain_load(ChainState *state, const char *filename);

#endif

CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -Iinclude
LDFLAGS = -lcrypto -lssl

SRC = src/types.c \
      src/crypto.c \
      src/merkle.c \
      src/block.c \
      src/transaction.c \
      src/mempool.c \
      src/account.c \
      src/utxo.c \
      src/policy.c \
      src/fraud.c \
      src/reinsurance.c \
      src/mining.c \
      src/blockchain.c \
      src/persistence.c \
      src/cli.c \
      src/main.c

OBJ = $(SRC:.c=.o)
TARGET = aht_blockchain

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

src/%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET) chain_state.dat

run: $(TARGET)
	./$(TARGET)

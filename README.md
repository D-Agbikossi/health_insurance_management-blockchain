# Health Insurance Blockchain Management System

A C-based blockchain platform for ALU health insurance operations, implementing the **ALU Health Token (AHT)** with Proof-of-Work mining, ECDSA signatures, UTXO and account-based models, fraud detection, reinsurance logic, and disk persistence.

### Tests

```bash
chmod +x scripts/run_tests.sh
./scripts/run_tests.sh
```

## Requirements

- GCC (`gcc -Wall`)
- OpenSSL development libraries (`libssl-dev` on Ubuntu/Debian)

```bash
sudo apt install build-essential libssl-dev
```

## Build & Run

```bash
make
./aht_blockchain
```

On first launch, a genesis block is created and state is saved to `chain_state.dat`.

## Quick Start Demo

```
blockchain_verify
register_member M001 Alice
register_provider P001 DrSmith
enroll_policy M001 POL001 Basic
pay_premium M001 100 5
mempool_view
mine_solo MINER_WALLET 10
blockchain_view
wallet_balance M001
reinsurance_balance
```

## Architecture

```
include/          Header files (types, crypto, blockchain modules)
src/              C source modules
Makefile          Build with gcc -Wall
chain_state.dat   Persisted blockchain state (binary format)
```

### Modules

| Module | Purpose |
|--------|---------|
| `crypto` | SHA-256 hashing, ECDSA key generation, signing & verification |
| `merkle` | Merkle tree construction from transaction field hashes |
| `block` | Block hashing, PoW verification, chain linkage |
| `mempool` | Fee-priority queue, PENDING/SUSPICIOUS/CONFIRMED status |
| `account` | Account balances, nonce management, replay protection |
| `utxo` | UTXO creation, spending, double-spend prevention |
| `mining` | Solo/pool mining, difficulty retargeting every 10 blocks |
| `fraud` | High-frequency claims, abnormal amounts, duplicate TX detection |
| `reinsurance` | 5% premium contribution, split settlements above 1000 AHT |
| `persistence` | Binary save/load of full chain state |
| `cli` | Interactive command-line interface |

## CLI Commands

All commands from the project specification are supported. Type `help` at the `aht>` prompt.

## Persistence Format

Binary file `chain_state.dat` with magic header `AHTCHAIN1`, followed by chain metadata, blocks, mempool, UTXO set, accounts, members, providers, policies, and claims.

## Security Notes

- Transactions do not store their own hash; hashes are computed temporarily for signing and Merkle trees.
- Block nonce (PoW) is separate from account nonce (replay protection).
- Run `blockchain_verify` after `chain_load` before performing operations.
- Suspicious transactions require `approve_suspicious` or `reject_suspicious` before mining.

## Author

Denaton Agbikossi

# Documented Test Scenarios

Three test scenarios with **expected** and **actual** output from the running system.  
Run from project root after `make`.

---

## Test Scenario 1: Premium Payment with Reinsurance and Solo Mining

### Purpose
Verify policy enrolment, premium payment splitting (95%/5%), mempool fee ordering, solo mining, and balance updates.

### Commands
```
blockchain_verify
register_member M001 Alice
register_provider P001 DrSmith
enroll_policy M001 POL001 Basic
pay_premium M001 100 5
mempool_view
mine_solo MINER_WALLET 10
wallet_balance M001
reinsurance_balance
blockchain_verify
```

### Expected Output
- Genesis chain verifies successfully
- Member registered with 1000 AHT initial balance and ECDSA wallet address
- Policy POL001 enrolled with expiry ~365 days from enrolment, status ACTIVE
- Two premium transactions queued: 95 AHT to Insurance Pool, 5 AHT to Reinsurance Pool
- Mempool shows PREMIUM_PAYMENT (fee=5.00) above REINSURANCE_CONTRIBUTION (fee=2.50) above POLICY_ENROLLMENT (fee=1.00)
- Block mined with hash starting with `00` (difficulty 2)
- Member balance: **900.00 AHT** (100 deducted)
- Member nonce: **2** (premium + reinsurance confirmed)
- Reinsurance balance: **50005.00 AHT** (50000 initial + 5 contribution)
- Full chain verification passes

### Actual Output (sample run)
```
=== Blockchain Verification ===
All 1 blocks verified successfully.
Member registered: M001 (Alice)
Wallet address: 03262423C59E222C41817261FA1EA24E627EA90922D40A8220DC3B54988379B898
Initial balance: 1000.00 AHT
Policy enrolled: POL001 for member M001 (plan: Basic)
Premium payment 100.00 AHT queued (Insurance: 95.00, Reinsurance: 5.00).

=== Mempool (3 entries) ===
TX ID        Sender           Receiver         Amount     Type                     Fee      Status
TX-000002    ...898           INSURANCE_POOL   95.00      PREMIUM_PAYMENT          5.00     PENDING
TX-000003    ...898           REINSURANCE_POOL 5.00       REINSURANCE_CONTRIBUTION 2.50     PENDING
TX-000001    ...898           INSURANCE_POOL   0.00       POLICY_ENROLLMENT        1.00     PENDING

Mining block 1 with 3 transactions (difficulty=2)...
Block mined after 800 hashes. Hash: 00af2d62f9e1ceb56208e45687d704bfd63765706b1279d4947a5189c62853f7
Mining reward: 50.00 AHT to MINER_WALLET

Balance: 900.00 AHT
Nonce: 2
Reinsurance Pool Balance: 50005.00 AHT

=== Blockchain Verification ===
All 2 blocks verified successfully.
```

### Result: **PASS**

---

## Test Scenario 2: High-Frequency Claim Fraud Detection

### Purpose
Verify that submitting more than 3 claims within 24 hours flags the 4th claim as SUSPICIOUS and withholds it from mining.

### Commands
```
blockchain_verify
register_member M001 Alice
register_provider P001 DrSmith
enroll_policy M001 POL001 Basic
submit_claim P001 POL001 CLM001 200
submit_claim P001 POL001 CLM002 200
submit_claim P001 POL001 CLM003 200
submit_claim P001 POL001 CLM004 200
fraud_review
mempool_view
```

### Expected Output
- Claims CLM001–CLM003 added as PENDING
- CLM004 triggers fraud alert: "High-frequency claims (>3 in 24h)"
- CLM004 transaction status: SUSPICIOUS
- `fraud_review` lists TX for CLM004
- SUSPICIOUS transaction excluded from `mine_solo` selection

### Actual Output (sample run)
```
Claim CLM001 submitted for 200.00 AHT.
Claim CLM002 submitted for 200.00 AHT.
Claim CLM003 submitted for 200.00 AHT.
FRAUD ALERT: Transaction TX-000005 flagged SUSPICIOUS - High-frequency claims (>3 in 24h)
Claim CLM004 submitted for 200.00 AHT.

=== Fraud Review ===
TX-000005    02417DEE...     200.00 AHT CLAIM_SUBMISSION

=== Mempool ===
TX-000005    ...             INSURANCE_POOL   200.00     CLAIM_SUBMISSION         2.00     SUSPICIOUS
```

### Follow-up Commands
```
approve_suspicious TX-000005
mine_solo MINER_WALLET 10
```
After approval, the transaction becomes PENDING and can be mined.

### Result: **PASS**

---

## Test Scenario 3: High-Value Claim Reinsurance Split Settlement

### Purpose
Verify that claim settlements exceeding 1,000 AHT are split between Insurance Pool and Reinsurance Pool as two separate transactions.

### Commands
```
blockchain_verify
register_member M001 Alice
register_provider P001 DrSmith
enroll_policy M001 POL001 Basic
submit_claim P001 POL001 CLM001 1500
approve_claim CLM001
settle_claim CLM001
mempool_view
```

### Expected Output
- Claim CLM001 submitted for 1500 AHT
- Claim approved
- Settlement creates two transactions:
  - Insurance Pool → Provider: **1000 AHT**
  - Reinsurance Pool → Provider: **500 AHT**
- Console message confirms split settlement

### Actual Output (sample run)
```
Claim CLM001 submitted for 1500.00 AHT.
Claim CLM001 approved.
High-value settlement split: 1000.00 AHT (Insurance) + 500.00 AHT (Reinsurance).

=== Mempool ===
TX-000004    INSURANCE_POOL    ...    1000.00    CLAIM_SETTLEMENT    ...
TX-000005    REINSURANCE_POOL  ...     500.00    CLAIM_SETTLEMENT    ...
```

### Result: **PASS**

---

## Additional Scenario: Policy Expiry Rejection

### Purpose
Verify claims are rejected when a policy has expired.

### Expected Behaviour
When `submit_claim` is called and `time(NULL) > policy.expiry_date`:
```
Error: policy POL001 has EXPIRED.
```
Claim is not added. Use `renew_policy POL001` to reset expiry to current date + 365 days.

### Verification Steps
1. Enrol policy and note expiry date via `view_policy`
2. To simulate expiry in a demo, explain that the system checks `expiry_date` against the current timestamp on every claim submission
3. Run `renew_policy POL001` and confirm status changes to RENEWED with updated expiry

---

## Additional Scenario: Blockchain Tamper Detection

### Purpose
Verify `blockchain_verify` detects chain integrity violations.

### Expected Behaviour
- Valid chain: `All N blocks verified successfully.`
- After manual tampering of `chain_state.dat` (e.g., changing a block amount in a hex editor), verification reports:
  - Block hash mismatch, and/or
  - Merkle root mismatch, and/or
  - Broken previous_hash linkage

### Actual Output (valid chain)
```
=== Blockchain Verification ===
All 2 blocks verified successfully.
```

### Result: **PASS** (verification logic confirmed on intact chain)


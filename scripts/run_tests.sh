#!/usr/bin/env bash
# Run documented test scenarios — ALU Health Insurance Blockchain
# Usage: ./scripts/run_tests.sh

set -e
cd "$(dirname "$0")/.."

PASS=0
FAIL=0

run_test() {
    local name="$1"
    local input="$2"
    local check="$3"

    echo ""
    echo "=== TEST: $name ==="
    rm -f chain_state.dat
    output=$(printf '%s\n' "$input" | ./aht_blockchain 2>&1)

    if echo "$output" | grep -q "$check"; then
        echo "PASS: $name"
        PASS=$((PASS + 1))
    else
        echo "FAIL: $name (expected pattern: $check)"
        echo "$output" | tail -15
        FAIL=$((FAIL + 1))
    fi
}

make -q 2>/dev/null || make

run_test "Premium payment and mining" \
"blockchain_verify
register_member M001 Alice
enroll_policy M001 POL001 Basic
pay_premium M001 100 5
mine_solo MINER_WALLET 10
wallet_balance M001" \
"Balance: 900.00 AHT"

run_test "Fraud detection (high-frequency claims)" \
"blockchain_verify
register_member M001 Alice
register_provider P001 DrSmith
enroll_policy M001 POL001 Basic
submit_claim P001 POL001 CLM001 200
submit_claim P001 POL001 CLM002 200
submit_claim P001 POL001 CLM003 200
submit_claim P001 POL001 CLM004 200" \
"High-frequency claims"

run_test "High-value reinsurance split" \
"blockchain_verify
register_member M001 Alice
register_provider P001 DrSmith
enroll_policy M001 POL001 Basic
submit_claim P001 POL001 CLM001 1500
approve_claim CLM001
settle_claim CLM001" \
"High-value settlement split"

run_test "Blockchain verification" \
"blockchain_verify
register_member M001 Alice
enroll_policy M001 POL001 Basic
mine_solo MINER_WALLET 5
blockchain_verify" \
"All 2 blocks verified successfully"

echo ""
echo "============================================"
echo "  Results: $PASS passed, $FAIL failed"
echo "============================================"

[ "$FAIL" -eq 0 ]

#!/usr/bin/env bash
set -euo pipefail

rm -rf data

if [ ! -f "./main" ]; then
  make
fi

input_sequence=$'1\nGunther\n1\nAlice\nUSD\n100\nn\nEUR\n\n3\n3\n'
output=$(printf "%s" "$input_sequence" | ./main --console)

echo "$output" | grep -q "Receipt #" >/dev/null
echo "$output" | grep -q "handled by Gunther" >/dev/null
echo "$output" | grep -q "Shutting down. Goodbye!" >/dev/null

echo "✅ Interactive session completed"

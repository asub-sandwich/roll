#!/usr/bin/bash
set -eu

TRIALS="${TRIALS:-10000}"
DICE_TYPES="${DICE_TYPES:-2 4 6 8 10 12 20 100}"
BIN="${BIN:-./bin/roll}"

if [[ ! -x "$BIN" ]]; then
  echo "Error: binary not found or not executable: $BIN" >&2
  echo "Build roll first! (e.g. 'make')." >&2
  exit 1
fi

printf "%-6s %-10s %-10s %-18s %-18s\n" "die" "trials" "hits(max)" "experimental P(max)" "theoretical P(max)"
printf "%-6s %-10s %-10s %-18s %-18s\n" "----" "------" "---------" "------------------" "-------------------"

for s in $DICE_TYPES; do
  hits=$(seq "$TRIALS" \
    | xargs -I{} "$BIN" "d$s" -q \
    | awk -v max="$s" '$1==max{c++} END{print c+0}')

  exp_prob=$(awk -v c="$hits" -v n="$TRIALS" 'BEGIN{printf "%.6f", c/n}')
  exp_pct=$(awk -v c="$hits" -v n="$TRIALS" 'BEGIN{printf "%.3f", (c/n)*100}')
  theo_prob=$(awk -v s="$s" 'BEGIN{printf "%.6f", 1/s}')
  theo_pct=$(awk -v s="$s" 'BEGIN{printf "%.3f", 100/s}')

  printf "d%-5s %-10s %-10s %-8s (%6s%%)   %-8s (%6s%%)\n" \
    "$s" "$TRIALS" "$hits" "$exp_prob" "$exp_pct" "$theo_prob" "$theo_pct"
done

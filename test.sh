#!/usr/bin/env bash
set -euo pipefail

TRIALS="${TRIALS:-10000}"
DICE_TYPES="${DICE_TYPES:-2 4 6 8 10 12 20 100}"
BIN="${BIN:-./bin/roll}"

if [[ ! -x "$BIN" ]]; then
  echo "error: BIN does not exist or is not executable: $BIN" >&2
  exit 1
fi

NPROC="$(getconf _NPROCESSORS_ONLN 2>/dev/null || echo 1)"
echo
printf "%-6s  %-20s  %-20s\n" "Die" "Experimental P(max)" "Theoretical P(max)"
printf "%-6s  %-20s  %-20s\n" "----" "--------------------" "--------------------"

for d in $DICE_TYPES; do
  count="$(
    seq "$TRIALS" \
    | xargs -P"$NPROC" -I{} "$BIN" "1d${d}" -q \
    | awk -v max="$d" '$1==max{c++} END{print c+0}'
  )"

  exp_pct="$(awk -v c="$count" -v n="$TRIALS" 'BEGIN{printf "%.2f%%", (100.0*c)/n}')"
  theo_pct="$(awk -v d="$d" 'BEGIN{printf "%.2f%%", 100.0/d}')"

  printf "d%-5s  %-20s  %-20s  (#%d/%d)\n" "$d" "$exp_pct" "$theo_pct" "$count" "$TRIALS"
done

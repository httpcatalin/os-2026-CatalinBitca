#!/bin/bash
for p in fire fire4; do
    echo "== $p"
    for i in $(seq 10); do
        ./$p | awk '/^expected/ {e = $2} /^counter/ {c = $2}
            END {printf "lost %8d of %d  (%4.1f%%)\n", e - c, e, 100 * (e - c) / e}'
    done
done

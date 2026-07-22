#!/usr/bin/env bash
# Refuse to ship if anything resembling real personal data is in the tree.
#
# This tool's whole promise is that it keeps personal data out of places it
# should not be. An earlier version of this repository had real inbox contents
# committed, so the repository now checks itself.
#
# Usage: make check   (or ./scripts/check-pii.sh)

set -uo pipefail
cd "$(dirname "$0")/.."

status=0

# Files the tool generates at runtime. They legitimately hold real data, so they
# must never be tracked by git.
generated=(data.json anonymized_data.json swapped.json output_data.csv finished_data.csv)

echo "Checking for generated files that should not be tracked..."
for f in "${generated[@]}"; do
    if git ls-files --error-unmatch "$f" >/dev/null 2>&1; then
        echo "  FAIL  $f is tracked by git — it can contain real data"
        status=1
    fi
done

echo "Checking for credentials..."
for f in secret.json token.json credentials.json; do
    if git ls-files --error-unmatch "$f" >/dev/null 2>&1; then
        echo "  FAIL  $f is tracked by git"
        status=1
    fi
done

# Only example.com / example.org / example.net addresses belong in this repo.
# Those domains are reserved by RFC 2606 precisely for documentation.
#
# -I skips binary files. Without it, grep reports "Binary file X matches" for
# any compressed file whose bytes happen to satisfy the pattern, which produced
# false failures on the screenshots. Images are checked by eye before being
# added; this pass is for text.
# vendor/ is third-party source and carries its own authors' addresses; this
# check is about our data, not theirs. check-pii.sh contains the patterns it
# searches for, so it also has to exclude itself.
echo "Checking for non-example email addresses in tracked text files..."
addresses=$(git ls-files -z \
    | grep -zZv -e '^vendor/' -e '^scripts/check-pii.sh$' \
    | xargs -0 grep -IhoE '[A-Za-z0-9._%+-]+@[A-Za-z0-9.-]+\.[A-Za-z]{2,}' 2>/dev/null \
    | grep -vE '@example\.(com|org|net)$' \
    | sort -u)

if [ -n "$addresses" ]; then
    echo "  FAIL  found addresses outside the example.* domains:"
    echo "$addresses" | sed 's/^/          /'
    status=1
fi

if [ "$status" -eq 0 ]; then
    echo
    echo "OK — nothing that looks like real personal data."
else
    echo
    echo "Personal data check FAILED. Do not commit."
fi

exit "$status"

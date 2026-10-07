#!/bin/sh
set -eu
cd "$(dirname "$0")/.." || exit 1
tmp=$(mktemp -d) || exit 1
trap 'rm -rf "$tmp"' EXIT
mkdir -p "$tmp/daichi" "$tmp/raizo"
if [ "$(uname)" = Linux ]; then
    command -v xvfb-run >/dev/null 2>&1 || { echo 'teste_arte: pulado (sem xvfb-run)'; exit 0; }
    APARA_ARENA_DIR="$tmp" xvfb-run -a ./arena_art_test
else
    APARA_ARENA_DIR="$tmp" ./arena_art_test
fi

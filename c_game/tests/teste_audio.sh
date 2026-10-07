#!/bin/sh
set -eu
cd "$(dirname "$0")/.." || exit 1
tmp=$(mktemp -d) || exit 1
trap 'rm -rf "$tmp"' EXIT
mkdir -p "$tmp/sfx" "$tmp/music" "$tmp/sfx/arashi" "$tmp/sfx/enjin"
# No Linux sem alto-falante, o ALSA null permite testar as mesmas APIs de reprodução.
if [ "$(uname)" = Linux ]; then
    printf 'pcm.!default { type null }\n' > "$tmp/alsa.conf"
    export ALSA_CONFIG_PATH="$tmp/alsa.conf"
fi
APARA_AUDIO_DIR="$tmp" ./audio_test

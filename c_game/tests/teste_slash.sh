#!/bin/sh
# O pack comprado substitui o rastro antigo sem mudar contatos/quadros.
# Arashi, Garfiel e os ecos do Oboro usam suas próprias armas.
set -eu
cd "$(dirname "$0")/.."
command -v xvfb-run >/dev/null 2>&1 || { echo 'teste_slash: pulado (sem xvfb-run)'; exit 0; }
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT
for master in 3 5 10 13; do
    for enabled in 0 1; do
        APARA_SEMENTE=11 APARA_LOG_IMPACTOS=1 APARA_LOG_POSTURAS=1 APARA_LOG_SLASH=1 APARA_SLASH="$enabled" APARA_REC_RAW=/dev/null \
            timeout 180 xvfb-run -a ./apara --demo --master "$master" --duel --fase 2 \
            --rec "$tmp/q" 0 12 > "$tmp/$enabled.log" 2>&1
        sed -n '/^IMPACTO /p' "$tmp/$enabled.log" > "$tmp/$enabled.imp"
    done
    cmp "$tmp/0.imp" "$tmp/1.imp"
    count=$(wc -l < "$tmp/1.imp")
    [ "$count" -gt 0 ]
    on=$(sed -n 's/^SLASHES //p' "$tmp/1.log")
    off=$(sed -n 's/^SLASHES //p' "$tmp/0.log")
    [ "$off" -eq 0 ]
    if [ "$master" -eq 3 ]; then [ "$on" -eq 0 ]; else [ "$on" -gt 0 ]; fi
    if [ "$master" -eq 13 ]; then
        # A fase 2 começa com terra: gesto, corte e aviso usam Daichi.
        sed -n '/^POSTURA /p' "$tmp/1.log" > "$tmp/feedback.log"
        head -1 "$tmp/feedback.log" | grep 'fonte=daichi som=6'
        grep -q 'fonte=daichi som=5' "$tmp/feedback.log"
        grep -q 'fonte=daichi som=0' "$tmp/feedback.log"
        ! grep -q 'fonte=oboro' "$tmp/feedback.log"
        grep -q 'quadro ATTACK_[123]_ECO_' "$tmp/1.imp"
    fi
    echo "  ok: mestre $master — $count contatos idênticos; pack $on desenhos, nativo $off desenhos extras"
done
echo 'teste_slash: substituição visível, assinatura da postura e julgamento preservados'

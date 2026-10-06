#!/bin/sh
# O pack comprado substitui o rastro antigo sem mudar contatos/quadros.
# Garfiel usa garras; Arashi usa o slash original em cinza de nuvem com raios azuis.
set -eu
cd "$(dirname "$0")/.."
command -v xvfb-run >/dev/null 2>&1 || { echo 'teste_slash: pulado (sem xvfb-run)'; exit 0; }
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT
for master in 7 8 6 13; do
    seconds=12
    # Uma amostra longa visita os ecos sorteados de Arashi e Garfiel.
    if [ "$master" -eq 13 ]; then seconds=90; fi
    for enabled in 0 1; do
        APARA_SEMENTE=11 APARA_LOG_IMPACTOS=1 APARA_LOG_POSTURAS=1 APARA_LOG_SLASH=1 APARA_SLASH="$enabled" APARA_REC_RAW=/dev/null \
            timeout 180 xvfb-run -a ./apara --demo --master "$master" --duel --fase 2 \
            --rec "$tmp/q" 0 "$seconds" > "$tmp/$enabled.log" 2>&1
        sed -n '/^IMPACTO /p' "$tmp/$enabled.log" > "$tmp/$enabled.imp"
    done
    cmp "$tmp/0.imp" "$tmp/1.imp"
    count=$(wc -l < "$tmp/1.imp")
    [ "$count" -gt 0 ]
    on=$(sed -n 's/^SLASHES //p' "$tmp/1.log")
    off=$(sed -n 's/^SLASHES //p' "$tmp/0.log")
    [ "$off" -eq 0 ]
    if [ "$master" -eq 7 ] || [ "$master" -eq 6 ]; then [ "$on" -eq 0 ]; else [ "$on" -gt 0 ]; fi
    if [ "$master" -eq 13 ]; then
        # Todo gesto, corte e aviso deve identificar a postura sorteada.
        sed -n '/^POSTURA /p' "$tmp/1.log" > "$tmp/feedback.log"
        grep -q 'som=6' "$tmp/feedback.log"
        grep -q 'som=5' "$tmp/feedback.log"
        grep -q 'som=0' "$tmp/feedback.log"
        ! grep -q 'fonte=oboro' "$tmp/feedback.log"
        grep -q 'quadro ATTACK_[123]_ECO_' "$tmp/1.imp"
    fi
    echo "  ok: mestre $master — $count contatos idênticos; garras $on desenhos, pack desligado $off"
done
echo 'teste_slash: substituição visível, assinatura da postura e julgamento preservados'

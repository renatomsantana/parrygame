#!/bin/sh
# O pack novo desenha cortes, mas não altera contato, resultado nem quadro do
# golpe. Mesmo robô, semente e passo fixo; duas armas e Oboro em fúria incluídos.
set -eu
cd "$(dirname "$0")/.."
[ -x ./apara ] || { echo 'teste_slash: falta ./apara (make)' >&2; exit 2; }
command -v xvfb-run >/dev/null 2>&1 || { echo 'teste_slash: pulado (sem xvfb-run)'; exit 0; }
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT
for master in 3 5 10 13; do
    for enabled in 0 1; do
        APARA_SEMENTE=11 APARA_LOG_IMPACTOS=1 APARA_LOG_SLASH=1 APARA_SLASH="$enabled" APARA_REC_RAW=/dev/null \
            timeout 180 xvfb-run -a ./apara --demo --master "$master" --duel --fase 3 \
            --rec "$tmp/q" 0 12 > "$tmp/$enabled.log" 2>&1
        sed -n '/^IMPACTO /p' "$tmp/$enabled.log" > "$tmp/$enabled.imp"
    done
    cmp "$tmp/0.imp" "$tmp/1.imp"
    count=$(wc -l < "$tmp/1.imp")
    [ "$count" -gt 0 ]
    on=$(sed -n 's/^SLASHES //p' "$tmp/1.log")
    off=$(sed -n 's/^SLASHES //p' "$tmp/0.log")
    [ "$on" -gt 0 ] && [ "$off" -eq 0 ]
    echo "  ok: mestre $master — $count impactos idênticos; slash ligado $on desenhos, desligado $off"
done
echo 'teste_slash: cortes visíveis sem alterar o julgamento nem os quadros dos golpes'

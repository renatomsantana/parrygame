#!/bin/sh
# As doze saídas, emboscadas e visitas, no jogo real. O avanço já foi salvo antes da cena.
set -eu
cd "$(dirname "$0")/.."
command -v xvfb-run >/dev/null 2>&1 || { echo 'teste_historia: pulado (sem xvfb-run)'; exit 0; }
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT
for mestre in 1 2 3 4 5 6 7 8 9 10 11 12; do
    APARA_AUTO=1 APARA_FPS=0 timeout 120 xvfb-run -a ./apara --master "$mestre" --duel --state cleared --rec "$tmp/quadros" 99999 99999 >"$tmp/$mestre.log" 2>&1
    awk '
        $1 == "TESTE_MARCO" && $2 == "saida_aprendiz" { split($3,a,"="); saida=a[2]; ns++ }
        $1 == "TESTE_MARCO" && $2 == "aprendiz_morto" { split($3,a,"="); morte=a[2]; nm++ }
        $1 == "TESTE_MARCO" && $2 == "estado_11" { split($3,a,"="); visita=a[2]; nv++ }
        END { exit !(ns == 1 && nm == 1 && nv == 1 && morte > saida + 1.8 && visita > morte) }
    ' "$tmp/$mestre.log" || { cat "$tmp/$mestre.log"; echo "FALHA: ordem das cenas do mestre $mestre"; exit 1; }
    echo "  ok: mestre $mestre — saída, contato fatal, cabana; uma vez cada"
done
echo 'teste_historia: 12 emboscadas concluídas, sem travar a progressão'

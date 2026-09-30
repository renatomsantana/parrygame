#!/bin/sh
# Teste do jogo de verdade (precisa de ./apara e xvfb-run): APARA_PERF=segundos joga esse tempo, escreve o
# relatório de desempenho (src/desempenho.h) e sai sozinho, com código 0. Confere que o relatório traz a carga,
# as seções do quadro, a contagem de quadros acima de 16,7 ms e a CPU, e que o jogo mediu quadros de verdade.
cd "$(dirname "$0")/.." || exit 1
[ -x ./apara ] || { echo "teste_desempenho: falta ./apara (make)"; exit 2; }
command -v xvfb-run >/dev/null 2>&1 || { echo "teste_desempenho: pulado (sem xvfb-run)"; exit 0; }
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT
FALHAS=0
confere() { # descrição, condição (0 = ok)
    if [ "$2" -eq 0 ]; then echo "  ok: $1"; else echo "  FALHA: $1"; FALHAS=$((FALHAS + 1)); fi
}
APARA_PERF=4 timeout 120 xvfb-run -a -s '-screen 0 1280x720x24' ./apara --demo --master 1 --duel >"$TMP/saida.log" 2>&1
SAIU=$?
confere "o jogo saiu sozinho depois de 4 s (código $SAIU)" "$([ "$SAIU" -eq 0 ]; echo $?)"
for LINHA in "^PERF carga até o primeiro quadro" "^PERF primeiro quadro" "^PERF seção" "^PERF quadro " "^PERF mundo " "^PERF composição " "^PERF swap " "^PERF quadros acima de" "^PERF cpu do processo"; do
    confere "o relatório tem '$(echo "$LINHA" | tr -d '^')'" "$(grep -q "$LINHA" "$TMP/saida.log"; echo $?)"
done
confere "o jogo não ficou pausado durante a medida" "$(grep -q '^PERF AVISO' "$TMP/saida.log"; [ $? -ne 0 ]; echo $?)"
QUADROS=$(awk '/quadros medidos/ {print $3}' "$TMP/saida.log")
confere "mediu quadros de verdade (${QUADROS:-0} depois do aquecimento)" "$([ "${QUADROS:-0}" -ge 10 ]; echo $?)"
if [ "$FALHAS" -eq 0 ]; then echo "teste_desempenho: tudo certo"; else echo "teste_desempenho: $FALHAS falha(s)"; cat "$TMP/saida.log" | tail -20; fi
exit "$FALHAS"

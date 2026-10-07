#!/bin/sh
# Teste do jogo de verdade (precisa de ./apara e de xvfb-run): o rastro fantasma do golpe é só
# visual. Luta os mesmos mestres, com a mesma semente e o mesmo passo de tempo fixo (--rec), com
# o rastro ligado e desligado (APARA_RASTRO), e compara cada impacto: o instante do contato, o
# julgamento, o erro do aperto e o quadro do mestre no impacto têm que sair idênticos. E o rastro
# tem que aparecer de fato (fantasmas desenhados só com ele ligado).
cd "$(dirname "$0")/.." || exit 1
[ -x ./apara ] || { echo "teste_rastro: falta ./apara (make)"; exit 2; }
command -v xvfb-run >/dev/null 2>&1 || { echo "teste_rastro: pulado (sem xvfb-run)"; exit 0; }

TMP=$(mktemp -d) || exit 1
trap 'rm -rf "$TMP"' EXIT
FALHAS=0
confere() { # descrição, condição (0 = ok)
    if [ "$2" -eq 0 ]; then echo "  ok: $1"; else echo "  FALHA: $1"; FALHAS=$((FALHAS + 1)); fi
}
luta() { # mestre, rastro (0/1), arquivo
    # o xvfb-run junta o stderr do jogo ao stdout
    APARA_AUTO=1 APARA_SEMENTE=11 APARA_LOG_IMPACTOS=1 APARA_RASTRO="$2" timeout 600 xvfb-run -a \
        ./apara --master "$1" --duel --rec "$TMP/q" 99999 99999 >"$3" 2>&1
}

for M in 8 9 11; do
    echo "mestre $M"
    luta "$M" 1 "$TMP/com.txt"
    luta "$M" 0 "$TMP/sem.txt"
    grep '^IMPACTO' "$TMP/com.txt" > "$TMP/com.imp"
    grep '^IMPACTO' "$TMP/sem.txt" > "$TMP/sem.imp"
    N=$(wc -l < "$TMP/com.imp")
    confere "$N impactos com o rastro; os mesmos, ao ms, sem ele (contato, julgamento, erro, quadro)" "$(cmp -s "$TMP/com.imp" "$TMP/sem.imp"; echo $?)"
    confere "a luta teve impactos ($N; o mínimo é 10: o tamanho da luta muda quando muda o quadro em que o robô do jogo aperta)" "$([ "$N" -ge 10 ]; echo $?)"
    FC=$(grep '^FANTASMAS' "$TMP/com.txt" | cut -d' ' -f2)
    FS=$(grep '^FANTASMAS' "$TMP/sem.txt" | cut -d' ' -f2)
    confere "o rastro aparece ligado ($FC fantasmas) e some desligado ($FS)" "$([ "${FC:-0}" -gt 0 ] && [ "${FS:-1}" -eq 0 ]; echo $?)"
done

if [ "$FALHAS" -eq 0 ]; then echo "teste_rastro: tudo certo"; else echo "teste_rastro: $FALHAS falha(s)"; fi
exit "$FALHAS"

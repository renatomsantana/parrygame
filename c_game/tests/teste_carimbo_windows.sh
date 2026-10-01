#!/bin/sh
# O carimbo do clique no Windows (src/entrada_win.c: Raw Input numa thread própria), de ponta a ponta, sem
# Windows: o programa tests/carimbo_win.c, compilado com o MinGW, roda no Wine sob o xvfb, e o tests/xclique.c
# manda cliques e Espaço de verdade pelo XTest, o mesmo caminho de um mouse e de um teclado (o Wine os entrega
# ao programa como WM_INPUT). Os relógios do Wine e do xclique têm origens diferentes, então a conta é pelos
# INTERVALOS: entre dois carimbos seguidos tem de haver o mesmo tempo que entre as duas injeções.
#   1. clique do mouse: um carimbo por clique, e cada intervalo igual ao injetado (mediana < 2 ms, 90% < 10 ms);
#   2. Espaço: o mesmo;
#   3. Espaço segurado (a tecla repete sem soltar): a repetição não é um aperto, um carimbo por aperto.
# Precisa do MinGW-w64, do Wine, do xvfb-run e da libX11/libXtst; sem eles, pula. É a mesma ideia do
# tests/teste_carimbo.sh (o Linux), e vale só como conferência do código do Windows: na máquina de verdade,
# o teste é `apara --carimbo`.
cd "$(dirname "$0")/.." || exit 1
CCW=${MINGW_CC:-x86_64-w64-mingw32-gcc}

if [ "$1" != "--dentro" ]; then
    command -v "$CCW" >/dev/null 2>&1 || { echo "teste_carimbo_windows: pulado (sem $CCW)"; exit 0; }
    WINE=$(command -v wine64 || command -v wine || ls /usr/lib/wine/wine64 2>/dev/null | head -1)
    [ -n "$WINE" ] || { echo "teste_carimbo_windows: pulado (sem o Wine)"; exit 0; }
    command -v xvfb-run >/dev/null 2>&1 || { echo "teste_carimbo_windows: pulado (sem xvfb-run)"; exit 0; }
    TMP=$(mktemp -d)
    trap 'rm -rf "$TMP"' EXIT
    cc -std=c11 -O1 -o "$TMP/xclique" tests/xclique.c -lX11 -lXtst 2>/dev/null ||
        cc -std=c11 -O1 -o "$TMP/xclique" tests/xclique.c -lX11 -l:libXtst.so.6 2>/dev/null ||
        { echo "teste_carimbo_windows: pulado (sem libX11/libXtst para compilar tests/xclique.c)"; exit 0; }
    if ! SAIDA=$($CCW -std=c11 -O1 -Wall -Wextra -Werror -static src/entrada_win.c tests/carimbo_win.c -o "$TMP/carimbo_win.exe" -luser32 2>&1); then
        echo "teste_carimbo_windows: não compila para Windows"; echo "$SAIDA" | head -8; exit 1
    fi
    TMP="$TMP" WINE="$WINE" xvfb-run -a -s '-screen 0 1280x720x24' sh "$0" --dentro
    exit $?
fi

export WINEPREFIX="${WINEPREFIX:-${XDG_CACHE_HOME:-$HOME/.cache}/apara-wine}" WINEDEBUG=-all
FALHAS=0
confere() { # descrição, condição (0 = ok)
    if [ "$2" -eq 0 ]; then echo "  ok: $1"; else echo "  FALHA: $1"; FALHAS=$((FALHAS + 1)); fi
}

# cenario NOME N COMO: abre o programa do Windows, injeta N apertos (COMO: clique, espaco ou repete) e
# confere os carimbos contra as injeções.
cenario() {
    NOME=$1; N=$2; COMO=$3
    [ "$COMO" = clique ] && ARG="" || ARG="$COMO"
    "$WINE" "$TMP/carimbo_win.exe" $((N / 2 + 6)) >"$TMP/$NOME.win" 2>&1 &
    PID=$!
    sleep 4     # o Wine demora a abrir a janela
    "$TMP/xclique" "$N" 250 $ARG >"$TMP/$NOME.inj" 2>"$TMP/$NOME.err"
    wait "$PID" 2>/dev/null
    grep -q "iniciar=true" "$TMP/$NOME.win"; confere "$NOME: a entrada bruta ligou (iniciar=true)" $?
    CARIMBOS=$(grep -c '^carimbo ' "$TMP/$NOME.win")
    confere "$NOME: $N apertos injetados, $CARIMBOS carimbos" $([ "$CARIMBOS" -eq "$N" ] && echo 0 || echo 1)
    # os intervalos: |carimbo[i] - carimbo[i-1] - (injecao[i] - injecao[i-1])|, em ms, contra o "antes" da injeção
    RES=$(awk -v n="$N" '
        FNR == NR { if ($1 == "carimbo") { c++; carimbo[c] = $2 } next }
        $1 == "INJECAO" { i++; inj[i] = $2 }
        END {
            k = 0
            for (j = 2; j <= n && j <= c && j <= i; j++) { d = ((carimbo[j] - carimbo[j-1]) - (inj[j] - inj[j-1])) * 1000; if (d < 0) d = -d; dif[++k] = d }
            for (a = 1; a <= k; a++) for (b = a + 1; b <= k; b++) if (dif[b] < dif[a]) { t = dif[a]; dif[a] = dif[b]; dif[b] = t }
            if (k == 0) { print "0 99999 99999"; exit }
            print k, dif[int((k + 1) / 2)], dif[int(k * 0.9 + 0.999)]
        }' "$TMP/$NOME.win" "$TMP/$NOME.inj")
    set -- $RES
    echo "      $1 intervalos: diferença mediana $2 ms, 90% até $3 ms"
    awk -v m="$2" -v p="$3" -v k="$1" 'BEGIN { exit !(k >= 3 && m < 2 && p < 10) }'
    confere "$NOME: cada carimbo no instante do clique (mediana < 2 ms, 90% < 10 ms)" $?
}

cenario mouse 12 clique
cenario espaco 12 espaco
cenario repete 10 repete

if [ "$FALHAS" -eq 0 ]; then echo "teste_carimbo_windows: tudo certo"; else echo "teste_carimbo_windows: $FALHAS falha(s)"; fi
exit "$FALHAS"

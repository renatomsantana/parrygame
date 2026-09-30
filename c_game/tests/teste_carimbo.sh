#!/bin/sh
# Teste do jogo de verdade (precisa de ./apara, xvfb-run, libX11 e libXtst): o carimbo do clique, o instante
# de hardware que o jogo usa no lugar do meio do quadro (src/entrada.c, src/entrada_linux.c).
# tests/xclique.c manda cliques e Espaço pelo XTest, o mesmo caminho de um mouse e de um teclado de verdade,
# em instantes que ele mesmo mede; o jogo, com APARA_LOG_CARIMBOS, escreve cada carimbo que recebeu e cada
# aperto que o duelo aplicou. O jogo roda em tempo real (APARA_FPS=60; sob xvfb, com CPU disputada, os
# quadros saem com 25 a 35 ms, bem mais longos que os de uma tela de verdade: o pior caso do meio do quadro).
#   1. clique do mouse: um carimbo por clique, nunca antes do clique, a menos de 1 ms dele (mediana) e a
#      menos de 3 ms (90%); o duelo aplica o aperto exatamente onde o carimbo manda (a conta é refeita aqui);
#   2. Espaço: o mesmo;
#   2b. Espaço segurado (a tecla repete sem soltar): a repetição não é um aperto, um carimbo por aperto;
#   3. janela sem foco (o jogo pausa): os cliques são carimbados e nenhum vira aperto do duelo;
#   4. APARA_SEM_CARIMBO=1: nenhum carimbo, nenhum aperto carimbado, o jogo segue vivo;
#   5. --carimbo: mede 20 cliques e sai sozinho.
# O apara_save.txt e o apara_opcoes.txt não são tocados (--teste e --carimbo não gravam).
cd "$(dirname "$0")/.." || exit 1

if [ "$1" != "--dentro" ]; then
    [ -x ./apara ] || { echo "teste_carimbo: falta ./apara (make)"; exit 2; }
    command -v xvfb-run >/dev/null 2>&1 || { echo "teste_carimbo: pulado (sem xvfb-run)"; exit 0; }
    grep -q XISelectEvents ./apara 2>/dev/null || { echo "teste_carimbo: pulado (o jogo foi compilado sem XInput2: esta plataforma não dá carimbo)"; exit 0; }
    TMP=$(mktemp -d)
    trap 'rm -rf "$TMP"' EXIT
    cc -std=c11 -O1 -o "$TMP/xtecla" tests/xtecla.c -lX11 2>/dev/null || { echo "teste_carimbo: pulado (sem libX11 para compilar tests/xtecla.c)"; exit 0; }
    cc -std=c11 -O1 -o "$TMP/xclique" tests/xclique.c -lX11 -lXtst 2>/dev/null ||
        cc -std=c11 -O1 -o "$TMP/xclique" tests/xclique.c -lX11 -l:libXtst.so.6 2>/dev/null ||
        { echo "teste_carimbo: pulado (sem libXtst para compilar tests/xclique.c)"; exit 0; }
    TMP="$TMP" xvfb-run -a -s '-screen 0 1280x720x24' sh "$0" --dentro
    exit $?
fi

FALHAS=0
confere() { # descrição, condição (0 = ok)
    if [ "$2" -eq 0 ]; then echo "  ok: $1"; else echo "  FALHA: $1"; FALHAS=$((FALHAS + 1)); fi
}

# joga NOME AMBIENTE ARGS...: abre o jogo (num duelo), dá o foco e tira a pausa (sem gerenciador de
# janelas ninguém dá foco à janela, e o jogo pausa o duelo sem foco), injeta e fecha. Deixa $TMP/NOME.jogo
# (o que o jogo escreveu) e $TMP/NOME.inj (o que o injetor mediu).
N=40
joga() {
    NOME=$1; AMBIENTE=$2; COMO=$3
    env $AMBIENTE APARA_FPS=60 APARA_LOG_CARIMBOS=1 ./apara --teste --master 1 --duel >"$TMP/$NOME.jogo" 2>&1 &
    PID=$!
    sleep 4
    if [ "$COMO" != semfoco ]; then
        "$TMP/xclique" 0 0
        "$TMP/xtecla" esc
        sleep 0.5
    fi
    "$TMP/xclique" "$N" 150 $COMO >"$TMP/$NOME.inj"
    sleep 0.5
    if kill -0 "$PID" 2>/dev/null; then VIVO=0; kill "$PID"; else VIVO=1; fi
    wait "$PID" 2>/dev/null
}

# confere_carimbos NOME: as contas dos cenários 1 e 2, sobre os dois arquivos
confere_carimbos() {
    SAIDA=$(awk -v n="$N" '
        FNR == NR { if ($1 == "INJECAO") { i++; antes[i] = $2; depois[i] = $3 } next }
        /^CARIMBO/ { split($2, a, "="); m++; ts[m] = a[2] }
        /^APERTO/ {
            ap++
            for (k = 2; k <= NF; k++) { split($k, kv, "="); v[kv[1]] = kv[2] }
            if (v["valido"] == 1) {
                va++
                esp = (v["corrido_ms"] - v["atraso_ms"]) * v["passo_ms"] / v["corrido_ms"]
                if (esp < 0) esp = 0
                if (esp > v["passo_ms"]) esp = v["passo_ms"]
                d = v["t_ms"] - esp; if (d < 0) d = -d
                if (d > 0.01) conta++
            }
        }
        END {
            f = 0
            printf("  %s: %d carimbos para %d cliques\n", (m == i && m == n) ? "ok" : "FALHA", m, i); if (!(m == i && m == n)) f++
            antesDoClique = 0
            for (k = 1; k <= m; k++) { e[k] = (ts[k] - depois[k]) * 1000; if (ts[k] < antes[k] - 0.0002) antesDoClique++ }
            for (a1 = 2; a1 <= m; a1++) { x = e[a1]; b1 = a1 - 1; while (b1 >= 1 && e[b1] > x) { e[b1 + 1] = e[b1]; b1-- } e[b1 + 1] = x }
            med = e[int((m + 1) / 2)]; p90 = e[int(m * 0.9)]
            printf("  %s: nenhum carimbo antes do clique (%d antes)\n", antesDoClique == 0 ? "ok" : "FALHA", antesDoClique); if (antesDoClique) f++
            printf("  %s: o carimbo vem %.3f ms depois do clique na mediana e %.3f ms no percentil 90 (limites 1 e 3 ms)\n", (med <= 1 && p90 <= 3) ? "ok" : "FALHA", med, p90); if (!(med <= 1 && p90 <= 3)) f++
            printf("  %s: %d apertos no duelo, %d com carimbo (um clique lido no quadro antes do carimbo chegar cai no meio do quadro: até 20%%)\n", (ap >= n / 2 && va * 5 >= ap * 4) ? "ok" : "FALHA", ap, va); if (!(ap >= n / 2 && va * 5 >= ap * 4)) f++
            printf("  %s: o duelo aplicou cada aperto onde o carimbo mandava (%d fora de 0,01 ms)\n", conta == 0 ? "ok" : "FALHA", conta + 0); if (conta) f++
            exit f
        }' "$TMP/$1.inj" "$TMP/$1.jogo")
    R=$?
    echo "$SAIDA"
    FALHAS=$((FALHAS + R))
}

echo "1. clique do mouse"
joga mouse "" ""
confere_carimbos mouse

echo "2. Espaço"
joga espaco "" espaco
confere_carimbos espaco

echo "2b. Espaço segurado: a repetição da tecla não é um aperto novo"
joga repete "" repete
confere_carimbos repete

echo "3. janela sem foco: cliques carimbados, nenhum vira aperto"
joga semfoco "" semfoco
CARIMBOS=$(grep -c '^CARIMBO' "$TMP/semfoco.jogo")
APERTOS=$(grep -c '^APERTO' "$TMP/semfoco.jogo")
confere "$CARIMBOS carimbos para $N cliques" "$([ "$CARIMBOS" -eq "$N" ]; echo $?)"
confere "$APERTOS apertos no duelo (pausado sem foco)" "$([ "$APERTOS" -eq 0 ]; echo $?)"

echo "4. APARA_SEM_CARIMBO=1: tudo no meio do quadro, como antes do carimbo"
joga sem "APARA_SEM_CARIMBO=1" ""
CARIMBOS=$(grep -c '^CARIMBO' "$TMP/sem.jogo")
APERTOS=$(grep -c '^APERTO' "$TMP/sem.jogo")
confere "nenhum carimbo ($CARIMBOS) e nenhum aperto carimbado ($APERTOS)" "$([ "$CARIMBOS" -eq 0 ] && [ "$APERTOS" -eq 0 ]; echo $?)"
confere "o jogo seguiu vivo até o fim do teste" "$VIVO"

echo "5. --carimbo mede 20 cliques e sai sozinho"
APARA_FPS=60 ./apara --carimbo >"$TMP/modo.log" 2>&1 &
PID=$!
sleep 4
"$TMP/xclique" 0 0
sleep 0.3
"$TMP/xclique" 20 120 >/dev/null
sleep 1
if kill -0 "$PID" 2>/dev/null; then kill "$PID"; SAIU=1; else SAIU=0; fi
wait "$PID" 2>/dev/null
confere "saiu sozinho depois do 20º clique" "$SAIU"
confere "escreveu a média dos 20 cliques ($(grep '^carimbo:' "$TMP/modo.log" | head -1))" "$(grep -q '^carimbo: 20 cliques' "$TMP/modo.log"; echo $?)"

if [ "$FALHAS" -eq 0 ]; then echo "teste_carimbo: tudo certo"; else echo "teste_carimbo: $FALHAS falha(s)"; fi
exit "$FALHAS"

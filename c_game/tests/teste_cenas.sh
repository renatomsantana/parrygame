#!/bin/sh
# Teste do jogo de verdade (precisa de ./apara e xvfb-run): os tempos parados das cenas, medidos no tempo do jogo
# (--rec: passo fixo de 1/30 s, então cada marco tem o tempo exato de jogo). O jogo joga sozinho (APARA_AUTO) e
# escreve "TESTE_MARCO nome t=segundos" a cada momento. Confere:
#   1. do golpe final ao começo da fala do vencido (o desarme: o hitstop, a câmera lenta, a espada que voa e crava,
#      e AJ_ESPADA_CRAVADA_ESPERA): 2,57 s hoje, 2,87 s com a espera de 1,1 s de antes;
#   2. da quebra de um selo do oboro ao começo da cena de fala (a câmera lenta da quebra e AJ_QUEBRA_ATE_A_CENA):
#      1,57 s hoje, 1,87 s com o 1,3 s de antes.
#   3. a tela de derrota (AJ_DERROTA_OPCOES): quem aperta sem parar, sem ler, só sai dela depois da trava (1,2 s hoje, 1,6 s
#      antes), e não antes;
#   4. a tela de vitória (AJ_VITORIA_TRAVA): o mesmo, 0,7 s hoje, 1,0 s antes.
#   5. o ponto vermelho dos cliques do robô (o gancho dos vídeos) sai nos quadros gravados.
# Nas duas últimas os cliques são de verdade (XTest, tests/xclique.c, a cada uns 40 ms). Os limites deixam uns 0,2 s de
# folga para cada lado: não valem com o valor antigo.
cd "$(dirname "$0")/.." || exit 1
[ -x ./apara ] || { echo "teste_cenas: falta ./apara (make)"; exit 2; }
command -v xvfb-run >/dev/null 2>&1 || { echo "teste_cenas: pulado (sem xvfb-run)"; exit 0; }
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT
FALHAS=0
cc -std=c11 -O1 -o "$TMP/xclique" tests/xclique.c -lX11 -lXtst 2>/dev/null || cc -std=c11 -O1 -o "$TMP/xclique" tests/xclique.c -lX11 -l:libXtst.so.6 2>/dev/null || SEM_XTEST=1
confere() { # descrição, condição (0 = ok)
    if [ "$2" -eq 0 ]; then echo "  ok: $1"; else echo "  FALHA: $1"; FALHAS=$((FALHAS + 1)); fi
}

# mede MESTRE MARCO_DE_PARTIDA MARCO_DE_CHEGADA: roda o jogo até os dois marcos e escreve o tempo entre eles
mede() {
    APARA_AUTO=1 APARA_SEMENTE=11 timeout 300 xvfb-run -a -s '-screen 0 1280x720x24' ./apara --master "$1" --duel --rec "$TMP/q" 99999 99999 >"$TMP/$1.log" 2>&1 &
    PID=$!
    N=0
    while [ "$N" -lt 3000 ] && kill -0 "$PID" 2>/dev/null; do
        grep -q "^TESTE_MARCO $3 " "$TMP/$1.log" 2>/dev/null && break
        sleep 0.1
        N=$((N + 1))
    done
    kill "$PID" 2>/dev/null
    wait "$PID" 2>/dev/null
    awk -v a="$2" -v b="$3" '$1 == "TESTE_MARCO" && $2 == a && ta == "" { split($3, x, "="); ta = x[2] } $1 == "TESTE_MARCO" && $2 == b && tb == "" { split($3, x, "="); tb = x[2] } END { if (ta != "" && tb != "") printf("%.3f\n", tb - ta); else print "faltou" }' "$TMP/$1.log"
}

echo "1. o desarme: do golpe final à fala do vencido (daichi)"
D=$(mede 1 vitoria_salva fala_do_vencido)
confere "$D s entre o golpe final e a fala do vencido (entre 2,35 e 2,75 s)" "$(awk -v d="$D" 'BEGIN { exit !(d != "faltou" && d >= 2.35 && d <= 2.75) }'; echo $?)"

echo "2. a quebra de um selo do oboro: da quebra à cena de fala"
D=$(mede 13 selo_quebrado cena_do_selo)
confere "$D s entre a quebra do selo e a cena (entre 1,35 e 1,75 s)" "$(awk -v d="$D" 'BEGIN { exit !(d != "faltou" && d >= 1.35 && d <= 1.75) }'; echo $?)"

# aperta ESTADO NUMERO_DO_ESTADO: o jogo abre direto na tela (ela começa em t = 0), um clique atrás do outro cai em cima
# dela; escreve o tempo de jogo entre o começo da tela e o primeiro marco de estado seguinte. Os cliques não param
# até o jogo sair da tela (com a máquina ocupada o jogo pode levar segundos para abrir, e 100 cliques de 40 ms
# acabavam antes dele: o teste dava "faltou" sem que nada estivesse errado); o teste espera até 60 s.
PROG_ESTADO='$1 == "TESTE_MARCO" { if (ini == "" && $2 == e) { split($3, x, "="); ini = x[2] } else if (ini != "" && fim == "" && $2 ~ /^estado_/ && $2 != e) { split($3, x, "="); fim = x[2] } } END { if (ini != "" && fim != "") printf("%.3f\n", fim - ini); else print "faltou" }'
aperta() {
    TENTATIVA=1
    while :; do
        R=$(aperta_uma "$1" "$2" "$3")
        [ "$R" != faltou ] || [ "$TENTATIVA" -ge 3 ] && break
        TENTATIVA=$((TENTATIVA + 1))   # o jogo nem chegou a sair da tela (Xvfb ou janela que demorou): tenta de novo
    done
    [ "$R" = faltou ] && { echo "  (o log do jogo, sem marcos de estado:)" >&2; tail -5 "$TMP/$1.log" >&2; }
    echo "$R"
}
aperta_uma() {
    PROG_ESTADO="$PROG_ESTADO" APARA_AUTO=1 APARA_SEMENTE=11 timeout 120 xvfb-run -a -s '-screen 0 1280x720x24' sh -c '
        ./apara --master 1 --duel --state '"$1"' >"'"$TMP"'/'"$1"'.log" 2>&1 &
        PID=$!
        (until "'"$TMP"'/xclique" 3000 40 '"$3"' >/dev/null 2>&1; do sleep 0.1; done) &
        CLIQUES=$!
        N=0
        while [ "$N" -lt 600 ] && kill -0 $PID 2>/dev/null; do
            [ "$(awk -v e="estado_'"$2"'" "$PROG_ESTADO" "'"$TMP"'/'"$1"'.log")" != faltou ] && break
            sleep 0.1; N=$((N + 1))
        done
        kill $CLIQUES $PID 2>/dev/null; wait $PID 2>/dev/null'
    awk -v e="estado_$2" "$PROG_ESTADO" "$TMP/$1.log"
}

if [ -n "$SEM_XTEST" ]; then
    echo "3 e 4. puladas (sem libXtst para compilar tests/xclique.c)"
else
    echo "3. a tela de derrota: clique sem parar, a saída só depois da trava"
    D=$(aperta defeat 8 opcao)
    confere "$D s de tela até sair, com cliques sem parar (entre 1,15 e 1,5 s)" "$(awk -v d="$D" 'BEGIN { exit !(d != "faltou" && d >= 1.15 && d <= 1.5) }'; echo $?)"
    echo "4. a tela de vitória: clique sem parar, a saída só depois da trava"
    D=$(aperta cleared 7)
    confere "$D s de tela até sair, com cliques sem parar (entre 0,65 e 0,95 s)" "$(awk -v d="$D" 'BEGIN { exit !(d != "faltou" && d >= 0.65 && d <= 0.95) }'; echo $?)"
fi

# 5. o ponto vermelho dos cliques do robô (APARA_CLIQUE_PERIODO, o gancho dos vídeos) sai na captura: 12 quadros a 120 por
# segundo da tela de derrota (a trava ignora os cliques) têm de ter o ponto no canto (255, 60, 60) em alguns quadros, e sem
# a variável, em nenhum. O ponto era desenhado depois de o lote ser descarregado e nunca chegava ao vídeo.
echo "5. o ponto dos cliques do robô aparece nos quadros gravados"
pontos() { # ENV...: quantos dos 12 quadros têm o ponto vermelho em (1240, 40)
    env "$@" APARA_SEMENTE=11 APARA_REC_FPS=120 APARA_REC_RAW="$TMP/cru.rgb" timeout 120 xvfb-run -a -s '-screen 0 1280x720x24' \
        ./apara --demo --master 1 --duel --state defeat --rec "$TMP/p" 0.5 0.6 >"$TMP/pontos.log" 2>&1
    QUADRO=$((1280 * 720 * 3)); N=0; ACHOU=0
    while [ "$N" -lt 12 ]; do
        COR=$(dd if="$TMP/cru.rgb" bs=1 skip=$((N * QUADRO + (40 * 1280 + 1240) * 3)) count=3 2>/dev/null | od -An -tu1 | tr -s ' ')
        [ "$COR" = " 255 60 60" ] && ACHOU=$((ACHOU + 1))
        N=$((N + 1))
    done
    echo "$ACHOU"
}
COM=$(pontos APARA_CLIQUE_PERIODO=0.05)
SEM=$(pontos APARA_X=1)
confere "$COM de 12 quadros com o ponto (com APARA_CLIQUE_PERIODO=0,05), $SEM sem a variável" "$([ "$COM" -ge 6 ] && [ "$SEM" -eq 0 ]; echo $?)"

if [ "$FALHAS" -eq 0 ]; then echo "teste_cenas: tudo certo"; else echo "teste_cenas: $FALHAS falha(s)"; fi
exit "$FALHAS"

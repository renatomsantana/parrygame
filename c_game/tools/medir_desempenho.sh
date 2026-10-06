#!/bin/sh
# Mede o desempenho do jogo, um mestre por vez, com o robô do demo jogando (APARA_PERF, src/desempenho.h), e
# escreve uma tabela: quadros por segundo, quadro médio, mediana, p95, p99 e máximo (ms), quantos quadros passam
# de 16,7 ms (a meta: nenhum, a 60 Hz), quanto do quadro vai no mundo, na composição e no swap (ms, em média)
# e a CPU do processo.
#   tools/medir_desempenho.sh                     os 13 mestres, mais o oboro na 3ª fase, 20 s cada
#   SEGUNDOS=30 tools/medir_desempenho.sh         mais tempo por mestre
#   MESTRES="1 13" tools/medir_desempenho.sh      só estes mestres
#   RASTRO=1 tools/medir_desempenho.sh            compara o oboro na 3ª fase e o karasu com e sem o rastro fantasma
# Rode com a janela em foco e o vsync ligado (o jogo liga sozinho): é onde a meta de 16,7 ms vale. Sem tela
# (Linux sem $DISPLAY), usa o xvfb-run, e aí o vídeo é por software e os números de vídeo não valem.
cd "$(dirname "$0")/.." || exit 1
[ -x ./apara ] || { echo "medir_desempenho: falta ./apara (make)"; exit 2; }
SEGUNDOS=${SEGUNDOS:-20}
MESTRES=${MESTRES:-1 2 3 4 5 6 7 8 9 10 11 12 13}
if [ -z "$DISPLAY" ] && [ "$(uname)" != Darwin ]; then
    command -v xvfb-run >/dev/null 2>&1 || { echo "medir_desempenho: sem \$DISPLAY e sem xvfb-run"; exit 2; }
    EXEC="xvfb-run -a -s '-screen 0 1280x720x24'"
else
    EXEC=""
fi

# roda ROTULO AMBIENTE ARGS...: uma linha da tabela
roda() {
    ROTULO=$1; AMBIENTE=$2; shift 2
    SAIDA=$(eval "env $AMBIENTE APARA_PERF=$SEGUNDOS $EXEC ./apara $*" 2>&1 | grep '^PERF')
    echo "$SAIDA" | awk -v rotulo="$ROTULO" '
        /^PERF AVISO/ { aviso = " (JOGO PAUSADO: não vale)" }
        /quadros medidos/ { fps = $(NF - 3) }
        /^PERF quadro / { medio = $3; p50 = $4; p95 = $5; p99 = $6; max = $7 }
        /^PERF mundo / { mundo = $3 }
        /^PERF composição / { compoe = $3 }
        /^PERF swap / { swap = $3 }
        /^PERF trabalho do jogo/ { for (i = 1; i <= NF; i++) if ($i == "média") trabalho = $(i + 1) }
        /^PERF quadros acima de/ { acima = $7 " de " $9 }
        /^PERF cpu do processo/ { if (match($0, /: [0-9]+%/)) cpu = substr($0, RSTART + 2, RLENGTH - 2) }
        END { printf("| %s%s | %s | %s | %s | %s | %s | %s | %s | %s | %s | %s | %s |\n", rotulo, aviso, fps, medio, p50, p95, p99, max, acima, mundo, compoe, swap, cpu) }'
    echo "$SAIDA" | grep '^PERF carga' > "${TMPDIR:-/tmp}/apara_perf_carga.txt"
}

echo "Desempenho, $SEGUNDOS s por linha, robô do demo, $(uname -sm), $([ -n "$EXEC" ] && echo 'xvfb (vídeo por software)' || echo "tela de $DISPLAY")"
echo
echo "| jogo | quadros/s | médio (ms) | p50 | p95 | p99 | máx | quadros > 16,7 ms | mundo (ms) | composição (ms) | swap (ms) | CPU do processo |"
echo "|---|---|---|---|---|---|---|---|---|---|---|---|"
for M in $MESTRES; do
    roda "mestre $M" "" --demo --master "$M" --duel
done
roda "oboro, 3ª fase" "" --demo --master 13 --duel --fase 3
if [ -n "$RASTRO" ]; then
    roda "oboro 3ª fase, sem rastro" "APARA_RASTRO=0" --demo --master 13 --duel --fase 3
    roda "karasu (10)" "" --demo --master 10 --duel
    roda "karasu (10), sem rastro" "APARA_RASTRO=0" --demo --master 10 --duel
fi
echo
echo "carga até o primeiro quadro:"
cat "${TMPDIR:-/tmp}/apara_perf_carga.txt"

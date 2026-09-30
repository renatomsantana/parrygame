#!/bin/sh
# Grava um trecho do jogo em vídeo (mp4, 60 quadros por segundo, com legenda), sob xvfb, sem depender de a máquina
# aguentar 60 quadros por segundo: o jogo roda com passo fixo (--rec) e escreve cada quadro em RGB cru
# (APARA_REC_RAW) para o ffmpeg, então o vídeo sai a 60 quadros por segundo de tempo de jogo, por mais lento que o
# vídeo por software seja. Sem áudio (o container não tem placa de som): ligue o F3 (APARA_DEBUG=1) para ver o
# aviso, o contato e o resultado de cada parry.
#
#   tools/gravar_video.sh SAIDA.mp4 T0 T1 "linha 1 da legenda" "linha 2" [VAR=valor ...] -- ARGUMENTOS DO JOGO
#
#   T0 T1     segundos de jogo (do começo do processo) que entram no vídeo
#   VAR=valor variáveis de ambiente do jogo (APARA_DEBUG=1, APARA_ROBO=cedo, APARA_SEMENTE=11, APARA_RASTRO=0...)
#   ARGUMENTOS   os do apara: --demo --master 1 --duel --fase 3 --state defeat ...
#
# Precisa de ./apara, xvfb-run e do ffmpeg (FFMPEG=/caminho/ffmpeg, ou o do PATH). FPS=60 e CRF=24 mudam a qualidade.
cd "$(dirname "$0")/.." || exit 1
[ $# -ge 6 ] || { sed -n '2,15p' "$0"; exit 2; }
SAIDA=$1; T0=$2; T1=$3; L1=$4; L2=$5; shift 5
AMBIENTE=""
while [ $# -gt 0 ] && [ "$1" != "--" ]; do AMBIENTE="$AMBIENTE $1"; shift; done
[ "$1" = "--" ] && shift
FFMPEG=${FFMPEG:-$(command -v ffmpeg)}
[ -x "$FFMPEG" ] || { echo "gravar_video: falta o ffmpeg (FFMPEG=...)"; exit 2; }
[ -x ./apara ] || { echo "gravar_video: falta ./apara (make)"; exit 2; }
FPS=${FPS:-60}; CRF=${CRF:-24}
FONTE=${FONTE:-/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf}   # a pasta dele vai para o libass (a família é "DejaVu Sans")
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT
mkfifo "$TMP/cru"
# a legenda vai numa faixa acima do jogo (1280 x 80), sem cobrir nada, escrita como legenda ASS (o ffmpeg de alguns
# pacotes não traz o drawtext, mas traz o libass); à direita, o tempo de jogo, atualizado 10 vezes por segundo
python3 - "$TMP/legenda.ass" "$L1" "$L2" "$T0" "$T1" <<'PY'
import sys
arq, l1, l2, t0, t1 = sys.argv[1], sys.argv[2], sys.argv[3], float(sys.argv[4]), float(sys.argv[5])
def hms(t): return "%d:%02d:%05.2f" % (int(t // 3600), int(t % 3600 // 60), t % 60)
esc = lambda x: x.replace("\\", "/").replace("{", "(").replace("}", ")").replace("\n", " ")
cab = """[Script Info]
ScriptType: v4.00+
PlayResX: 1280
PlayResY: 800

[V4+ Styles]
Format: Name, Fontname, Fontsize, PrimaryColour, SecondaryColour, OutlineColour, BackColour, Bold, Italic, Underline, StrikeOut, ScaleX, ScaleY, Spacing, Angle, BorderStyle, Outline, Shadow, Alignment, MarginL, MarginR, MarginV, Encoding
Style: L1,DejaVu Sans,30,&H00FFFFFF,&H00FFFFFF,&H00000000,&H00000000,-1,0,0,0,100,100,0,0,1,0,0,7,16,16,8,1
Style: L2,DejaVu Sans,22,&H00D2C8C8,&H00D2C8C8,&H00000000,&H00000000,0,0,0,0,100,100,0,0,1,0,0,7,16,16,46,1
Style: T,DejaVu Sans,26,&H0070D0FF,&H0070D0FF,&H00000000,&H00000000,-1,0,0,0,100,100,0,0,1,0,0,9,16,16,10,1

[Events]
Format: Layer, Start, End, Style, Name, MarginL, MarginR, MarginV, Effect, Text
"""
dur = t1 - t0
ev = ["Dialogue: 0,%s,%s,L1,,0,0,0,,%s" % (hms(0), hms(dur + 1), esc(l1)),
      "Dialogue: 0,%s,%s,L2,,0,0,0,,%s" % (hms(0), hms(dur + 1), esc(l2))]
n = int(dur * 10) + 1
for i in range(n):
    ev.append("Dialogue: 0,%s,%s,T,,0,0,0,,jogo %.1f s" % (hms(i / 10), hms((i + 1) / 10), t0 + i / 10))
open(arq, "w", encoding="utf-8").write(cab + "\n".join(ev) + "\n")
PY
VF="pad=1280:800:0:80:black,ass=$TMP/legenda.ass:fontsdir=$(dirname "$FONTE")"
"$FFMPEG" -hide_banner -loglevel error -y -f rawvideo -pixel_format rgb24 -video_size 1280x720 -framerate "$FPS" -i "$TMP/cru" \
    -vf "$VF" -c:v libx264 -preset veryfast -crf "$CRF" -pix_fmt yuv420p -movflags +faststart -r "$FPS" "$SAIDA" &
FFPID=$!
# shellcheck disable=SC2086
env $AMBIENTE APARA_REC_RAW="$TMP/cru" APARA_REC_FPS="$FPS" timeout "${LIMITE:-900}" xvfb-run -a -s '-screen 0 1280x720x24' ./apara "$@" --rec "$TMP/x" "$T0" "$T1" >"$TMP/jogo.log" 2>&1
wait "$FFPID"
grep -E "^(REC_RAW|TESTE_MARCO)" "$TMP/jogo.log" | head -5
ls -l "$SAIDA" | awk '{printf "%s: %.1f MB\n", $NF, $5/1048576}'

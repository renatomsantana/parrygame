#!/bin/sh
# Instala o zip das animações da Mattz Art ("all the animations") nas pastas
# que o gerador e o jogo leem, com os nomes certos. Depois: make sprites.
#
#   tools/instalar_packs.sh ~/Downloads/all_the_animations.zip
#
# As tiras são de packs pagos e ficam fora do git (ver .gitignore).
set -eu

zip=${1:-}
if [ -z "$zip" ] || [ ! -f "$zip" ]; then
    echo "uso: $0 arquivo.zip" >&2
    exit 1
fi
cd "$(dirname "$0")/.."
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT
unzip -q -o -j "$zip" -x '__MACOSX/*' -d "$tmp"

S=assets/sprites
mkdir -p "$S/_original" "$S/_packs/espadao" "$S/_packs/samurai4" "$S/_packs/samurai5" \
    "$S/_packs/demon" "$S/_packs/hanzo" "$S/_fx" "$S/_ui"

ok=0
falta=0
# nome no zip | destino
while IFS='|' read -r de para; do
    [ -z "$de" ] && continue
    if [ -f "$tmp/$de" ]; then
        cp "$tmp/$de" "$S/$para"
        ok=$((ok + 1))
    else
        echo "  não veio no zip: $de" >&2
        falta=$((falta + 1))
    fi
done <<'LISTA'
ATTACK 1 hat.png|_original/ATTACK_1.png
ATTACK 2 HAT.png|_original/ATTACK_2.png
ATTACK 3 HAT.png|_original/ATTACK_3.png
DASH HAT.png|_original/DASH.png
DASH ATTACK HAT.png|_original/DASH_ATTACK.png
IDLE HAT .png|_original/IDLE.png
DEFEND HAT.png|_original/DEFEND.png
HURT HAT.png|_original/HURT.png
DEATH HAT.png|_original/DEATH.png
STRONG ATTACK HAT .png|_original/STRONG_ATTACK.png
THROW HAT.png|_original/THROW.png
JUMP HAT .png|_original/JUMP.png
RUN HAT.png|_original/RUN.png
ATTACK 1 BS.png|_packs/espadao/ATTACK_1.png
ATTACK 2 BS.png|_packs/espadao/ATTACK_2.png
ATTACK 3 BS .png|_packs/espadao/ATTACK_3.png
DEFEND BS .png|_packs/espadao/DEFEND.png
DEATH BS .png|_packs/espadao/DEATH.png
IDLE BS .png|_packs/espadao/IDLE.png
HURT BS .png|_packs/espadao/HURT.png
JUMP BS .png|_packs/espadao/JUMP.png
RUN BS .png|_packs/espadao/RUN.png
ATTACK 2 LH.png|_packs/samurai4/ATTACK_1.png
ATTACK 1 LH.png|_packs/samurai4/ATTACK_2.png
ATTACK 3 LH.png|_packs/samurai4/ATTACK_3.png
DEFEND LH.png|_packs/samurai4/DEFEND.png
DEATH LH .png|_packs/samurai4/DEATH.png
HURT LH.png|_packs/samurai4/HURT.png
IDLE LH.png|_packs/samurai4/IDLE.png
JUMP LH.png|_packs/samurai4/JUMP.png
RUN LH.png|_packs/samurai4/RUN.png
THROW LH.png|_packs/samurai4/THROW.png
ATTACK 2 DS.png|_packs/samurai5/ATTACK_1.png
ATTACK 1 DS.png|_packs/samurai5/ATTACK_2.png
ATTACK 3 DS.png|_packs/samurai5/ATTACK_3.png
DEFEND DS.png|_packs/samurai5/DEFEND.png
DEATH DS.png|_packs/samurai5/DEATH.png
HURT DS .png|_packs/samurai5/HURT.png
IDLE DS .png|_packs/samurai5/IDLE.png
JUMP DS .png|_packs/samurai5/JUMP.png
RUN DS .png|_packs/samurai5/RUN.png
ATTACK 1 DEMON.png|_packs/demon/ATTACK_1.png
ATTACK 2 DEMON.png|_packs/demon/ATTACK_2.png
ATTACK 3 DEMOON.png|_packs/demon/ATTACK_3.png
JUMP ATTACK DEMON.png|_packs/demon/STRONG_ATTACK.png
DEFEND DEMON .png|_packs/demon/DEFEND.png
HURT DEMON.png|_packs/demon/HURT.png
IDLE DEMON.png|_packs/demon/IDLE.png
RUN DEMON.png|_packs/demon/RUN.png
DEATH DEMON.png|_packs/demon/DEATH.png
SHOUTDEMON.png|_packs/demon/SHOUT.png
ATTACK 1 (FLAMING SWORD).png|_packs/demon/ATTACK_1_FURIA.png
ATTACK 2 (FLAMING SWORD).png|_packs/demon/ATTACK_2_FURIA.png
ATTACK 3 (FLAMING SWORD).png|_packs/demon/ATTACK_3_FURIA.png
JUMP ATTACK (FLAMING SWORD).png|_packs/demon/STRONG_ATTACK_FURIA.png
HURT (FLAMING SWORD).png|_packs/demon/HURT_FURIA.png
IDLE (FLAMING SWORD).png|_packs/demon/IDLE_FURIA.png
RUN (FLAMING SWORD).png|_packs/demon/RUN_FURIA.png
IDLE HANZO.png|_packs/hanzo/IDLE.png
ATTACK 1 HANZO.png|_packs/hanzo/ATTACK_1.png
HURT HANZO.png|_packs/hanzo/HURT.png
RUN HANZO.png|_packs/hanzo/RUN.png
buttons-spritesheet.png|_ui/buttons-spritesheet.png
buttons-pressed-spritesheet.png|_ui/buttons-pressed-spritesheet.png
extra-buttons-spritesheet.png|_ui/extra-buttons-spritesheet.png
extra-buttons-pressed-spritesheet.png|_ui/extra-buttons-pressed-spritesheet.png
mouse-spritesheet.png|_ui/mouse-spritesheet.png
LISTA

# Os efeitos vêm numerados (03.png, 652.png...): vão com o mesmo número.
nfx=0
for f in "$tmp"/[0-9]*.png; do
    [ -f "$f" ] || continue
    cp "$f" "$S/_fx/"
    nfx=$((nfx + 1))
done

echo "$ok tiras e $nfx folhas de efeito instaladas em $S ($falta faltando)."
echo "Agora: make sprites"

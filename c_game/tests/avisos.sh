#!/bin/sh
# Nenhum aviso de compilador: compila cada .c (jogo, núcleo, testes e ferramentas) com -Wall -Wextra
# -Werror, em -O1, -O2 e -O3 (vários avisos, como o de string truncada, só aparecem com otimização),
# com o gcc e com o clang, quando existem. Sem raylib instalada, pula o que precisa dela.
#   tests/avisos.sh            (make avisos; RAYLIB_CFLAGS vem do Makefile)
cd "$(dirname "$0")/.." || exit 1
FLAGS="-std=c11 -Wall -Wextra -Wno-missing-field-initializers -Werror -Isrc"

tem_raylib() { printf '#include "raylib.h"\n' | ${CC:-cc} $RAYLIB_CFLAGS -E -x c - >/dev/null 2>&1; }
tem_x11() { printf '#include <X11/Xlib.h>\n' | ${CC:-cc} -E -x c - >/dev/null 2>&1; }

PUROS="src/ajuste.c src/core.c src/roster.c src/robo.c src/fonte.c src/salvar.c src/entrada.c tests/entrada_test.c src/desempenho.c tests/desempenho_test.c tests/core_test.c tests/robos.c tests/curva.c tests/ritmo.c tests/save_test.c tests/fonte_test.c"
[ -f tests/fuzz.c ] && PUROS="$PUROS tests/fuzz.c"
JOGO="src/main.c src/rig.c src/sprites.c src/pixelize.c src/katana3d.c src/arenas.c src/audio.c src/fx.c src/lore.c tools/personagens.c"
ARQUIVOS="$PUROS"
if tem_raylib; then ARQUIVOS="$ARQUIVOS $JOGO"; else echo "avisos: sem raylib, pulando o jogo e as ferramentas"; fi
if tem_x11; then ARQUIVOS="$ARQUIVOS tests/xtecla.c tests/xclique.c"; else echo "avisos: sem X11, pulando tests/xtecla.c e tests/xclique.c"; fi
ARQUIVOS="$ARQUIVOS src/entrada_stub.c"
if printf '#include <X11/extensions/XInput2.h>\n' | ${CC:-cc} -E -x c - >/dev/null 2>&1; then ARQUIVOS="$ARQUIVOS src/entrada_linux.c"; else echo "avisos: sem XInput2, pulando src/entrada_linux.c"; fi

FALHAS=0
TOTAL=0
for CC in gcc clang; do
    command -v "$CC" >/dev/null 2>&1 || continue
    for O in -O1 -O2 -O3; do
        for F in $ARQUIVOS; do
            TOTAL=$((TOTAL + 1))
            if ! SAIDA=$($CC $FLAGS $O $RAYLIB_CFLAGS -c "$F" -o /dev/null 2>&1); then
                echo "avisos: $CC $O $F"
                echo "$SAIDA" | grep -E "warning|error" | head -5
                FALHAS=$((FALHAS + 1))
            fi
        done
    done
done
if [ "$FALHAS" -eq 0 ]; then echo "avisos: nenhum ($TOTAL compilações, com -Wall -Wextra -Werror)"; else echo "avisos: $FALHAS compilação(ões) com aviso"; fi
exit "$FALHAS"

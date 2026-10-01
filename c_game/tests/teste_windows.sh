#!/bin/sh
# O jogo é para Windows, e o núcleo e as ferramentas de teste têm de se comportar lá como no Linux.
# Compila para Windows (MinGW-w64: x86_64-w64-mingw32-gcc, -Wall -Wextra -Werror) o núcleo, os testes e as
# ferramentas de medição, e, havendo o Wine, roda os executáveis: os testes passam, e a tabela da curva
# (make curva) sai idêntica à do Linux, número por número. Sem o MinGW, ou sem o Wine, pula o que precisa dele.
#   tests/teste_windows.sh          (make teste-windows; MINGW_CC troca o compilador)
cd "$(dirname "$0")/.." || exit 1
CCW=${MINGW_CC:-x86_64-w64-mingw32-gcc}
if ! command -v "$CCW" >/dev/null 2>&1; then echo "teste_windows: sem $CCW (MinGW-w64), pulando"; exit 0; fi
WINE=$(command -v wine64 || command -v wine || ls /usr/lib/wine/wine64 2>/dev/null | head -1)
T=$(mktemp -d) || exit 1
trap 'rm -rf "$T"' EXIT
FLAGS="-std=c11 -O2 -Wall -Wextra -Wno-missing-field-initializers -Werror -static"
CORE="src/ajuste.c src/core.c src/roster.c src/robo.c"
FALHAS=0
compila() {    # nome, fontes..., depois as bibliotecas
    NOME=$1; shift
    if ! SAIDA=$($CCW $FLAGS "$@" -o "$T/$NOME.exe" 2>&1); then
        echo "teste_windows: $NOME não compila para Windows"; echo "$SAIDA" | head -8; FALHAS=$((FALHAS + 1))
    fi
}
compila core_test $CORE tests/core_test.c -lm
compila entrada_test $CORE src/entrada.c tests/entrada_test.c -lm
compila save_test src/salvar.c src/core.c src/roster.c src/ajuste.c tests/save_test.c -lm
compila fonte_test src/fonte.c tests/fonte_test.c
compila desempenho_test src/desempenho.c tests/desempenho_test.c -lm
compila fuzz_test $CORE tests/fuzz.c -lm
compila curva $CORE tests/curva.c -lm -lpthread
compila robos $CORE tests/robos.c -lm -lpthread
compila ritmo $CORE tests/ritmo.c -lm
# a camada de carimbo do Windows (Raw Input): só compila aqui; o jeito de conferir no Windows é `apara --carimbo`
if ! SAIDA=$($CCW $FLAGS -c src/entrada_win.c -o "$T/entrada_win.o" 2>&1); then
    echo "teste_windows: src/entrada_win.c não compila"; echo "$SAIDA" | head -8; FALHAS=$((FALHAS + 1))
fi
if [ "$FALHAS" -ne 0 ]; then echo "teste_windows: $FALHAS ferramenta(s) sem compilar para Windows"; exit 1; fi
echo "teste_windows: 9 programas e a camada de carimbo (src/entrada_win.c) compilam para Windows, sem aviso"
if [ -z "$WINE" ]; then echo "teste_windows: sem o Wine, não rodo os executáveis"; exit 0; fi
# o prefixo do Wine (a "pasta C:") fica em cache: criá-lo leva uns 10 s, e a pasta temporária some ao fim
export WINEPREFIX="${WINEPREFIX:-${XDG_CACHE_HOME:-$HOME/.cache}/apara-wine}" WINEDEBUG=-all
unset DISPLAY
roda() {       # nome, argumentos...
    NOME=$1; shift
    if ! SAIDA=$("$WINE" "$T/$NOME.exe" "$@" 2>&1); then
        echo "teste_windows: $NOME falhou no Windows"; echo "$SAIDA" | tail -8; FALHAS=$((FALHAS + 1))
    else
        echo "  ok: $NOME $(echo "$SAIDA" | tail -1 | tr -d '\r' | cut -c1-110)"
    fi
}
roda core_test
roda entrada_test
roda save_test
roda fonte_test assets/fonts/Tiny5-Regular.ttf src/*.c
roda desempenho_test
roda fuzz_test 100 1
roda robos --taxas 20
roda curva --teste 100
roda ritmo --teste 20
# a mesma tabela, número por número, no Linux e no Windows (1000 lutas por mestre e robô)
cc -std=c11 -O2 $CORE tests/curva.c -o "$T/curva_linux" -lm -lpthread || exit 1
"$T/curva_linux" 1000 0 > "$T/tabela_linux.txt"
"$WINE" "$T/curva.exe" 1000 0 | tr -d '\r' > "$T/tabela_windows.txt"
if cmp -s "$T/tabela_linux.txt" "$T/tabela_windows.txt"; then
    echo "  ok: a tabela da curva (1000 lutas por mestre e robô, 60 e 144 Hz) sai idêntica no Windows e no Linux"
else
    echo "teste_windows: a tabela da curva difere entre Windows e Linux"; diff "$T/tabela_linux.txt" "$T/tabela_windows.txt" | head -10; FALHAS=$((FALHAS + 1))
fi
if [ "$FALHAS" -eq 0 ]; then echo "teste_windows: tudo certo"; else echo "teste_windows: $FALHAS falha(s)"; fi
exit "$FALHAS"

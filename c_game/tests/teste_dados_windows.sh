#!/bin/sh
# O progresso e as opções no Windows, com o apara.exe de verdade rodando no Wine (sob o xvfb): o caminho de Windows
# (%LOCALAPPDATA%\Apara, barras invertidas, MoveFileEx, _mkdir) que o teste do Linux não alcança. Precisa do apara.exe da
# versão atual (make apara OS=Windows_NT CC=x86_64-w64-mingw32-gcc ..., ver tests/Dockerfile.windows), do Wine e do xvfb-run;
# sem um deles, pula. O prefixo do Wine é novo e descartável: nada de quem roda é tocado.
#   tests/teste_dados_windows.sh            (APARA_EXE=caminho troca o ./apara.exe)
#   1. sem nada: a pasta de dados nasce em %LOCALAPPDATA%\Apara;
#   2. o save e as opções ao lado do .exe (versões antigas) são copiados para lá, e o original fica;
#   3. de novo: o que já está na pasta de dados manda; lixo ao lado do .exe não é copiado;
#   4. APARA_DADOS com caminho de Windows (C:\...\dados): vale, e cria as pastas de cima;
#   5. calibrar grava as opções na pasta de dados.
cd "$(dirname "$0")/.." || exit 1
EXE=${APARA_EXE:-./apara.exe}
[ -f "$EXE" ] || { echo "teste_dados_windows: sem $EXE (o jogo compilado para Windows), pulando"; exit 0; }
WINE=$(command -v wine64 || command -v wine || ls /usr/lib/wine/wine64 2>/dev/null | head -1)
[ -n "$WINE" ] || { echo "teste_dados_windows: sem o Wine, pulando"; exit 0; }
command -v xvfb-run >/dev/null 2>&1 || { echo "teste_dados_windows: sem xvfb-run, pulando"; exit 0; }

T=$(mktemp -d) || exit 1
trap 'rm -rf "$T"' EXIT
EXE_ABS=$(cd "$(dirname "$EXE")" && pwd)/$(basename "$EXE")
mkdir -p "$T/jogo"
cp "$EXE_ABS" "$T/jogo/apara.exe"
ln -s "$PWD/assets" "$T/jogo/assets"
export WINEPREFIX="$T/prefixo" WINEDEBUG=-all
unset DISPLAY
FALHAS=0
confere() { # descrição, condição (0 = ok)
    if [ "$2" -eq 0 ]; then echo "  ok: $1"; else echo "  FALHA: $1"; FALHAS=$((FALHAS + 1)); fi
}
roda() { # VARIAVEL=valor... -- argumentos do jogo: abre o apara.exe da cópia (cwd = a pasta dele), que fecha sozinho
    VARS=""
    while [ "$#" -gt 0 ] && [ "$1" != "--" ]; do VARS="$VARS $1"; shift; done
    shift
    (cd "$T/jogo" && APARA_AUTO=1 timeout 180 xvfb-run -a -s '-screen 0 1280x720x24' env $VARS "$WINE" explorer /desktop=aparar,1280x720 ./apara.exe "$@") >"$T/wine.log" 2>&1
}
# a pasta de dados dentro do prefixo (o Wine escolhe o usuário e o nome da pasta AppData\Local)
dados() { find "$WINEPREFIX/drive_c/users" -type d -name Apara 2>/dev/null | head -1; }
apaga_dados() { find "$WINEPREFIX/drive_c/users" -type d -name Apara -exec rm -rf {} + 2>/dev/null; }

echo "1. sem nada ao lado do .exe: a pasta de dados nasce em %LOCALAPPDATA%\\Apara"
roda -- --shot titulo.png 0.5
D=$(dados)
confere "a pasta Apara existe no perfil do usuário ($D)" "$([ -n "$D" ] && [ -d "$D" ]; echo $?)"
confere "o jogo abriu (houve captura)" "$([ -s "$T/jogo/titulo.png" ]; echo $?)"

echo "2. o save e as opções ao lado do .exe: copiados para a pasta de dados, o original fica"
apaga_dados
printf 'APARA-C 2\n5 31 0 1\n' > "$T/jogo/apara_save.txt"
printf 'atraso_video_ms 40\natraso_audio_ms 25\n' > "$T/jogo/apara_opcoes.txt"
cp "$T/jogo/apara_save.txt" "$T/save.base"
cp "$T/jogo/apara_opcoes.txt" "$T/opcoes.base"
roda -- --shot titulo.png 0.5
D=$(dados)
confere "o save foi copiado, igual byte a byte" "$(cmp -s "$D/apara_save.txt" "$T/save.base"; echo $?)"
confere "as opções também" "$(cmp -s "$D/apara_opcoes.txt" "$T/opcoes.base"; echo $?)"
confere "o original ao lado do .exe continua intacto" "$(cmp -s "$T/jogo/apara_save.txt" "$T/save.base" && cmp -s "$T/jogo/apara_opcoes.txt" "$T/opcoes.base"; echo $?)"
confere "sem .tmp nem .bak sobrando" "$([ -z "$(ls "$D" | grep -e '\.tmp$' -e '\.bak$')" ]; echo $?)"

echo "3. de novo: a pasta de dados manda, e lixo ao lado do .exe não é copiado"
printf 'APARA-C 2\n9 511 0 1\n' > "$D/apara_save.txt"
cp "$D/apara_save.txt" "$T/mais_novo.txt"
roda -- --shot titulo.png 0.5
confere "o save da pasta de dados não foi sobrescrito pelo antigo" "$(cmp -s "$D/apara_save.txt" "$T/mais_novo.txt"; echo $?)"
apaga_dados
printf 'ZZZ\n' > "$T/jogo/apara_save.txt"
roda -- --shot titulo.png 0.5
D=$(dados)
confere "um save antigo estragado não é copiado (nem vira .bak)" "$([ ! -e "$D/apara_save.txt" ] && [ ! -e "$T/jogo/apara_save.txt.bak" ] && [ "$(cat "$T/jogo/apara_save.txt")" = ZZZ ]; echo $?)"

echo "4. APARA_DADOS com caminho de Windows: vale, e cria as pastas de cima"
rm -f "$T/jogo/apara_save.txt" "$T/jogo/apara_opcoes.txt"
roda 'APARA_DADOS=C:\teste\fundo\dados' -- --shot titulo.png 0.5
confere "a pasta C:/teste/fundo/dados existe" "$([ -d "$WINEPREFIX/drive_c/teste/fundo/dados" ]; echo $?)"

echo "5. calibrar grava as opções na pasta de dados"
roda 'APARA_DADOS=C:\teste\fundo\dados' APARA_CALIBRA_PRONTA=40,25 -- --state calibra --shot cal.png 4
confere "apara_opcoes.txt tem os atrasos calibrados" "$([ "$(tr -d '\r' < "$WINEPREFIX/drive_c/teste/fundo/dados/apara_opcoes.txt" 2>/dev/null)" = "$(printf 'atraso_video_ms 40\natraso_audio_ms 25')" ]; echo $?)"

if [ "$FALHAS" -eq 0 ]; then echo "teste_dados_windows: tudo certo"; else echo "teste_dados_windows: $FALHAS falha(s)"; fi
exit "$FALHAS"

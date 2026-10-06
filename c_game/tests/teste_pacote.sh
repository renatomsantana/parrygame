#!/bin/sh
# O pacote de distribuição (tools/empacotar_jogo.sh): o que ele entrega e o que ele recusa. Precisa de ./apara (make), do xvfb-run e do zip/tar;
# o que depende do MinGW e do Wine pula sem eles (APARA_EXE_WINDOWS=caminho do apara.exe confere também o pacote de Windows).
#   1. o pacote de Linux de verdade: abre, tem o que o jogador precisa e nada de fonte, prancha-fonte, save ou opções;
#   2. recusa um executável que depende de uma biblioteca que não vem com o sistema;
#   3. recusa um jogo que grava ao lado do executável (o pacote seria alterado por quem joga);
#   4. recusa um jogo que abre com a tela vazia;
#   5. recusa um executável de Windows com uma DLL que não é do sistema (libwinpthread-1.dll, por exemplo);
#   6. o pacote de Windows de verdade, se houver o apara.exe e o Wine.
cd "$(dirname "$0")/.." || exit 1
[ -x ./apara ] || { echo "teste_pacote: falta ./apara (make)"; exit 2; }
command -v xvfb-run >/dev/null 2>&1 || { echo "teste_pacote: pulado (sem xvfb-run)"; exit 0; }
command -v tar >/dev/null 2>&1 || { echo "teste_pacote: pulado (sem tar)"; exit 0; }
T=$(mktemp -d) || exit 1
trap 'rm -rf "$T"' EXIT
FALHAS=0
confere() { # descrição, condição (0 = ok)
    if [ "$2" -eq 0 ]; then echo "  ok: $1"; else echo "  FALHA: $1"; FALHAS=$((FALHAS + 1)); fi
}
empacota() { # plataforma, executável: saída em $T/saida.txt, pacotes em $T/dist
    VERSAO=teste sh tools/empacotar_jogo.sh "$1" "$2" "$T/dist" >"$T/saida.txt" 2>&1
}

echo "1. o pacote de Linux de verdade"
rm -rf "$T/dist"
empacota linux ./apara
confere "empacotou (código $?)" "$([ -f "$T/dist/apara-teste-linux-x64.tar.gz" ]; echo $?)"
confere "abriu o jogo de dentro do pacote e ele não gravou ao lado do executável" "$(grep -q 'abre de dentro da pasta' "$T/saida.txt" && grep -q 'abre: sim' "$T/saida.txt"; echo $?)"
mkdir -p "$T/aberto" && tar -xzf "$T/dist/apara-teste-linux-x64.tar.gz" -C "$T/aberto"
P="$T/aberto/apara-teste-linux-x64"
LISTA=$(cd "$P" && find . -type f | sort)
confere "tem o executável, o leia-me, os créditos, a versão e as licenças" "$([ -x "$P/apara" ] && [ -f "$P/LEIA-ME.txt" ] && [ -f "$P/CREDITOS.txt" ] && [ -f "$P/VERSAO.txt" ] && [ -f "$P/LICENCAS/raylib-zlib.txt" ] && [ -f "$P/LICENCAS/OFL-Tiny5.txt" ]; echo $?)"
confere "tem os sprites de todos os mestres, a fonte e o modelo da katana" "$([ -f "$P/assets/sprites/kojiro/sprite.txt" ] && [ -f "$P/assets/sprites/oboro/sprite.txt" ] && [ -f "$P/assets/fonts/Tiny5-Regular.ttf" ] && [ -f "$P/assets/katana/katana.glb" ]; echo $?)"
confere "sem pranchas-fonte, código, Makefile, save nem opções" "$(! echo "$LISTA" | grep -q -e '/_original/' -e '/_packs/' -e '/_folhas/' -e '\.c$' -e '\.h$' -e 'Makefile' -e 'apara_save' -e 'apara_opcoes' -e '\.tmp$' -e '\.bak$'; echo $?)"
confere "o leia-me do jogador diz onde fica o progresso" "$(grep -q 'LOCALAPPDATA' "$P/LEIA-ME.txt" && grep -q 'apara_save.txt' "$P/LEIA-ME.txt"; echo $?)"
confere "recusa empacotar por cima de um pacote que já existe" "$(empacota linux ./apara; [ "$?" -ne 0 ]; echo $?)"

echo "2. recusa um executável que depende de uma biblioteca que não vem com o sistema"
mkdir -p "$T/falsa"
printf 'int soma(int a, int b) { return a + b; }\n' > "$T/falsa/lib.c"
printf 'int soma(int, int);\nint main(void) { return soma(1, 2) == 3 ? 0 : 1; }\n' > "$T/falsa/main.c"
cc -shared -fPIC -o "$T/falsa/libfaltando.so" "$T/falsa/lib.c" && cc -o "$T/falsa/apara_falta" "$T/falsa/main.c" -L"$T/falsa" -lfaltando -Wl,-rpath,"$T/falsa/nao_existe" && rm "$T/falsa/libfaltando.so"
rm -rf "$T/dist"
empacota linux "$T/falsa/apara_falta"
confere "recusou (código $?) e disse que faltam bibliotecas" "$([ "$?" -ne 0 ] || true; grep -q 'bibliotecas que faltam' "$T/saida.txt"; echo $?)"
confere "nada foi empacotado" "$([ ! -e "$T/dist" ] || [ -z "$(ls "$T/dist")" ]; echo $?)"

echo "3. recusa um jogo que grava ao lado do executável"
cat > "$T/falsa/apara_grava" <<'FIM'
#!/bin/sh
echo "progresso" > "$(dirname "$0")/apara_save.txt"
head -c 40000 /dev/zero > "$2"
FIM
chmod +x "$T/falsa/apara_grava"
rm -rf "$T/dist"
empacota linux "$T/falsa/apara_grava"
confere "recusou e apontou o arquivo gravado" "$(grep -q 'gravou dentro da pasta do pacote' "$T/saida.txt" && grep -q 'apara_save.txt' "$T/saida.txt"; echo $?)"

echo "4. recusa um jogo que abre com a tela vazia"
cat > "$T/falsa/apara_vazio" <<'FIM'
#!/bin/sh
printf 'PNG' > "$2"
FIM
chmod +x "$T/falsa/apara_vazio"
rm -rf "$T/dist"
empacota linux "$T/falsa/apara_vazio"
confere "recusou: captura pequena demais" "$(grep -q 'pequena demais' "$T/saida.txt"; echo $?)"

echo "5. recusa um executável de Windows com uma DLL que não é do sistema"
CCW=${MINGW_CC:-x86_64-w64-mingw32-gcc}
if command -v "$CCW" >/dev/null 2>&1; then
    printf '#include <pthread.h>\nstatic void *f(void *p) { return p; }\nint main(void) { pthread_t t; pthread_create(&t, 0, f, 0); pthread_join(t, 0); return 0; }\n' > "$T/falsa/pt.c"
    "$CCW" -o "$T/falsa/apara_dll.exe" "$T/falsa/pt.c" -pthread -Wl,-Bdynamic 2>"$T/falsa/cc.err" || true
    if [ -f "$T/falsa/apara_dll.exe" ] && x86_64-w64-mingw32-objdump -p "$T/falsa/apara_dll.exe" | grep -qi 'libwinpthread'; then
        rm -rf "$T/dist"
        empacota windows "$T/falsa/apara_dll.exe"
        confere "recusou e citou a DLL" "$(grep -qi 'libwinpthread-1.dll' "$T/saida.txt" && grep -q 'não é uma DLL do sistema' "$T/saida.txt"; echo $?)"
    else
        echo "  (o MinGW daqui liga a libwinpthread de forma estática: sem como montar o executável de teste, pulando)"
    fi
else
    echo "  (sem o MinGW, pulando)"
fi

echo "6. o pacote de Windows de verdade"
if [ -n "${APARA_EXE_WINDOWS:-}" ] && [ -f "$APARA_EXE_WINDOWS" ] && { command -v wine64 >/dev/null 2>&1 || command -v wine >/dev/null 2>&1 || [ -x /usr/lib/wine/wine64 ]; }; then
    rm -rf "$T/dist"
    empacota windows "$APARA_EXE_WINDOWS"
    confere "empacotou, abriu no Wine de dentro do pacote e ele não foi alterado" "$([ -f "$T/dist/apara-teste-windows-x64.zip" ] && grep -q 'abre: sim' "$T/saida.txt"; echo $?)"
else
    echo "  (APARA_EXE_WINDOWS=caminho/apara.exe e o Wine conferem o pacote de Windows; pulando)"
fi

if [ "$FALHAS" -eq 0 ]; then echo "teste_pacote: tudo certo"; else echo "teste_pacote: $FALHAS falha(s)"; fi
exit "$FALHAS"

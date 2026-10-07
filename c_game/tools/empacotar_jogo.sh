#!/bin/sh
# O pacote de distribuição: o executável, os assets de execução, o leia-me do jogador, os créditos e as licenças, numa pasta que abre sem
# compilador, sem raylib e sem instalar nada. Confere antes de empacotar e confere o pacote pronto.
#   tools/empacotar_jogo.sh PLATAFORMA EXECUTAVEL PASTA_DE_SAIDA        (make pacote PLATAFORMA=... EXE=... OUT=...)
#   PLATAFORMA: windows | linux | macos   (o executável tem de ser da plataforma: cada uma se compila na sua, ou cruzada)
#   VERSAO=1.0 troca o nome da versão (padrão: git describe)
# O que confere:
#   1. os assets estão completos (tools/verificar_assets.sh);
#   2. o executável só depende do sistema (Windows: nenhuma DLL fora da lista de DLLs do sistema; Linux: nenhuma biblioteca
#      "not found" e nem a libraylib; macOS: nada fora de /usr/lib e /System);
#   3. o pacote montado ABRE: o jogo é aberto de dentro da pasta do pacote, de outra pasta de trabalho, tira uma captura, e a pasta do pacote
#      continua exatamente como estava (o jogo não grava ao lado do executável). Windows: no Wine; Linux: sob o xvfb-run; macOS: só no
#      próprio macOS. Sem o que precisa para abrir, avisa "SEM TESTE DE ABERTURA" e empacota mesmo assim (APARA_PACOTE_EXIGE_TESTE=1 aborta).
set -eu
cd "$(dirname "$0")/.." || exit 1
plataforma=${1:-}; exe=${2:-}; saida=${3:-}
[ -n "$plataforma" ] && [ -n "$exe" ] && [ -n "$saida" ] || { echo 'uso: tools/empacotar_jogo.sh windows|linux|macos EXECUTAVEL PASTA_DE_SAIDA' >&2; exit 2; }
case "$plataforma" in windows|linux|macos) ;; *) echo "plataforma desconhecida: $plataforma (windows, linux ou macos)" >&2; exit 2;; esac
[ -f "$exe" ] || { echo "não achei o executável: $exe" >&2; exit 2; }
exe=$(cd "$(dirname "$exe")" && pwd)/$(basename "$exe")
case "$saida" in /*) ;; *) saida="$PWD/$saida";; esac
versao=${VERSAO:-$(git describe --tags --always --dirty 2>/dev/null || echo dev)}
nome="apara-$versao-$plataforma-x64"
nome_exe=apara; [ "$plataforma" = windows ] && nome_exe=apara.exe

falha() { echo "empacotar_jogo: $*" >&2; exit 1; }

sh tools/verificar_assets.sh

# 2. o executável só depende do sistema
case "$plataforma" in
    windows)
        OBJDUMP=$(command -v x86_64-w64-mingw32-objdump || command -v objdump || true)
        [ -n "$OBJDUMP" ] || falha "sem o objdump (MinGW-w64 ou binutils) para conferir as DLLs de $exe"
        # DLLs que todo Windows tem; qualquer outra (libgcc_s_seh-1, libwinpthread-1, raylib.dll...) teria de ir junto
        sistema='kernel32 user32 gdi32 opengl32 winmm shell32 msvcrt advapi32 ole32 imm32 shlwapi comdlg32 bcrypt ucrtbase'
        deps=$("$OBJDUMP" -p "$exe" | awk '/DLL Name:/ { print tolower($3) }' | sort -u)
        [ -n "$deps" ] || falha "$exe não parece um executável de Windows (nenhuma DLL listada)"
        for d in $deps; do
            base=${d%.dll}
            case "$base" in api-ms-win-crt-*) continue;; esac
            case " $sistema " in *" $base "*) ;; *) falha "$exe depende de $d, que não é uma DLL do sistema: compile com -static (ou leve a DLL junto)";; esac
        done
        echo "executável: só DLLs do sistema ($(echo $deps | tr ' ' ','))" ;;
    linux)
        saida_ldd=$(ldd "$exe" 2>&1) || true
        if echo "$saida_ldd" | grep -q 'not found'; then falha "bibliotecas que faltam neste sistema: $(echo "$saida_ldd" | grep 'not found' | tr -s ' \t' ' ' | tr '\n' ';')"; fi
        if echo "$saida_ldd" | grep -qi 'libraylib'; then falha "$exe depende da libraylib como biblioteca dinâmica: compile com a raylib estática"; fi
        echo "executável: $(echo "$saida_ldd" | awk '{ print $1 }' | grep -v '^linux-vdso\|^/lib' | tr '\n' ' ')" ;;
    macos)
        if command -v otool >/dev/null 2>&1; then
            fora=$(otool -L "$exe" | tail -n +2 | awk '{ print $1 }' | grep -v '^/usr/lib/\|^/System/' || true)
            [ -z "$fora" ] || falha "$exe depende de bibliotecas fora do sistema: $fora (compile com a raylib estática)"
            echo "executável: só bibliotecas do sistema"
        else
            echo "AVISO: sem o otool, as dependências do executável de macOS não foram conferidas" >&2
        fi ;;
esac

# 3. montar
tmp=$(mktemp -d) || exit 1
trap 'rm -rf "$tmp"' EXIT
pasta="$tmp/$nome"
mkdir -p "$pasta/LICENCAS"
cp "$exe" "$pasta/$nome_exe"
chmod +x "$pasta/$nome_exe"
sh tools/copiar_assets_runtime.sh "$pasta"
cp tools/pacote/LEIA-ME.txt tools/pacote/CREDITOS.txt "$pasta/"
cp assets/fonts/OFL-Tiny5.txt "$pasta/LICENCAS/OFL-Tiny5.txt"
cp tools/pacote/LICENCA-raylib-zlib.txt "$pasta/LICENCAS/raylib-zlib.txt"
{
    echo "aparar $versao ($plataforma x64)"
    echo "commit: $(git rev-parse --short HEAD 2>/dev/null || echo desconhecido)"
    echo "empacotado em: $(date -u +%Y-%m-%dT%H:%M:%SZ)"
} > "$pasta/VERSAO.txt"
[ -d "$pasta/assets/sprites/kojiro" ] && [ -f "$pasta/assets/fonts/Tiny5-Regular.ttf" ] || falha "o pacote ficou sem os assets mínimos (sprites do Kojiro e a fonte)"
if grep -q 'a equipe confirma' "$pasta/CREDITOS.txt"; then
    echo "AVISO: CREDITOS.txt ainda tem itens 'a equipe confirma' (licenças e nomes): revise antes de publicar" >&2
fi

# 4. conferir o pacote pronto: abre de dentro dele, de outra pasta, e ele não é alterado
abre=nao
antes=$(cd "$pasta" && find . | sort)
teste_abertura() {
    trabalho="$tmp/trabalho"; mkdir -p "$trabalho"
    rm -f "$trabalho/captura.png"
    case "$plataforma" in
        linux)
            command -v xvfb-run >/dev/null 2>&1 || return 1
            (cd "$trabalho" && APARA_AUTO=1 APARA_DADOS="$tmp/dados" timeout 120 xvfb-run -a "$pasta/apara" --shot "$trabalho/captura.png" 0.5) >"$tmp/abertura.log" 2>&1 || true ;;
        windows)
            WINE=$(command -v wine64 || command -v wine || ls /usr/lib/wine/wine64 2>/dev/null | head -1 || true)
            [ -n "$WINE" ] && command -v xvfb-run >/dev/null 2>&1 || return 1
            # o prefixo do Wine é descartável (nada do usuário é tocado); a captura sai na pasta de trabalho
            (cd "$trabalho" && WINEPREFIX="$tmp/prefixo" WINEDEBUG=-all APARA_AUTO=1 APARA_DADOS='C:\apara_dados_pacote' timeout 180 xvfb-run -a -s '-screen 0 1280x720x24' "$WINE" explorer /desktop=aparar,1280x720 "$pasta/apara.exe" --shot "$trabalho/captura.png" 0.5) >"$tmp/abertura.log" 2>&1 || true ;;
        macos)
            [ "$(uname)" = Darwin ] || return 1
            (cd "$trabalho" && APARA_AUTO=1 APARA_DADOS="$tmp/dados" timeout 120 "$pasta/apara" --shot "$trabalho/captura.png" 0.5) >"$tmp/abertura.log" 2>&1 || true ;;
    esac
    return 0
}
if teste_abertura; then
    captura="$tmp/trabalho/captura.png"
    [ -s "$captura" ] || { tail -5 "$tmp/abertura.log" >&2; falha "o jogo do pacote não abriu (sem captura de tela)"; }
    [ "$(wc -c < "$captura")" -gt 20000 ] || falha "a captura do jogo do pacote é pequena demais ($(wc -c < "$captura") bytes): tela vazia?"
    depois=$(cd "$pasta" && find . | sort)
    [ "$antes" = "$depois" ] || falha "o jogo gravou dentro da pasta do pacote: $(echo "$antes
$depois" | sort | uniq -u | tr '\n' ' ')"
    abre=sim
    echo "pacote: abre de dentro da pasta (captura de $(wc -c < "$captura") bytes) e não grava ao lado do executável"
else
    echo "SEM TESTE DE ABERTURA: faltam as ferramentas para abrir o pacote de $plataforma aqui" >&2
    [ "${APARA_PACOTE_EXIGE_TESTE:-0}" != 1 ] || falha "APARA_PACOTE_EXIGE_TESTE=1 e o pacote não pôde ser aberto"
fi

mkdir -p "$saida"
if [ "$plataforma" = linux ]; then
    arq="$saida/$nome.tar.gz"
    [ ! -e "$arq" ] || falha "o destino já existe: $arq"
    (cd "$tmp" && tar -czf "$arq" "$nome")
else
    command -v zip >/dev/null 2>&1 || falha "sem o zip"
    arq="$saida/$nome.zip"
    [ ! -e "$arq" ] || falha "o destino já existe: $arq"
    (cd "$tmp" && zip -qr "$arq" "$nome")
fi
tamanho=$(wc -c < "$arq")
soma=$(sha256sum "$arq" 2>/dev/null | cut -d' ' -f1 || shasum -a 256 "$arq" | cut -d' ' -f1)
echo "pacote: $arq ($((tamanho / 1048576)) MB, abre: $abre)"
echo "sha256: $soma"

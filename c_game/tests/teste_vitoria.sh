#!/bin/sh
# Teste do jogo de verdade (precisa de ./apara, xvfb-run e libX11): a vitória é salva no golpe
# final, antes de qualquer clique na tela de vitória. O jogo joga sozinho (APARA_AUTO) desde o
# título, com um save que já viu a abertura; assim que o arquivo mostra o primeiro mestre vencido,
# o teste tira o jogo do ar de quatro jeitos e confere que o progresso continua no arquivo:
#   1. kill -9 (queda de luz, travamento);
#   2. ESC e Q de verdade, mandados à janela (tests/xtecla.c);
#   3. o pedido de fechar a janela (WM_DELETE_WINDOW), como o botão de fechar faz;
#   4. ESC e M (volta ao menu, que também salva);
#   5. o oboro: o golpe final salva a trilha completa antes de a escolha do final aparecer.
# E, em todos, o marco "tela_de_vitoria" ainda não tinha saído: o jogo estava antes da tela de vitória.
# O jogo roda com um passo de tempo fixo (--rec), mais rápido que o relógio; APARA_VITORIA_LENTA o
# põe em tempo real a partir da vitória salva, para as teclas de verdade chegarem a tempo.
# O progresso e as opções vão para uma pasta de dados temporária (APARA_DADOS): os de quem roda não são tocados.
cd "$(dirname "$0")/.." || exit 1

if [ "$1" != "--dentro" ]; then
    [ -x ./apara ] || { echo "teste_vitoria: falta ./apara (make)"; exit 2; }
    command -v xvfb-run >/dev/null 2>&1 || { echo "teste_vitoria: pulado (sem xvfb-run)"; exit 0; }
    TMP=$(mktemp -d)
    trap 'rm -rf "$TMP"' EXIT
    cc -std=c11 -O1 -o "$TMP/xtecla" tests/xtecla.c -lX11 2>/dev/null || { echo "teste_vitoria: pulado (sem libX11 para compilar tests/xtecla.c)"; exit 0; }
    TMP="$TMP" xvfb-run -a -s '-screen 0 1280x720x24' sh "$0" --dentro
    exit $?
fi

export APARA_DADOS="$TMP/dados"
mkdir -p "$APARA_DADOS"
SAVE="$APARA_DADOS/apara_save.txt"
FALHAS=0
confere() { # descrição, condição (0 = ok)
    if [ "$2" -eq 0 ]; then echo "  ok: $1"; else echo "  FALHA: $1"; FALHAS=$((FALHAS + 1)); fi
}
# o primeiro mestre vencido: mestre 1 na máscara e a trilha no seguinte
INICIAL='APARA-C 2
0 0 0 1'
DEPOIS_DO_PRIMEIRO='APARA-C 2
1 1 0 1'
# a luta final: os doze aprendizes vencidos (máscara 4095), e o oboro é o 13º (bit 12)
ANTES_DO_OBORO='APARA-C 2
12 4095 0 1'
DEPOIS_DO_OBORO='APARA-C 2
12 8191 1 1'

cenario() { # nome, como tirar o jogo do ar (kill | esc_q | fechar | esc_m), save inicial, save esperado, marco que ainda não pode ter saído
    NOME=$1
    COMO=$2
    INICIAL=$3
    ESPERADO=$4
    MARCO_PROIBIDO=$5
    echo "$NOME"
    printf '%s\n' "$INICIAL" > "$SAVE"
    printf '%s\n' "$INICIAL" > "$TMP/inicial.txt"
    APARA_AUTO=1 APARA_SEMENTE=11 APARA_VITORIA_LENTA=1 ./apara --rec "$TMP/q" 99999 99999 >"$TMP/$NOME.log" 2>&1 &
    PID=$!
    # espera o arquivo mudar do save inicial: nada além da vitória salva o progresso neste roteiro
    N=0
    while [ "$N" -lt 6000 ]; do
        cmp -s "$SAVE" "$TMP/inicial.txt" || break
        kill -0 "$PID" 2>/dev/null || break
        sleep 0.05
        N=$((N + 1))
    done
    ANTES_DA_TELA=$(grep -c "^TESTE_MARCO $MARCO_PROIBIDO" "$TMP/$NOME.log")
    case "$COMO" in
        kill) kill -9 "$PID"; wait "$PID" 2>/dev/null ;;
        esc_q) "$TMP/xtecla" esc; sleep 0.3; "$TMP/xtecla" q ;;
        fechar) "$TMP/xtecla" fechar ;;
        esc_m) "$TMP/xtecla" esc; sleep 0.3; "$TMP/xtecla" m; sleep 1; kill -9 "$PID"; wait "$PID" 2>/dev/null ;;
    esac
    if [ "$COMO" = esc_q ] || [ "$COMO" = fechar ]; then
        # saída normal do jogo: espera até 20 s
        N=0
        while kill -0 "$PID" 2>/dev/null && [ "$N" -lt 400 ]; do sleep 0.05; N=$((N + 1)); done
        if kill -0 "$PID" 2>/dev/null; then
            kill -9 "$PID"; wait "$PID" 2>/dev/null
            confere "o jogo saiu sozinho depois de $COMO" 1
        else
            wait "$PID" 2>/dev/null
            confere "o jogo saiu sozinho depois de $COMO (código $?)" "$([ "$?" -eq 0 ]; echo $?)"
        fi
    fi
    confere "a vitória já estava salva antes de '$MARCO_PROIBIDO' (marcos desse tipo vistos ao tirar o jogo do ar: $ANTES_DA_TELA)" "$([ "$ANTES_DA_TELA" -eq 0 ] && grep -q '^TESTE_MARCO vitoria_salva' "$TMP/$NOME.log"; echo $?)"
    grep '^TESTE_MARCO' "$TMP/$NOME.log" | tr '\n' ' '; echo
    confere "o save guarda a vitória: $(tr '\n' '|' < "$SAVE")" "$([ "$(cat "$SAVE")" = "$ESPERADO" ]; echo $?)"
}

cenario "1. kill -9 no instante da vitória" kill "$INICIAL" "$DEPOIS_DO_PRIMEIRO" tela_de_vitoria
cenario "2. ESC e Q de verdade" esc_q "$INICIAL" "$DEPOIS_DO_PRIMEIRO" tela_de_vitoria
cenario "3. fechar a janela" fechar "$INICIAL" "$DEPOIS_DO_PRIMEIRO" tela_de_vitoria
cenario "4. ESC e M (volta ao menu)" esc_m "$INICIAL" "$DEPOIS_DO_PRIMEIRO" tela_de_vitoria
cenario "5. oboro: o golpe final salva a trilha completa, antes da escolha do final" kill "$ANTES_DO_OBORO" "$DEPOIS_DO_OBORO" escolha_final

echo "6. reabrir depois de tudo isso: o título carrega o save sem perder nada"
ASAN_OPTIONS= timeout 60 ./apara --shot "$TMP/titulo.png" 1.0 >"$TMP/reabrir.log" 2>&1
confere "o jogo abriu e fechou sem erro (código $?)" "$([ "$?" -eq 0 ]; echo $?)"
confere "o save continua com a vitória" "$([ "$(cat "$SAVE")" = "$DEPOIS_DO_OBORO" ]; echo $?)"

if [ "$FALHAS" -eq 0 ]; then echo "teste_vitoria: tudo certo"; else echo "teste_vitoria: $FALHAS falha(s)"; fi
exit "$FALHAS"

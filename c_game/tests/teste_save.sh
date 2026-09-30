#!/bin/sh
# Teste do jogo de verdade (precisa de ./apara e de xvfb-run): as teclas de teste só existem
# com --teste, e nada disso grava o progresso. O jogo joga sozinho (APARA_AUTO: o robô do demo,
# mas salvando) e "aperta" R, V, P, N, B e 2 durante a luta (APARA_TECLAS; o 2 só age no oboro).
#   1. controle, sem opção nenhuma, do título: o jogo salva (senão o teste não prova nada);
#   2. --teste --master 1, com as teclas: as cinco que valem para o mestre agem e o save não muda;
#   3. --master 1 --duel, com as teclas: nenhuma age e o save não muda.
# O apara_save.txt e o apara_opcoes.txt de quem roda são guardados e devolvidos.
cd "$(dirname "$0")/.." || exit 1
[ -x ./apara ] || { echo "teste_save: falta ./apara (make)"; exit 2; }
command -v xvfb-run >/dev/null 2>&1 || { echo "teste_save: pulado (sem xvfb-run)"; exit 0; }

TMP=$(mktemp -d)
cp apara_save.txt "$TMP/save.orig" 2>/dev/null
cp apara_opcoes.txt "$TMP/opcoes.orig" 2>/dev/null
volta() {
    rm -rf apara_save.txt apara_save.txt.tmp apara_save.txt.bak
    if [ -f "$TMP/save.orig" ]; then cp "$TMP/save.orig" apara_save.txt; fi
    if [ -f "$TMP/opcoes.orig" ]; then cp "$TMP/opcoes.orig" apara_opcoes.txt; else rm -f apara_opcoes.txt; fi
    rm -rf "$TMP"
}
trap volta EXIT

FALHAS=0
confere() { # descrição, condição (0 = ok)
    if [ "$2" -eq 0 ]; then echo "  ok: $1"; else echo "  FALHA: $1"; FALHAS=$((FALHAS + 1)); fi
}
roda() { # VARIAVEL=valor..., ./apara e as opções do jogo: joga até a cabana do primeiro mestre
    # o xvfb-run junta o stderr do jogo ao stdout: guarda os dois
    APARA_AUTO=1 timeout 600 xvfb-run -a env "$@" >"$TMP/saida.txt" 2>&1
}

echo "1. controle: do título, sem opções"
rm -f apara_save.txt
roda ./apara --rec "$TMP/q" 99999 99999
confere "o jogo salvou o progresso ao vencer" "$([ -f apara_save.txt ] && grep -q 'APARA-C 2' apara_save.txt && [ "$(sed -n 2p apara_save.txt | cut -d' ' -f2)" != 0 ]; echo $?)"

echo "2. --teste --master 1, com as teclas de teste"
printf 'APARA-C 2\n0 0 0 1\n' > apara_save.txt
cp apara_save.txt "$TMP/save.base"
roda APARA_TECLAS=1 ./apara --teste --master 1 --rec "$TMP/q" 99999 99999
AGIU=$(grep -c '^TESTE_TECLA' "$TMP/saida.txt")
confere "as teclas R V P N B agiram ($AGIU de 5; o 2 só age no oboro)" "$([ "$AGIU" -eq 5 ]; echo $?)"
confere "o save ficou como estava" "$(cmp -s apara_save.txt "$TMP/save.base"; echo $?)"

echo "3. --master 1 --duel (sem --teste), com as teclas de teste e o F3 ligado"
printf 'APARA-C 2\n0 0 0 1\n' > apara_save.txt
roda APARA_DEBUG=1 APARA_TECLAS=1 ./apara --master 1 --duel --rec "$TMP/q" 99999 99999
AGIU=$(grep -c '^TESTE_TECLA' "$TMP/saida.txt")
confere "nenhuma tecla de teste agiu ($AGIU)" "$([ "$AGIU" -eq 0 ]; echo $?)"
confere "o save ficou como estava" "$(cmp -s apara_save.txt "$TMP/save.base"; echo $?)"

echo "4. saves estragados: o jogo abre, avisa e guarda o arquivo no .bak"
estragado() { # nome, conteúdo, o que esperar (bom | estragado)
    rm -f apara_save.txt apara_save.txt.bak
    printf '%s' "$2" > apara_save.txt
    cp apara_save.txt "$TMP/caso.txt"
    APARA_AUTO=1 timeout 60 xvfb-run -a ./apara --shot "$TMP/titulo.png" 0.3 >"$TMP/caso.log" 2>&1
    CODIGO=$?
    if [ "$3" = bom ]; then
        confere "$1: abre sem cair (código $CODIGO), sem aviso, e o save não é tocado" \
            "$([ "$CODIGO" -eq 0 ] && ! grep -q '^TESTE_MARCO save_' "$TMP/caso.log" && cmp -s apara_save.txt "$TMP/caso.txt" && [ ! -e apara_save.txt.bak ]; echo $?)"
    else
        confere "$1: abre sem cair (código $CODIGO), avisa, e guarda o arquivo byte a byte no .bak" \
            "$([ "$CODIGO" -eq 0 ] && grep -q '^TESTE_MARCO save_corrompido' "$TMP/caso.log" && grep -q 'guardei uma cópia' "$TMP/caso.log" && cmp -s apara_save.txt.bak "$TMP/caso.txt" && [ ! -e apara_save.txt ]; echo $?)"
    fi
}
estragado "save bom" "APARA-C 2
5 31 0 1
" bom
estragado "vazio" "" estragado
estragado "lixo" "ZZZ
" estragado
estragado "truncado" "APARA-C 2
5 31" estragado
estragado "mestre fora da trilha" "APARA-C 2
99 0 0 1
" estragado
estragado "mestre negativo" "APARA-C 2
-3 0 0 1
" estragado
estragado "máscara, trilha completa e abertura fora do intervalo" "APARA-C 2
5 4294967295 7 9
" estragado
estragado "versão 1" "APARA-C 1
5 31 0 1
" estragado
estragado "trilha completa no meio da trilha" "APARA-C 2
5 31 1 1
" estragado

echo "5. gravar impossível: o jogo avisa, e não deixa .tmp nem estraga nada"
rm -rf apara_save.txt apara_save.txt.bak
mkdir apara_save.txt
# do título, sem save: a abertura acaba e o jogo tenta gravar (finish_lore); espera o aviso e fecha
APARA_AUTO=1 xvfb-run -a ./apara --rec "$TMP/q" 99999 99999 >"$TMP/falha.log" 2>&1 &
PID=$!
N=0
while [ "$N" -lt 1200 ] && kill -0 "$PID" 2>/dev/null && ! grep -q '^TESTE_MARCO save_falhou' "$TMP/falha.log"; do sleep 0.1; N=$((N + 1)); done
sleep 0.5
kill "$PID" 2>/dev/null; wait "$PID" 2>/dev/null
pkill -f "apara --rec $TMP/q" 2>/dev/null
confere "o jogo avisou no terminal e na faixa (marco save_falhou) em vez de perder calado" "$(grep -q '^TESTE_MARCO save_falhou' "$TMP/falha.log" && grep -q 'não consegui gravar o progresso' "$TMP/falha.log"; echo $?)"
confere "sem .tmp sobrando, e a pasta continua lá" "$([ ! -e apara_save.txt.tmp ] && [ -d apara_save.txt ]; echo $?)"
rmdir apara_save.txt 2>/dev/null

if [ "$FALHAS" -eq 0 ]; then echo "teste_save: tudo certo"; else echo "teste_save: $FALHAS falha(s)"; fi
exit "$FALHAS"

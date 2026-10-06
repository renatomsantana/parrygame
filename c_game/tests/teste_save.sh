#!/bin/sh
# Teste do jogo de verdade (precisa de ./apara e de xvfb-run): as teclas de teste só existem
# com --teste, e nada disso grava o progresso. O jogo joga sozinho (APARA_AUTO: o robô do demo,
# mas salvando) e "aperta" R, V, P, N, B e 2 durante a luta (APARA_TECLAS; o 2 só age no oboro).
#   1. controle, sem opção nenhuma, do título: o jogo salva (senão o teste não prova nada);
#   2. --teste --master 1, com as teclas: as cinco que valem para o mestre agem e o save não muda;
#   3. --master 1 --duel, com as teclas: nenhuma age e o save não muda.
#   4. saves estragados; 5. gravar impossível;
#   6-8. o save e as opções que as versões antigas guardavam ao lado do executável: copiados para a pasta de dados
#        do usuário (sem tocar o original, sem sobrescrever o novo, sem copiar lixo), e o que acontece quando a pasta
#        de dados não existe e não dá para criá-la;
#   9. as opções (a calibração): gravadas, gravar impossível avisa, arquivo estragado vai para o .bak.
# O progresso e as opções vão para uma pasta de dados temporária (APARA_DADOS), ou para uma cópia do jogo numa pasta
# temporária (cenários 6 a 8): os arquivos de quem roda não são tocados.
cd "$(dirname "$0")/.." || exit 1
[ -x ./apara ] || { echo "teste_save: falta ./apara (make)"; exit 2; }
command -v xvfb-run >/dev/null 2>&1 || { echo "teste_save: pulado (sem xvfb-run)"; exit 0; }

TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT
export APARA_DADOS="$TMP/dados"
mkdir -p "$APARA_DADOS"
SAVE="$APARA_DADOS/apara_save.txt"
OPCOES="$APARA_DADOS/apara_opcoes.txt"

FALHAS=0
confere() { # descrição, condição (0 = ok)
    if [ "$2" -eq 0 ]; then echo "  ok: $1"; else echo "  FALHA: $1"; FALHAS=$((FALHAS + 1)); fi
}
roda() { # VARIAVEL=valor..., ./apara e as opções do jogo: joga até a cabana do primeiro mestre
    # o xvfb-run junta o stderr do jogo ao stdout: guarda os dois
    APARA_AUTO=1 timeout 600 xvfb-run -a env "$@" >"$TMP/saida.txt" 2>&1
}

echo "1. controle: do título, sem opções"
rm -f "$SAVE"
roda ./apara --rec "$TMP/q" 99999 99999
confere "o jogo salvou o progresso ao vencer" "$([ -f "$SAVE" ] && grep -q 'APARA-C 2' "$SAVE" && [ "$(sed -n 2p "$SAVE" | cut -d' ' -f2)" != 0 ]; echo $?)"

echo "2. --teste --master 1, com as teclas de teste"
printf 'APARA-C 2\n0 0 0 1\n' > "$SAVE"
cp "$SAVE" "$TMP/save.base"
roda APARA_TECLAS=1 ./apara --teste --master 1 --rec "$TMP/q" 99999 99999
AGIU=$(grep -c '^TESTE_TECLA' "$TMP/saida.txt")
confere "as teclas R V P N B agiram ($AGIU de 5; o 2 só age no oboro)" "$([ "$AGIU" -eq 5 ]; echo $?)"
confere "o save ficou como estava" "$(cmp -s "$SAVE" "$TMP/save.base"; echo $?)"

echo "3. --master 1 --duel (sem --teste), com as teclas de teste e o F3 ligado"
printf 'APARA-C 2\n0 0 0 1\n' > "$SAVE"
roda APARA_DEBUG=1 APARA_TECLAS=1 ./apara --master 1 --duel --rec "$TMP/q" 99999 99999
AGIU=$(grep -c '^TESTE_TECLA' "$TMP/saida.txt")
confere "nenhuma tecla de teste agiu ($AGIU)" "$([ "$AGIU" -eq 0 ]; echo $?)"
confere "o save ficou como estava" "$(cmp -s "$SAVE" "$TMP/save.base"; echo $?)"

echo "4. saves estragados: o jogo abre, avisa e guarda o arquivo no .bak"
estragado() { # nome, conteúdo, o que esperar (bom | estragado)
    rm -f "$SAVE" "$SAVE.bak"
    printf '%s' "$2" > "$SAVE"
    cp "$SAVE" "$TMP/caso.txt"
    APARA_AUTO=1 timeout 60 xvfb-run -a ./apara --shot "$TMP/titulo.png" 0.3 >"$TMP/caso.log" 2>&1
    CODIGO=$?
    if [ "$3" = bom ]; then
        confere "$1: abre sem cair (código $CODIGO), sem aviso, e o save não é tocado" \
            "$([ "$CODIGO" -eq 0 ] && ! grep -q '^TESTE_MARCO save_' "$TMP/caso.log" && cmp -s "$SAVE" "$TMP/caso.txt" && [ ! -e "$SAVE.bak" ]; echo $?)"
    else
        confere "$1: abre sem cair (código $CODIGO), avisa, e guarda o arquivo byte a byte no .bak" \
            "$([ "$CODIGO" -eq 0 ] && grep -q '^TESTE_MARCO save_corrompido' "$TMP/caso.log" && grep -q 'cópia em' "$TMP/caso.log" && cmp -s "$SAVE.bak" "$TMP/caso.txt" && [ ! -e "$SAVE" ]; echo $?)"
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
rm -rf "$SAVE" "$SAVE.bak"
mkdir "$SAVE"
# do título, sem save: a abertura acaba e o jogo tenta gravar (finish_lore); espera o aviso e fecha
APARA_AUTO=1 xvfb-run -a ./apara --rec "$TMP/q" 99999 99999 >"$TMP/falha.log" 2>&1 &
PID=$!
N=0
while [ "$N" -lt 1200 ] && kill -0 "$PID" 2>/dev/null && ! grep -q '^TESTE_MARCO save_falhou' "$TMP/falha.log"; do sleep 0.1; N=$((N + 1)); done
sleep 0.5
kill "$PID" 2>/dev/null; wait "$PID" 2>/dev/null
pkill -f "apara --rec $TMP/q" 2>/dev/null
confere "o jogo avisou no terminal e na faixa (marco save_falhou) em vez de perder calado" "$(grep -q '^TESTE_MARCO save_falhou' "$TMP/falha.log" && grep -q 'não consegui gravar o progresso' "$TMP/falha.log"; echo $?)"
confere "sem .tmp sobrando, e a pasta continua lá" "$([ ! -e "$SAVE.tmp" ] && [ -d "$SAVE" ]; echo $?)"
rmdir "$SAVE" 2>/dev/null


# Uma cópia do jogo numa pasta temporária: o "ao lado do executável" dela é de mentira, e HOME e XDG_DATA_HOME
# também, então os arquivos de quem roda ficam fora disso. Sem APARA_DADOS, para valer o caminho de verdade.
JOGO="$TMP/jogo"
mkdir -p "$JOGO"
cp ./apara "$JOGO/apara"
ln -s "$PWD/assets" "$JOGO/assets"
mkdir -p "$TMP/home"
copia() { # VARIAVEL=valor..., o jogo da cópia, aberto e fechado no título (--shot)
    env -u APARA_DADOS HOME="$TMP/home" XDG_DATA_HOME="$TMP/xdg" APARA_AUTO=1 timeout 60 xvfb-run -a env "$@" "$JOGO/apara" --shot "$TMP/titulo.png" 0.3 >"$TMP/copia.log" 2>&1
}
ANTIGO_SAVE='APARA-C 2
5 31 0 1
'
ANTIGO_OPCOES='atraso_video_ms 40
atraso_audio_ms 25
'
NOVO_SAVE="$TMP/xdg/apara/apara_save.txt"
NOVA_OPCOES="$TMP/xdg/apara/apara_opcoes.txt"

echo "6. o save e as opções ao lado do executável (versões antigas): copiados para a pasta de dados, o original fica"
printf '%s' "$ANTIGO_SAVE" > "$JOGO/apara_save.txt"
printf '%s' "$ANTIGO_OPCOES" > "$JOGO/apara_opcoes.txt"
copia
confere "o jogo abriu sem cair (código $?)" "$([ "$?" -eq 0 ]; echo $?)"
confere "o save foi copiado para a pasta de dados do usuário, igual byte a byte" "$([ "$(cat "$NOVO_SAVE" 2>/dev/null)" = "$(printf '%s' "$ANTIGO_SAVE")" ]; echo $?)"
confere "as opções também" "$([ "$(cat "$NOVA_OPCOES" 2>/dev/null)" = "$(printf '%s' "$ANTIGO_OPCOES")" ]; echo $?)"
confere "o original ao lado do jogo continua lá, intacto" "$([ "$(cat "$JOGO/apara_save.txt")" = "$(printf '%s' "$ANTIGO_SAVE")" ] && [ "$(cat "$JOGO/apara_opcoes.txt")" = "$(printf '%s' "$ANTIGO_OPCOES")" ]; echo $?)"
confere "o jogo avisou do que copiou (marcos save_migrado e opcoes_migrado) e não reclamou de nada" "$(grep -q '^TESTE_MARCO save_migrado' "$TMP/copia.log" && grep -q '^TESTE_MARCO opcoes_migrado' "$TMP/copia.log" && ! grep -q '^TESTE_MARCO .*\(falhou\|invalido\|corrompid\|ilegiv\|sem_pasta\)' "$TMP/copia.log"; echo $?)"
confere "sem .tmp nem .bak sobrando" "$([ ! -e "$NOVO_SAVE.tmp" ] && [ ! -e "$NOVO_SAVE.bak" ] && [ ! -e "$JOGO/apara_save.txt.bak" ]; echo $?)"
printf 'APARA-C 2\n9 511 0 1\n' > "$NOVO_SAVE"
cp "$NOVO_SAVE" "$TMP/mais_novo.txt"
copia
confere "de novo: o save da pasta de dados manda e não é sobrescrito pelo antigo (sem migrar outra vez)" "$(cmp -s "$NOVO_SAVE" "$TMP/mais_novo.txt" && ! grep -q '^TESTE_MARCO save_migrado' "$TMP/copia.log"; echo $?)"

echo "7. o antigo é lixo: não é copiado nem tocado, e o jogo abre do começo"
rm -rf "$TMP/xdg"
printf 'ZZZ\n' > "$JOGO/apara_save.txt"
printf 'lixo\n' > "$JOGO/apara_opcoes.txt"
copia
confere "o jogo abriu sem cair (código $?)" "$([ "$?" -eq 0 ]; echo $?)"
confere "nada foi copiado, e o jogo disse por quê" "$([ ! -e "$NOVO_SAVE" ] && [ ! -e "$NOVA_OPCOES" ] && grep -q '^TESTE_MARCO save_antigo_invalido' "$TMP/copia.log" && grep -q '^TESTE_MARCO opcoes_antigo_invalido' "$TMP/copia.log"; echo $?)"
confere "os originais continuam como estavam, sem .bak" "$([ "$(cat "$JOGO/apara_save.txt")" = ZZZ ] && [ "$(cat "$JOGO/apara_opcoes.txt")" = lixo ] && [ ! -e "$JOGO/apara_save.txt.bak" ]; echo $?)"

echo "8. a pasta de dados não existe e não dá para criar: o jogo avisa e guarda ao lado do jogo, sem perder nada"
rm -rf "$TMP/xdg"
printf 'um arquivo onde devia haver uma pasta' > "$TMP/xdg"
rm -f "$JOGO/apara_save.txt" "$JOGO/apara_opcoes.txt"
APARA_AUTO=1 env -u APARA_DADOS HOME="$TMP/home" XDG_DATA_HOME="$TMP/xdg" timeout 600 xvfb-run -a "$JOGO/apara" --rec "$TMP/q" 99999 99999 >"$TMP/semdados.log" 2>&1
confere "o jogo avisou (marco dados_sem_pasta, e a faixa)" "$(grep -q '^TESTE_MARCO dados_sem_pasta' "$TMP/semdados.log" && grep -q 'não consegui criar a pasta de dados' "$TMP/semdados.log"; echo $?)"
confere "e o progresso foi para o arquivo ao lado do jogo" "$([ -f "$JOGO/apara_save.txt" ] && grep -q 'APARA-C 2' "$JOGO/apara_save.txt" && [ "$(sed -n 2p "$JOGO/apara_save.txt" | cut -d' ' -f2)" != 0 ]; echo $?)"
rm -rf "$TMP/xdg"

echo "9. as opções (calibração): gravadas, gravar impossível avisa, arquivo estragado vai para o .bak"
opcoes() { # VARIAVEL=valor...: abre a calibração já na tela de confirmar (APARA_CALIBRA_PRONTA) e o jogo confirma sozinho
    APARA_AUTO=1 timeout 60 xvfb-run -a env "$@" ./apara --state calibra --shot "$TMP/cal.png" 4 >"$TMP/opcoes.log" 2>&1
}
rm -rf "$OPCOES" "$OPCOES.bak"
opcoes APARA_CALIBRA_PRONTA=40,25
confere "calibrar grava as opções (marco opcoes_gravadas): $(tr '\n' '|' < "$OPCOES" 2>/dev/null)" "$(grep -q '^TESTE_MARCO opcoes_gravadas' "$TMP/opcoes.log" && [ "$(cat "$OPCOES")" = "$(printf 'atraso_video_ms 40\natraso_audio_ms 25\n')" ] && [ ! -e "$OPCOES.tmp" ]; echo $?)"
rm -rf "$OPCOES"
mkdir "$OPCOES"
opcoes APARA_CALIBRA_PRONTA=40,25
confere "gravar as opções impossível: o jogo avisa (marco opcoes_falhou) em vez de calar" "$(grep -q '^TESTE_MARCO opcoes_falhou' "$TMP/opcoes.log" && grep -q 'não consegui gravar as opções' "$TMP/opcoes.log"; echo $?)"
confere "sem .tmp sobrando, e o que estava no lugar continua lá" "$([ ! -e "$OPCOES.tmp" ] && [ -d "$OPCOES" ]; echo $?)"
rmdir "$OPCOES"
printf 'atraso_video_ms quarenta\n' > "$OPCOES"
cp "$OPCOES" "$TMP/opcoes_ruins.txt"
APARA_AUTO=1 timeout 60 xvfb-run -a ./apara --shot "$TMP/titulo.png" 0.3 >"$TMP/opcoes.log" 2>&1
confere "opções estragadas: o jogo abre, avisa (marco opcoes_corrompidas) e guarda o arquivo byte a byte no .bak" "$(grep -q '^TESTE_MARCO opcoes_corrompidas' "$TMP/opcoes.log" && cmp -s "$OPCOES.bak" "$TMP/opcoes_ruins.txt" && [ ! -e "$OPCOES" ]; echo $?)"
rm -f "$OPCOES.bak"
printf 'ZZZ\n' > "$SAVE"
printf 'lixo\n' > "$OPCOES"
APARA_AUTO=1 timeout 60 xvfb-run -a ./apara --shot "$TMP/titulo.png" 0.3 >"$TMP/opcoes.log" 2>&1
confere "save e opções estragados juntos: a faixa mostra os dois avisos, um não apaga o outro" "$(grep '^TESTE_FAIXA' "$TMP/opcoes.log" | tail -1 | grep -q 'apara_save.txt.bak' && grep '^TESTE_FAIXA' "$TMP/opcoes.log" | tail -1 | grep -q 'apara_opcoes.txt.bak'; echo $?)"
rm -f "$SAVE.bak" "$OPCOES.bak" "$SAVE"
printf 'atraso_video_ms 7000\natraso_audio_ms 3\n' > "$OPCOES"
APARA_AUTO=1 APARA_DEBUG=1 timeout 60 xvfb-run -a ./apara --shot "$TMP/titulo.png" 0.3 >"$TMP/opcoes.log" 2>&1
confere "opções fora do limite não são erro: o jogo abre sem aviso e o arquivo fica como está" "$(! grep -q '^TESTE_MARCO opcoes_' "$TMP/opcoes.log" && [ "$(cat "$OPCOES")" = "$(printf 'atraso_video_ms 7000\natraso_audio_ms 3\n')" ] && [ ! -e "$OPCOES.bak" ]; echo $?)"

echo "10. a pasta de dados é criada, e as de cima também (APARA_DADOS)"
APARA_AUTO=1 APARA_DADOS="$TMP/novo/fundo/dados" timeout 60 xvfb-run -a ./apara --shot "$TMP/titulo.png" 0.3 >"$TMP/criar.log" 2>&1
confere "pasta criada com as de cima" "$([ -d "$TMP/novo/fundo/dados" ]; echo $?)"
mkdir -p "$TMP/cwd"
(cd "$TMP/cwd" && APARA_AUTO=1 APARA_DADOS="relativa" timeout 60 xvfb-run -a "$JOGO/apara" --shot "$TMP/titulo.png" 0.3 >"$TMP/criar.log" 2>&1)
confere "APARA_DADOS relativa vale a partir de onde o jogo foi aberto, não da pasta do executável" "$([ -d "$TMP/cwd/relativa" ] && [ ! -d "$JOGO/relativa" ]; echo $?)"

if [ "$FALHAS" -eq 0 ]; then echo "teste_save: tudo certo"; else echo "teste_save: $FALHAS falha(s)"; fi
exit "$FALHAS"

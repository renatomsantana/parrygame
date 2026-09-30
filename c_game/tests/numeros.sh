#!/bin/sh
# Nenhum número mágico nas regras: em src/core.c e src/robo.c, todo número que não seja 0, 1, 2 ou 0,5
# tem nome, e o nome mora no src/ajuste.h (as constantes globais) ou no src/roster.c (as de cada mestre).
# Comentários e strings não contam. Uma linha com "num-ok" é de um algoritmo (o gerador de números
# aleatórios, constantes matemáticas): o comentário diz por quê.
#   tests/numeros.sh          (make numeros)
cd "$(dirname "$0")/.." || exit 1
ACHOU=0
for F in src/core.c src/robo.c; do
    SAIDA=$(awk '
        { linha = $0 }
        /num-ok/ { next }
        {
            # comentários de bloco, de uma ou de várias linhas
            while (1) {
                if (dentro) {
                    if (match(linha, /\*\//)) { linha = substr(linha, RSTART + 2); dentro = 0 } else { linha = ""; break }
                } else if (match(linha, /\/\*/)) {
                    resto = substr(linha, RSTART + 2)
                    linha = substr(linha, 1, RSTART - 1)
                    if (match(resto, /\*\//)) linha = linha " " substr(resto, RSTART + 2)
                    else { dentro = 1; break }
                } else break
            }
            sub(/\/\/.*/, "", linha)
            gsub(/"[^"]*"/, "\"\"", linha)            # strings
            gsub(/'"'"'[^'"'"']'"'"'/, "", linha)
            while (match(linha, /[^A-Za-z_0-9.][0-9]+\.?[0-9]*([eE][-+]?[0-9]+)?[fFuUlL]*/)) {
                n = substr(linha, RSTART + 1, RLENGTH - 1)
                linha = substr(linha, RSTART + RLENGTH)
                if (n ~ /^(0|1|2|0\.0|1\.0|2\.0|0\.5|0\.|1\.)[fFuU]*$/) continue
                printf "%s:%d: %s   -> %s\n", FILENAME, NR, n, $0
            }
        }' "$F")
    if [ -n "$SAIDA" ]; then echo "$SAIDA"; ACHOU=1; fi
done
if [ "$ACHOU" -eq 0 ]; then echo "numeros: nenhum número mágico em src/core.c e src/robo.c"; else echo "numeros: dê nome ao número no src/ajuste.h (ou no src/roster.c)"; fi
exit "$ACHOU"

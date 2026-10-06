#!/bin/sh
# Conferência sem GPU: o clone precisa receber os PNGs de runtime e seus manifests.
set -eu
cd "$(dirname "$0")/.."
falhas=0
tiras=0
for nome in kojiro hanzo hanzo_mascara daichi genbu raizo shizuku garfiel karasu hayate enjin suiren arashi yoru jinshi oboro oboro_mascara; do
    pasta="assets/sprites/$nome"
    if [ ! -s "$pasta/sprite.txt" ]; then echo "FALTA: $pasta/sprite.txt"; falhas=$((falhas + 1)); continue; fi
    for anim in $(awk '$1 == "anim" { print $2 }' "$pasta/sprite.txt"); do
        case "$anim" in *[!A-Za-z0-9_]*|'') echo "Manifest inválido: $pasta ($anim)"; exit 1;; esac
        if [ ! -s "$pasta/$anim.png" ]; then echo "FALTA: $pasta/$anim.png"; falhas=$((falhas + 1)); fi
        if [ -s assets/sprites/_fx/slash.png ]; then
            case "$nome/$anim" in
                garfiel/ATTACK*|garfiel/ESPECIAL|arashi/ATTACK*|arashi/ESPECIAL|oboro*/ATTACK*ECO_GARFIEL|oboro*/ATTACK*ECO_ARASHI|oboro*/ATTACK*FURIA)
                    for ext in png txt; do
                        if [ ! -s "$pasta/_clean_$anim.$ext" ]; then echo "FALTA: $pasta/_clean_$anim.$ext"; falhas=$((falhas + 1)); fi
                    done;;
            esac
        fi
        case "$nome/$anim" in
            kojiro/*|hanzo*/*) ;;
            daichi/ATTACK*|genbu/ATTACK*|raizo/ATTACK*|shizuku/ATTACK*|garfiel/ATTACK*|karasu/ATTACK*|hayate/ATTACK*|enjin/ATTACK*|suiren/ATTACK*|arashi/ATTACK*|yoru/ATTACK*|jinshi/ATTACK*|oboro*/ATTACK*|*/ESPECIAL|*/STRONG_ATTACK*)
                for layer in weapon steel; do
                    if [ ! -s "$pasta/_${layer}_$anim.png" ]; then echo "FALTA: $pasta/_${layer}_$anim.png"; falhas=$((falhas + 1)); fi
                done;;
        esac
        tiras=$((tiras + 1))
    done
done
[ "$falhas" -eq 0 ] || { echo "assets: $falhas arquivos ausentes; instale os packs e execute make sprites"; exit 1; }
echo "assets: 17 conjuntos, $tiras tiras declaradas presentes (a geometria é conferida por make test-assets)"

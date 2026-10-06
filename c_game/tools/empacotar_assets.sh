#!/bin/sh
# Pacote privado de runtime para a equipe; não inclui as pranchas-fonte nem progresso.
set -eu
cd "$(dirname "$0")/.."
out=${1:-}
[ -n "$out" ] || { echo 'uso: make pacote-assets OUT=/caminho/assets-runtime.zip' >&2; exit 2; }
case "$out" in /*) ;; *) out="$PWD/$out";; esac
[ ! -e "$out" ] || { echo "O destino já existe: $out" >&2; exit 2; }
sh tools/verificar_assets.sh
tmp=$(mktemp -d)
trap 'rm -rf "$tmp"' EXIT
sh tools/copiar_assets_runtime.sh "$tmp"
mkdir -p "$(dirname "$out")"
(cd "$tmp" && zip -qr "$out" assets)
echo "Pacote de runtime: $out (descompactar na pasta c_game do clone)"

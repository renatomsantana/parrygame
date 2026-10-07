#!/bin/sh
# Traz a arte do jogo (assets/sprites e assets/katana) do repositório privado parrygame-assets, que o parrygame público não leva.
#   sh tools/baixar_assets.sh                 clona o repositório (precisa de acesso a ele: git já autenticado ou token no ambiente)
#   APARA_ASSETS=/caminho sh tools/baixar_assets.sh   usa uma cópia local dele em vez de clonar
#   APARA_ASSETS_REPO=url  troca o endereço do repositório
set -eu
cd "$(dirname "$0")/.." || exit 1
REPO=${APARA_ASSETS_REPO:-https://github.com/renatomsantana/parrygame-assets}
tmp=""
if [ -n "${APARA_ASSETS:-}" ]; then
    origem=$APARA_ASSETS
else
    tmp=$(mktemp -d) || exit 1
    trap 'rm -rf "$tmp"' EXIT
    git clone --depth 1 "$REPO" "$tmp/assets" || { echo "baixar_assets: não consegui clonar $REPO (o repositório é privado: confira o acesso)"; exit 1; }
    origem=$tmp/assets
fi
[ -d "$origem/sprites/kojiro" ] && [ -d "$origem/katana" ] || { echo "baixar_assets: $origem não tem sprites/ e katana/"; exit 1; }
mkdir -p assets
cp -R "$origem/sprites" "$origem/katana" assets/
echo "baixar_assets: assets/sprites e assets/katana prontos"
sh tools/verificar_assets.sh

#!/bin/sh
# Copia para DESTINO/assets os arquivos de execução do jogo: as pastas de personagens (sem as pranchas-fonte _original, _packs e
# _folhas), as fontes, o modelo da katana e, se existirem, o áudio e os cenários da equipe. Usado pelo pacote de assets e pelo pacote do jogo.
#   tools/copiar_assets_runtime.sh DESTINO
set -eu
cd "$(dirname "$0")/.."
destino=${1:-}
[ -n "$destino" ] || { echo 'uso: tools/copiar_assets_runtime.sh DESTINO' >&2; exit 2; }
mkdir -p "$destino/assets/sprites"
for pasta in assets/sprites/*; do
    [ -d "$pasta" ] || continue
    case "${pasta##*/}" in _original|_packs|_folhas) continue;; esac
    cp -R "$pasta" "$destino/assets/sprites/"
done
for pasta in fonts katana audio arenas; do
    [ ! -d "assets/$pasta" ] || cp -R "assets/$pasta" "$destino/assets/"
done

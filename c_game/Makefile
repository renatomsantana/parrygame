# APARA em C. Requer raylib (brew install raylib).
CC ?= cc
CFLAGS ?= -std=c11 -O2 -Wall -Wextra -Wno-missing-field-initializers
RAYLIB_CFLAGS := $(shell pkg-config --cflags raylib 2>/dev/null)
RAYLIB_LIBS := $(shell pkg-config --libs raylib 2>/dev/null)
# Sem pkg-config: tenta o prefixo do Homebrew.
ifeq ($(RAYLIB_LIBS),)
  RAYLIB_PREFIX := $(shell brew --prefix raylib 2>/dev/null)
  ifneq ($(RAYLIB_PREFIX),)
    RAYLIB_CFLAGS := -I$(RAYLIB_PREFIX)/include
    RAYLIB_LIBS := -L$(RAYLIB_PREFIX)/lib -lraylib
  else
    RAYLIB_LIBS := -lraylib
  endif
endif
ifeq ($(shell uname),Darwin)
  RAYLIB_LIBS += -framework Cocoa -framework IOKit -framework CoreVideo -framework OpenGL
endif

CORE := src/core.c src/roster.c
GAME := src/main.c src/rig.c src/sprites.c src/katana3d.c src/arenas.c src/audio.c src/fx.c src/lore.c

all: apara

# Gera os lutadores a partir das pranchas do Samurai #3 (ver ../docs/PERSONAGENS.md).
personagens: tools/personagens.c
	$(CC) $(CFLAGS) -ffp-contract=off $(RAYLIB_CFLAGS) tools/personagens.c -o $@ $(RAYLIB_LIBS) -lm

sprites: personagens
	./personagens --folhas

# Põe o zip das animações da Mattz Art nas pastas certas: make packs ZIP=arquivo.zip
packs:
	tools/instalar_packs.sh "$(ZIP)"

apara: $(CORE) $(GAME) src/*.h
	$(CC) $(CFLAGS) $(RAYLIB_CFLAGS) $(CORE) $(GAME) -o $@ $(RAYLIB_LIBS) -lm

core_test: $(CORE) tests/core_test.c src/core.h
	$(CC) $(CFLAGS) $(CORE) tests/core_test.c -o $@ -lm

test: core_test
	./core_test

run: apara
	./apara

clean:
	rm -f apara core_test personagens

.PHONY: packs all test run clean sprites

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

CORE := src/ajuste.c src/core.c src/roster.c src/robo.c
GAME := src/main.c src/fonte.c src/rig.c src/sprites.c src/pixelize.c src/katana3d.c src/arenas.c src/audio.c src/fx.c src/lore.c

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

core_test: $(CORE) tests/core_test.c src/core.h src/ajuste.h src/robo.h
	$(CC) $(CFLAGS) $(CORE) tests/core_test.c -o $@ -lm

# Curva de dificuldade: os robôs contra cada aprendiz (make robos LUTAS=300).
LUTAS ?= 300
robos_relatorio: $(CORE) tests/robos.c src/core.h src/ajuste.h src/robo.h
	$(CC) $(CFLAGS) $(CORE) tests/robos.c -o $@ -lm

HZ ?= 60
QUADROS ?= 0
robos: robos_relatorio
	./robos_relatorio $(LUTAS) $(HZ) $(QUADROS)

# Todo caractere não ASCII das strings do jogo tem glifo na fonte da interface.
fonte_test: src/fonte.c src/fonte.h tests/fonte_test.c
	$(CC) $(CFLAGS) src/fonte.c tests/fonte_test.c -o $@

test-fonte: fonte_test
	./fonte_test assets/fonts/Tiny5-Regular.ttf src/*.c

test: core_test test-fonte
	./core_test

run: apara
	./apara

clean:
	rm -f apara core_test fonte_test robos_relatorio personagens

# O jogo de verdade, sob xvfb: as teclas de teste só existem com --teste e nada grava o progresso.
teste-jogo: apara
	sh tests/teste_save.sh
	sh tests/teste_rastro.sh

.PHONY: packs all test run clean sprites robos teste-jogo

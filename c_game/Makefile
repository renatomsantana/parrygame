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
GAME := src/main.c src/fonte.c src/salvar.c src/entrada.c src/rig.c src/sprites.c src/pixelize.c src/katana3d.c src/arenas.c src/audio.c src/fx.c src/lore.c

# O carimbo do clique (src/entrada_plat.h): um arquivo por plataforma. Sem XInput2 (Wayland puro, Windows),
# o stub: o jogo funciona igual, com o aperto no meio do quadro.
ifeq ($(shell uname),Darwin)
  ENTRADA_PLAT := src/entrada_mac.m
else
  ifeq ($(shell pkg-config --exists xi x11 2>/dev/null && echo sim),sim)
    ENTRADA_PLAT := src/entrada_linux.c
    ENTRADA_CFLAGS := $(shell pkg-config --cflags xi x11)
    ENTRADA_LIBS := $(shell pkg-config --libs xi x11) -lpthread
  else
    ENTRADA_PLAT := src/entrada_stub.c
  endif
endif

all: apara

# Gera os lutadores a partir das pranchas do Samurai #3 (ver ../docs/PERSONAGENS.md).
personagens: tools/personagens.c
	$(CC) $(CFLAGS) -ffp-contract=off $(RAYLIB_CFLAGS) tools/personagens.c -o $@ $(RAYLIB_LIBS) -lm

sprites: personagens
	./personagens --folhas

# Põe o zip das animações da Mattz Art nas pastas certas: make packs ZIP=arquivo.zip
packs:
	tools/instalar_packs.sh "$(ZIP)"

apara: $(CORE) $(GAME) $(ENTRADA_PLAT) src/*.h
	$(CC) $(CFLAGS) $(RAYLIB_CFLAGS) $(ENTRADA_CFLAGS) $(CORE) $(GAME) $(ENTRADA_PLAT) -o $@ $(RAYLIB_LIBS) $(ENTRADA_LIBS) -lm

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

# O arquivo de progresso: formato, validação, .bak e gravação atômica.
save_test: src/salvar.c src/salvar.h src/core.h tests/save_test.c
	$(CC) $(CFLAGS) src/salvar.c src/core.c src/roster.c src/ajuste.c tests/save_test.c -o $@ -lm

test-save: save_test
	./save_test

# Fuzz do núcleo (tests/fuzz.c): make fuzz N=3000. O make teste roda 600 cenários por modo.
N ?= 3000
fuzz_test: $(CORE) tests/fuzz.c src/core.h src/robo.h
	$(CC) $(CFLAGS) $(CORE) tests/fuzz.c -o $@ -lm

fuzz: fuzz_test
	./fuzz_test $(N) 1

fuzz-rapido: fuzz_test
	./fuzz_test 600 1

# A mesma luta a 30, 60, 120, 144 e 240 Hz tem que dar o mesmo resultado (tests/robos.c --taxas).
TAXAS_LUTAS ?= 100
robos-taxas: robos_relatorio
	./robos_relatorio --taxas $(TAXAS_LUTAS)

# O carimbo do clique: a conversão para o relógio do núcleo, os carimbos inválidos e as lutas dos robôs com ele.
entrada_test: $(CORE) src/entrada.c src/entrada.h tests/entrada_test.c
	$(CC) $(CFLAGS) $(CORE) src/entrada.c tests/entrada_test.c -o $@ -lm

test-entrada: entrada_test
	./entrada_test

# Nenhum número mágico nas regras (src/core.c, src/robo.c e src/entrada.c): os números têm nome no ajuste.h ou no roster.c.
numeros:
	sh tests/numeros.sh

# Nenhum aviso do gcc nem do clang (-Wall -Wextra -Werror em -O1, -O2 e -O3).
avisos:
	RAYLIB_CFLAGS="$(RAYLIB_CFLAGS)" sh tests/avisos.sh

test: core_test test-fonte test-save test-entrada
	./core_test

# O jogo de verdade sob xvfb (precisa do ./apara, do xvfb-run e da libX11; sem eles cada script pula):
# as teclas de teste só existem com --teste e nada grava o progresso (teste_save), o rastro é só visual
# (teste_rastro), a vitória é salva no golpe final e sobrevive a kill, ESC e Q e fechar (teste_vitoria), e o
# carimbo do clique chega ao duelo no instante certo (teste_carimbo, com libXtst).
teste-jogo: apara
	sh tests/teste_save.sh
	sh tests/teste_rastro.sh
	sh tests/teste_vitoria.sh
	sh tests/teste_carimbo.sh

# Tudo, com um comando: núcleo, fonte, save, fuzz, taxas de quadros, números, avisos e o jogo real.
teste:
	$(MAKE) test
	$(MAKE) fuzz-rapido
	$(MAKE) robos-taxas
	$(MAKE) numeros
	$(MAKE) avisos
	$(MAKE) teste-jogo
	@echo "teste: tudo certo"

run: apara
	./apara

clean:
	rm -f apara core_test fonte_test save_test entrada_test fuzz_test robos_relatorio personagens

.PHONY: all packs sprites run clean robos robos-taxas fuzz fuzz-rapido numeros avisos test test-fonte test-save test-entrada teste teste-jogo

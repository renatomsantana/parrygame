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
GAME := src/main.c src/fonte.c src/salvar.c src/entrada.c src/desempenho.c src/rig.c src/sprites.c src/pixelize.c src/katana3d.c src/arenas.c src/audio.c src/fx.c src/lore.c

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
	$(CC) $(CFLAGS) $(CORE) tests/robos.c -o $@ -lm -lpthread

HZ ?= 60
QUADROS ?= 0
robos: robos_relatorio
	./robos_relatorio $(LUTAS) $(HZ) $(QUADROS)

# A curva de dificuldade por mestre com os cinco robôs (primeira vez, casual que decora, reação 250, perfeito, spam),
# a 60 e a 144 Hz (make curva LUTAS=10000). Só mede; o perfeito vence tudo e o spam perde de tudo.
CURVA_LUTAS ?= 10000
curva_relatorio: $(CORE) tests/curva.c src/core.h src/ajuste.h src/robo.h
	$(CC) $(CFLAGS) $(CORE) tests/curva.c -o $@ -lm -lpthread

curva: curva_relatorio
	./curva_relatorio $(CURVA_LUTAS) $(QUADROS)

# A mesma medição, reprovando se algo cair fora da curva alvo de docs/CURVA.md (faixa por mestre e robô, ordem e degrau do casual).
# No make test, 10 mil lutas por mestre; no make teste, 100 mil (make curva-alvo CURVA_LUTAS=100000).
curva-alvo: curva_relatorio
	./curva_relatorio --alvo $(CURVA_LUTAS)

# O robô da primeira vez só reage (sem variação é o robo_reacao), não muda com a taxa, e o perfeito e o spam valem sempre.
test-curva: curva_relatorio
	./curva_relatorio --teste

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

# A medida de quadros do APARA_PERF (média, mediana, p95, p99, máximo).
desempenho_test: src/desempenho.c src/desempenho.h tests/desempenho_test.c
	$(CC) $(CFLAGS) src/desempenho.c tests/desempenho_test.c -o $@ -lm

test-desempenho: desempenho_test
	./desempenho_test

# A curva do casual tem de cair do karasu ao oboro, com folga de 0,5 ponto (100 mil lutas por mestre, todos os núcleos).
ORDEM_LUTAS ?= 100000
curva-ordem: robos_relatorio
	./robos_relatorio --ordem $(ORDEM_LUTAS)

# Nenhum número mágico nas regras (src/core.c, src/robo.c e src/entrada.c): os números têm nome no ajuste.h ou no roster.c.
numeros:
	sh tests/numeros.sh

# Nenhum aviso do gcc nem do clang (-Wall -Wextra -Werror em -O1, -O2 e -O3).
avisos:
	RAYLIB_CFLAGS="$(RAYLIB_CFLAGS)" sh tests/avisos.sh

test: core_test test-fonte test-save test-entrada test-desempenho test-curva curva-alvo
	./core_test

# Regressões de apresentação: seleciona sprites e acompanha armas sem abrir janela.
presentation_test: $(CORE) $(GAME) $(ENTRADA_PLAT) tests/presentation_test.c src/*.h
	$(CC) $(CFLAGS) $(RAYLIB_CFLAGS) $(ENTRADA_CFLAGS) $(CORE) $(filter-out src/main.c,$(GAME)) $(ENTRADA_PLAT) tests/presentation_test.c -o $@ $(RAYLIB_LIBS) $(ENTRADA_LIBS) -lm

test-visual: presentation_test
	./presentation_test

test-assets: presentation_test
	./presentation_test --assets

# O jogo de verdade sob xvfb (precisa do ./apara, do xvfb-run e da libX11; sem eles cada script pula):
# as teclas de teste só existem com --teste e nada grava o progresso (teste_save), o rastro é só visual
# (teste_rastro), a vitória é salva no golpe final e sobrevive a kill, ESC e Q e fechar (teste_vitoria), e o
# carimbo do clique chega ao duelo no instante certo (teste_carimbo, com libXtst), e o APARA_PERF mede e sai
# (teste_desempenho), e os tempos parados das cenas do desarme e do selo do oboro (teste_cenas).
teste-jogo: apara
	sh tests/teste_save.sh
	sh tests/teste_rastro.sh
	sh tests/teste_vitoria.sh
	sh tests/teste_carimbo.sh
	sh tests/teste_desempenho.sh
	sh tests/teste_cenas.sh

# Tudo, com um comando: núcleo, fonte, save, fuzz, taxas de quadros, a ordem da curva, números, avisos e o jogo real.
teste:
	$(MAKE) test
	$(MAKE) fuzz-rapido
	$(MAKE) robos-taxas
	$(MAKE) curva-ordem
	$(MAKE) curva-alvo CURVA_LUTAS=100000
	$(MAKE) numeros
	$(MAKE) avisos
	$(MAKE) teste-jogo
	$(MAKE) test-visual
	@echo "teste: tudo certo"

run: apara
	./apara

clean:
	rm -f apara core_test fonte_test save_test entrada_test desempenho_test fuzz_test presentation_test robos_relatorio curva_relatorio personagens

teste-vitoria: apara
	sh tests/teste_vitoria.sh

.PHONY: all packs sprites run clean robos robos-taxas curva curva-alvo test-curva curva-ordem fuzz fuzz-rapido numeros avisos test test-fonte test-save test-entrada test-desempenho test-visual test-assets teste teste-jogo teste-vitoria

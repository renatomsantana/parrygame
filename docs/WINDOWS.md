# O jogo no Windows

O Windows é a plataforma padrão. Aqui está o que já foi conferido, o que falta e como compilar. Tudo foi conferido num Linux com
MinGW-w64 (o compilador de Windows) e Wine (que roda os `.exe`): **não houve Windows de verdade**, e o que depende dele está marcado.

## Estado

| O quê | Estado |
|---|---|
| Núcleo (`core`, `roster`, `ajuste`, `robo`, `entrada`, `salvar`, `fonte`, `desempenho`) | compila para Windows com `-Wall -Wextra -Werror`, sem mudar nada |
| Testes (`core_test`, `entrada_test`, `save_test`, `fonte_test`, `desempenho_test`, fuzz, `robos`, `curva`) | compilam e passam como programas Windows no Wine, com os **mesmos números do Linux**: 12066 verificações, 20280 golpes com janela viável, 7775 cliques no hitstop; a tabela da curva sai idêntica, número por número (`make teste-windows`) |
| Carimbo do clique (`src/entrada_win.c`) | Raw Input numa thread própria. No Wine, com cliques e Espaço injetados pelo XTest: 12 de 12 cliques, 12 de 12 Espaços e 10 de 10 Espaços repetidos viram 12, 12 e 10 carimbos, com intervalo igual ao injetado (mediana 0,3 a 0,7 ms). No jogo inteiro (`apara.exe`), 30 cliques deram 30 carimbos. **Não conferido em Windows de verdade**: lá, o teste é `apara --carimbo` (o atraso do poll em relação ao clique tem de dar uns milissegundos, e não 0) |
| O jogo inteiro (`apara.exe`) | compila e liga sem nenhum aviso (MinGW) com os arquivos da junção com a `mac-integracao`, e abre e luta no Wine (assets, duelo, relatório `APARA_PERF`: 195 quadros, lógica de no máximo 0,06 ms). As duas correções que ele pedia (abaixo) já estão no `main.c` e no `katana3d.c` |
| Áudio, vsync, 144 Hz, gamepad, instalador | não conferidos (o Wine do teste não tinha placa de som nem GPU) |

## Por que o carimbo importa tanto

Sem carimbo (o `entrada_stub.c`, que era o que o Windows usava até agora), o clique entra no meio do quadro: ±½ quadro de erro, 8 ms a
60 Hz, quando a janela perfeita dos mestres finais tem 43 ms. A curva de dificuldade de `docs/CURVA.md` foi medida com o clique em ms
exato; sem o carimbo, a 60 Hz, o casual fica uns 6 pontos mais difícil (Oboro 34,6% em vez de 41,0%) e várias células saem da faixa alvo.
A 144 Hz a diferença é de menos de 1 ponto. Com o `entrada_win.c` o Windows volta a ter o carimbo, e qualquer falha dele (sem a thread,
sem a janela, sem o Raw Input) cai de volta no meio do quadro, como antes.

## As duas correções que o jogo inteiro pedia (já aplicadas)

Dois arquivos do jogo não compilavam para Windows. Estavam num patch porque são arquivos em que a sessão do Mac mexe; depois da junção com a `mac-integracao` entraram no `main.c` e no `katana3d.c`:

1. **`main.c`**: usava `sys/resource.h` e `getrusage` no relatório do `APARA_PERF`. Usa `perf_cpu_do_processo()` (em `desempenho.c`: `getrusage` no POSIX, `GetProcessTimes` no Windows).
2. **`katana3d.c`**: o `<GL/gl.h>` do Windows puxa o `windows.h`, que choca com os nomes do raylib (`Rectangle`, `CloseWindow`, `ShowCursor`). No Windows declara só o `glClear` e o `GL_DEPTH_BUFFER_BIT` que o arquivo usa (a `opengl32.dll` exporta `glClear`).

**Regra daqui para frente: o `windows.h` só entra em arquivos que não incluem o `raylib.h`** (hoje: `entrada_win.c` e `desempenho.c`); o resto do jogo pede ao sistema por uma função desses arquivos.

## Compilar no Windows

Com o MSYS2 (terminal MINGW64):

```
pacman -S make mingw-w64-x86_64-gcc mingw-w64-x86_64-raylib mingw-w64-x86_64-pkgconf
cd c_game
make            # o Makefile vê OS=Windows_NT: usa src/entrada_win.c e liga -lopengl32 -lgdi32 -lwinmm
make test       # núcleo, fonte, save, entrada, desempenho, curva (o make test-curva usa pthreads: o MinGW traz)
./apara.exe --carimbo
```

(O `make` do MSYS2 não foi rodado aqui; o que foi rodado é a linha do compilador abaixo, que é o que o Makefile monta.)

Cruzado, de um Linux (como foi conferido aqui): `apt install mingw-w64 wine64`; o raylib 6 compilado com
`make PLATFORM=PLATFORM_DESKTOP OS=Windows_NT CC=x86_64-w64-mingw32-gcc AR=x86_64-w64-mingw32-ar RAYLIB_LIBTYPE=STATIC`
(em `raylib/src`) e depois

```
x86_64-w64-mingw32-gcc -std=c11 -O2 -Wall -Wextra -Wno-missing-field-initializers -I<raylib>/src \
  src/ajuste.c src/core.c src/roster.c src/robo.c src/main.c src/fonte.c src/salvar.c src/entrada.c src/desempenho.c \
  src/rig.c src/sprites.c src/pixelize.c src/katana3d.c src/arenas.c src/audio.c src/fx.c src/lore.c src/entrada_win.c \
  -o apara.exe -L<raylib-lib> -lraylib -lopengl32 -lgdi32 -lwinmm -luser32 -lm -static
```

O `main.c` compila sem nenhum aviso para Windows.

## Os testes

- `make teste-windows` (e dentro do `make teste`): `tests/teste_windows.sh` compila 8 programas e o `entrada_win.c` para Windows com
  `-Werror` e, havendo o Wine, roda todos e compara a tabela da curva com a do Linux; `tests/teste_carimbo_windows.sh` roda o carimbo de ponta
  a ponta no Wine sob o xvfb. Sem o MinGW, ou sem o Wine, pulam o que precisam. Eles pegam: o que não compila para Windows, e qualquer diferença
  entre o núcleo do Windows e o do Linux.
- O que eles **não** pegam: a latência absoluta do carimbo (os relógios do Wine e do injetor têm origens diferentes: só os intervalos são
  conferidos) e o caminho carimbo → duelo no `apara.exe` (sem gerenciador de janelas o Wine não dá o foco, e o jogo pausa o duelo sem foco).
  Os dois se conferem na máquina de verdade, com `apara --carimbo` e jogando.

## Outras coisas do Windows que não foram olhadas

- As ferramentas `tools/gravar_video.sh` e os testes de jogo (`teste_*.sh`, xvfb, XTest) são do Linux.
- A resolução do relógio do Windows (15,6 ms por padrão) não afeta o núcleo (ele conta em ms pelo carimbo, em `QueryPerformanceCounter`), mas o
  ritmo dos quadros depende do vsync do driver: confira com `APARA_PERF=10 apara.exe --demo --master 1 --duel`.
- O `salvar.c` já tem os caminhos de `_WIN32` (gravação atômica); o `save_test` passa no Windows (81 verificações).

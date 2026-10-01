# Fluidez da luta

Só a luta: sem cenas, cutscenes nem mensagens. Quatro frentes, e o que já está feito, o que é patch e o que espera a sua decisão.

| Frente | Estado |
|---|---|
| Travadas e FPS | **causa achada e medida**; correção pronta em patch (`docs/fluidez/pre_carga_efeitos.patch`), com teste |
| Ritmo do duelo | ferramenta e teste no repositório (`make ritmo`, `make test-ritmo`); duas mudanças propostas, **nenhuma aplicada** |
| Resposta do aperto | o julgamento já é exato (carimbo); o que sobra é visual e está no `main.c`: análise abaixo |
| Fluxo visual dos golpes | é o item 2 (golpes novos por arma): espera a branch da outra sessão |

## 1. Travadas: o jogo carrega cada folha de efeito na primeira vez que a usa

**O que acontece.** `spr_fx()` (`sprites.c`) carrega o PNG do efeito (decodifica e sobe a textura) **na primeira vez que o duelo pede**. Os pedidos
caem nos dois instantes que mais importam: o primeiro aviso de cada mestre (`tell_fx`) e o primeiro parry perfeito, bom ou erro (`on_impact`).

**Como medi.** Cronometrei por dentro do `handle_events()` numa cópia do jogo (Linux, demo, `APARA_PERF`). Sob o Xvfb a placa de vídeo é por software,
então só a seção **lógica** vale (mundo e composição não); a lógica de um quadro normal leva 0,01 ms.

| Quando | Folha | Custo no quadro |
|---|---|---|
| 1º aviso do daichi | `70` | 3,9 ms |
| 1º parry perfeito (todos os mestres) | `652` | 6,0 a 7,6 ms |
| 1º aviso do oboro | `197` | 10,4 ms |
| onda de choque do oboro | `184` | 5,6 ms |

O orçamento de um quadro é 16,7 ms a 60 Hz e 6,9 ms a 144 Hz. Não medi com placa de vídeo de verdade, mas 4 a 10 ms de CPU a mais no meio de um quadro arriscam
perdê-lo a 60 Hz e perdem a 144 Hz, justamente no primeiro aviso e no primeiro parry perfeito.

**A correção** (`docs/fluidez/pre_carga_efeitos.patch`, aplica limpo no `HEAD`): `precarrega_efeitos()` chama `spr_fx()` para as 15 folhas na abertura,
depois de `spr_init()`. Custa **85 ms na abertura** (a carga até o primeiro quadro vai de 173 para 258 ms) e tira os travamentos:

| | lógica máxima numa luta de 8 s |
|---|---|
| antes | daichi 9,3 ms, oboro 9,5 ms (karasu 11,1 ms) |
| depois | daichi 0,05 ms, oboro 0,06 ms |

O patch traz também o teste (`tests/teste_desempenho.sh`): a lógica de um quadro nunca passa de 3 ms na luta (daichi e oboro, até 3 tentativas cada, para a
máquina ocupada não reprovar). Conferido nos dois sentidos: **reprova no `HEAD`** (9,3 e 9,5 ms) e **passa com o patch** (0,05 e 0,06 ms). A lista de 15 folhas
acompanha os `vfx("...")` e a tabela `TELL` do `main.c`: ao criar um efeito novo, inclua a folha na lista, senão o teste acusa.

**Atenção: a sessão do Mac tem o commit "Carrega efeitos e ícones antes da luta" (`01eeb13`), que provavelmente faz o mesmo.** Se for o caso, não
aplique a pré-carga: aplique só o teste do patch, que é o que garante que a travada não volta.

**No Windows**, para medir de verdade (com a placa de vídeo real): `set APARA_PERF=20` e `apara.exe --demo --master 6 --duel`; o relatório vem
no terminal. Olhe `PERF lógica` (o máximo tem de ficar abaixo de 3 ms) e os `PERF pico`.

## 2. Ritmo do duelo

`make ritmo` mede, em **tempo real** (núcleo mais o que o hitstop congela) e com o robô perfeito:

| # | Mestre | luta (s) | golpes/min | abertura (s) | contato→aviso (s) mediana / p95 | aviso→contato (s) mín / mediana | na sequência (s) mín / mediana | congelado |
|---|---|---|---|---|---|---|---|---|
| 1 | daichi | 21,0 | 42,9 | 1,20 | 0,99 / 1,14 | 0,45 / 0,45 | 1,00 / 1,00 | 6,8% |
| 2 | genbu | 16,0 | 56,2 | 1,00 | 0,81 / 0,90 | 0,44 / 0,44 | 0,42 / 0,60 | 8,9% |
| 3 | raizo | 17,7 | 50,8 | 1,01 | 0,84 / 0,88 | 0,42 / 0,42 | 0,70 / 0,85 | 8,0% |
| 4 | shizuku | 12,7 | 70,8 | 0,86 | 0,69 / 0,74 | 0,41 / 0,41 | 0,40 / 0,40 | 11,2% |
| 5 | garfiel | 12,8 | 89,3 | 0,86 | 0,69 / 0,74 | 0,40 / 0,40 | 0,40 / 0,40 | 13,9% |
| 6 | karasu | 15,6 | 65,2 | 0,93 | 0,71 / 0,86 | 0,38 / 0,38 | 0,45 / 0,50 | 10,2% |
| 7 | hayate | 16,5 | 72,8 | 0,87 | 0,70 / 0,84 | 0,37 / 0,37 | 0,40 / 0,45 | 11,3% |
| 8 | enjin | 15,3 | 82,3 | 0,83 | 0,66 / 0,71 | 0,36 / 0,36 | 0,40 / 0,45 | 12,8% |
| 9 | suiren | 21,2 | 67,9 | 1,05 | 0,75 / 0,88 | 0,35 / 0,35 | 0,45 / 0,50 | 10,5% |
| 10 | arashi | 11,5 | 78,0 | 0,89 | 0,72 / 0,77 | 0,33 / 0,33 | 0,40 / 0,40 | 12,3% |
| 11 | yoru | 21,0 | 71,3 | 0,94 | 0,75 / 0,83 | 0,32 / 0,32 | 0,40 / 0,45 | 11,0% |
| 12 | jinshi | 13,0 | 69,5 | 1,00 | 0,81 / 1,00 | 0,35 / 0,35 | 0,40 / 0,50 | 11,0% |
| 13 | oboro | 48,8 | 75,1 | 1,03 | 0,82 / 2,96 | 0,32 / 0,35 | 0,40 / 0,45 | 11,7% |

Leitura:

- **A abertura** (do começo da luta ao primeiro aviso) é de 0,83 a 1,20 s, e **o tempo sem nada a reagir** (do contato ao aviso seguinte) de 0,66 a 0,99 s (mediana).
  O daichi é o mais lento: 43 golpes por minuto, abertura de 1,20 s, 1,00 s entre os dois golpes da sequência dele (os outros ficam em 0,40 a 0,85 s).
- **O aviso** (do aviso ao contato do primeiro golpe) fica em 0,32 a 0,45 s: nunca abaixo dos 300 ms da regra (o piso é 320).
- **Os golpes de uma sequência** nunca ficam a menos de 0,40 s um do outro (`AJ_CADEIA_MIN`).
- **O hitstop congela de 7% (daichi) a 14% (garfiel) da luta.** Nos mestres rápidos, cada golpe de uma sequência de 0,40 s tem 0,09 s de congelamento dentro:
  o corpo para um quinto do tempo da sequência.

**Duas mudanças propostas (nenhuma aplicada; preciso do seu OK):**

| | O que muda | Efeito medido |
|---|---|---|
| A. Daichi mais ágil | só a espera dele antes do aviso (`waitScale`, `roster.c`): ×0,50 → ×0,30 | 42,9 → 48,7 golpes/min; abertura 1,20 → 0,98 s; contato→aviso 1,01 → 0,80 s. Não mexe no aviso nem nas janelas |
| B. Hitstop menor dentro da sequência | ×0,6 nos golpes que ainda têm outro depois (o último da sequência e a quebra de postura ficam inteiros); `duel_hitstop_for` ganha o argumento "em sequência" | tempo congelado: garfiel 14,0 → 10,4%, enjin 12,8 → 10,0%, jinshi 11,0 → 8,5%, oboro 11,7 → 8,6%; daichi 6,8 → 6,3% (quase não muda) |

Nenhuma das duas toca o aviso, as janelas perfeita e boa nem a menor partida da lâmina. A B mexe no núcleo (e precisa do teste de hitstop em tempo real ajustado),
e as duas mudam um pouco a curva do casual (o intervalo medido por ele encurta): se aprovadas, passo o `make curva-alvo` e reajusto o que sair da faixa.

**O que o teste garante agora** (`make test-ritmo`, dentro do `make test`): o aviso do primeiro golpe nunca baixa de 300 ms; dois contatos de uma sequência nunca
ficam mais perto que `AJ_CADEIA_MIN`; nunca há mais de 3 s sem aviso (fora a pausa do selo do oboro); o hitstop nunca passa de 20% de uma luta. Conferido por
três mutações (piso do aviso mais alto, `AJ_CADEIA_MIN` mais alto, hitstop de 0,9 s).

## 3. Resposta do aperto

O que já é exato: o clique é carimbado pelo sistema (Linux, macOS e agora Windows) e o núcleo o julga no instante dele, em qualquer taxa de quadros; o hitstop é gasto em
tempo real. Ver `docs/CURVA.md` e `docs/WINDOWS.md`.

O que sobra é **visual**, e fica no `main.c` (que não toquei): o jogo só **lê** o clique no começo do quadro seguinte (até 1 quadro depois: 16,7 ms a 60 Hz, 6,9 ms
a 144 Hz); a pose de parry é posta no mesmo quadro (`EV_PRESS` → `rig_pose(..., 0.06f)` e `sprite_press()`), mas só aparece no vblank seguinte (até mais 1 quadro). No pior
caso são 2 quadros entre o clique e o parry na tela: 33 ms a 60 Hz, 14 ms a 144 Hz. As saídas, que não mexem no julgamento:

1. monitor de 144 Hz ou mais (corta o pior caso para menos da metade), e, no Windows, janela sem borda ou tela cheia (o compositor do sistema soma um quadro);
2. a pose de parry chega em 60 ms (`rig_pose(r, parry_pose, 0.06f)`): é a transição visual, não o julgamento. Encurtar para 30 a 40 ms faria o parry "estalar" mais
   cedo. É uma decisão de arte (e fica no `main.c`), por isso só proponho.

Não dá para medir isso aqui (sem tela de verdade). Num Windows com câmera de celular a 240 fps, ou com o `apara --carimbo` (que mostra o atraso do poll), dá.

## 4. Fluxo visual dos golpes

É o item 2 do plano (golpes novos por arma, o campo visual do golpe e o rastro): depende da branch da outra sessão e continua parado. Nada foi tocado.

## 5. O que preciso de você

1. A pré-carga de efeitos: aplico o patch inteiro, ou só o teste (se o `01eeb13` do Mac já cobre)? Em ambos os casos o `main.c` é da outra sessão: o patch segue como arquivo.
2. Ritmo: aprova A (Daichi), B (hitstop em sequência), as duas ou nenhuma?
3. Pose de parry: quer que eu proponha 30 a 40 ms (patch no `main.c`)?

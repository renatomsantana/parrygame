# Áudio: pedidos para o sound designer

A planilha completa é [`AUDIO_PEDIDOS.csv`](AUDIO_PEDIDOS.csv) (separador `;`, abre no Excel, Google Planilhas e LibreOffice). Cada linha é um arquivo: o momento do jogo, a duração atual, o limite, as variações, o volume e o pitch que o código aplica, a prioridade e a direção sonora.

Hoje todos os sons são **sintetizados pelo próprio jogo** (placeholders). Eles servem de referência de duração e de momento, não de identidade final. Os mesmos sons, em WAV 16 bits, 44,1 kHz, mono, estão em `c_game/assets/audio/referencia_atual/` (pasta que o Git ignora): `cue.wav`, `perfect.wav`, `good.wav` e assim por diante. Para gerar de novo: `APARA_SFX=pasta ./apara --shot x.png 0.3`, que grava `sfx_00.wav` a `sfx_18.wav` na ordem da tabela (`cue`, `perfect`, `good`, `bad`, `break`, `swing`, `gesture`, `ui`, `type`, `gem`, `seal`, `drum`, `thunder`, `victory`, `defeat`, `thud`, `clap`, `koiguchi`, `saque`).

## O que o jogo tem (e não tem)

É um duelo de parry de um botão. **Não há** passos, pulo, esquiva nem barra de vida com alarme: não peça esses sons. O som existe para três coisas: **avisar** (`cue`), **responder ao aperto** (`gesture`) e **dar o resultado** (`perfect`, `good`, `bad`). O resto é cena, menu e texto.

## Prioridade

| | O que | Por quê |
|---|---|---|
| **P1** | `cue`, `perfect`, `good`, `bad`, `swing`, `gesture` | É o que o jogador ouve centenas de vezes e o que dá a sensação de timing. Os três resultados e o aviso precisam ser inconfundíveis entre si. |
| **P2** | `break`, `seal`, `koiguchi`, `saque`, `victory`, `defeat`, `ui`, música do título e da cabana do Hanzo | Momentos marcantes, mas raros. |
| **P3** | `type`, `gem`, `thunder`, `drum`, `thud`, `clap`, `cue`/`gesture`/`swing` por mestre, uma música por duelo | Acabamento. Sem eles o jogo usa o som global ou o embutido. |

Um começo barato: **P1 com 2 ou 3 variações cada** (`nome_01.wav` a `nome_03.wav`), feitas uma vez só em 3 famílias (leve, pesada, especial) para `swing`.

## Regras técnicas

- WAV PCM, 16 ou 24 bits, 44,1 ou 48 kHz. Música: OGG Vorbis ou WAV, estéreo, pronta para loop.
- **Sem silêncio no começo** de `cue` e dos impactos: o jogo alinha o pico do `swing` ao contato e dispara o resto no instante do evento.
- `perfect`, `good`, `bad` e `swing`: **no máximo 1,6 s, já com a cauda** (reverb dentro do arquivo). Arquivos maiores são recusados com aviso no terminal e o jogo mantém o som embutido.
- Até 4 variações por som (`nome_01.wav` a `nome_04.wav`), tocadas em rodízio. Elas têm prioridade sobre `nome.wav`.
- O código aplica uma variação de pitch pequena em alguns sons (coluna `volume_e_pitch_no_codigo`; por exemplo `perfect` de 1,00 a 1,08). Entregue o arquivo no tom base que quer ouvir.
- Por mestre: `sfx/<mestre>/cue.wav`, `gesture.wav` e `swing.wav`. Mestres: daichi, genbu, raizo, shizuku, garfiel, karasu, hayate, enjin, suiren, arashi, yoru, jinshi, oboro. Jinshi não tem som de aviso. O Oboro usa o banco do aprendiz cuja postura copia; a pasta `oboro/` é só para a postura própria dele.

## Como testar o que chegar

1. Soltar os arquivos em `c_game/assets/audio/` (ou apontar outra pasta com `APARA_AUDIO_DIR=/pasta`).
2. `APARA_LOG_AUDIO=1 ./apara` mostra os arquivos aceitos, os recusados e as variações tocadas.
3. Calibrar o atraso de áudio do aparelho no menu (tecla L) antes de avaliar o timing.
4. `make test-audio` confere os arquivos e a duração.

## Licenças

Anote em `c_game/tools/pacote/CREDITOS.txt` a fonte e a licença de cada arquivo que não for seu. CC0 não exige nada; CC-BY exige crédito; evite licenças "não comercial" se o jogo for vendido. O áudio fica fora do Git até a decisão do pacote de distribuição (`assets/audio/` está no `.gitignore`).

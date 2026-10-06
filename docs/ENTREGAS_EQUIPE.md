# Entregas da equipe

Arquivos opcionais: sem uma entrega válida, o jogo usa os cenários e sons embutidos. Reinicie após trocar os arquivos.

## Primeiro fundo — Daichi

Celeiro rural japonês ao entardecer: céu quente, montanhas distantes, madeira e cerca; vegetação com movimento suave. Faixa central livre para ler as armas e o parry. Sem personagens pintados no fundo.

- Base: `c_game/assets/arenas/daichi/back.png`, **320 × 180**, opaco, chão em **y=150**.
- Frente opcional: `front.png`, mesma dimensão, com transparência; evitar cobrir personagens entre x=100 e x=230.
- Animação opcional: tira horizontal, quadros de 320 × 180, até 32 quadros, **8 FPS em loop**. Quatro quadros → 1280 × 180. Pode entregar a base estática primeiro.
- Movimento leve: folhas, vegetação e luz distante. Não deslocar o chão.
- As cores do PNG são preservadas. Sem `front.png`, continuam as partículas procedurais.

Outras pastas: `genbu`, `raizo`, `shizuku`, `garfiel`, `karasu`, `hayate`, `enjin`, `suiren`, `arashi`, `yoru`, `jinshi` e `oboro`. A cabana de Hanzo continua procedural. Pasta externa: `APARA_ARENA_DIR=/pasta ./apara`.

## Efeitos sonoros

WAV PCM, 16 ou 24 bits, 44,1 ou 48 kHz. Sem silêncio no início do aviso ou impacto. A cauda/reverb vem no arquivo. Preservar diferença entre bom, perfeito e erro.

Pasta: `c_game/assets/audio/sfx/`. Um efeito aceita `nome.wav` ou até quatro variações `nome_01.wav` a `nome_04.wav`, em rodízio. As versões numeradas válidas têm prioridade.

| Nome | Momento |
|---|---|
| `cue` | Aviso antes do contato |
| `perfect`, `good`, `bad` | Perfeito, bom, dano recebido |
| `break`, `seal` | Quebra de postura e selo |
| `swing`, `gesture` | Corte e gesto |
| `koiguchi`, `saque` | Soltar a bainha e desembainhar |
| `thunder`, `drum`, `thud`, `clap` | Raio, tambor, queda e palmas |
| `ui`, `type`, `gem` | Menu, texto e progressão |
| `victory`, `defeat` | Resultado do duelo |

**Perfect, good, bad e swing: máximo de 1,6 s**, incluindo cauda. O banco existente de vozes comporta sequências e duplos dentro desse limite. Arquivos maiores ou inválidos são recusados com aviso no terminal, preservando o som embutido. Os demais efeitos não têm esse limite.

## Composição

Pasta: `c_game/assets/audio/music/`. OGG Vorbis ou WAV, estéreo, pronto para loop. Nomes: `daichi`, `genbu`, `raizo`, `shizuku`, `garfiel`, `karasu`, `hayate`, `enjin`, `suiren`, `arashi`, `yoru`, `jinshi`, `oboro`, `hanzo`, `title`.

Há fade de entrada/saída e redução durante falas e impactos. A escolha final deixa só o vento embutido. Oboro recebe uma faixa única: stems por fase ainda não fazem parte da integração. Pasta externa: `APARA_AUDIO_DIR=/pasta ./apara`. `APARA_LOG_AUDIO=1` mostra arquivos aceitos e variações tocadas.

## Passar o projeto para outra máquina

### Pack de slash

O pack comprado `vfx_slash` entra sem modificar o PNG original: copiar
`sprite_sheets_96x96/vfx_slash-Sheet.png` para
`c_game/assets/sprites/_fx/slash.png`. São células de 96 × 96 numa folha de
864 × 1152. O arquivo permanece privado, fora do Git; `make pacote-assets` o inclui.

Cortes limpos entram nas katanas, odachi e foices; garras no Garfiel, raios no
Arashi e fogo no Enjin/Oboro em fúria. Estocadas e o apagão do Yoru conservam sua
leitura. O arco principal coincide com o contato, congela no hitstop e a cauda
dura no máximo 120 ms do relógio do duelo. Cada arma usa seu próprio ponto no PNG.
`APARA_SLASH=0 ./apara` compara com o fio anterior. Sem o arquivo válido, esse fio
continua disponível. Não altera dano, parry, hitboxes ou cadência dos sprites.

O Git não contém os PNGs pagos nem as novas entregas. Na máquina que tem os arquivos:

```sh
cd c_game
make assets-prontos
make pacote-assets OUT=/caminho/assets-runtime.zip
```

Descompactar o ZIP na pasta `c_game` do clone. O pacote inclui assets de execução, sem pranchas-fonte dos packs, saves ou opções. Compilar o binário na plataforma de destino. A validação completa dos PNGs é `make test-assets`.

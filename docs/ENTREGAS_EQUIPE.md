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
| `swing`, `gesture` | Corte do adversário e gesto de Kojiro |
| `koiguchi`, `saque` | Soltar a bainha e desembainhar |
| `thunder`, `drum`, `thud`, `clap` | Raio, tambor, queda e palmas |
| `ui`, `type`, `gem` | Menu, texto e progressão |
| `victory`, `defeat` | Resultado do duelo |

**Perfect, good, bad e swing: máximo de 1,6 s**, incluindo cauda. O banco existente de vozes comporta sequências e duplos dentro desse limite. Arquivos maiores ou inválidos são recusados com aviso no terminal, preservando o som embutido. Os demais efeitos não têm esse limite.

### Assinatura sonora de cada postura

Entregar `sfx/<mestre>/cue.wav`, `gesture.wav` e `swing.wav` (ou `_01.wav` até `_04.wav`). Exemplo: `sfx/arashi/swing_01.wav`. Oboro consulta a pasta do aprendiz cuja postura está copiando; o mesmo banco e suas vozes são compartilhados. Não entregar uma cópia dos mesmos sons em `oboro/`. Esta pasta serve para a postura própria do Oboro.

Sem arquivo válido, o jogo retorna ao som global/embutido; o gesto usa o mesmo tom do aprendiz. WAVs específicos preservam seu timbre. `swing` mantém o limite de 1,6 s e quatro vozes por variante; arquivos inválidos são recusados. Jinshi continua sem som de aviso. `APARA_LOG_POSTURAS=1` registra a origem escolhida em cada evento.

O aviso toca somente no evento de aviso. O corte é agendado pela energia máxima em blocos de 5 ms de cada variação WAV: o pico é alinhado ao contato sem acelerar ou desacelerar o arquivo. Evitar silêncio inicial longo e ataques de volume muito tardios; a antecedência máxima disponível é a viagem da lâmina. O gesto de Kojiro toca no aperto, e o resultado do parry toca no impacto. O movimento preparatório não deve adicionar outro corte.

## Composição

Pasta: `c_game/assets/audio/music/`. OGG Vorbis ou WAV, estéreo, pronto para loop. Nomes: `daichi`, `genbu`, `raizo`, `shizuku`, `garfiel`, `karasu`, `hayate`, `enjin`, `suiren`, `arashi`, `yoru`, `jinshi`, `oboro`, `hanzo`, `title`.

Há fade de entrada/saída e redução durante falas e impactos. A escolha final deixa só o vento embutido. Oboro recebe uma faixa única: stems por fase ainda não fazem parte da integração. Pasta externa: `APARA_AUDIO_DIR=/pasta ./apara`. `APARA_LOG_AUDIO=1` mostra arquivos aceitos e variações tocadas.

## Passar o projeto para outra máquina

### Pack de slash

O pack comprado `vfx_slash` entra sem modificar o PNG original: copiar
`sprite_sheets_96x96/vfx_slash-Sheet.png` para
`c_game/assets/sprites/_fx/slash.png`. São células de 96 × 96 numa folha de
864 × 1152. A folha usada pelo jogo está versionada; `make pacote-assets` também a inclui.

O pack substitui o rastro do Garfiel (garras laranja, linha 3), inclusive na katana do Oboro quando ele rouba essa postura. A recoloração ocorre no shader e preserva os highlights brancos; o PNG comprado permanece intacto. Arashi voltou ao formato do slash branco das poses originais do Samurai #5, recolorido em cinza de nuvem, com pequenos raios azul-elétrico saindo da borda. O Oboro usa a mesma paleta e os mesmos detalhes no slash original da própria katana. Os slashes completos das linhas 1/7/10 não são usados nas lutas.

O gerador produz 28 camadas `_clean_*.png` e seus pontos de arma `_clean_*.txt`, nas pastas `arashi`, `garfiel`, `oboro` e `oboro_mascara`. As camadas do Arashi ficaram disponíveis, mas o jogo desenha sua tira com o slash original cinza e raios azuis. Elas retiram apenas o efeito antigo usando as tags semânticas do gerador e preservam corpo e arma. Essas camadas de execução estão versionadas e também entram no pacote de assets. Quem já tem os packs-fonte pode gerar novamente com `make sprites`; quem recebe só o runtime precisa do pacote atualizado, incluindo as tiras atualizadas de Shizuku, Suiren, Arashi e os respectivos ecos do Oboro com e sem máscara. `make assets-prontos` confere as camadas quando o pack slash está instalado. `APARA_SLASH=0` permite comparar as garras com o rastro anterior.

O runtime também precisa das máscaras `_weapon_<animação>.png` e `_steel_<animação>.png` em cada pasta de adversário, além do `DESARMADO.png` e do `sprite.txt` atualizados. A primeira camada recorta a arma real para o desarme; a segunda mantém os efeitos no aço. O gerador produz essas máscaras com as mesmas células dos ataques. `make assets-prontos` exige as camadas dos ataques para impedir uma entrega incompleta.

O Git inclui os 17 conjuntos de personagens em uso, seus manifestos e máscaras, os quatro VFX ativos e os dois spritesheets de botões. Os packs-fonte, estudos, saídas `*_gerado` e entregas ainda não integradas continuam locais. Um clone já tem a arte necessária para compilar e jogar; o ZIP continua disponível para distribuir o runtime sem Git:

```sh
cd c_game
make assets-prontos
make pacote-assets OUT=/caminho/assets-runtime.zip
```

Descompactar o ZIP na pasta `c_game` do clone. O pacote inclui assets de execução, sem pranchas-fonte dos packs, saves ou opções. Compilar o binário na plataforma de destino. A validação completa dos PNGs é `make test-assets`.

O pacote para quem só quer jogar (executável + assets + leia-me + créditos + licenças, sem compilador) é `make pacote PLATAFORMA=windows EXE=apara.exe OUT=dist` (ver `c_game/LEIA-ME.md`). Ele leva sozinho o `assets/audio` e o `assets/arenas` que estiverem na pasta no momento de empacotar: **a música e os efeitos finais entram assim que os arquivos da equipe estiverem em `c_game/assets/audio`**, sem mudar código (`make test-audio` confere o formato).

## Cores fixas de gelo, mar e tempestade

As cores de efeitos ficam em `c_game/src/cores_posturas.h` e no quadro de `JOGO.md`. Shizuku usa branco/ciano claro, Suiren usa turquesa e Arashi usa cinza de nuvem com descarga azul-elétrico. PNGs de golpes, aura e partículas do aviso devem seguir a mesma família, incluindo as cópias de postura do Oboro. Esta revisão preserva as cores de roupa e pele dos personagens.

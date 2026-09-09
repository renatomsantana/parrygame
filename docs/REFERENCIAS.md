# Referências visuais

A direção do APARA é neo-noir, com *Katana Zero* como referência declarada.
Não existe um breakdown técnico oficial daquele jogo; o que existe é isto.

## Para estudar a arte

| Fonte | O que dá para tirar |
| --- | --- |
| [The Spriters Resource — Katana ZERO](https://www.spriters-resource.com/pc_computer/katanazero/) | Os sheets do jogo. É a fonte mais útil: dá para contar as cores por personagem, ver o tamanho real dos quadros e como as poses de golpe são desenhadas |
| [Concept art oficial da Askiisoft](https://blog.askiisoft.com/post/185309489923/the-concept-art-of-katana-zero) | Evolução das silhuetas ao longo dos sete anos de desenvolvimento. É retrospectiva de personagem, não técnica |
| [Entrevista de Justin Stander no MCV](https://mcvuk.com/business-news/askiisoft-katana-zero/) | O achado mais aproveitável: o neon "cobria todos os defeitos" e é o que faz arte de dezenas de artistas parecer uma coisa só |
| [Estudo de Artem Samoilov no ArtStation](https://www.artstation.com/artwork/qA5dKa) | Composição de fundo em camadas |
| [Cena animada na Workshop do Steam](https://steamcommunity.com/sharedfiles/filedetails/?id=2405240007) | Fundo em camadas, com movimento |
| [Thread no fórum do GameMaker sobre as luzes de rua](https://forum.gamemaker.io/index.php?threads/how-to-get-street-lights-effects-like-in-katana-zero.116367/) | Discussão do efeito em engine: superfície de luz, mistura aditiva e desfoque |

## Ouvir uma referência dentro do jogo

`art/audio/<nome>.mp3` (ou `.ogg`) substitui o som sintetizado do mesmo nome
enquanto o arquivo estiver na pasta: `parry`, `perfect`, `hit`, `hurt`,
`swing`, `cue`, `armor`, `feint`. Tirando o arquivo, o sintetizado volta.

Serve para ouvir a referência **no lugar dela**, com o hitstop e o tremor em
volta, que é a única forma de julgar se um som de impacto funciona. O que
aprender ali volta para os números em `feedback.gd`.

Vale o mesmo que para os sheets abaixo: som de jogo comercial é material de
estudo, não de publicação.

## O que já foi trazido para o projeto

O ponto da entrevista virou regra: o neon é a última etapa do shader e a única
que escapa da paleta fechada. Está descrito em `DESIGN.md`, seção
**O neon como camada**.

O que ainda separa o projeto da referência é a técnica: lá a arte é desenhada
quadro a quadro, com a luz pintada dentro do sprite; aqui é geometria iluminada
em tempo real. A junta para trocar uma coisa pela outra está descrita em
`DESIGN.md`, seção **Trocar os modelos por sprites desenhados**.

Os sheets acima são material de referência de terceiros. Servem para estudo de
proporção, contagem de cores e leitura de pose — não para uso no jogo. O mesmo
vale para qualquer áudio colocado em `art/audio/`.

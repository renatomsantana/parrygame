# Pasta de criação de arte

| Pasta | Destino |
| --- | --- |
| `models/samurai/` | Modelos de ronin, katana e peças de cenário samurai |
| `models/medieval/` | Cavaleiro, espada e peças de castelo |
| `textures/` | Texturas e mapas de materiais que criarmos |
| `audio/` | Sons produzidos, e sons de referência para comparar com os sintetizados |
| `concepts/` | Esboços e referências visuais do projeto |

Um arquivo `audio/<nome>.mp3` ou `.ogg` **substitui** o som sintetizado do
mesmo nome enquanto estiver ali — `parry`, `perfect`, `hit`, `hurt`, `swing`,
`cue`, `armor`, `feint`. Tirando o arquivo, o sintetizado volta sozinho.
Serve para ouvir uma referência no lugar dela ao ajustar a síntese.

Som de jogo comercial é material de estudo, não de publicação: usar para
comparar timbre e tempo é uma coisa, distribuir junto é outra. Por isso nada
do que `feedback.gd` gera depende de arquivo nenhum desta pasta.

Essas pastas são pontos de entrada para assets futuros; não contêm modelos
finais. As bases geométricas existentes estão em `scenes/themes/` e já podem
ser editadas diretamente no Godot. O guia `docs/DESIGN.md` explica escala,
materiais e conexões que os novos modelos devem preservar.

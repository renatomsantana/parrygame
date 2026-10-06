# Aparar — estado do jogo

Duelo de um botão, em C11 e raylib. Kojiro lê o aviso, apara e quebra a postura do adversário. Os doze aprendizes são desarmados; matar só é uma escolha no final de Oboro. A história vigente, com spoilers, está em `../aparar_lore.md`.

## Combate

O relógio do núcleo agenda preparação, aviso, partida da lâmina e contato. Um aperto após o aviso vale uma tentativa. Antes do aviso, o aperto cedo recebe recarga limitada para nunca bloquear a janela boa. Há parry perfeito, bom e erro; a tolerância tardia é de 30 ms e a calibração vai até 120 ms.

O perfeito causa mais dano de postura, cura uma porcentagem da vida e apaga a brasa de Enjin. O bom apara com recompensa menor. Nos golpes de duas lâminas, o perfeito segura as duas; o bom deixa entrar a segunda. Errar causa dano e a morte de Kojiro tem hitstop antes da queda. Não há ataque livre, dash controlável nem invencibilidade concedida ao jogador.

O aviso varia de 450 ms no Daichi a 320 ms nos últimos; Jinshi tem aviso visual de pelo menos 350 ms e nenhum som de aviso. Hayate e Jinshi conservam variação na preparação. A lâmina viaja em tempo variável do Garfiel em diante, ligável em `ajuste.h`; aviso e contato não se deslocam. Sequências têm piso de 400 ms entre contatos. Hitstop é descontado da preparação seguinte.

Oboro tem três selos: fundamentos de Hanzo, doze ecos e a forma Oni. Recupera a vida do jogador a cada selo; a segunda fase reduz o dano pela metade. A terceira usa os ataques de espada em chamas e preparação ×0,85, com aviso nunca inferior a 320 ms. O HUD não revela selos restantes.

## Lutadores

| Ordem | Nome | Arma | Postura |
|---|---|---|---|
| 1 | Daichi | Katana | Terra |
| 2 | Genbu | Katana | Tartaruga |
| 3 | Raizo | Odachi | Montanha / pedra |
| 4 | Shizuku | Florete | Gelo |
| 5 | Garfiel | Garras | Tigre |
| 6 | Karasu | Katana e wakizashi | Corvo |
| 7 | Hayate | Duas foices | Vento |
| 8 | Enjin | Katana em chamas | Chama |
| 9 | Suiren | Lança | Mar |
| 10 | Arashi | Duas katanas | Tempestade |
| 11 | Yoru | Duas adagas invertidas | Noite |
| 12 | Jinshi | Katana branca | Lua |
| 13 | Oboro | Katana / espada em chamas | Hanzo, doze ecos, Oni |

Todos os padrões e seus tempos estão em `c_game/src/roster.c`. Os objetivos da curva estão em `CURVA.md`; a análise de ritmo, em `FLUIDEZ.md`. As propostas de reduzir hitstop em sequência e acelerar Daichi continuam sem aplicação.

## Apresentação e arquivos

Resolução de arte: **320 × 180**, chão em y=150. Janela inicial: **1280 × 720**, filtro de ponto, alvo normal de 60 FPS. Personagens e retratos usam somente PNGs derivados dos packs Mattz Artz. O rig auxilia a coreografia e as coordenadas; não desenha um personagem substituto quando faltam os PNGs.

Os fundos procedurais continuam disponíveis. Os PNGs do artista substituem o fundo de cada arena, com sua paleta preservada e camadas animáveis. Os sons sintetizados continuam como fallback; WAVs e músicas da equipe entram sem alterar o núcleo. Formatos e nomes: `ENTREGAS_EQUIPE.md`.

Os cortes usam os rastros coloridos dos próprios PNGs, alinhados quadro a quadro à arma. A sobreposição do pack novo foi retirada; as cópias de corpo ficam desligadas por padrão (`APARA_RASTRO=1` serve para comparação). Oboro na fase 2 conserva suas animações de katana e usa as variantes `_ECO_<POSTURA>` inclusive nos contatos duplos, pesados e finais. Partículas, aura, aviso, gesto e som de corte consultam a mesma postura de origem do aprendiz. Jinshi conserva o aviso visual sem som. A fase 3 continua usando as ações da espada flamejante. Nenhum PNG, duração de quadro, âncora, hitbox ou julgamento foi editado nesta correção.

O save é validado, guarda corrupção em `.bak` e troca um `.tmp` sem apagar o arquivo anterior primeiro. A vitória é registrada no golpe final, antes de qualquer clique. Opções têm arquivo separado. Os modos de teste não gravam o progresso.

## Organização

| Arquivo | Responsabilidade |
|---|---|
| `src/core.c`, `roster.c`, `ajuste.h` | Julgamento, padrões, balanceamento e relógio |
| `src/entrada.c`, `entrada_{mac,linux,win,stub}` | Carimbo nativo do clique; fallback no meio do quadro |
| `src/main.c` | Telas, HUD, coreografia e cenas |
| `src/sprites.c`, `tools/personagens.c` | Leitura e geração dos PNGs dos packs |
| `src/arenas.c`, `pixelize.c` | Fundos da equipe e fundos procedurais |
| `src/audio.c`, `vozes.h` | Sons, variações, polifonia, música e fades |
| `src/salvar.c`, `fonte.c` | Persistência e glifos |
| `src/desempenho.c` | Medição por seção do quadro |

Mac usa NSEvent, Linux/X11 usa XInput2 e Windows usa Raw Input. Gamepad e plataformas sem carimbo confiável usam o meio do quadro. Wine verifica o código de Windows; não substitui equipamento real.

`make teste` executa as verificações disponíveis; qualquer teste anunciado como “pulado” precisa de seu ambiente. `make test-assets` confere as pranchas na GPU e `make assets-prontos` verifica todos os arquivos sem abrir janela. Instruções: `../c_game/LEIA-ME.md`.

A validação de 5 de outubro está em `VALIDACAO.json`: núcleo, saves, entradas, cenas, dois finais, PNGs e integrações passaram nas verificações automatizadas descritas. Sensação ao jogar, gamepad e execução gráfica no Windows real continuam pendentes, assim como receber as entregas finais da equipe. O histórico de Unity e RPG Maker está separado do jogo atual nesse relatório.

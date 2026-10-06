# Aparar — estado do jogo

Duelo de um botão, em C11 e raylib. Kojiro lê o aviso, apara e quebra a postura do adversário. Os doze aprendizes são desarmados; matar só é uma escolha no final de Oboro. A história vigente, com spoilers, está em `../aparar_lore.md`.

## Combate

O relógio do núcleo agenda preparação, aviso, partida da lâmina e contato. Um aperto após o aviso vale uma tentativa. Antes do aviso, o aperto cedo recebe recarga limitada para nunca bloquear a janela boa. Há parry perfeito, bom e erro; a tolerância tardia é de 30 ms e a calibração vai até 120 ms.

O perfeito causa mais dano de postura, cura uma porcentagem da vida e apaga a brasa de Enjin, exceto na forma Oni de Oboro: ali o perfeito também custa vida e não cura. O bom apara com recompensa menor. Nos golpes de duas lâminas, o perfeito segura as duas; o bom deixa entrar a segunda. Errar causa dano e a morte de Kojiro tem hitstop antes da queda. Não há ataque livre, dash controlável nem invencibilidade concedida ao jogador.

O aviso varia de 450 ms no Daichi a 320 ms nos últimos; Jinshi tem aviso visual de pelo menos 350 ms e nenhum som de aviso. Hayate e Jinshi conservam variação na preparação. A lâmina viaja em tempo variável do quinto aprendiz (Enjin) em diante, ligável em `ajuste.h`; aviso e contato não se deslocam. Sequências têm piso de 400 ms entre contatos. Hitstop é descontado da preparação seguinte.

Oboro tem três selos: fundamentos de Hanzo, devorador de posturas e a forma Oni. Na fase 2, sorteia entre 50 sequências de ataque das 12 posturas, desde o primeiro golpe; a postura anterior não se repete na sequência seguinte. A fase 3 tem 12 sequências próprias da forma Oni, sorteadas, com cortes, estocadas, avanços, saltos e golpes pesados da espada flamejante. Cada nova tentativa recebe uma semente diferente, salvo quando `APARA_SEMENTE` é fixada para testes. Recupera a vida do jogador a cada selo; a segunda fase reduz o dano pela metade. A terceira usa efeitos vermelhos, preparação ×0,60 e combos com contatos a cada 400–440 ms, com aviso inicial nunca inferior a 320 ms. Com 250 de vida, perfeito custa 5, bom custa 15 e erro custa 62,5; o perfeito não cura. Sua postura é 448, para essa explosão de golpes não se alongar demais. Se um parry zerar a vida de Kojiro, é derrota mesmo que também quebre a postura de Oboro. O HUD não revela selos restantes. A resistência dos primeiros oito aprendizes aumentou 20%; Suiren, Karasu, Yoru e Jinshi receberam 30%. Oboro recebeu 40% no total, distribuído proporcionalmente pelos selos: 504 / 2520 / 448 (total 3472).

## Lutadores

| Ordem | Nome | Arma | Postura |
|---|---|---|---|
| 1 | Daichi | Katana | Terra |
| 2 | Genbu | Katana | Tartaruga |
| 3 | Hayate | Duas foices | Vento |
| 4 | Shizuku | Florete | Gelo |
| 5 | Enjin | Katana em chamas | Chama |
| 6 | Arashi | Duas katanas | Tempestade |
| 7 | Raizo | Odachi | Montanha / pedra |
| 8 | Garfiel | Garras | Tigre |
| 9 | Suiren | Lança | Mar |
| 10 | Karasu | Katana e wakizashi | Corvo |
| 11 | Yoru | Duas adagas invertidas | Noite |
| 12 | Jinshi | Katana branca | Lua |
| 13 | Oboro | Katana / espada em chamas | Hanzo, doze ecos, Oni |

A posição na trilha determina o nível, as janelas e o progresso salvo. A identidade do personagem mantém sua arma, postura, arena, roupa, efeitos e sons. Os ecos do Oboro são sorteados independentemente da ordem da trilha.

Todos os padrões e seus tempos estão em `c_game/src/roster.c`. Os objetivos da curva estão em `CURVA.md`; a análise de ritmo, em `FLUIDEZ.md`. As propostas de reduzir hitstop em sequência e acelerar Daichi continuam sem aplicação.

## Apresentação e arquivos

Resolução de arte: **320 × 180**, chão em y=150. Janela inicial: **1280 × 720**, filtro de ponto, alvo normal de 60 FPS. Personagens e retratos usam somente PNGs derivados dos packs Mattz Artz. O rig auxilia a coreografia e as coordenadas; não desenha um personagem substituto quando faltam os PNGs.

Os fundos procedurais continuam disponíveis. Os PNGs do artista substituem o fundo de cada arena, com sua paleta preservada e camadas animáveis. Os sons sintetizados continuam como fallback; WAVs e músicas da equipe entram sem alterar o núcleo. Formatos e nomes: `ENTREGAS_EQUIPE.md`.

As três posturas que antes se pareciam têm paletas fixas, compartilhadas pelo gerador e pelo jogo em `src/cores_posturas.h`:

| Mestre | Elemento | Corte e aura | Detalhes |
|---|---|---|---|
| Shizuku | Gelo | Branco gelado / ciano claro: `#f4fcff`, `#bfefff`, `#72cfeb` | Lascas claras |
| Suiren | Mar / água | Turquesa: `#b6ffe8`, `#22bdaa`, `#096c75` | Gotas e ondas da mesma paleta |
| Arashi | Tempestade / chuva | Cinza de nuvem: `#d9dfe8`, `#8c99ad`, `#3e526d` | Ramos de raio azul-elétrico `#497eff`, ponta clara `#f4f8ff` |

**Choque do Arashi (relâmpago).** Quando o relâmpago chega caem raios do céu em volta de quem aparou ou, se pegou, de Kojiro, que fica em choque, meio paralisado:
erro (as duas lâminas entram) é o choque cheio, aparo bom (entra uma) é o meio choque, perfeito nada. O choque vale **só para o golpe seguinte**: a janela perfeita dele
encolhe 40% no choque cheio (20% no meio: 46 ms viram 28 ms) e a boa não muda, então um aperto que seria perfeito vira bom (`AJ_CHOQUE_JANELA`, no núcleo; o F3 mostra
a janela "(choque)"). Kojiro treme, pisca em azul e cai devagar por uns 0,9 s, e fica um brilho azul fraco até esse golpe ser julgado. O eco do relâmpago no Oboro só tem os raios, sem choque.

Arashi voltou às poses de ataque e ao formato do slash branco original do Samurai #5. O gerador recolore esse corte em cinza e acrescenta dois ramos curtos ligados à borda, sem trocar o arco por um zigue-zague ou sobrepor o slash comprado. O aço mantém seus highlights claros. Garfiel usa as três garras laranja do pack comprado `vfx_slash`; a garra de espera só recebe um segundo efeito nos golpes duplos. Camadas auxiliares `_clean_*` removem o rastro anterior e preservam corpo e arma. `APARA_SLASH=0`, pack ausente ou camada ausente restauram o rastro nativo do Garfiel.

Oboro na fase 2 conserva suas animações de katana e as variantes `_ECO_<POSTURA>` inclusive nos contatos duplos, pesados e finais. Seus ecos herdam as três paletas acima, os ramos elétricos do Arashi e as garras laranja do Garfiel, sempre na própria katana. Partículas, aura, aviso, gesto e som de corte consultam a postura de origem. Jinshi conserva o aviso visual sem som. A fase 3 combina as ações existentes da espada flamejante em sequências próprias; corte, aviso, aura e fio da espada em repouso usam vermelho. Não há novos quadros desenhados. As tiras derivadas dos três aprendizes e dos respectivos ecos do Oboro com e sem máscara foram regeneradas; durações, número de quadros, âncoras, alcance configurado e julgamento permaneceram iguais. Os pontos físicos de arma do Arashi foram atualizados para as poses restauradas. As cópias de corpo ficam desligadas por padrão (`APARA_RASTRO=1` serve para comparação).

A coreografia foi revista sem deslocar os avisos ou contatos do núcleo: Genbu prioriza cortes acima da cabeça e permanece no chão; Shizuku usa mais avanços e saltos, com deslocamento do dash ×1,15; Garfiel avança e salta nos padrões existentes; Suiren mantém mais distância e usa saltos e água. Hayate tem foices ligeiramente maiores, Suiren tem haste e ponta de lança mais legíveis, e a wakizashi de Karasu é menor. O teleporte de Karasu some em um quadro e reaparece no contato, mantendo o aviso anterior. A recuperação dos golpes percorre os quadros nativos restantes, sem comprimir a cauda; a forja de Enjin perdeu os círculos e lampejos de lava que competiam com a luta.

Os cortes não tocam mais na preparação ou em duplicidade no aviso. O som `cue` segue o aviso, `swing` é agendado para alinhar o pico do arquivo ao contato, e `perfect`/`good`/`bad` seguem o julgamento. Arquivos de corte conservam o timbre; se o pico exigir mais antecedência que a viagem disponível, o som começa na partida da lâmina. A precisão da apresentação sonora fica limitada ao quadro e ao dispositivo de áudio.

Todos os adversários têm `DESARMADO` com mais de um quadro, derivado da queda nativa do pack. A arma voando usa pixels recortados do ataque em curso, separados nas camadas `_weapon_*`; `_steel_*` mantém os efeitos na lâmina e não no rosto. As máscaras acompanham o deslocamento do quadro. Nos ataques com duas armas identificadas, ambas voam; Oboro mantém sua própria katana, inclusive nas posturas copiadas e na forma flamejante.

Ajuste final dos chefes (6 de outubro): o erro de Raizo passou a custar **26,25** de vida (5% acima do erro de Oboro na fase 2) e os golpes dele ficaram **mais lentos**: preparação ×1,25 e intervalo entre os golpes de uma sequência ×1,3, sem mexer no aviso nem nas janelas (os ecos de Raizo no Oboro copiam os padrões novos). A primeira simulação tinha quebrado a curva porque um erro mais barato deixava Raizo fácil demais; o que devolveu a dificuldade foi alongar a luta (postura 552 → 768) e abrir os intervalos, que só pesam para quem decora o ritmo. **Daichi** mostra seus especiais (duas pancadas, avanço, salto e golpe pesado) em 54% das sequências (antes 45%) e **Garfiel** faz os combos de seis a oito golpes em 39% (antes 26%): só pesos de sorteio, sem golpes novos. A curva do casual não saiu da faixa em nenhum mestre (`docs/CURVA.md`). A forma definitiva das adagas de Yoru aguarda referência; sua aura foi reduzida e concentrada nas lâminas.

O save é validado, guarda corrupção em `.bak` e troca um `.tmp` sem apagar o arquivo anterior primeiro. A vitória é registrada no golpe final, antes de qualquer clique. Opções (a calibração) têm arquivo separado, com as mesmas garantias: gravação atômica, validação, `.bak` e aviso na faixa quando não grava. Os dois ficam na pasta de dados do usuário (`%LOCALAPPDATA%\Apara`, `~/Library/Application Support/Apara`, `~/.local/share/apara`; `APARA_DADOS` troca), e um save ao lado do executável, de versões antigas, é copiado para lá na primeira vez, sem tocar o original (ver `c_game/LEIA-ME.md`). Os modos de teste não gravam o progresso.

## Organização

| Arquivo | Responsabilidade |
|---|---|
| `src/core.c`, `roster.c`, `ajuste.h` | Julgamento, padrões, balanceamento e relógio |
| `src/entrada.c`, `entrada_{mac,linux,win,stub}` | Carimbo nativo do clique; fallback no meio do quadro |
| `src/main.c` | Telas, HUD, coreografia e cenas |
| `src/sprites.c`, `tools/personagens.c` | Leitura e geração dos PNGs dos packs |
| `src/arenas.c`, `pixelize.c` | Fundos da equipe e fundos procedurais |
| `src/audio.c`, `vozes.h` | Sons, variações, polifonia, música e fades |
| `src/salvar.c`, `opcoes.c`, `gravar.c`, `pasta_dados.c`, `fonte.c` | Persistência (progresso, opções, gravação atômica, pasta de dados) e glifos |
| `src/desempenho.c` | Medição por seção do quadro |

Mac usa NSEvent, Linux/X11 usa XInput2 e Windows usa Raw Input. Gamepad e plataformas sem carimbo confiável usam o meio do quadro. Wine verifica o código de Windows; não substitui equipamento real.

`make teste` executa as verificações disponíveis; qualquer teste anunciado como “pulado” precisa de seu ambiente. `make test-assets` confere as pranchas na GPU e `make assets-prontos` verifica todos os arquivos sem abrir janela. Instruções: `../c_game/LEIA-ME.md`.

A validação de 5 de outubro está em `VALIDACAO.json`: núcleo, saves, entradas, cenas, dois finais, PNGs e integrações passaram nas verificações automatizadas descritas. Sensação ao jogar, gamepad e execução gráfica no Windows real continuam pendentes, assim como receber as entregas finais da equipe. O histórico de Unity e RPG Maker está separado do jogo atual nesse relatório.

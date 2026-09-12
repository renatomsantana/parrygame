# APARA — A Trilha dos Sete Mestres (Unity 6)

Duelo de katana de um botão, pixel art, sete mestres, cada um no seu cenário.
Versão 0.5.0. Este repositório é só o projeto Unity: a raiz é a pasta do
projeto (`Assets/`, `Packages/`, `ProjectSettings/`).

## Abrir

1. Unity Hub → **Add** → esta pasta (Unity 6000.0 ou mais novo; o Hub oferece
   migrar a versão).
2. Espere a importação. Abra `Assets/Scenes/Main.unity` (só uma câmera; já
   está nas Build Settings) e aperte **Play**: o jogo se monta sozinho a
   partir de `Assets/Resources/Art`. Qualquer outra cena também serve.
3. A tela inicial mostra a trilha: escolha um mestre liberado e clique. As
   falas de abertura vêm antes do duelo.

## Controles

| Ação | Controle |
|---|---|
| Parry / avançar fala | Clique esquerdo; espaço é alternativa |
| Pausar/voltar | Esc |
| Recomeçar o duelo atual | R |
| Ativar/desativar som | M |
| Ativar/desativar tremor | F |
| Salvar captura de tela | P (grava `apara_HHMMSS.png` na pasta do projeto) |
| Escolher mestre na trilha | ← → ou A D; só os já liberados |
| Modo treino | T: barra de timing com as janelas e o valor do parry em ms |
| Pular para um mestre (teste) | 1 a 8; não salva progresso |
| Apagar o progresso salvo | 0 |

Funciona com o Input Manager antigo e com o Input System novo. O mestre
atual fica salvo ao vencer (PlayerPrefs) e some ao terminar a trilha.

## O jogo

Ren, de cabelo curto e casaco laranja assimétrico, enfrenta a Liga dos
Mestres Dissidentes. **Um clique faz o parry.** O mestre prepara o golpe;
quem clica perto do contato defende.

| Timing antes do contato | Resultado | Efeito |
|---|---|---|
| Até a janela perfeita do mestre | Perfeito | −8 de vida e −25 de postura do mestre; flash branco |
| Até a janela boa do mestre | Bom | Bloqueia sem dano; faísca amarela discreta |
| Antes disso, ou sem defesa no contato | Ruim | −25 de vida de Ren; tremor e flash vermelho |

Ren tem 100 de vida e morre no quarto erro. Cada mestre começa com 100 de
vida e 100 de postura. Postura zerada causa **30 de dano extra**, congela o
mestre e se recompõe na próxima preparação. Com metade da vida, o mestre
prepara os golpes 10% mais rápido. Cada golpe aceita **uma tentativa**.

| Mestre | Cenário | Perfeito / Bom | Fintas | Estilo |
|---|---|---:|---:|---|
| 1. Gorou, Mestre do Dojo | Dojo de tatame e bambu | 90 / 220 ms | 0% | Ritmo direto; tutorial |
| 2. Neon Jax, Campeão da Balada | Rave underground | 80 / 200 ms | 20% | Sincopado: a preparação sai do compasso |
| 3. Cavan, Titã do Campo | Celeiro ao entardecer | 75 / 190 ms | 30% | Finta pesada: atraso longo |
| 4. Vance, Executivo S.A. | Cobertura corporativa | 70 / 180 ms | 40% | Finta dupla: dois instantes falsos |
| 5. Kaelen, Pintor Visionário | Galeria surrealista | 65 / 170 ms | 50% | Cortes cegantes: sinal escondido |
| 6. Eleonor, Grã-Duquesa Esgrimista | Salão de baile com espelhos | 60 / 160 ms | 60% | Finta tripla, estocadas rápidas |
| 7. Sombra, o Reflexo de Ren | Jardim de Vidro | 50 / 140 ms | 75% | Mímica: finge o parry e ataca no cooldown |
| 8. Mestre Supremo (provisório) | Salão da Liga | Alta 70/180 · Baixa 45/130 ms | 30% / 50% | Troca de postura, três fases, golpes compostos |

**Gemas:** cada um dos sete mestres roubou uma gema da empunhadura; vencer
devolve a gema, e a trilha mostra as sete. O Boss Final não guarda gema.

**Finta:** a preparação e o sinal (mais agudo) chegam em um ou mais instantes
falsos; o contato real vem depois. Quem clica num instante falso gasta a
única tentativa e leva o corte. Duas falas antes e uma depois de cada mestre.
Vencer passa ao próximo; perder repete. O Boss Final é provisório: tem as
três mecânicas do roteiro (troca de postura, fases, golpes compostos), mas
nome, cenário e falas esperam a próxima fase do roteiro. O roteiro completo está em `docs/DESIGN.md`; a especificação de
origem em `docs/ESPECIFICACAO_v0.5.0.md`.

## Onde mexer

| Arquivo | Responsabilidade |
|---|---|
| `Assets/Scripts/Core/CombatSettings.cs` | Vida, dano, ritmo, atraso da finta e pausas de impacto |
| `Assets/Scripts/Core/BossRoster.cs` | Os sete mestres e o Boss Final provisório: janelas, preparações, estilo de finta, posturas, fases, cenário, cores e falas |
| `Assets/Scripts/BossDefinition.cs` | Mestre como asset: **APARA → Exportar mestres** cria um por mestre em `Resources/Bosses`; a partir daí o Inspector manda |
| `Assets/Scripts/Core/CombatCore.cs` | Relógio, tentativa, instantes falsos, classificação, vida, postura e vitória |
| `Assets/Scripts/Core/Campaign.cs` | Posição na trilha |
| `Assets/Scripts/Core/SpriteSheetData.cs` | Leitura de `frames.json` (recortes e âncoras) |
| `Assets/Scripts/AparaCombatBridge.cs` | Monta a cena, lê input, liga eventos a animação, som e HUD |
| `Assets/Scripts/View/ActorView.cs` | Reprodução dos frames com pivô no pé; a Sombra usa a prancha de Ren |
| `Assets/Scripts/View/FeedbackView.cs` | Sons sintetizados, faíscas e anel |
| `Assets/Scripts/View/HudView.cs` | Canvas 640 × 360: barras, falas, painéis, flash |
| `Assets/Scripts/Editor/ArtImportSettings.cs` | Pixel art sem filtro; menu **APARA → Rodar testes do núcleo** |
| `Assets/Resources/Art/` | As duas pranchas, a arena comum e `frames.json` |

## Arte: para depois

Cada mestre nomeia seu cenário em `Assets/Resources/Art/Arenas/`
(`arena_dojo.png`, `arena_balada.png`, `arena_campo.png`,
`arena_escritorio.png`, `arena_galeria.png`, `arena_salao.png`,
`arena_jardim.png`, `arena_liga.png`). Enquanto o arquivo não existe, a arena comum entra com
a tonalidade do mestre. Pranchas próprias entram por chave nova em
`frames.json`. Referências: KATANA ZERO e Children of Morta. Detalhes e
prompts em `docs/ARTE.md`.

## Verificação

Dentro do Unity: **Window → General → Test Runner → EditMode → Run All**, ou
o menu **APARA → Rodar testes do núcleo**. Fora do Unity, só com o Windows:

```powershell
powershell -ExecutionPolicy Bypass -File Tools/RunCoreTests.ps1
```

Com o Unity instalado, sem abrir a interface (os scripts acham o editor da
versão do projeto no Unity Hub, ou aceitam `-UnityExe caminho\Unity.exe`):

```powershell
powershell -ExecutionPolicy Bypass -File Tools/RunUnityTests.ps1   # EditMode → Builds/tests.xml
powershell -ExecutionPolicy Bypass -File Tools/Build.ps1           # Builds/Windows/APARA.exe
```

Os dois também estão no menu **APARA** do editor.

O núcleo em `Assets/Scripts/Core` não depende do Unity e é escrito em C# 5
de propósito, para compilar com o csc.exe do .NET Framework.

Em 11/09/2026: 584 verificações, 0 falhas. **O Unity não estava instalado na
máquina onde este código foi escrito**: cena, HUD, áudio e input ainda não
foram executados. O primeiro Play deve conferir importação das pranchas com
alfa, pivô nos pés, falas, sinal, os três timings, fintas e a tela final.

# APARA — A Trilha dos Sete Mestres

Duelo de katana de um botão, pixel art, sete mestres, cada um no seu cenário.
Versão 0.5.0, em duas plataformas com as mesmas regras:

| Pasta | Plataforma | Como verificar sem abrir o editor |
|---|---|---|
| `unity_project/` | Unity 6 | `powershell -ExecutionPolicy Bypass -File unity_project/Tools/RunCoreTests.ps1` |
| `rpgmaker_mz/` | RPG Maker MZ | `node rpgmaker_mz/tests/core_test.js` |

Cada pasta tem o seu `LEIA-ME.md` com instalação e controles. Este arquivo
resume o jogo; o roteiro completo está em `docs/DESIGN.md` e a arte em
`docs/ARTE.md`. `script novo o true.md` é a especificação de origem;
`script.md` guarda a versão anterior em Godot, autossuficiente.

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

## A trilha

| Mestre | Cenário | Perfeito / Bom | Fintas | Estilo |
|---|---|---:|---:|---|
| 1. Gorou, Mestre do Dojo | Dojo de tatame e bambu | 90 / 220 ms | 0% | Ritmo direto; tutorial |
| 2. Neon Jax, Campeão da Balada | Rave underground | 80 / 200 ms | 20% | Sincopado: a preparação sai do compasso |
| 3. Cavan, Titã do Campo | Celeiro ao entardecer | 75 / 190 ms | 30% | Finta pesada: atraso longo |
| 4. Vance, Executivo S.A. | Cobertura corporativa | 70 / 180 ms | 40% | Finta dupla: dois instantes falsos |
| 5. Kaelen, Pintor Visionário | Galeria surrealista | 65 / 170 ms | 50% | Cortes cegantes: sinal escondido |
| 6. Eleonor, Grã-Duquesa Esgrimista | Salão de baile com espelhos | 60 / 160 ms | 60% | Finta tripla, estocadas rápidas |
| 7. Sombra, o Reflexo de Ren | Jardim de Vidro | 50 / 140 ms | 75% | Mímica: finge o parry e ataca no cooldown |

**Finta:** a preparação e o sinal (mais agudo) chegam em um ou mais instantes
falsos; o contato real vem depois. Quem clica num instante falso gasta a
única tentativa e leva o corte. Duas falas antes e uma depois de cada mestre.
Vencer passa ao próximo; perder repete. Depois da Sombra, o Boss Final fica
"a definir".

## Arte: para depois

O código já tem os ganchos. Cada mestre nomeia seu cenário (`arena_dojo`,
`arena_balada`, ...) e sua prancha; enquanto os arquivos não existem, a arena
comum entra com a tonalidade do mestre e a prancha padrão (a Sombra usa a de
Ren espelhada e escura). Os nomes esperados estão em `docs/ARTE.md`.

## Estado da verificação (11/09/2026)

| Camada | Resultado |
|---|---|
| Núcleo C# (regras, mestres, fintas, trilha, prancha) | 474 verificações, 0 falhas, compilado com o csc.exe do Windows |
| Núcleo JavaScript do plugin | 275 verificações, 0 falhas, Node 24 |
| Cena, HUD, áudio e input no Unity | Não executados: Unity não instalado aqui |
| Sprites, HUD, falas e eventos no RPG Maker | Não executados: RPG Maker não instalado aqui |

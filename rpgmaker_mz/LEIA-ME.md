# APARA para RPG Maker MZ

Plugin único: `js/plugins/AparaDuelCore.js`. Mesmas regras, mestres, fintas,
falas e Boss Final provisório (troca de postura, fases, golpes compostos) do
projeto Unity; o núcleo em JavaScript é testado fora do RPG Maker.

## Instalar

1. Copie `js/plugins/AparaDuelCore.js` para `js/plugins/` do seu projeto MZ.
2. Em **Plugin Manager**, adicione `AparaDuelCore` e confira os parâmetros:
   switch ativador (10), variáveis de mestre (20), vida de Ren (21), vida do
   mestre (22), resultado (23) e gemas recuperadas (24; o plugin usa também a
   variável 1024 como memória de quais mestres já deram gema), SV battler de
   Ren, falas automáticas, tom de cenário e volume.
3. O plugin usa só recursos do RTP: sons `Wind7`, `Parry`, `Iron1`, `Blow3`,
   `Collapse1` e os SV battlers `Actor3_4`, `Actor2_3`, `Actor2_1`, `Actor1_7`,
   `Actor3_6`, `Actor1_5`, `Actor1_3` e `Actor2_5` (Boss Final provisório).
   Troque pelos seus em `createRoster` quando a arte pixel art existir.

## Um mapa por mestre

Cada mestre tem seu cenário: um mapa. Enquanto a arte não chega, o plugin
aplica uma tonalidade de tela por mestre (dojo quente, rave roxa, campo âmbar,
escritório frio, galeria saturada, salão dourado, jardim escuro). Desligue em
**Aplicar tom do cenário** quando o mapa já tiver a cara certa.

No evento do mestre (toque ou interação):

```
◆ Plugin Command: AparaDuelCore, Iniciar duelo  [Mestre: 3]
◆ Wait: 1 frame
◆ Loop
  ◆ If: Switch [10] is OFF
    ◆ Break Loop
  ◆ End
  ◆ Wait: 1 frame
◆ Repeat Above
◆ If: Variable [23] = 1        (vitória)
  ◆ Transfer Player: mapa do próximo mestre
◆ Else                         (derrota)
  ◆ Show Choices: Tentar de novo / Voltar
◆ End
```

Ou como no documento original: coloque o número do mestre na variável 20 e
ligue o switch 10. O plugin desliga o switch ao terminar e grava o resultado
na variável 23 (1 vitória, 2 derrota). As falas antes e depois são mostradas
pelo plugin com **Show Text** (nome do falante + texto); desative em
**Mostrar falas do plugin** para escrever as suas no evento.

## Durante o duelo

OK ou toque = parry. O menu fica desativado. HUD com vida de Ren, vida e
postura do mestre, placar de perfeitos, mensagem do timing e o sinal
(losango + AGORA / PREPARE-SE). Trocar de mapa no meio encerra o duelo como
derrota.

Vencer um dos sete mestres devolve uma gema da empunhadura (variável 24,
0 a 7, contada uma vez por mestre); o HUD mostra "GEMAS n / 7".

Efeitos por resultado, como no documento: perfeito = `Parry` (tom 120) com
`Damage5` sincronizado + flash branco;
bom = `Iron1` + flash amarelo; ruim = `Blow3` (tom 90) + tremor + flash vermelho;
quebra de postura = `Collapse1` + congelamento do mestre. O sinal `Wind7` sai
mais agudo nas fintas e mais baixo contra Kaelen.

## Testar sem o RPG Maker

```sh
node rpgmaker_mz/tests/core_test.js
```

Em 11/09/2026: 331 verificações, 0 falhas, em Node 24. **O RPG Maker não
estava instalado nesta máquina**: a camada de cena (sprites, HUD, falas,
switch e variáveis) ainda não foi executada no editor.

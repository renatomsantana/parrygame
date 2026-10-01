# Golpes novos por arma (item 2): a primeira etapa está aplicada

A **primeira etapa** (só o núcleo: `roster.c` e testes, sem tocar o `main.c`) foi aplicada, um mestre por commit, depois da sua aprovação. A **segunda etapa** (o desenho próprio de cada golpe, no `main.c`) segue como proposta (seção 4).
Os números abaixo vêm do protótipo que precedeu os commits, e foram conferidos de novo a cada commit (`make test` com a `curva-alvo`) e no repertório final. Como ficou, em commits: seção 5.

## 1. O que entra, o que não entra

- **Fica com a outra sessão, não toco:** Shizuku (florete: 4 padrões e a finta; `thrustOnly`, `feint`), Raizo (odachi), o sumiço do Karasu (`sumiço` e `corvo fantasma`, `LOOK_WARP`), o Hanzo, os desarmes, o rastro fantasma e os clipes.
- **Daichi e Genbu ficam em 7 golpes:** é a sua regra dos três primeiros mestres (o Raizo também fica em 7). O item 2 vale para os **oito** mestres seguintes, menos a Shizuku: Garfiel, Karasu, Hayate, Enjin, Suiren, Arashi, Yoru e Jinshi.
  Se quiser golpes novos no Daichi e no Genbu também, a regra dos sete precisa mudar (medi: as duas versões passam na curva, o casual deles é 100% de qualquer jeito).
- **Oboro não muda:** os doze ecos copiam o golpe do aprendiz **pelo nome** (a tabela `ECO[12]` do `core_test.c`), e nenhum golpe existente muda.
- **Aviso, janelas e tempos de quadro não mudam.** O aviso continua fixo por postura (320 a 450 ms), a janela perfeita e a boa de cada mestre são as mesmas, nenhuma âncora, hitbox ou duração de quadro é tocada, e todos os intervalos entre contatos são **0,40 s ou mais** (`AJ_CADEIA_MIN` fica como está).

## 2. Primeira etapa: só o núcleo (roster e testes), sem tocar o `main.c`

Cada golpe novo é um `Move` do `roster.c` (contatos, intervalos, preparação, peso, olhar, duplo), desenhado pelo que o jogo já faz: o olhar escolhe a animação (alto → `ATTACK_3`, baixo → `ATTACK_2`, estocada → `ATTACK_1` ou
`DASH_ATTACK`, de longe → a lança) e o golpe duplo já desenha as duas lâminas. Ou seja: variedade de **ritmo e de combinação**, jogável já, sem arte e sem código de desenho. A segunda etapa (seção 4) é o desenho próprio
de cada golpe, e fica no `main.c`.

| Mestre | Golpe novo | Contatos (intervalos, s) | Prep. (s) | Peso | Olhar | Duplo | Quanto aparece |
|---|---|---|---|---|---|---|---|
| Garfiel (garras) | arranhão | 4 (0,40 · 0,40 · 0,40) | 0,85 | 1,0 | baixo | | 8,6% |
| | duas patas | 1 | 0,80 | 0,3 | alto | as duas garras | 2,6% |
| Karasu (katana + wakizashi) | cruz de penas | 1 | 0,95 | 0,4 | alto | longa e curta cruzadas | 2,9% |
| | corte curto | 2 (0,40) | 0,85 | 1,0 | estocada | | 7,4% |
| Hayate (duas kamas) | gancho duplo | 2 (0,40) | 0,70 | 1,0 | alto | | 7,5% |
| | ceifada em X | 1 | 0,80 | 0,25 | baixo | as duas foices | 1,9% |
| | vento partido | 3 (0,40 · 0,45) | 0,75 | 0,8 | estocada | | 6,0% |
| Enjin (katana de fogo) | labareda larga | 1 | 0,75 | 1,2 | alto | | 9,0% |
| | ferro em brasa | 2 (0,55) | 0,70 | 0,7 | alto | | 5,2% |
| | chicote de chamas | 2 (0,45) | 0,65 | 1,0 | baixo | | 7,5% |
| Suiren (lança) | arpão duplo | 2 (0,55) | 1,10 | 0,35 | de longe | | 2,5% |
| | varredura de maré | 2 (0,60) | 1,15 | 1,0 | baixo | | 7,1% |
| Arashi (duas lâminas) | descarga | 2 (0,45) | 0,85 | 0,5 | estocada | | 3,9% |
| | cruz elétrica | 1 | 0,80 | 0,8 | alto | as duas lâminas | 6,3% |
| Yoru (duas adagas) | picada | 1 | 0,80 | 0,5 | estocada | | 4,2% |
| | tesoura | 1 | 0,85 | 0,2 | alto | as duas adagas | 1,7% |
| | esquerda e direita | 3 (0,40 · 0,45) | 0,90 | 0,5 | baixo | | 4,2% |
| Jinshi (katana branca) | quarto crescente | 2 (0,55) | 0,90 | 0,4 | alto | | 3,3% |
| | maré de luar | 3 (0,50 · 0,90) | 0,85 | 0,8 | baixo | | 6,7% |

**19 golpes novos**: o Garfiel vai de 9 para 11 golpes, o Enjin de 8 para 11, e os outros seis (Karasu, Hayate, Suiren, Arashi, Yoru, Jinshi) para 12. "Quanto aparece" é a fração das sequências do mestre. É pouco de propósito: ver o porquê na seção 3.

## 3. O que foi medido (e por que os pesos são baixos)

A curva de dificuldade aprovada (`docs/CURVA.md`) é a trava: um golpe novo muda o casual, e a faixa por mestre e a ordem entre os mestres precisam ficar. O efeito de **cada golpe sozinho** no casual (10 mil lutas, peso cheio):

| Efeito no casual | Golpes |
|---|---|
| **endurece muito** | os golpes duplos de um contato só: ceifada em X (−7,4 pontos), cruz de penas (−3,2), cruz elétrica (−2,1); o arpão duplo (−2,4, a lança de longe) |
| **facilita** | as sequências curtas de intervalo de 0,40 a 0,45 s: descarga (+3,8), esquerda e direita (+3,1), tesoura em duas estocadas (+2,9), picada (+2,3), quarto crescente (+2,0), corte curto (+1,7), gancho duplo (+1,7) |
| **não mexe** | labareda larga, chicote de chamas, maré de luar, arranhão |

Com os tempos e pesos "cheios" a curva **quebrava** (3 mestres fora da faixa, Yoru mais fácil que o Arashi). Foi preciso calibrar o peso de cada golpe para que, dentro de cada mestre, o que facilita compense o que endurece: por isso os duplos
têm peso de 0,2 a 0,4 e entram pouco (cerca de 2% das sequências). O Yoru pede o cuidado maior: os três golpes que eu tinha escolhido o facilitavam em 8 pontos juntos, e a `tesoura` virou um golpe duplo (as duas adagas) para
compensar.

**Resultado (100 mil lutas por mestre, a 60 Hz, tempo exato; antes = ponta da `mac-integracao`):**

| Mestre | Casual antes → depois | Reação 250 antes → depois | Faixa alvo do casual |
|---|---|---|---|
| garfiel | 99,8 → 99,8 | 9,6 → 8,7 | 98 a 100 |
| karasu | 90,6 → 91,1 | 8,3 → 7,6 | 88 a 94 |
| hayate | 84,1 → 85,3 | 6,7 → 5,8 | 81 a 87 |
| enjin | 79,7 → 79,5 | 2,9 → 2,7 | 76 a 82 |
| suiren | 72,4 → 71,2 | 0 → 0 | 70 a 76 |
| arashi | 63,8 → 63,6 | 0,4 → 0,4 | 62 a 68 |
| yoru | 59,0 → 58,4 | 1,0 → 0,8 | 57 a 63 |
| jinshi | 55,9 → 55,3 | 0,6 → 0,6 | 52 a 58 |
| oboro | 37,8 → 37,8 | 0 → 0 | 37 a 43 |

`make curva-alvo` fica em **0 itens fora da faixa ou da ordem** (a 10 mil e a 100 mil lutas, a 60 e a 144 Hz), com as **mesmas faixas** do `tests/curva.c`. O degrau do casual Yoru → Jinshi continua em 3,1 pontos (o mínimo é 3,0). Os dois
que mais se mexem são o Hayate (+1,2) e a Suiren (−1,2). Se quiser os golpes novos **mais presentes**, é preciso subir o peso e, junto, endurecer o resto do mestre para a curva se manter: é uma conta maior, e eu só faço se você pedir.

**O que passa sem mudar nenhum teste:** a janela viável de **toda** sequência de todo mestre (o `test_janelas_viaveis` exige ver cada sequência ao menos uma vez: perfeita e boa existem, sem apertar dá erro, cedo e tarde dão erro, aviso de
320 ms ou mais), o resultado idêntico a 30, 60, 120, 144 e 240 Hz (30680 golpes, diferença máxima 0,000 ms), a calibração alta (32400 intervalos, menor partida da lâmina 140 ms) e os cliques no hitstop (14630 contatos).

## 4. Segunda etapa: o desenho próprio de cada golpe (`main.c`), depois

O que faria cada golpe parecer novo, e não só um ritmo novo: o arco de corte desenhado por código na cor do mestre (inclinação de 10 a 15°, esticada horizontal), o raio, o fogo e a lua nos golpes de cada elemento, e a assinatura do aviso por
família (brilho e som; o tempo do aviso não muda). Isso mexe no `main.c` (desenho do golpe, rastro, cores), que a outra sessão está mudando muito (701 linhas na `mac-integracao`). Proponho **só depois** de a outra sessão fechar o `main.c`,
em commits pequenos por família, para não brigar com ela. Até lá, os 19 golpes já jogam com o que existe.

## 5. Como entrou

Aprovados os quatro pontos (e a primeira etapa), em cima da junção com a `mac-integracao`, em commits pequenos, nesta ordem:

1. **Testes presos à semente** (`026fafd`): o "tarde" passa a usar três sementes por mestre e o do golpe duplo do Arashi mantém a vida cheia; o que cada um verifica não mudou.
2. **Piso do Jinshi** no `test_curva` de 55 para 52 (`3e0e3ea`), a faixa aprovada.
3. **Regra do tamanho** do repertório de "7 a 10" para "7 a 12" nos mestres comuns (`c125708`).
4. **Teste novo `test_cada_golpe`** (`6a1f7d1`): cada golpe de cada mestre comum, isolado, com a perfeita e a boa viáveis com 120 ms de atraso e igual a 60 e 144 Hz (hoje 119 golpes, 252 contatos).
5. **Um commit por mestre**, com o `roster.c` e a linha do `docs/JOGO.md`: Karasu, Hayate, Enjin, Suiren, Arashi, Yoru, Jinshi e, por último, o Garfiel. A ordem não é arbitrária: aplicado antes do Karasu, o Garfiel (Reação 250 de 9,4 para 8,5) deixava só 0,3 ponto
   até o Karasu de hoje (8,2), e a ordem da curva exige 0,5; com o Karasu antes (7,7), o degrau é 0,8. Cada commit passa o `make test` inteiro, com a `curva-alvo` em 0 itens.

O que **não** entrou: golpes novos no Daichi e no Genbu (a regra dos sete), nada na Shizuku, no Raizo nem no sumiço do Karasu (da outra sessão), e o desenho próprio de cada golpe (segunda etapa).

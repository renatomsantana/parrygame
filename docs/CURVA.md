# A curva de dificuldade, mestre a mestre

Atualização vigente: resistência aumentada em 6 de outubro; os novos valores e referências de regressão estão na última seção. As medições anteriores ficam como histórico.

Medição com robôs (`make curva`, `c_game/tests/curva.c`) e a curva alvo, **aprovada e aplicada** (só o que mudou no `roster.c`: três
mestres, ver "O que mudou"). Os robôs são estimativa, não verdade: o humano deles não vê a arte, não sofre com a escuridão do
Yoru e não se distrai.

## Os cinco robôs

| Robô | O que faz |
|---|---|
| Primeira vez | só reage ao último sinal (a lâmina partindo, que se vê e se ouve, ou o aviso, o que vier depois); não decora padrão nenhum. Reação de **260 ms à imagem e 210 ms ao som** (a mesma conta do Reação 250), sorteada a cada golpe com desvio de 45 ms e piso de 150 ms, mão de 25 ms e **3% dos golpes nem recebem aperto**. Constantes em `robo.h` (`ROBO_PRIMEIRA_VEZ_*`) |
| Casual decora | decora o padrão do mestre e mede o intervalo até o contato com 8% de erro mais 25 ms de mão (`ROBO_HUMANO_CASUAL`) |
| Reação 250 | só reage ao último sinal, sempre em 250 ms à imagem (200 ao som), mão de 20 ms (`robo_reacao`) |
| Perfeito | o do demo: aperta no máximo 30 ms antes do contato |
| Spam | aperta a cada 150 ms sem olhar |

A reação média de 260 ms à imagem (210 ao som) foi escolhida por medida: a 300 ms à imagem o robô da primeira vez perde até do
daichi (0,0% a 0,7%), porque a lâmina parte **220 ms** antes do contato nos quatro primeiros mestres e ninguém que reage em 250 ms
ao som aperta 30 ms depois do contato, no limite da tolerância tardia (30 ms). Varrendo a reação à imagem (desvio de 45 ms, 3% de falha), ele vence
os quatro primeiros em 96/96/91/88% a 250 ms, 88/86/75/70% a 260, 64/60/46/40% a 270 e 0% a 300: o degrau é íngreme, e 260 fica no meio da faixa de 250 a 300 pedida.

O que `make test-curva` (dentro do `make test`) garante: sem variação nem falha o robô da primeira vez é exatamente o `robo_reacao`;
com 100% de falha é o que nunca defende; o resultado é o mesmo a 30, 60, 120, 144 e 240 Hz; ele nunca aperta antes do sinal a que
reage, e a reação varia de golpe a golpe; o perfeito vence todos os mestres e o spam perde de todos.

## Antes (medido, sem nenhuma regra mudada)

10 mil lutas por mestre e robô (±1 ponto a 95%), kojiro chegando a cada mestre depois de vencer os anteriores. **A 60 e a 144 Hz, com o
clique no instante exato (o carimbo do M3), a tabela é idêntica** (`make robos-taxas` garante; a exceção conhecida é o enjin, com
~0,3% das lutas diferentes por causa das brasas).

| # | Mestre | Primeira vez | Casual decora | Reação 250 | Perfeito | Spam |
|---|---|---|---|---|---|---|
| 1 | daichi | 87,6 | 100,0 | 100,0 | 100,0 | 0,0 |
| 2 | genbu | 85,8 | 100,0 | 100,0 | 100,0 | 0,0 |
| 3 | raizo | 74,8 | 99,9 | 100,0 | 100,0 | 0,0 |
| 4 | shizuku | 69,7 | 100,0 | 100,0 | 100,0 | 0,0 |
| 5 | garfiel | 0,0 | 99,8 | 9,4 | 100,0 | 0,0 |
| 6 | karasu | 0,4 | 90,6 | 17,6 | 100,0 | 0,0 |
| 7 | hayate | 0,0 | 83,9 | 6,9 | 100,0 | 0,0 |
| 8 | enjin | 0,0 | 79,5 | 2,8 | 100,0 | 0,0 |
| 9 | suiren | 0,0 | 72,6 | 0,0 | 100,0 | 0,0 |
| 10 | arashi | 0,1 | 63,9 | 4,7 | 100,0 | 0,0 |
| 11 | yoru | 0,0 | 59,0 | 1,0 | 100,0 | 0,0 |
| 12 | jinshi | 0,1 | 57,6 | 8,6 | 100,0 | 0,0 |
| 13 | oboro | 0,0 | 41,0 | 0,0 | 100,0 | 0,0 |

Sem o carimbo do M3 (plataforma sem ele: o clique entra no meio do quadro), a 60 Hz:

| # | Mestre | Primeira vez | Casual decora | Reação 250 | Perfeito | Spam |
|---|---|---|---|---|---|---|
| 1 | daichi | 86,0 | 100,0 | 100,0 | 100,0 | 0,0 |
| 2 | genbu | 84,0 | 100,0 | 100,0 | 100,0 | 0,0 |
| 3 | raizo | 73,7 | 99,8 | 100,0 | 100,0 | 0,0 |
| 4 | shizuku | 65,4 | 100,0 | 100,0 | 100,0 | 0,0 |
| 5 | garfiel | 0,1 | 99,3 | 7,4 | 100,0 | 0,0 |
| 6 | karasu | 0,3 | 88,6 | 15,6 | 100,0 | 0,0 |
| 7 | hayate | 0,0 | 80,6 | 6,0 | 100,0 | 0,0 |
| 8 | enjin | 0,0 | 72,9 | 2,1 | 100,0 | 0,0 |
| 9 | suiren | 0,0 | 67,1 | 0,0 | 100,0 | 0,0 |
| 10 | arashi | 0,1 | 59,6 | 4,0 | 100,0 | 0,0 |
| 11 | yoru | 0,0 | 53,2 | 0,9 | 100,0 | 0,0 |
| 12 | jinshi | 0,1 | 52,6 | 6,9 | 100,0 | 0,0 |
| 13 | oboro | 0,0 | 34,6 | 0,0 | 100,0 | 0,0 |

(A 144 Hz sem carimbo a tabela fica a menos de 1 ponto da de cima: o erro do meio do quadro é metade de um quadro, 8,3 ms a 60 Hz e 3,5 ms a 144.)

### Por que a curva é assim (golpes por luta e quanto cada robô apara)

10 mil lutas por mestre, 60 Hz, ms exato. "Golpes" é quantos golpes o mestre lança até perder a postura contra o perfeito.

| Mestre | erros p/ cair | golpes | Reação 250 apara | Casual apara | Reação 250 vence | Casual vence |
|---|---|---|---|---|---|---|
| daichi | 8 | 15 | 99,4% | 92,9% | 100,0 | 100,0 |
| genbu | 8 | 15 | 99,4% | 94,1% | 100,0 | 100,0 |
| raizo | 7 | 15 | 99,4% | 91,7% | 100,0 | 99,9 |
| shizuku | 7 | 15 | 99,4% | 94,8% | 100,0 | 100,0 |
| garfiel | 6 | 19 | 74,1% | 94,1% | 9,4 | 99,8 |
| karasu | 7 | 17 | 78,0% | 91,9% | 17,6 | 90,6 |
| hayate | 6 | 20 | 76,9% | 91,0% | 6,9 | 83,9 |
| enjin | 6 | 21 | 75,3% | 91,0% | 2,8 | 79,5 |
| suiren | 6 | 24 | 60,4% | 89,2% | 0,0 | 72,6 |
| arashi | 10 (dano ×1,2) | 15 | 75,0% | 91,8% | 4,7 | 63,9 |
| yoru | 5 | 25 | 75,4% | 89,3% | 1,0 | 59,0 |
| jinshi | 5 | 15 | 73,7% | 85,4% | 8,6 | 57,6 |
| oboro | 5 (selos 5/10/4) | 61 | 72,3% | 89,4% | 0,0 | 41,0 |

O que aparece:

1. **Quem só reage tem um precipício natural no garfiel.** Nos quatro primeiros a lâmina parte sempre 220 ms antes do contato e
   um robô que reage em 250 ms (200 ao som) apara 99,4% dos golpes. Do garfiel em diante a partida da lâmina varia de 140 a 320 ms
   (`AJ_LAMINA_VARIA_DESDE`) e o mesmo robô cai para ~75% dos golpes (60% no suiren, a lança): como os erros acumulam, ele passa de 100%
   para ≤ 18% de uma vez. Por isso o robô da primeira vez, que reage mais devagar que ele, perde "quase sempre a partir do garfiel"
   sem nenhuma regra nova: 0,0% no garfiel e de 0,0 a 0,4% nos oito seguintes.
2. **O Reação 250 tem três picos** (a curva dele sobe do mestre anterior para o seguinte): karasu 17,6 depois de garfiel 9,4
   (+8,2), arashi 4,7 depois de suiren 0,0 (+4,7) e jinshi 8,6 depois de yoru 1,0 (+7,6). As causas estão na tabela: o karasu tem 7
   erros para cair (o garfiel, 6) e lança 17 golpes (o garfiel, 19); o arashi e o jinshi têm só 15 golpes por luta (suiren e yoru, 24 e 25).
   Quem mede só o reflexo não nota os picos no casual: o casual vence 90,6 / 83,9 / 79,5 / 72,6 / 63,9 / 59,0 / 57,6 / 41,0, que cai sempre.
3. **O casual decora fica em ≥ 99,8% nos cinco primeiros (saturado) e cai em degraus de 4 a 9 pontos do garfiel ao yoru**, com duas exceções:
   yoru → jinshi cai só 1,4 (59,0 → 57,6) e jinshi → oboro cai 16,6 (57,6 → 41,0).
4. O arashi tem menos erros por luta do casual que o karasu (2,0 contra 2,3), mas o casual vence 27 pontos menos dele: o golpe duplo só é segurado
   pelo parry perfeito, então o parry bom também custa vida e isso não aparece em "erros". Olhar só "erros por luta" engana nele.
5. O perfeito vence 100,0% em todos os mestres, em todas as taxas, e o spam perde de todos (0,0%): o que tem de valer sempre está no teste.

## Alvo (aprovado e aplicado)

Vitórias (%), ms exato, medidas com 100 mil lutas por mestre (±0,3 pt); a tolerância é ±3 pontos no alvo, e a regra de ordem é a do
`make curva-ordem`: cada mestre tem de ser ao menos 0,5 ponto mais difícil que o anterior onde o robô ainda vence mais de 2%.

| # | Mestre | Primeira vez | Reação 250 | Casual decora | Perfeito | Spam |
|---|---|---|---|---|---|---|
| 1 | daichi | 88 | 100 | 100 | 100 | 0 |
| 2 | genbu | 86 | 100 | 100 | 100 | 0 |
| 3 | raizo | 75 | 100 | ≥ 99 | 100 | 0 |
| 4 | shizuku | 70 | 100 | ≥ 99 | 100 | 0 |
| 5 | garfiel | ≤ 1 | 10 | ≥ 99 | 100 | 0 |
| 6 | karasu | ≤ 1 | 8 | 91 | 100 | 0 |
| 7 | hayate | ≤ 1 | 6 | 84 | 100 | 0 |
| 8 | enjin | ≤ 1 | 3 | 79 | 100 | 0 |
| 9 | suiren | ≤ 1 | ≤ 1 | 73 | 100 | 0 |
| 10 | arashi | ≤ 1 | ≤ 1 | 65 | 100 | 0 |
| 11 | yoru | ≤ 1 | ≤ 1 | 60 | 100 | 0 |
| 12 | jinshi | ≤ 1 | ≤ 1 | 55 | 100 | 0 |
| 13 | oboro | 0 | 0 | 40 | 100 | 0 |

- **Primeira vez:** vence os quatro primeiros (88 / 86 / 75 / 70, uma descida sem pico) e perde quase sempre do garfiel em diante (≤ 1%).
  Já era assim: não precisou mudar nada.
- **Perfeito** 100% em todos, **spam** 0% em todos: já eram assim.
- **Casual decora:** o alvo era o de antes com uma correção: o jinshi desce de 57,6 para 55 (yoru → jinshi passa de −1,4 para −3) e os degraus
  do karasu ao oboro ficam em 8 / 7 / 5 / 6 / 8 / 5 / 5 / 15 pontos. Os cinco primeiros ficam saturados (≥ 99%): é a entrada do jogo.
  Cabe nas faixas do `test_curva` que já existe (casual ≥ 95 nos quatro primeiros, jinshi 55-75, oboro 30-55).
- **Reação 250:** os quatro primeiros em 100%, depois 10 / 8 / 6 / 3 e, do suiren em diante, ≤ 1%: o mesmo precipício do garfiel, mas sempre
  descendo, sem os picos do karasu, do arashi e do jinshi. Tirou ~9 pontos do karasu, ~4 do arashi e ~8 do jinshi sem mexer no casual.

`make curva` marca com `*` a célula fora da faixa e lista o que está fora da faixa, da ordem (cada mestre ao menos 0,5 ponto mais difícil que
o anterior, entre 2% e 98%) e do degrau mínimo de 3 pontos do casual. `make curva-alvo` é o mesmo com erro de saída: roda no `make test`
(10 mil lutas por mestre) e no `make teste` (100 mil). Sem a mudança no `roster.c` ele apontava exatamente o que a medição "antes" mostra
(karasu, arashi e jinshi do Reação 250, o karasu depois do garfiel, o jinshi depois do yoru no casual); com ela, 0 itens. O verificador em si é
testado no `make test-curva` com tabelas sintéticas (a do meio das faixas passa; um pico, um degrau curto e um perfeito que perde são
apontados), conferido por mutação.

## Depois (medido, 100 mil lutas por mestre e robô)

A 60 e a 144 Hz, com o clique em ms exato, idêntica (o mesmo teste de taxas de antes). O casual do karasu ao oboro passa no `make curva-ordem`
com folga de 0,5 ponto (karasu 90,64, hayate 84,07, enjin 79,65, suiren 72,40, arashi 63,82, yoru 59,04, jinshi 55,89, oboro 41,16:
os degraus são 6,6 / 4,4 / 7,3 / 8,6 / 4,8 / 3,2 / 14,7).

| # | Mestre | Primeira vez | Casual decora | Reação 250 | Perfeito | Spam |
|---|---|---|---|---|---|---|
| 1 | daichi | 87,0 | 100,0 | 100,0 | 100,0 | 0,0 |
| 2 | genbu | 84,9 | 100,0 | 100,0 | 100,0 | 0,0 |
| 3 | raizo | 73,8 | 99,9 | 100,0 | 100,0 | 0,0 |
| 4 | shizuku | 68,9 | 100,0 | 100,0 | 100,0 | 0,0 |
| 5 | garfiel | 0,1 | 99,8 | 9,6 | 100,0 | 0,0 |
| 6 | karasu | 0,1 | 90,6 | 8,3 | 100,0 | 0,0 |
| 7 | hayate | 0,0 | 84,1 | 6,7 | 100,0 | 0,0 |
| 8 | enjin | 0,0 | 79,7 | 2,9 | 100,0 | 0,0 |
| 9 | suiren | 0,0 | 72,4 | 0,0 | 100,0 | 0,0 |
| 10 | arashi | 0,0 | 63,8 | 0,4 | 100,0 | 0,0 |
| 11 | yoru | 0,0 | 59,0 | 1,0 | 100,0 | 0,0 |
| 12 | jinshi | 0,0 | 55,9 | 0,6 | 100,0 | 0,0 |
| 13 | oboro | 0,0 | 41,2 | 0,0 | 100,0 | 0,0 |

| Mestre | Reação 250: antes | depois | Casual: antes | depois |
|---|---|---|---|---|
| karasu | 17,6 | **8,3** | 90,6 | 90,6 |
| arashi | 4,7 | **0,4** | 63,9 | 63,8 |
| jinshi | 8,6 | **0,6** | 57,6 | **55,9** |

Os outros dez mestres não mudaram (nem uma lâmina, nem uma postura).

## Depois da integração com a `mac-integracao` e dos golpes novos (medido, 100 mil lutas por mestre e robô)

Duas coisas mexeram na curva depois da tabela acima, e as duas passam nas mesmas faixas (`make curva-alvo`, 0 itens fora da faixa ou da ordem, a 10 mil e a 100 mil lutas, a 60 e a 144 Hz):

1. **A reformulação da Shizuku, da outra sessão** (9 golpes para 4): o `eco do gelo` do Oboro, que copia a dupla do florete, passou de 4 golpes para 2, e o casual do Oboro foi de 41,2 para 37,8 (faixa 37 a 43). Das 65 células da tabela, só essa mudou.
2. **Os 19 golpes novos** em oito mestres (`docs/GOLPES_NOVOS.md`), com o peso de cada um calibrado para a curva ficar onde estava.

| # | Mestre | Primeira vez | Casual decora | Reação 250 | Perfeito | Spam |
|---|---|---|---|---|---|---|
| 1 | daichi | 87,0 | 100,0 | 100,0 | 100,0 | 0,0 |
| 2 | genbu | 84,9 | 100,0 | 100,0 | 100,0 | 0,0 |
| 3 | raizo | 73,8 | 99,9 | 100,0 | 100,0 | 0,0 |
| 4 | shizuku | 68,9 | 100,0 | 100,0 | 100,0 | 0,0 |
| 5 | garfiel | 0,1 | 99,8 | 8,7 | 100,0 | 0,0 |
| 6 | karasu | 0,0 | 91,1 | 7,6 | 100,0 | 0,0 |
| 7 | hayate | 0,0 | 85,3 | 5,8 | 100,0 | 0,0 |
| 8 | enjin | 0,0 | 79,5 | 2,7 | 100,0 | 0,0 |
| 9 | suiren | 0,0 | 71,2 | 0,0 | 100,0 | 0,0 |
| 10 | arashi | 0,0 | 63,6 | 0,4 | 100,0 | 0,0 |
| 11 | yoru | 0,0 | 58,4 | 0,8 | 100,0 | 0,0 |
| 12 | jinshi | 0,0 | 55,3 | 0,6 | 100,0 | 0,0 |
| 13 | oboro | 0,0 | 37,8 | 0,0 | 100,0 | 0,0 |

Os degraus do casual, do Karasu ao Oboro, são 5,8 / 5,8 / 8,3 / 7,6 / 5,2 / 3,1 / 17,5 (o mínimo, do Garfiel ao Oboro, é 3,0; o do Yoru ao Jinshi, 3,1, é o apertado). Daichi, Genbu, Raizo e Shizuku não mudam, e o Oboro só mudou pela Shizuku.
O piso do Jinshi no `test_curva` do `core_test.c` passou de 55 para 52 (a faixa aprovada), porque o teste usa 300 lutas e varia 3 pontos.

## O que mudou no `roster.c`

Três linhas de cabeçalho, nada de golpes:

| Mestre | Mudança | Efeito |
|---|---|---|
| karasu | `bladeMax = 0.239` | a lâmina parte de 140 a 239 ms antes do contato (antes, até 320): o Reação 250 cai de 17,6 para 8,3. Cada 0,001 a mais ou a menos move ~0,7 ponto, então o valor saiu de varredura |
| arashi | `bladeMax = 0.220` | de 140 a 220 ms (o tempo fixo dos quatro primeiros mestres): o Reação 250 cai de 4,7 para 0,4 |
| jinshi | `bladeMax = 0.220` e `posture` 610 → 630 | o Reação 250 cai de 8,6 para 0,6; a postura maior alonga a luta e o casual cai de 57,6 para 55,9 |

O aviso, as janelas perfeita e boa e a menor partida da lâmina (140 ms) não mudaram; o casual não sente o teto da lâmina (mede pelo aviso).

### O que mexe em quê (alavancas, só `roster.c`)

| Alavanca | Afeta o casual? | Afeta quem só reage? |
|---|---|---|
| erros até cair (`hitsToFall`) | sim, muito | sim, muito |
| postura do mestre (duração da luta: nº de golpes) | sim | sim |
| peso dos padrões (mais sequências, mais golpes por luta) | sim | sim |
| faixa da partida da lâmina por mestre (`MasterProfile.bladeMax`: 0 é o global, nunca sobe o teto global) | **não** (ele mede pelo aviso) | **sim**, é a que derruba o karasu, o arashi e o jinshi sem mexer no casual |

Nada de aviso abaixo de 300 ms, nenhuma janela perfeita mais estreita que a já medida como viável. Se um dia a curva sair do lugar (outro mestre
ganha golpes, outra regra muda), o `make curva-alvo` reprova e mostra qual célula, qual ordem ou qual degrau quebrou.

## Reordenação de 5 de outubro de 2026

Pedido: Hayate no nível 3 e Raizo no 7; Enjin no 5 e Garfiel no 8; Arashi no 6 e Karasu no 10. Os demais mantêm suas posições. O `id` representa a posição/nível e uma `identity` estável conserva roupas, arma, arena, som e VFX do personagem. Progresso salvo continua por posição. As visitas a Hanzo anunciam o próximo adversário correto; a primeira volta dos ecos de Oboro segue a nova trilha.

Trocar apenas janelas e postura entre posições não conservou a curva por causa das brasas, golpes duplos e combos longos. O ajuste final mantém os padrões, animações, quadros, âncoras, hitboxes e regras do núcleo; calibra as janelas, aviso, recuperação de postura a partir do quinto e estes valores:

| Nível | Mestre | Postura | Parâmetro de erros (`hitsToFall`) | Teto da lâmina (ms) |
|---|---|---|---|---|
| 3 | Hayate | 300 | 7 | fixo |
| 5 | Enjin | 530 | 7 | 260 |
| 6 | Arashi | 400 | 11 | 235 |
| 7 | Raizo | 460 | 6 | 224 |
| 8 | Garfiel | 1350 | 7 | global |
| 10 | Karasu | 780 | 7 | 220 |

Arashi mantém dano ×1,2 e golpes duplos; Enjin mantém brasas. Por isso `hitsToFall` não é uma promessa de quantidade literal de apertos errados nessas lutas. Hayate mantém a variação de preparação de ±120 ms. A lâmina variável começa no nível 5, agora Enjin; os quatro primeiros continuam fixos.

### Antes e depois, por nível

Antes: 10 mil lutas por mestre/robô no commit `2fcfd4b`. Depois: 100 mil lutas por mestre/robô, a 60 e 144 Hz com aperto em ms exato. As duas taxas deram a mesma tabela. Valores são estimativas dos robôs.

| Nível | Mestre antes | Casual antes (%) | Mestre depois | Casual depois (%) | Primeira vez (%) | Reação 250 (%) |
|---|---|---|---|---|---|---|
| 1 | daichi | 100.0 | daichi | 100.0 | 87.0 | 100.0 |
| 2 | genbu | 100.0 | genbu | 100.0 | 84.9 | 100.0 |
| 3 | raizo | 99.9 | hayate | 100.0 | 74.6 | 100.0 |
| 4 | shizuku | 100.0 | shizuku | 100.0 | 68.9 | 100.0 |
| 5 | garfiel | 99.8 | enjin | 99.2 | 0.1 | 10.8 |
| 6 | karasu | 91.3 | arashi | 91.1 | 0.1 | 8.0 |
| 7 | hayate | 85.6 | raizo | 84.9 | 0.0 | 5.7 |
| 8 | enjin | 79.7 | garfiel | 79.8 | 0.0 | 0.0 |
| 9 | suiren | 71.2 | suiren | 71.2 | 0.0 | 0.0 |
| 10 | arashi | 63.9 | karasu | 64.6 | 0.0 | 0.1 |
| 11 | yoru | 58.6 | yoru | 58.4 | 0.0 | 0.8 |
| 12 | jinshi | 55.4 | jinshi | 55.3 | 0.0 | 0.6 |
| 13 | oboro | 37.4 | oboro | 37.9 | 0.0 | 0.0 |

Perfeito: 100% em todos antes/depois. Spam: 0% em todos antes/depois. A curva casual mantém todas as faixas anteriores por posição e cai pelo menos 3 pontos a partir do sexto nível; os quatro primeiros ficam saturados em aproximadamente 100%. A faixa de Reação 250 do oitavo nível passa de 0,5–6% para 0–6%: os combos longos do Garfiel nessa dificuldade zeram a vitória desse robô. As demais faixas e verificações de ordem não foram relaxadas.

Validação Mac: 15.379 verificações do núcleo, 45.565 de apresentação sem assets e 173.479 com os PNGs reais, zero falhas. Saves, entradas, fontes, desempenho, ritmo, fuzz (600 cenários em cada um dos quatro modos), assets e 168 compilações com avisos como erro passaram. Todo contato mantém janela viável, inclusive com calibração de 120 ms. A exceção já conhecida das brasas do Enjin entre taxas deu 9 diferenças em 3.200 comparações (0,28%, abaixo de 2%); os demais deram zero em 38.400.

Jogo real: os seis aprendizes trocados e Oboro foram executados no Mac com logs de fonte sonora, efeitos e impactos; Oboro percorreu os doze ecos na nova ordem. No Linux/Xvfb, 173.479 verificações com os PNGs reais passaram, assim como `teste_slash.sh` (84 contatos idênticos com pack ligado/desligado) e `teste_rastro.sh` (90 contatos idênticos com silhuetas ligadas/desligadas). Garfiel e seu eco desenham as garras compradas; Arashi não usa esse pack. Windows e sensação ao jogar não foram revalidados nesta alteração.

## Oboro: 12 posturas e 50 padrões aleatórios (5 de outubro)

Correção solicitada depois da reordenação: Oboro **não percorre a trilha em ordem**. As fases 2 e 3 têm o mesmo repertório de **50 sequências distintas** dos 12 aprendizes, sorteadas desde a primeira. Cada escolha sorteia uma postura com a mesma chance que as outras, exclui a postura imediatamente anterior e então escolhe um golpe dela. A sequência mantém seu ritmo interno; não se sorteia outro golpe no meio de um combo. A fase 1 mantém os sete fundamentos e sua abertura de sete contatos.

| Origem | Sequências disponíveis em cada fase final |
|---|---|
| Daichi, Genbu, Hayate, Shizuku, Enjin, Arashi | 4 de cada |
| Raizo, Suiren, Karasu, Yoru | 4 de cada |
| Garfiel, Jinshi | 5 de cada |
| Total | 50 |

As duas fases usam as mesmas 50 sequências; a fase 3 conserva preparação ×0,85, dano ×1,25, aviso mínimo de 320 ms e ações da espada flamejante. O Oboro conserva a própria katana na fase 2 e os sons, partículas, aura e paleta da origem. Não foram adicionados nem alterados quadros de sprite. Uma identidade explícita no golpe resolve sua origem, sem depender do nome mostrado.

Apenas liberar os novos padrões levou o casual a 48,2% (10 mil lutas). A postura do terceiro selo passou de 450 para 550, mantendo vida, dano e janelas: 39,7% em 100 mil lutas, a 60 e 144 Hz, contra 37,9% na versão anterior. Perfeito continua em 100%; primeira vez, reação 250 e spam em 0%. Os 12 aprendizes mantiveram resultados idênticos à medição anterior. Nenhuma faixa do verificador da curva foi relaxada nesta mudança.

Testes novos: cópia dos 50 padrões confrontada com a origem, incluindo intervalos, direção, golpes duplos e finta; 2.000 sequências em cada fase final cobrem todos os golpes e posturas, sem repetição imediata, com reprodução exata para a mesma semente e variação entre sementes. Os 216 contatos dos 100 padrões (50 em cada fase final) foram isolados e testados com atraso máximo de 120 ms a 60 e 144 Hz: parry perfeito viável e tempos idênticos.

A semente normal combina relógio e contador de tentativas para variar também nas reinicializações da luta no mesmo segundo. `APARA_SEMENTE` continua reproduzível, para depuração. O sorteio não garante que todos os 50 padrões apareçam antes da vitória em uma única luta. Medições e logs: `outputs/oboro-50-05out/`.

Validação final: 26.173 verificações do núcleo, 55.209 de apresentação sem assets e 234.123 com os PNGs reais no Mac; 234.123 com os PNGs no Linux. Fontes, save, entrada, desempenho, ritmo, fuzz (600 cenários em cada um dos quatro modos) e 168 compilações com avisos como erro passaram. O jogo real no Linux conservou os mesmos 104 contatos com os efeitos ligados/desligados, incluindo 59 de Oboro; as garras copiadas foram desenhadas 102 vezes. Windows e revisão humana da sensação não foram repetidos nesta mudança.

No Mac, quatro amostras do jogo real (fases 2 e 3, sementes 11 e 12, 45 s cada) produziram roteiros diferentes: 42/45 contatos na fase 2 e 18/14 na fase 3. A fase 2 usou variantes da própria katana e todos os contatos da fase 3 usaram ações `_FURIA` da espada flamejante.


## Oboro: repertório próprio da forma Oni (6 de outubro)

Novo pedido substitui a fase 3 descrita acima. A fase 2 conserva os 50 padrões dos 12 aprendizes. A fase 3 sorteia **12 sequências próprias**, com 2 a 6 contatos, usando as ações existentes da espada flamejante: cortes altos/baixos, estocadas, avanços, saltos e golpes pesados. Preparação ×0,60; intervalos internos de 400–440 ms; aviso inicial de 320 ms; janelas de perfeito/bom e durações dos quadros preservadas.

O dano atravessa a defesa: com 250 de vida, **perfeito tira 5 e não cura; bom tira 24 (4 normais + 20 adicionais); erro tira 62,5**. Constantes por selo em `roster.c` usam 2% da vida no perfeito e 8% adicionais no bom. Se Kojiro morrer no parry que quebra a postura de Oboro, perde; não se emite vitória nem se salva progresso.

| Medição | Casual (%) | Perfeito (%) | Spam (%) |
|---|---|---|---|
| Antes, fase 3 com ecos (100 mil lutas) | 39,7 | 100 | 0 |
| Novos ataques e dano, postura antiga de 550 (10 mil) | 6,0 | 100 | 0 |
| Fase própria, postura reduzida a 320 (100 mil) | 38,4 | 100 | 0 |

As medições finais a 60 e 144 Hz, com aperto em ms exato, foram idênticas. Os 12 aprendizes continuam com a mesma tabela; a curva mantém as faixas e a ordem existentes, sem relaxar o verificador. São estimativas de robôs, não uma avaliação da sensação ao jogar.

Validação: 20.936 verificações do núcleo, incluindo dano de perfeito/bom/erro e parry fatal; 2.000 sorteios por fase final cobrem os 50 ecos e os 12 padrões próprios. Os 149 contatos dos 62 padrões finais foram isolados com calibração de 120 ms a 60 e 144 Hz: todos têm perfeito viável e tempos iguais. Fuzz: 600 cenários em cada um de quatro modos, zero violações. Fontes, save, entrada, desempenho, apresentação e 168 compilações com avisos como erro passaram.

Apresentação com PNGs reais: 208.450 verificações no Mac e no Linux, zero falhas. No Linux/Xvfb, 96 contatos do jogo real foram idênticos com pack ligado/desligado, incluindo 51 de Oboro; a fase 2 usou sons das posturas e a fase 3 só os de Oboro, com ações `_FURIA`. No Mac, oito perfeitos consecutivos na fase 3 baixaram a vida de 250 para 210. O fio da espada em espera foi recolorido de laranja para vermelho em oito tiras locais de Oboro/com máscara; dimensões, alfa, corpo, duração, número de quadros e âncoras permaneceram iguais. Gerador atualizado para reproduzir a cor. Amostra de oito segundos com SFX básicos em `outputs/oni-novo-05out/video/13_oboro.mp4`.

Windows e avaliação humana não foram repetidos nessa mudança. Logs e tabelas em `outputs/oni-novo-05out/`.


## Aumento de resistência dos bosses (6 de outubro)

Pedido aplicado: primeiros oito aprendizes +20%; últimos quatro aprendizes (Suiren, Karasu, Yoru e Jinshi) +30%; Oboro +40% no total, distribuído proporcionalmente entre as três fases. A barra dos adversários é postura: os valores foram aumentados nela. Vida de Kojiro, janelas, dano de postura dos parrys, padrões, hitstop e sprites não mudaram.

Oboro: fases **360 → 504**, **1800 → 2520**, **320 → 448**; total **2480 → 3472**. Na fase Oni, o bom agora custa **15** de vida (4 normais + 11 adicionais = 4,4% de 250); perfeito continua custando 5, sem cura, e erro 62,5. O jogo real confirmou descontos de 15 nos bons.

Antes/depois: 10 mil lutas por mestre e robô, mesmos parâmetros e sementes, aperto em ms exato. Resultados idênticos a 60 e 144 Hz.

| Mestre | Postura antes → depois | Casual antes → depois (%) |
|---|---|---|
| daichi | 300 → 360 | 100.0 → 100.0 |
| genbu | 330 → 396 | 100.0 → 100.0 |
| hayate | 300 → 360 | 100.0 → 100.0 |
| shizuku | 380 → 456 | 100.0 → 100.0 |
| enjin | 530 → 636 | 99.2 → 98.6 |
| arashi | 400 → 480 | 91.4 → 86.0 |
| raizo | 460 → 552 | 84.7 → 77.9 |
| garfiel | 1350 → 1620 | 80.1 → 71.1 |
| suiren | 840 → 1092 | 71.2 → 55.8 |
| karasu | 780 → 1014 | 64.6 → 46.5 |
| yoru | 970 → 1261 | 58.6 → 41.9 |
| jinshi | 630 → 819 | 55.4 → 37.3 |
| oboro | 2480 → 3472 | 38.3 → 24.8 |


O robô perfeito vence todos; spam perde de todos. A curva casual continua crescente em dificuldade. A verificação independente de ordem em 100 mil lutas por mestre mediu Jinshi em **37,17%** e Oboro em **24,53%**. O aumento de resistência pedido torna as lutas mais longas e reduz essas taxas; são estimativas dos robôs.

As referências antigas (Jinshi 52–58%, Oboro 37–43%) ficaram incompatíveis com o novo pedido. As novas faixas de regressão ficam em 34–40% e 22–28%; as demais acompanham a tabela medida com margem de cerca de 3 pontos. Essas faixas documentam a nova configuração, não uma avaliação humana de dificuldade. O teste mantém a ordem, o degrau mínimo de 3 pontos, perfeito 100%, spam 0% e viabilidade dos golpes. O próprio teste do verificador continua provando que detecta pico de reação, degrau insuficiente e derrota do perfeito.

A luta mais longa expôs um erro de medição de ritmo: na semente 1025, o verificador contava o brilho e o som do mesmo aviso duas vezes e incluía a cena do selo no segundo. Passou a medir apenas o aviso visual; o limite de 3 segundos fora da cena permanece. Nenhum tempo do jogo foi alterado nessa correção.

Validação: 20.964 verificações do núcleo; 36.170 de apresentação sem assets; 215.038 com PNGs reais no Mac e no Linux; zero falhas. Fuzz com 600 cenários em cada um de quatro modos: zero violações. Save, fonte, entrada, desempenho, ritmo, taxas de quadros, curva e compilação com avisos como erro passaram. Windows e sensação ao jogar não foram repetidos. Logs e tabelas completos: `outputs/postura-06out/`.

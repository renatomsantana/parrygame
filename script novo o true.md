# APARA — Documento Técnico, Narrativa & Especificação do Projeto (v0.5.0)

Sistema de duelo de precisão e parry de katana, implementado para **RPG Maker MZ/MV** (com recursos RTP nativos e diálogos) e replicável em **Unity 2D** (usando as mesmas folhas de sprite clássicas e regras mecânicas).

---

## 1. Premissa Narrativa & Motivação

### O Protagonista: Ren, o Guardião da Lâmina Inquebrável
* **Motivação:** O mestre de Ren foi traído e desarmado pela *Liga dos Mestres Dissidentes*, uma coalizão que distorceu os caminhos marciais para controlar os setores da sociedade moderna/fantástica (o submundo noturno, os latifúndios, as finanças corporativas, as artes eruditas e as arenas nobres). Cada um roubou uma das sete gemas da empunhadura sagrada. Para restaurar o legado do seu mestre e impedir o desfecho orquestrado pelo Mestre Supremo (Boss Final), Ren precisa desafiar cada mestre em seu próprio terreno ("Dojo Moderno"), quebrando sua postura e provando a pureza da técnica sobre os truques sujos de cada ambiente.

---

## 2. A Trilha dos 7 Mestres (Progressão, Fintas e Dificuldade)

Cada chefe possui um cenário característico, diálogos breves pré e pós-batalha, mecânicas progressivas de finta (ataques falsificados/atrasados) e janelas de parry cada vez mais implacáveis.

| Fase / Chefe | Conceito & Cenário | Janela Perfeita / Boa | Mecânica Especial & Fintas | Sprite RTP (Inimigo) |
|---|---|---|---|---|
| **1. Mestre do Dojo Tradicional (Gorou)** | Dojo clássico de tatame e bambu. O guardião do limiar. | 90ms / 220ms (Fácil) | Ataques rítmicos diretos, sem fintas. Serve de tutorial prático. | `Actor3_4` / `People4_2` |
| **2. O Campeão da Balada (Neon Jax)** | Clube noturno rave underground com luzes estroboscópicas. | 80ms / 200ms | **Ataque Sincopado:** O ritmo quebra com a batida da música. Ele ameaça pular e atrasa a descida. | `Actor2_3` / `Evil1` |
| **3. O Titã do Campo (Cavan)** | Campo aberto/celeiro rústico ao entardecer. Usa foice/lâmina pesada. | 75ms / 190ms | **Finta Pesada:** Falsa partida lenta seguida de golpe com delay inesperado. | `Actor2_1` / `People1_4` |
| **4. O Executivo S.A. (Vance)** | Escritório corporativo no topo de um arranha-céu. Guarda-chuva/espada oculta. | 70ms / 180ms | **Finta Dupla & Guarda-Chuva:** Ele abre a lâmina falsa para forçar o parry antecipado e pune cliques afobados. | `Actor1_7` / `People3_1` |
| **5. O Pintor Visionário (Kaelen)** | Galeria de arte surrealista com telas rasgadas e tintas brilhantes. | 65ms / 170ms | **Cortes Cegantes:** Manchas de tinta distorcem a silhueta da lâmina antes da estocada. | `Actor3_6` / `People2_3` |
| **6. A Grã-Duquesa Esgrimista (Eleonor)** | Salão de baile nobre/mansão imperial com espelhos. Florete afiado. | 60ms / 160ms | **Finta Tripla & Estocadas Rápidas:** Avanço instantâneo com delay variável de fração de frame. | `Actor1_5` / `Actor2_8` |
| **7. A Sombra / O Reflexo Espelhado (Ren Negro)** | O Jardim de Vidro / Altar da Alma. Cópia idêntica do protagonista. | 50ms / 140ms (Extremo) | **Mímica & Cancelamento:** Conhece o seu tempo de parry; finge o próprio parry e ataca no intervalo de cooldown. | `Actor1_3` (paleta invertida) |
| **8. Boss Final** | *A ser definido na próxima fase do projeto.* | Customizado | Ataques compostos, troca de postura e fases múltiplas. | A definir |

---

## 3. Roteiro dos Diálogos Breves (RTP Show Text)

### Chefe 1: O Mestre do Dojo (Gorou)
* **Antes:**
  * `Gorou:` "Vejo a lâmina do seu mestre em suas mãos, rapaz. Mas sem calma de espírito, ela é apenas ferro frio. Mostre-me se você sabe esperar a hora certa."
  * `Ren:` "Não vim buscar conselhos, Gorou. Vim buscar o que pertence ao templo."
* **Depois da Derrota:**
  * `Gorou:` "Incrível... o som da lâmina desviando... Você ainda se lembra da verdadeira essência. Vá em frente."

### Chefe 2: O Vilão da Balada (Neon Jax)
* **Antes:**
  * `Neon Jax:` "Olha só quem invadiu a pista! Desliga essa postura séria, ronin. Aqui a lâmina dança no ritmo do grave!"
  * `Ren:` "Sua música é alta demais para esconder o som do seu medo."
* **Depois da Derrota:**
  * `Neon Jax:` "Que batida foi essa...? O cara cortou no compasso perfeito..."

### Chefe 3: O Titã do Campo (Cavan)
* **Antes:**
  * `Cavan:` "Gente da cidade pensa que lutar é arte de salão. Na colheita, você aprende a cortar de verdade. Prepare-se para ser ceifado!"
  * `Ren:` "Sua foice é pesada, Cavan. Mas quanto mais pesada a lâmina, maior a queda."
* **Depois da Derrota:**
  * `Cavan:` "Minhas mãos... calejadas por nada... Você é firme como carvalho, garoto."

### Chefe 4: O Executivo S.A. (Vance)
* **Antes:**
  * `Vance:` "Tempo é dinheiro, Ren. Sua cruzada pessoal não tem liquidez alguma. Vamos encerrar este contrato aqui mesmo."
  * `Ren:` "Seus lucros acabam no fio da minha espada, Vance."
* **Depois da Derrota:**
  * `Vance:` "Uma rescisão... violenta demais para o meu gosto..."

### Chefe 5: O Pintor Visionário (Kaelen)
* **Antes:**
  * `Kaelen:` "A vida é uma tela em branco entediante. O sangue que espirrar da sua guarda será o meu tom de vermelho favorito!"
  * `Ren:` "Sua arte não passa de decadência disfarçada. Vou quebrar a sua ilusão."
* **Depois da Derrota:**
  * `Kaelen:` "Que composição primorosa... a faísca do seu bloqueio foi... sublime..."

### Chefe 6: A Esgrimista (Eleonor)
* **Antes:**
  * `Eleonor:` "A katana é uma relíquia bárbara e sem precisão. O florete é uma agulha que costura o destino. Dê o primeiro passo, se tiver coragem."
  * `Ren:` "Não importa a finura do aço quando a mão que o segura hesita."
* **Depois da Derrota:**
  * `Eleonor:` "Como minha ponta pôde ser desviada três vezes seguidas...?! Impossível!"

### Chefe 7: A Sombra (Reflexo de Ren)
* **Antes:**
  * `Sombra:` "Você derrotou todos eles e acha que está purificando o caminho? Olhe para você. O mesmo casaco, a mesma frieza. Eu sou o que resta de você quando a espada não tiver mais quem cortar."
  * `Ren:` "Você é apenas a dúvida que deixei para trás no tatame. Vou cortar meu próprio reflexo se for preciso."
* **Depois da Derrota:**
  * `Sombra:` "Se você hesitar contra ele... eu retornarei..."

---

## 4. Efeitos Sonoros, Flashes e Animações RTP Sugeridas

*(Marcados para fácil substituição por SFX customizados no futuro)*

* **Aviso de Ataque / Finta (Cue):** `Wind7` (pitch 120-140) ou `Flash1`.
* **Parry Perfeito:**
  * *Áudio:* `Parry` ou `Sword5` (pitch 120) + `Damage5` sincronizado.
  * *Visual:* Flash branco total na tela (`$gameScreen.startFlash([255, 255, 255, 220], 8)`) + Animação RTP `SlashLight` / `HitPhysical`.
* **Parry Bom (Bloqueio Comum):**
  * *Áudio:* `Iron1` ou `Guard`.
  * *Visual:* Faísca sutil amarelada (`$gameScreen.startFlash([255, 200, 50, 100], 6)`).
* **Falha / Dano Sofrido:**
  * *Áudio:* `Blow3` ou `Damage3` (pitch 90).
  * *Visual:* Shake de tela (`$gameScreen.startShake(7, 7, 15)`) + Flash vermelho escuro (`$gameScreen.startFlash([255, 0, 0, 180], 18)`).
* **Quebra de Postura (Break / Stun):**
  * *Áudio:* `Thunder2` ou `Collapse1`.
  * *Visual:* Congelamento breve do chefe em frame de Stumble + animação `Stun`.

---

## 5. Implementação no RPG Maker MZ/MV (Plugin Multi-Boss com Fintas)

Arquivo: `js/plugins/AparaDuelCore.js`

```javascript
//=============================================================================
// AparaDuelCore.js - Sistema Multi-Boss com Fintas & Diálogos (Sem IA / RTP)
//=============================================================================
/*:
 * @target MZ
 * @plugindesc Duelos de parry progressivos com fintas, 7 chefes e suporte a diálogos.
 * @author Equipe APARA
 *
 * @param SwitchId
 * @text ID do Switch Ativador do Duelo
 * @type switch
 * @default 10
 *
 * @param BossIdVar
 * @text Variável: ID do Chefe Atual (1 a 7)
 * @type variable
 * @default 20
 *
 * @param PlayerHPVar
 * @text Variável: HP Jogador
 * @type variable
 * @default 21
 *
 * @param BossHPVar
 * @text Variável: HP Chefe
 * @type variable
 * @default 22
 *
 * @help
 * Como Usar:
 * 1. Antes de iniciar o duelo, configure a Variável 20 com o número do chefe (1 a 7).
 * 2. Crie eventos comuns com os diálogos antes de ligar o Switch 10.
 * 3. Ligue o Switch 10. O mini-game assume o controle e resolve a batalha.
 * 4. Ao final, o Switch 10 se desliga e a Variável 22 estará em 0 (vitória) ou 21 estará em 0 (derrota).
 */

(() => {
    const params = PluginManager.parameters("AparaDuelCore");
    const SWITCH_ID = Number(params["SwitchId"] || 10);
    const VAR_BOSS_ID = Number(params["BossIdVar"] || 20);
    const VAR_PLAYER_HP = Number(params["PlayerHPVar"] || 21);
    const VAR_BOSS_HP = Number(params["BossHPVar"] || 22);

    // Definição dos chefes, suas janelas de quadros a 60 FPS e padrões de finta
    const BOSS_PROFILES = {
        1: { name: "Gorou", perfectFrames: 5, goodFrames: 13, windups: [66, 58, 62], feintChance: 0.0 },
        2: { name: "Neon Jax", perfectFrames: 5, goodFrames: 12, windups: [54, 48, 66], feintChance: 0.2 },
        3: { name: "Cavan", perfectFrames: 4, goodFrames: 11, windups: [72, 60, 80], feintChance: 0.3 },
        4: { name: "Vance", perfectFrames: 4, goodFrames: 11, windups: [50, 68, 52], feintChance: 0.4 },
        5: { name: "Kaelen", perfectFrames: 4, goodFrames: 10, windups: [46, 58, 44], feintChance: 0.5 },
        6: { name: "Eleonor", perfectFrames: 3, goodFrames: 9, windups: [42, 50, 38], feintChance: 0.6 },
        7: { name: "Sombra", perfectFrames: 3, goodFrames: 8, windups: [36, 44, 34], feintChance: 0.75 }
    };

    class AparaDuelManager {
        constructor() {
            this.init();
        }

        init() {
            this.playerHp = 100;
            this.bossHp = 100;
            this.bossPosture = 100;
            this.state = 'IDLE'; // IDLE, READY, WINDUP, RECOVERY, FINISHED
            this.frameClock = 0;
            this.strikeFrame = 0;
            this.cueFrame = 0;
            this.lastInputFrame = -999;
            this.hasAttempted = false;
            this.cuePlayed = false;
            this.isFeint = false;
            this.patternIndex = 0;
            this.recoveryDuration = 45;
            this.currentBoss = BOSS_PROFILES[1];
        }

        startDuel(bossId) {
            this.init();
            this.currentBoss = BOSS_PROFILES[bossId] || BOSS_PROFILES[1];
            $gameVariables.setValue(VAR_PLAYER_HP, this.playerHp);
            $gameVariables.setValue(VAR_BOSS_HP, this.bossHp);
            this.scheduleNextAttack(50);
        }

        scheduleNextAttack(delayFrames) {
            this.state = 'READY';
            this.frameClock = 0;
            this.nextAttackDelay = delayFrames;
        }

        startWindup() {
            this.state = 'WINDUP';
            this.frameClock = 0;
            this.hasAttempted = false;
            this.cuePlayed = false;

            let pattern = this.currentBoss.windups[this.patternIndex % this.currentBoss.windups.length];
            this.isFeint = Math.random() < this.currentBoss.feintChance;

            // Se for finta, adiciona um atraso de 18 a 28 frames no final da animação
            let extraDelay = this.isFeint ? Math.floor(Math.random() * 10 + 18) : 0;
            this.strikeFrame = pattern + extraDelay;
            this.cueFrame = Math.max(1, this.strikeFrame - 11);
            this.patternIndex++;
        }

        update() {
            if (this.state === 'IDLE' || this.state === 'FINISHED') return;

            this.frameClock++;

            if (this.state === 'READY') {
                if (this.frameClock >= this.nextAttackDelay) {
                    this.startWindup();
                }
                return;
            }

            if (this.state === 'WINDUP') {
                if (!this.cuePlayed && this.frameClock >= this.cueFrame) {
                    this.cuePlayed = true;
                    // SFX Cue
                    AudioManager.playSe({ name: "Wind7", volume: 65, pitch: this.isFeint ? 160 : 130, pan: 0 });
                }

                if (this.frameClock >= this.strikeFrame) {
                    this.resolveHit();
                }
                return;
            }

            if (this.state === 'RECOVERY') {
                if (this.frameClock >= this.recoveryDuration) {
                    if (this.playerHp <= 0 || this.bossHp <= 0) {
                        this.state = 'FINISHED';
                        $gameSwitches.setValue(SWITCH_ID, false);
                    } else {
                        this.scheduleNextAttack(24);
                    }
                }
            }
        }

        onPlayerPress() {
            if (this.state === 'IDLE' || this.state === 'FINISHED') return;
            // Cooldown de ~19 frames (320ms)
            if (this.frameClock - this.lastInputFrame < 19) return;
            if (this.state === 'WINDUP' && this.hasAttempted) return;

            this.lastInputFrame = this.frameClock;
            if (this.state === 'WINDUP') {
                this.hasAttempted = true;
            }
        }

        resolveHit() {
            this.state = 'RECOVERY';
            this.frameClock = 0;
            this.recoveryDuration = 45;

            let lead = this.hasAttempted ? (this.strikeFrame - this.lastInputFrame) : -1;

            if (this.hasAttempted && lead >= 0 && lead <= this.currentBoss.perfectFrames) {
                // Parry Perfeito
                AudioManager.playSe({ name: "Parry", volume: 95, pitch: 110, pan: 0 });
                $gameScreen.startFlash([255, 255, 255, 220], 8);
                
                this.bossHp = Math.max(0, this.bossHp - 8);
                this.bossPosture = Math.max(0, this.bossPosture - 25);

                if (this.bossPosture <= 0) {
                    // Stun / Break de Postura
                    AudioManager.playSe({ name: "Collapse1", volume: 100, pitch: 90, pan: 0 });
                    this.bossHp = Math.max(0, this.bossHp - 30);
                    this.bossPosture = 100;
                    this.recoveryDuration = 90;
                }
            } else if (this.hasAttempted && lead >= 0 && lead <= this.currentBoss.goodFrames) {
                // Parry Bom (Bloqueio)
                AudioManager.playSe({ name: "Iron1", volume: 75, pitch: 100, pan: 0 });
                $gameScreen.startFlash([255, 200, 50, 100], 6);
            } else {
                // Falha / Golpe sofrido
                AudioManager.playSe({ name: "Blow3", volume: 85, pitch: 100, pan: 0 });
                this.playerHp = Math.max(0, this.playerHp - 25);
                $gameScreen.startShake(7, 7, 15);
                $gameScreen.startFlash([255, 0, 0, 180], 18);
            }

            $gameVariables.setValue(VAR_PLAYER_HP, this.playerHp);
            $gameVariables.setValue(VAR_BOSS_HP, this.bossHp);
        }
    }

    const duel = new AparaDuelManager();

    const _Scene_Map_update = Scene_Map.prototype.update;
    Scene_Map.prototype.update = function() {
        _Scene_Map_update.call(this);

        if ($gameSwitches.value(SWITCH_ID)) {
            if (duel.state === 'IDLE' || duel.state === 'FINISHED') {
                const bossId = $gameVariables.value(VAR_BOSS_ID) || 1;
                duel.startDuel(bossId);
            }

            duel.update();

            if (Input.isTriggered("ok") || TouchInput.isTriggered()) {
                duel.onPlayerPress();
            }
        }
    };
})();
```

---

## 6. Implementação em Unity 2022+ / 6 (C# com Fintas e Dificuldade)

Arquivo: `CombatCore.cs`

```csharp
using System;
using UnityEngine;

[System.Serializable]
public struct BossProfile
{
    public string name;
    public float perfectWindow;
    public float goodWindow;
    public float[] windups;
    public float feintChance;
}

public class CombatCore
{
    public enum Phase { Ready, Windup, Recovery, Finished }

    public int playerHealth = 100;
    public int bossHealth = 100;
    public int bossStability = 100;
    public float inputCooldown = 0.320f;
    public float cueLead = 0.180f;
    public float recoveryTime = 0.75f;
    public float breakRecovery = 1.50f;

    public BossProfile currentBoss;
    public Phase CurrentPhase { get; private set; } = Phase.Ready;
    public float Clock { get; private set; } = 0f;
    public float PhaseEnd { get; private set; } = 0.65f;
    public float StrikeAt { get; private set; } = 0f;
    private float lastPress = -100f;
    private bool attempted = false;
    private bool cuePlayed = false;
    private int attackIndex = 0;
    private bool isFeint = false;

    public Action<float, bool> OnWindupStarted;
    public Action OnCue;
    public Action OnParryPressed;
    public Action<string, float, bool> OnImpact;
    public Action<bool> OnFinished;

    public void SetBoss(BossProfile profile)
    {
        currentBoss = profile;
        playerHealth = 100;
        bossHealth = 100;
        bossStability = 100;
        CurrentPhase = Phase.Ready;
        Clock = 0f;
        PhaseEnd = 0.8f;
    }

    public void Tick(float delta)
    {
        if (CurrentPhase == Phase.Finished) return;

        Clock += Mathf.Max(0f, delta);

        if (CurrentPhase == Phase.Ready || CurrentPhase == Phase.Recovery)
        {
            if (Clock >= PhaseEnd)
                BeginAttack();
            return;
        }

        if (CurrentPhase == Phase.Windup)
        {
            if (!cuePlayed && Clock >= StrikeAt - cueLead)
            {
                cuePlayed = true;
                OnCue?.Invoke();
            }
            if (Clock >= StrikeAt)
            {
                Resolve();
            }
        }
    }

    public bool Press()
    {
        if (CurrentPhase == Phase.Finished) return false;
        if (Clock - lastPress < inputCooldown) return false;
        if (CurrentPhase == Phase.Windup && attempted) return false;

        lastPress = Clock;
        if (CurrentPhase == Phase.Windup)
            attempted = true;

        OnParryPressed?.Invoke();
        return true;
    }

    private void BeginAttack()
    {
        CurrentPhase = Phase.Windup;
        attempted = false;
        cuePlayed = false;

        float baseDuration = currentBoss.windups[attackIndex % currentBoss.windups.Length];
        isFeint = UnityEngine.Random.value < currentBoss.feintChance;
        float delay = isFeint ? UnityEngine.Random.Range(0.20f, 0.40f) : 0f;

        StrikeAt = Clock + baseDuration + delay;
        attackIndex++;
        OnWindupStarted?.Invoke(baseDuration + delay, isFeint);
    }

    private void Resolve()
    {
        CurrentPhase = Phase.Recovery;
        PhaseEnd = Clock + recoveryTime;

        float lead = attempted ? (StrikeAt - lastPress) : -1f;
        string result = "ruim";
        bool broke = false;

        if (attempted && lead >= 0f && lead <= currentBoss.perfectWindow + 0.0001f)
        {
            result = "perfeito";
            bossHealth = Mathf.Max(0, bossHealth - 8);
            bossStability = Mathf.Max(0, bossStability - 25);
            if (bossStability <= 0)
            {
                broke = true;
                bossHealth = Mathf.Max(0, bossHealth - 30);
                bossStability = 100;
                PhaseEnd = Clock + breakRecovery;
            }
        }
        else if (attempted && lead >= 0f && lead <= currentBoss.goodWindow + 0.0001f)
        {
            result = "bom";
        }
        else
        {
            result = "ruim";
            playerHealth = Mathf.Max(0, playerHealth - 25);
        }

        OnImpact?.Invoke(result, lead, broke);

        if (bossHealth <= 0 || playerHealth <= 0)
        {
            CurrentPhase = Phase.Finished;
            OnFinished?.Invoke(bossHealth <= 0);
        }
    }
}
```

---

## 7. Estrutura de Pastas do Repositório

```text
/
├── assets/
│   ├── audio/           # SFX clássicos do RTP (Wind7, Parry, Iron1, Blow3, Collapse1)
│   └── sprites/         # SV Battlers do RTP para os 7 chefes (Actor3_4, Actor2_3, etc.)
├── rpgmaker_mz/
│   ├── js/plugins/
│   │   └── AparaDuelCore.js
│   └── data/
└── unity_project/
    └── Assets/
        └── Scripts/
            ├── CombatCore.cs
            └── AparaCombatBridge.cs
```

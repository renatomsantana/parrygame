//=============================================================================
// AparaDuelCore.js - APARA: A Trilha dos Sete Mestres (v0.5.0)
// Duelos de parry de um botão com fintas, sete mestres e falas. Sem IA.
//=============================================================================
/*:
 * @target MZ
 * @plugindesc APARA v0.5.0 - duelo de parry de um botão: sete mestres, fintas por estilo, falas e HUD.
 * @author Equipe APARA
 * @url https://github.com/
 *
 * @param SwitchId
 * @text Switch ativador do duelo
 * @desc Ligar este switch inicia o duelo do mestre indicado na variável de ID. Desliga sozinho ao terminar.
 * @type switch
 * @default 10
 *
 * @param BossIdVar
 * @text Variável: ID do mestre (1 a 8)
 * @type variable
 * @default 20
 *
 * @param PlayerHPVar
 * @text Variável: vida de Ren
 * @type variable
 * @default 21
 *
 * @param BossHPVar
 * @text Variável: vida do mestre
 * @type variable
 * @default 22
 *
 * @param ResultVar
 * @text Variável: resultado (1 vitória, 2 derrota)
 * @type variable
 * @default 23
 *
 * @param PlayerBattler
 * @text SV Battler de Ren
 * @desc Nome em img/sv_actors (sem extensão).
 * @type string
 * @default Actor1_1
 *
 * @param ShowDialogues
 * @text Mostrar falas do plugin
 * @desc Mostra as duas falas antes e a fala depois de cada mestre. Desligue para escrever as suas nos eventos.
 * @type boolean
 * @default true
 *
 * @param ApplyTint
 * @text Aplicar tom do cenário
 * @desc Aplica a tonalidade de tela de cada mestre durante o duelo (cenário provisório até a arte chegar).
 * @type boolean
 * @default true
 *
 * @param Volume
 * @text Volume dos efeitos
 * @type number
 * @min 0
 * @max 100
 * @default 85
 *
 * @command startDuel
 * @text Iniciar duelo
 * @desc Inicia o duelo contra um mestre (1 a 8; o 8 é o Boss Final provisório) e liga o switch ativador.
 *
 * @arg bossId
 * @text Mestre
 * @type number
 * @min 1
 * @max 8
 * @default 1
 *
 * @help
 * ============================================================================
 * APARA: A Trilha dos Sete Mestres
 * ============================================================================
 * Um botão. O mestre prepara o golpe; quem clica perto do contato defende.
 *
 *   Perfeito  até a janela perfeita do mestre   -8 vida, -25 postura dele
 *   Bom       até a janela boa do mestre         bloqueia sem dano
 *   Ruim      antes disso, ou sem defesa         -25 de vida de Ren
 *
 * Quatro perfeitos quebram a postura (+30 de dano). Oito vencem. Quatro
 * erros derrotam Ren. Cada golpe aceita uma tentativa: vale o primeiro
 * clique. Fintas mostram a partida e o sinal em instantes falsos; o contato
 * real chega depois. Clicar no sinal falso gasta a tentativa. O Boss Final
 * (8, provisório) troca de postura, tem três fases por vida e golpes
 * compostos: vários contatos seguidos, um parry por contato.
 *
 * Como usar (um mapa por mestre = um cenário por mestre):
 *   1. Em cada mapa, crie o evento do mestre. No evento, use o comando de
 *      plugin "Iniciar duelo" com o número dele (1 a 8). Ou faça como o
 *      documento: coloque o ID na variável de mestre e ligue o switch.
 *   2. O plugin assume o controle: falas (se ativas), duelo, HUD, resultado.
 *   3. Ao terminar, o switch desliga e a variável de resultado recebe
 *      1 (vitória) ou 2 (derrota). Vida de Ren e do mestre ficam nas
 *      variáveis correspondentes. Ramifique o evento a partir daí:
 *      vitória -> transferir para o mapa do próximo mestre;
 *      derrota -> oferecer tentar de novo.
 *
 * Controles durante o duelo: OK / toque = parry. O menu fica desativado.
 *
 * Sons (RTP): Wind7 sinal (agudo na finta), Parry perfeito, Iron1 bom,
 * Blow3 ruim, Collapse1 quebra de postura. Sprites: SV battlers do RTP,
 * um por mestre, listados em BOSS_ROSTER. Troque pelos seus quando existirem.
 *
 * O núcleo de regras (AparaCore) é idêntico ao do projeto Unity e pode ser
 * testado fora do RPG Maker com Node: node rpgmaker_mz/tests/core_test.js
 */

(function() {
    "use strict";

    //=========================================================================
    // Núcleo puro: as mesmas regras do CombatCore em C#, em segundos.
    //=========================================================================
    var AparaCore = {};

    AparaCore.Settings = function() {
        this.playerHealth = 100;
        this.badDamage = 25;
        this.bossHealth = 100;
        this.bossPosture = 100;
        this.perfectHealthDamage = 8;
        this.perfectPostureDamage = 25;
        this.postureBreakDamage = 30;
        this.inputCooldown = 0.320;
        this.attackLead = 0.220;
        this.cueLead = 0.180;
        this.recovery = 0.800;
        this.breakRecovery = 1.500;
        this.phaseTwoSpeed = 0.90;
        this.feintDelayMin = 0.200;
        this.feintDelayMax = 0.400;
        this.goodHitstop = 0.045;
        this.perfectHitstop = 0.090;
        this.badHitstop = 0.045;
        this.breakHitstop = 0.160;
    };

    /** Gerador determinístico (mulberry32) para sequências reproduzíveis por semente. */
    AparaCore.SeededRandom = function(seed) {
        this._state = (seed >>> 0) || 1;
    };
    AparaCore.SeededRandom.prototype.next = function() {
        var t = (this._state += 0x6D2B79F5) >>> 0;
        t = Math.imul(t ^ (t >>> 15), t | 1);
        t ^= t + Math.imul(t ^ (t >>> 7), t | 61);
        return ((t ^ (t >>> 14)) >>> 0) / 4294967296;
    };

    function make(spec) {
        var p = {
            id: 1, name: "Mestre", title: "", venue: "", special: "",
            perfectWindow: 0.070, goodWindow: 0.180, windups: [1.10, 0.90, 1.25],
            feintChance: 0, falseCues: 1, feintDelayMin: 0, feintDelayMax: 0,
            rhythmJitter: 0, cueVisibility: 1, mimicParry: false,
            arenaAsset: "arena", sheetAsset: "boss", mirrorHero: false,
            svBattler: "Actor3_4", tint: [0, 0, 0, 0],
            stances: [], phases: [], comboWindup: 0.50, comboGap: 0.10, provisional: false,
            intro: [], outro: []
        };
        for (var key in spec) p[key] = spec[key];
        return p;
    }

    /** Postura derivada dos campos do próprio perfil (mestres sem posturas próprias). */
    AparaCore.defaultStance = function(p) {
        return { name: "", perfectWindow: p.perfectWindow, goodWindow: p.goodWindow, windups: p.windups.slice(),
            feintChance: p.feintChance, falseCues: p.falseCues, feintDelayMin: p.feintDelayMin,
            feintDelayMax: p.feintDelayMax, mimicParry: p.mimicParry };
    };

    AparaCore.PREMISE = "O mestre de Ren foi traído pela Liga dos Mestres Dissidentes. Cada um roubou " +
        "uma das sete gemas da empunhadura sagrada. Ren desafia cada mestre em seu próprio terreno.";
    AparaCore.FINAL_NOTE = "O Boss Final ainda é provisório: o roteiro definirá nome, cenário e falas na próxima fase.";

    /** A Trilha dos Sete Mestres. Preparações em segundos (quadros a 60 FPS / 60). */
    AparaCore.createRoster = function() {
        return [
            make({ id: 1, name: "Gorou", title: "Mestre do Dojo Tradicional", venue: "Dojo de tatame e bambu",
                special: "Ataques rítmicos e diretos, sem fintas.",
                perfectWindow: 0.09, goodWindow: 0.22, windups: [66 / 60, 58 / 60, 62 / 60],
                feintChance: 0, falseCues: 0, arenaAsset: "arena_dojo", svBattler: "Actor3_4", tint: [20, 0, -20, 40],
                intro: ["Gorou|Vejo a lâmina do seu mestre em suas mãos, rapaz. Mas sem calma de espírito, ela é apenas ferro frio. Mostre-me se você sabe esperar a hora certa.",
                        "Ren|Não vim buscar conselhos, Gorou. Vim buscar o que pertence ao templo."],
                outro: ["Gorou|Incrível... o som da lâmina desviando... Você ainda se lembra da verdadeira essência. Vá em frente."] }),
            make({ id: 2, name: "Neon Jax", title: "Campeão da Balada", venue: "Rave underground, luzes estroboscópicas",
                special: "Ataque sincopado: o ritmo quebra com a batida; ele ameaça e atrasa a descida.",
                perfectWindow: 0.08, goodWindow: 0.20, windups: [54 / 60, 48 / 60, 66 / 60],
                feintChance: 0.2, falseCues: 1, feintDelayMin: 0.22, feintDelayMax: 0.35, rhythmJitter: 0.12,
                arenaAsset: "arena_balada", svBattler: "Actor2_3", tint: [-30, -40, 40, 0],
                intro: ["Neon Jax|Olha só quem invadiu a pista! Desliga essa postura séria, ronin. Aqui a lâmina dança no ritmo do grave!",
                        "Ren|Sua música é alta demais para esconder o som do seu medo."],
                outro: ["Neon Jax|Que batida foi essa...? O cara cortou no compasso perfeito..."] }),
            make({ id: 3, name: "Cavan", title: "Titã do Campo", venue: "Celeiro rústico ao entardecer",
                special: "Finta pesada: falsa partida lenta seguida de golpe com atraso inesperado.",
                perfectWindow: 0.075, goodWindow: 0.19, windups: [72 / 60, 60 / 60, 80 / 60],
                feintChance: 0.3, falseCues: 1, feintDelayMin: 0.35, feintDelayMax: 0.55,
                arenaAsset: "arena_campo", svBattler: "Actor2_1", tint: [40, 10, -40, 20],
                intro: ["Cavan|Gente da cidade pensa que lutar é arte de salão. Na colheita, você aprende a cortar de verdade. Prepare-se para ser ceifado!",
                        "Ren|Sua foice é pesada, Cavan. Mas quanto mais pesada a lâmina, maior a queda."],
                outro: ["Cavan|Minhas mãos... calejadas por nada... Você é firme como carvalho, garoto."] }),
            make({ id: 4, name: "Vance", title: "Executivo S.A.", venue: "Cobertura corporativa",
                special: "Finta dupla: a lâmina falsa força o parry antecipado e pune cliques afobados.",
                perfectWindow: 0.07, goodWindow: 0.18, windups: [50 / 60, 68 / 60, 52 / 60],
                feintChance: 0.4, falseCues: 2, feintDelayMin: 0.40, feintDelayMax: 0.55,
                arenaAsset: "arena_escritorio", svBattler: "Actor1_7", tint: [-40, -20, 20, 60],
                intro: ["Vance|Tempo é dinheiro, Ren. Sua cruzada pessoal não tem liquidez alguma. Vamos encerrar este contrato aqui mesmo.",
                        "Ren|Seus lucros acabam no fio da minha espada, Vance."],
                outro: ["Vance|Uma rescisão... violenta demais para o meu gosto..."] }),
            make({ id: 5, name: "Kaelen", title: "Pintor Visionário", venue: "Galeria surrealista, telas rasgadas",
                special: "Cortes cegantes: manchas de tinta distorcem a silhueta da lâmina antes da estocada.",
                perfectWindow: 0.065, goodWindow: 0.17, windups: [46 / 60, 58 / 60, 44 / 60],
                feintChance: 0.5, falseCues: 1, feintDelayMin: 0.20, feintDelayMax: 0.40, cueVisibility: 0.35,
                arenaAsset: "arena_galeria", svBattler: "Actor3_6", tint: [30, -30, 30, 0],
                intro: ["Kaelen|A vida é uma tela em branco entediante. O sangue que espirrar da sua guarda será o meu tom de vermelho favorito!",
                        "Ren|Sua arte não passa de decadência disfarçada. Vou quebrar a sua ilusão."],
                outro: ["Kaelen|Que composição primorosa... a faísca do seu bloqueio foi... sublime..."] }),
            make({ id: 6, name: "Eleonor", title: "Grã-Duquesa Esgrimista", venue: "Salão de baile com espelhos",
                special: "Finta tripla e estocadas rápidas: avanço instantâneo com atraso variável.",
                perfectWindow: 0.06, goodWindow: 0.16, windups: [42 / 60, 50 / 60, 38 / 60],
                feintChance: 0.6, falseCues: 3, feintDelayMin: 0.54, feintDelayMax: 0.72,
                arenaAsset: "arena_salao", svBattler: "Actor1_5", tint: [40, 30, -10, 0],
                intro: ["Eleonor|A katana é uma relíquia bárbara e sem precisão. O florete é uma agulha que costura o destino. Dê o primeiro passo, se tiver coragem.",
                        "Ren|Não importa a finura do aço quando a mão que o segura hesita."],
                outro: ["Eleonor|Como minha ponta pôde ser desviada três vezes seguidas...?! Impossível!"] }),
            make({ id: 7, name: "Sombra", title: "O Reflexo de Ren", venue: "Jardim de Vidro, Altar da Alma",
                special: "Mímica e cancelamento: finge o próprio parry e ataca no intervalo de cooldown.",
                perfectWindow: 0.05, goodWindow: 0.14, windups: [36 / 60, 44 / 60, 34 / 60],
                feintChance: 0.75, falseCues: 1, feintDelayMin: 0.30, feintDelayMax: 0.36, mimicParry: true,
                arenaAsset: "arena_jardim", sheetAsset: "hero", mirrorHero: true, svBattler: "Actor1_3", tint: [-60, -60, -40, 120],
                intro: ["Sombra|Você derrotou todos eles e acha que está purificando o caminho? Olhe para você. O mesmo casaco, a mesma frieza. Eu sou o que resta de você quando a espada não tiver mais quem cortar.",
                        "Ren|Você é apenas a dúvida que deixei para trás no tatame. Vou cortar meu próprio reflexo se for preciso."],
                outro: ["Sombra|Se você hesitar contra ele... eu retornarei..."] }),
            // Boss Final (provisório): duas posturas, três fases por vida e golpes compostos.
            make({ id: 8, name: "Mestre Supremo", title: "Líder da Liga Dissidente", venue: "Salão da Liga, sete gemas vazias",
                special: "Ataques compostos, troca de postura e fases múltiplas.", provisional: true,
                perfectWindow: 0.045, goodWindow: 0.13, windups: [0.60, 0.55, 0.65],
                arenaAsset: "arena_liga", svBattler: "Actor2_5", tint: [-20, -40, 20, 30],
                stances: [
                    { name: "Alta", perfectWindow: 0.07, goodWindow: 0.18, windups: [1.05, 0.95, 1.15],
                      feintChance: 0.3, falseCues: 2, feintDelayMin: 0.40, feintDelayMax: 0.55, mimicParry: false },
                    { name: "Baixa", perfectWindow: 0.045, goodWindow: 0.13, windups: [0.60, 0.55, 0.65],
                      feintChance: 0.5, falseCues: 1, feintDelayMin: 0.30, feintDelayMax: 0.36, mimicParry: true }
                ],
                phases: [
                    { name: "Primeira fase", hpFraction: 1.0, speedMultiplier: 1, comboChance: 0, comboStrikes: 1, stanceSwitchEvery: 4 },
                    { name: "Segunda fase", hpFraction: 0.66, speedMultiplier: 1, comboChance: 0.5, comboStrikes: 2, stanceSwitchEvery: 3 },
                    { name: "Terceira fase", hpFraction: 0.34, speedMultiplier: 0.9, comboChance: 0.7, comboStrikes: 3, stanceSwitchEvery: 2 }
                ],
                intro: ["Mestre Supremo|Sete gemas, sete quedas. Você limpou o caminho até mim, e por isso agradeço: ninguém mais ficará entre nós.",
                        "Ren|Não vim agradecer. Vim devolver a empunhadura ao meu mestre."],
                outro: ["Mestre Supremo|A técnica pura... então ela ainda existia..."] })
        ];
    };

    /** Posição na trilha. */
    AparaCore.Campaign = function(roster) {
        this.bosses = roster;
        this.reset();
    };
    AparaCore.Campaign.prototype.reset = function() { this.index = 0; this.completed = false; };
    AparaCore.Campaign.prototype.current = function() { return this.bosses[this.index]; };
    AparaCore.Campaign.prototype.stage = function() { return this.index + 1; };
    AparaCore.Campaign.prototype.total = function() { return this.bosses.length; };
    AparaCore.Campaign.prototype.advance = function() {
        if (this.index + 1 < this.bosses.length) { this.index += 1; return true; }
        this.completed = true;
        return false;
    };

    var Phase = { READY: 0, WINDUP: 1, RECOVERY: 2, FINISHED: 3 };
    AparaCore.Phase = Phase;

    /**
     * Regra pura do duelo. O relógio só avança quando o jogo está ativo. Cada golpe
     * aceita uma tentativa. Uma finta mostra a partida e o sinal em um ou mais
     * instantes falsos; o contato real chega depois. Eventos: windupStarted(duration,
     * feint), feintStarted(), attackStarted(), cue(feint), parryPressed(),
     * impact(result, lead, broke), finished(victory).
     */
    AparaCore.Combat = function(settings, profile, random) {
        this.settings = settings;
        this.random = random || new AparaCore.SeededRandom(Date.now() & 0x7fffffff);
        this.listeners = {};
        this.setBoss(profile || make({}));
    };
    AparaCore.Combat.prototype.on = function(name, fn) {
        (this.listeners[name] = this.listeners[name] || []).push(fn);
        return this;
    };
    AparaCore.Combat.prototype.emit = function(name, a, b, c) {
        var list = this.listeners[name] || [];
        for (var i = 0; i < list.length; i++) list[i](a, b, c);
    };
    AparaCore.Combat.prototype.setBoss = function(profile) {
        this.boss = profile;
        this._defaultStance = AparaCore.defaultStance(profile);
        this.reset();
    };
    /** Postura em vigor: a do mestre, ou a derivada do perfil quando ele não troca. */
    AparaCore.Combat.prototype.rules = function() {
        var st = this.boss.stances;
        return st.length > 0 ? st[Math.min(this.stanceIndex, st.length - 1)] : this._defaultStance;
    };
    /** Fase em vigor, ou null para mestres sem fases próprias. */
    AparaCore.Combat.prototype.currentRule = function() {
        var ph = this.boss.phases;
        return ph.length > 0 ? ph[Math.min(this.phaseIndex, ph.length - 1)] : null;
    };
    AparaCore.Combat.prototype.perfectWindow = function() { return this.rules().perfectWindow; };
    AparaCore.Combat.prototype.goodWindow = function() { return this.rules().goodWindow; };
    AparaCore.Combat.prototype.inCombo = function() { return this.comboRemaining > 0 || this.comboStrike > 0; };
    AparaCore.Combat.prototype.reset = function() {
        this.clock = 0;
        this.phase = Phase.READY;
        this.phaseEnd = 0.65;
        this.strikeAt = 0;
        this.fakeStrikeAt = 0;
        this.fakeStrikeAts = [];
        this.windupDuration = 1;
        this.isFeint = false;
        this.lastPress = -100;
        this.attempted = false;
        this.attackLaunched = false;
        this.feintLaunched = false;
        this.cuePlayed = false;
        this.fakeCuePlayed = false;
        this.playerHp = this.settings.playerHealth;
        this.bossHp = this.settings.bossHealth;
        this.bossStability = this.settings.bossPosture;
        this.attacks = 0;
        this.feints = 0;
        this.perfectCount = 0;
        this.goodCount = 0;
        this.badCount = 0;
        this.secondPhase = false;
        this.stanceIndex = 0;
        this.phaseIndex = 0;
        this.comboRemaining = 0;
        this.comboStrike = 0;
        this._schedule = [];
        this._scheduleIndex = 0;
    };
    AparaCore.Combat.prototype.tick = function(delta) {
        if (this.phase === Phase.FINISHED) return;
        this.clock += Math.max(0, delta);
        if (this.phase === Phase.READY || this.phase === Phase.RECOVERY) {
            if (this.clock >= this.phaseEnd) this._beginAttack();
            return;
        }
        if (this.phase !== Phase.WINDUP) return;
        // Os eventos disparam na ordem do tempo, mesmo num passo grande.
        while (this._scheduleIndex < this._schedule.length && this.clock >= this._schedule[this._scheduleIndex].time) {
            this._fire(this._schedule[this._scheduleIndex].kind);
            this._scheduleIndex++;
        }
        if (this.clock >= this.strikeAt) this._resolve();
    };
    AparaCore.Combat.prototype._fire = function(kind) {
        switch (kind) {
            case "fakeLaunch": this.feintLaunched = true; this.emit("feintStarted"); break;
            case "fakeCue": this.fakeCuePlayed = true; this.emit("cue", true); break;
            case "launch": this.attackLaunched = true; this.emit("attackStarted"); break;
            case "realCue": this.cuePlayed = true; this.emit("cue", false); break;
        }
    };
    AparaCore.Combat.prototype.press = function() {
        if (this.phase === Phase.FINISHED) return false;
        if (this.clock - this.lastPress < this.settings.inputCooldown) return false;
        if (this.phase === Phase.WINDUP && this.attempted) return false;
        this.lastPress = this.clock;
        if (this.phase === Phase.WINDUP) this.attempted = true;
        this.emit("parryPressed");
        return true;
    };
    /** Segundos até o contato real, ou -1 fora da preparação. */
    AparaCore.Combat.prototype.timeToImpact = function() {
        if (this.phase !== Phase.WINDUP) return -1;
        return Math.max(0, this.strikeAt - this.clock);
    };
    /** Segundos até o próximo instante mostrado (falso ou real): o que a tela deve usar. */
    AparaCore.Combat.prototype.timeToNextInstant = function() {
        if (this.phase !== Phase.WINDUP) return -1;
        var next = this.strikeAt;
        for (var i = 0; i < this.fakeStrikeAts.length; i++) {
            var t = this.fakeStrikeAts[i];
            if (t >= this.clock && t < next) next = t;
        }
        return Math.max(0, next - this.clock);
    };
    AparaCore.Combat.prototype._updatePhase = function() {
        // A fase mais avançada cujo limite de vida já foi alcançado. Nunca volta.
        var target = this.phaseIndex;
        for (var i = this.phaseIndex + 1; i < this.boss.phases.length; i++) {
            if (this.bossHp <= this.boss.phases[i].hpFraction * this.settings.bossHealth) target = i;
        }
        if (target !== this.phaseIndex) {
            this.phaseIndex = target;
            this.emit("phaseChanged", this.phaseIndex, this.boss.phases[this.phaseIndex].name);
        }
    };
    AparaCore.Combat.prototype._beginAttack = function() {
        var s = this.settings, b = this.boss;
        this.phase = Phase.WINDUP;
        if (this.bossStability === 0) this.bossStability = s.bossPosture;
        this.attempted = false;
        this.attackLaunched = false;
        this.feintLaunched = false;
        this.cuePlayed = false;
        this.fakeCuePlayed = false;
        var continuing = this.comboRemaining > 0;
        if (continuing) { this.comboRemaining -= 1; this.comboStrike += 1; }
        else { this.comboStrike = 0; this._updatePhase(); }
        var rule = this.currentRule();
        if (!continuing && rule && rule.stanceSwitchEvery > 0 && b.stances.length > 1 &&
            this.attacks > 0 && this.attacks % rule.stanceSwitchEvery === 0) {
            this.stanceIndex = (this.stanceIndex + 1) % b.stances.length;
            this.emit("stanceChanged", this.stanceIndex, b.stances[this.stanceIndex].name);
        }
        var stance = this.rules();
        var duration;
        if (continuing) {
            // Golpe seguinte de um composto: preparação curta e sem finta.
            duration = b.comboWindup;
        } else {
            duration = stance.windups[this.attacks % stance.windups.length];
            if (b.rhythmJitter > 0) duration += (this.random.next() * 2 - 1) * b.rhythmJitter;
        }
        if (rule) {
            duration *= rule.speedMultiplier;
        } else {
            this.secondPhase = this.bossHp <= s.bossHealth / 2;
            if (this.secondPhase) duration *= s.phaseTwoSpeed;
        }
        this.windupDuration = Math.max(duration, s.attackLead + 0.1);
        var delay = 0, cues = 0;
        this.isFeint = false;
        if (!continuing) {
            var roll = this.random.next();
            this.isFeint = stance.feintChance > 0 && stance.falseCues > 0 && roll < stance.feintChance;
            if (this.isFeint) {
                var min = stance.feintDelayMax > 0 ? stance.feintDelayMin : s.feintDelayMin;
                var max = stance.feintDelayMax > 0 ? stance.feintDelayMax : s.feintDelayMax;
                delay = min + this.random.next() * (max - min);
                cues = stance.falseCues;
                this.feints += 1;
            }
            if (rule && rule.comboStrikes > 1 && rule.comboChance > 0 && this.random.next() < rule.comboChance) {
                this.comboRemaining = rule.comboStrikes - 1;
                this.emit("comboStarted", rule.comboStrikes);
            }
        }
        var first = this.clock + this.windupDuration;
        this.strikeAt = first + delay;
        this.fakeStrikeAts = [];
        for (var i = 0; i < cues; i++) this.fakeStrikeAts.push(first + delay * i / cues);
        this.fakeStrikeAt = cues > 0 ? this.fakeStrikeAts[0] : this.strikeAt;
        this.attacks += 1;
        this._buildSchedule();
        this.emit("windupStarted", this.windupDuration, this.isFeint);
    };
    AparaCore.Combat.prototype._buildSchedule = function() {
        var s = this.settings, list = [];
        for (var i = 0; i < this.fakeStrikeAts.length; i++) {
            list.push({ time: this.fakeStrikeAts[i] - s.attackLead, kind: "fakeLaunch", order: list.length });
            list.push({ time: this.fakeStrikeAts[i] - s.cueLead, kind: "fakeCue", order: list.length });
        }
        list.push({ time: this.strikeAt - s.attackLead, kind: "launch", order: list.length });
        list.push({ time: this.strikeAt - s.cueLead, kind: "realCue", order: list.length });
        list.sort(function(a, b) { return a.time - b.time || a.order - b.order; });
        this._schedule = list;
        this._scheduleIndex = 0;
    };
    AparaCore.Combat.prototype._resolve = function() {
        var s = this.settings, b = this.boss;
        // Consumir o golpe antes dos eventos impede dano duplicado/reentrância.
        this.phase = Phase.RECOVERY;
        this.phaseEnd = this.clock + (this.comboRemaining > 0 ? b.comboGap : s.recovery);
        var stance = this.rules();
        var lead = this.attempted ? this.strikeAt - this.lastPress : -1;
        var result = "ruim", broke = false;
        if (this.attempted && lead >= 0 && lead <= stance.perfectWindow + 0.000001) {
            result = "perfeito";
            this.perfectCount += 1;
            this.bossHp = Math.max(0, this.bossHp - s.perfectHealthDamage);
            this.bossStability = Math.max(0, this.bossStability - s.perfectPostureDamage);
            if (this.bossStability === 0) {
                broke = true;
                this.bossHp = Math.max(0, this.bossHp - s.postureBreakDamage);
                this.phaseEnd = this.clock + s.breakRecovery;
                // A quebra de postura interrompe o golpe composto.
                this.comboRemaining = 0;
            }
        } else if (this.attempted && lead >= 0 && lead <= stance.goodWindow + 0.000001) {
            result = "bom";
            this.goodCount += 1;
        } else {
            this.badCount += 1;
            this.playerHp = Math.max(0, this.playerHp - s.badDamage);
        }
        this.emit("impact", result, lead, broke);
        if (this.bossHp <= 0 || this.playerHp <= 0) {
            this.phase = Phase.FINISHED;
            this.comboRemaining = 0;
            this.emit("finished", this.bossHp <= 0);
        }
    };

    // Exposição: global no jogo, módulo no Node (para os testes).
    if (typeof window !== "undefined") window.AparaCore = AparaCore;
    if (typeof module !== "undefined" && module.exports) module.exports = AparaCore;

    // Fora do RPG Maker (testes em Node), o plugin termina aqui.
    if (typeof PluginManager === "undefined") return;

    //=========================================================================
    // Camada RPG Maker MZ: parâmetros, comandos, cena, sprites, HUD e falas.
    //=========================================================================
    var PLUGIN = "AparaDuelCore";
    var params = PluginManager.parameters(PLUGIN);
    var SWITCH_ID = Number(params.SwitchId || 10);
    var VAR_BOSS_ID = Number(params.BossIdVar || 20);
    var VAR_PLAYER_HP = Number(params.PlayerHPVar || 21);
    var VAR_BOSS_HP = Number(params.BossHPVar || 22);
    var VAR_RESULT = Number(params.ResultVar || 23);
    var PLAYER_BATTLER = String(params.PlayerBattler || "Actor1_1");
    var SHOW_DIALOGUES = String(params.ShowDialogues || "true") === "true";
    var APPLY_TINT = String(params.ApplyTint || "true") === "true";
    var VOLUME = Number(params.Volume || 85);
    var FRAME = 1 / 60;

    PluginManager.registerCommand(PLUGIN, "startDuel", function(args) {
        $gameVariables.setValue(VAR_BOSS_ID, Number(args.bossId || 1));
        $gameSwitches.setValue(SWITCH_ID, true);
    });

    function se(name, pitch, volume) {
        AudioManager.playSe({ name: name, volume: Math.round(VOLUME * (volume === undefined ? 1 : volume)), pitch: pitch || 100, pan: 0 });
    }

    // Motions do SV battler (layout padrão 9 x 6 células de 64 x 64).
    var MOTIONS = { walk: 0, wait: 1, chant: 2, guard: 3, damage: 4, evade: 5, thrust: 6, swing: 7, missile: 8,
        skill: 9, spell: 10, item: 11, escape: 12, victory: 13, dying: 14, abnormal: 15, sleep: 16, dead: 17 };

    /** Sprite de battler com relógio próprio: pausa e hitstop congelam a pose. */
    function Sprite_AparaBattler() { this.initialize.apply(this, arguments); }
    Sprite_AparaBattler.prototype = Object.create(Sprite.prototype);
    Sprite_AparaBattler.prototype.constructor = Sprite_AparaBattler;
    Sprite_AparaBattler.prototype.initialize = function(battlerName, flip) {
        Sprite.prototype.initialize.call(this);
        this.bitmap = ImageManager.loadSvActor(battlerName);
        this.anchor.x = 0.5;
        this.anchor.y = 1;
        this.scale.x = flip ? -2 : 2;
        this.scale.y = 2;
        this._time = 0;
        this._rate = 1;
        this._loop = true;
        this._hold = -1;
        this._motion = MOTIONS.wait;
        this._flash = 0;
        this.setMotion("wait", 1, true);
    };
    /** motion: nome; frames por segundo relativos (1 = 6 fps); loop; hold: frame em que parar (-1 = nenhum). */
    Sprite_AparaBattler.prototype.setMotion = function(name, rate, loop, hold, firstFrame) {
        this._motion = MOTIONS[name] !== undefined ? MOTIONS[name] : MOTIONS.wait;
        this._rate = rate || 1;
        this._loop = !!loop;
        this._hold = hold === undefined ? -1 : hold;
        this._time = (firstFrame || 0) / 6;
        this._show();
    };
    Sprite_AparaBattler.prototype.tick = function(delta) {
        this._time += delta * this._rate;
        this._flash = Math.max(0, this._flash - delta);
        this._show();
    };
    Sprite_AparaBattler.prototype.flash = function(strength) {
        this._flash = 0.10;
        this._flashStrength = strength === undefined ? 1 : strength;
    };
    Sprite_AparaBattler.prototype._show = function() {
        var cursor = this._time * 6;
        var frame = Math.floor(cursor);
        if (this._hold >= 0 && frame > this._hold) frame = this._hold;
        if (frame >= 3) frame = this._loop ? frame % 3 : 2;
        if (!this.bitmap || !this.bitmap.isReady()) return;
        var cw = this.bitmap.width / 9, ch = this.bitmap.height / 6;
        var cx = Math.floor(this._motion / 6) * 3 + frame;
        var cy = this._motion % 6;
        this.setFrame(cx * cw, cy * ch, cw, ch);
        var f = this._flash > 0 ? Math.round(200 * (this._flashStrength || 1)) : 0;
        this.setBlendColor([255, 240, 200, f]);
    };

    /** HUD desenhado num bitmap: barras, nomes, placar, mensagem e sinal. */
    function Sprite_AparaHud() { this.initialize.apply(this, arguments); }
    Sprite_AparaHud.prototype = Object.create(Sprite.prototype);
    Sprite_AparaHud.prototype.constructor = Sprite_AparaHud;
    Sprite_AparaHud.prototype.initialize = function() {
        Sprite.prototype.initialize.call(this, new Bitmap(Graphics.width, Graphics.height));
        this._last = "";
    };
    Sprite_AparaHud.prototype.paint = function(s) {
        var key = JSON.stringify(s);
        if (key === this._last) return;
        this._last = key;
        var b = this.bitmap, w = Graphics.width;
        b.clear();
        b.fontSize = 18;
        b.fillRect(0, 0, w, 70, "rgba(9, 6, 20, 0.86)");
        b.textColor = "#f0a044";
        b.drawText("REN", 20, 8, 200, 24, "left");
        b.textColor = "#f5eddc";
        b.fontSize = 14;
        b.drawText(s.hp + " / " + s.maxHp, 150, 10, 100, 24, "left");
        bar(b, 20, 36, 240, 8, s.hp / s.maxHp, "#f0a044");
        b.fontSize = 14;
        b.drawText(s.bossName.toUpperCase(), w - 280, 8, 200, 24, "left");
        b.fontSize = 11;
        b.textColor = s.bossColor;
        b.drawText(s.bossTitle, w - 280, 10, 260, 24, "right");
        bar(b, w - 280, 36, 260, 8, s.bossHp / s.maxBossHp, s.bossColor);
        b.textColor = "#f5eddc";
        b.drawText("POSTURA", w - 280, 48, 80, 20, "left");
        bar(b, w - 200, 54, 180, 6, s.posture / s.maxPosture, "#73d2de");
        b.fontSize = 22;
        b.drawText("APARA", 0, 4, w, 30, "center");
        b.fontSize = 11;
        b.textColor = "#f0a044";
        b.drawText("MESTRE " + s.stage + " / " + s.stages + " · " + s.venue, 0, 30, w, 20, "center");
        b.textColor = "#f5eddc";
        b.drawText("PERFEITOS  " + String(s.perfects).padZero(2), 20, 48, 200, 20, "left");
        if (s.message) {
            b.fontSize = 26;
            b.textColor = s.messageColor;
            b.drawText(s.message, 0, 120, w, 36, "center");
            b.fontSize = 14;
            b.textColor = "#f5eddc";
            b.drawText(s.detail, 0, 156, w, 24, "center");
        }
        b.fontSize = 14;
        if (s.cueOn) {
            var tint = s.cuePerfect ? "#f5eddc" : "#f0a044";
            b.paintOpacity = Math.round(255 * s.cueVisibility);
            b.fillRect(w / 2 - 6, 200, 12, 12, tint);
            b.paintOpacity = 255;
            b.textColor = tint;
            b.drawText(s.cuePerfect ? "AGORA" : "PREPARE-SE", 0, Graphics.height - 60, w, 24, "center");
        } else {
            b.textColor = "#f5eddc";
            b.drawText(s.special, 0, Graphics.height - 60, w, 24, "center");
        }
        b.fontSize = 12;
        b.drawText("OK / TOQUE · PARRY", 0, Graphics.height - 32, w, 24, "center");
    };
    function bar(b, x, y, w, h, amount, color) {
        b.fillRect(x, y, w, h, "#302b43");
        b.fillRect(x, y, Math.round(w * Math.max(0, Math.min(1, amount))), h, color);
    }

    /**
     * Gerente do duelo: relógio em subpassos de 1/240 s por frame, falas, sprites,
     * HUD, hitstop, flashes e tremor, e a ligação com switch e variáveis.
     */
    function AparaDuelManager() {
        this.settings = new AparaCore.Settings();
        this.roster = AparaCore.createRoster();
        this.combat = new AparaCore.Combat(this.settings, this.roster[0], null);
        this.state = "idle"; // idle, intro, play, result, outro, done
        this.hitstop = 0;
        this.grace = 0;
        this.finishAge = 0;
        this.messageLife = 0;
        this.message = "";
        this.detail = "";
        this.messageColor = "#ffffff";
        this.victory = false;
        this._bind();
    }
    AparaDuelManager.prototype._bind = function() {
        var self = this, c = this.combat;
        c.on("windupStarted", function(duration) {
            self.boss.setMotion("chant", 3 / (6 * Math.max(0.1, duration - self.settings.attackLead)), false, 2);
        });
        c.on("feintStarted", function() {
            if (c.rules().mimicParry) self.boss.setMotion("guard", 3 / (6 * self.settings.attackLead), false, 2);
            else self.boss.setMotion("thrust", 1 / (6 * self.settings.attackLead), false, 1);
            se("Wind7", 115, 0.6);
        });
        c.on("attackStarted", function() {
            self.boss.setMotion("swing", 2 / (6 * self.settings.attackLead), false, -1);
        });
        c.on("cue", function(feint) {
            var v = self.profile.cueVisibility;
            se("Wind7", feint ? 160 : 130, v);
            self.boss.flash(v);
        });
        c.on("parryPressed", function() {
            self.hero.setMotion("guard", 2, false, 2);
            se("Wind7", 90, 0.5);
        });
        c.on("stanceChanged", function(index, name) {
            self.message = "POSTURA " + name.toUpperCase();
            self.detail = index === 0 ? "Lenta e telegrafada" : "Rápida e curta";
            self.messageColor = "#f0a044";
            self.messageLife = 0.9;
            se("Wind7", 80, 0.7);
        });
        c.on("phaseChanged", function(index, name) {
            self.message = name.toUpperCase();
            self.detail = "O mestre muda o jogo";
            self.messageColor = "#ffe4a0";
            self.messageLife = 1.1;
            se("Collapse1", 120, 0.8);
            $gameScreen.startShake(5, 5, 20);
            $gameScreen.startFlash([255, 230, 150, 100], 12);
        });
        c.on("comboStarted", function(strikes) {
            self.message = "GOLPE COMPOSTO ×" + strikes;
            self.detail = "Um parry por contato";
            self.messageColor = "#ec7285";
            self.messageLife = 0.8;
        });
        c.on("impact", function(result, lead, broke) { self._impact(result, lead, broke); });
        c.on("finished", function(won) { self._finished(won); });
    };
    AparaDuelManager.prototype.start = function(scene, bossId) {
        this.profile = this.roster[Math.max(1, Math.min(this.roster.length, bossId)) - 1];
        this.combat.setBoss(this.profile);
        this.scene = scene;
        this.victory = false;
        this.hitstop = 0;
        this.grace = 0.2;
        this.finishAge = 0;
        this.messageLife = 0;
        this.message = "";
        $gameSystem.disableMenu();
        $gameVariables.setValue(VAR_PLAYER_HP, this.combat.playerHp);
        $gameVariables.setValue(VAR_BOSS_HP, this.combat.bossHp);
        $gameVariables.setValue(VAR_RESULT, 0);
        if (APPLY_TINT) $gameScreen.startTint(this.profile.tint, 30);
        this._buildSprites();
        if (SHOW_DIALOGUES) {
            this._say(this.profile.intro);
            this.state = "intro";
        } else {
            this.state = "play";
        }
    };
    AparaDuelManager.prototype._buildSprites = function() {
        var layer = new Sprite();
        var cx = Graphics.width / 2, floor = Math.round(Graphics.height * 0.72);
        this.hero = new Sprite_AparaBattler(PLAYER_BATTLER, false);
        this.hero.x = cx - 110;
        this.hero.y = floor;
        this.boss = new Sprite_AparaBattler(this.profile.svBattler, true);
        this.boss.x = cx + 110;
        this.boss.y = floor;
        this.hud = new Sprite_AparaHud();
        layer.addChild(this.hero);
        layer.addChild(this.boss);
        layer.addChild(this.hud);
        this.layer = layer;
        this.scene.addChild(layer);
    };
    AparaDuelManager.prototype._say = function(lines) {
        for (var i = 0; i < lines.length; i++) {
            var parts = lines[i].split("|");
            var speaker = parts.length > 1 ? parts[0] : "";
            var text = parts.length > 1 ? parts.slice(1).join("|") : parts[0];
            $gameMessage.setSpeakerName(speaker);
            $gameMessage.add(text);
            $gameMessage.newPage();
        }
    };
    AparaDuelManager.prototype.update = function() {
        if (this.state === "idle" || this.state === "done") return;
        if ($gameMessage.isBusy()) return;
        if (this.state === "intro") { this.state = "play"; this.grace = 0.2; return; }
        if (this.state === "outro") { this._end(); return; }
        var delta = FRAME;
        this.hud.paint(this._snapshot());
        if (this.state === "result") {
            this.finishAge += delta;
            this.hero.tick(delta);
            this.boss.tick(delta);
            if (this.finishAge >= 1.2) {
                if (SHOW_DIALOGUES && this.victory) { this._say(this.profile.outro); this.state = "outro"; }
                else this._end();
            }
            return;
        }
        if (this.grace > 0) { this.grace -= delta; return; }
        if (this.hitstop > 0) { this.hitstop -= delta; return; }
        // Subpassos: contato e animação próximos mesmo com um frame inteiro de atraso.
        var pressed = Input.isTriggered("ok") || TouchInput.isTriggered();
        var remaining = delta;
        while (remaining > 0) {
            var step = Math.min(remaining, 1 / 240);
            this.hero.tick(step);
            this.boss.tick(step);
            this.messageLife = Math.max(0, this.messageLife - step);
            this.combat.tick(step);
            remaining -= step;
            if (this.hitstop > 0) break;
        }
        if (pressed && this.state === "play") this.combat.press();
    };
    AparaDuelManager.prototype._impact = function(result, lead, broke) {
        var s = this.settings, self = this;
        this.message = "TIMING " + result.toUpperCase();
        this.messageLife = 0.7;
        if (result === "perfeito") {
            this.hitstop = s.perfectHitstop;
            this.messageColor = "#ffe4a0";
            this.detail = "−" + s.perfectHealthDamage + " vida · −" + s.perfectPostureDamage + " postura do mestre";
            se("Parry", 110);
            $gameScreen.startFlash([255, 255, 255, 220], 8);
            this.hero.setMotion("guard", 1, false, 2, 2);
            this.boss.setMotion("damage", 1, false, 2);
            if (broke) {
                this.message = "POSTURA QUEBRADA";
                this.detail = "Parry perfeito · −" + s.postureBreakDamage + " de vida extra";
                this.messageLife = 1.25;
                this.hitstop = s.breakHitstop;
                se("Collapse1", 90);
                this.boss.setMotion("dying", 0.6, false, 2);
                this.hero.setMotion("swing", 1, false, 2);
            }
        } else if (result === "bom") {
            this.hitstop = s.goodHitstop;
            this.messageColor = "#73d2de";
            this.detail = "Golpe bloqueado";
            se("Iron1", 100);
            $gameScreen.startFlash([255, 200, 50, 100], 6);
            this.hero.setMotion("guard", 1, false, 2, 2);
            this.boss.setMotion("swing", 1, false, 2, 2);
        } else {
            this.hitstop = s.badHitstop;
            this.messageColor = "#ec7285";
            if (lead < 0) this.detail = "Golpe recebido · −" + s.badDamage + " vida";
            else if (this.combat.isFeint) this.detail = "Caiu na finta · −" + s.badDamage + " vida";
            else this.detail = "Muito cedo · −" + s.badDamage + " vida";
            se("Blow3", 100);
            $gameScreen.startShake(7, 7, 15);
            $gameScreen.startFlash([255, 0, 0, 180], 18);
            this.hero.setMotion("damage", 1, false, 2);
            this.boss.setMotion("swing", 1, false, 2, 2);
        }
        $gameVariables.setValue(VAR_PLAYER_HP, this.combat.playerHp);
        $gameVariables.setValue(VAR_BOSS_HP, this.combat.bossHp);
        setTimeout(function() { self.hero.setMotion("wait", 1, true); }, 600);
    };
    AparaDuelManager.prototype._finished = function(won) {
        this.state = "result";
        this.victory = won;
        this.finishAge = 0;
        if (won) { this.boss.setMotion("dead", 1, false, 0); se("Parry", 130); }
        else this.hero.setMotion("dead", 1, false, 0);
    };
    AparaDuelManager.prototype._end = function() {
        this.state = "done";
        $gameVariables.setValue(VAR_RESULT, this.victory ? 1 : 2);
        $gameSwitches.setValue(SWITCH_ID, false);
        $gameSystem.enableMenu();
        if (APPLY_TINT) $gameScreen.startTint([0, 0, 0, 0], 30);
        if (this.layer && this.layer.parent) this.layer.parent.removeChild(this.layer);
        this.layer = null;
        this.state = "idle";
    };
    AparaDuelManager.prototype._snapshot = function() {
        var c = this.combat, p = this.profile, s = this.settings;
        var lead = c.timeToNextInstant();
        var cueOn = lead >= 0 && lead <= s.cueLead;
        return {
            hp: c.playerHp, maxHp: s.playerHealth, bossHp: c.bossHp, maxBossHp: s.bossHealth,
            posture: c.bossStability, maxPosture: s.bossPosture, perfects: c.perfectCount,
            bossName: p.name, bossTitle: p.stances.length > 0 ? p.title + " · " + c.rules().name : p.title,
            bossColor: "#ec7285", venue: p.venue, special: p.special,
            stage: p.id, stages: this.roster.length,
            message: this.messageLife > 0 ? this.message : "", detail: this.detail, messageColor: this.messageColor,
            cueOn: cueOn, cuePerfect: cueOn && lead <= c.perfectWindow(), cueVisibility: p.cueVisibility
        };
    };

    var duel = new AparaDuelManager();
    window.AparaDuel = duel;

    var _Scene_Map_update = Scene_Map.prototype.update;
    Scene_Map.prototype.update = function() {
        _Scene_Map_update.call(this);
        if ($gameSwitches.value(SWITCH_ID)) {
            if (duel.state === "idle") duel.start(this, $gameVariables.value(VAR_BOSS_ID) || 1);
            duel.update();
        }
    };

    // Ao trocar de mapa no meio de um duelo, o duelo é encerrado sem resultado.
    var _Scene_Map_terminate = Scene_Map.prototype.terminate;
    Scene_Map.prototype.terminate = function() {
        if (duel.state !== "idle" && duel.state !== "done") {
            duel.victory = false;
            duel._end();
        }
        _Scene_Map_terminate.call(this);
    };
})();

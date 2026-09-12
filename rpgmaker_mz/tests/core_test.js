// Testes do núcleo do plugin, fora do RPG Maker: node rpgmaker_mz/tests/core_test.js
// Espelham CoreSelfTest.cs do projeto Unity: as regras precisam ser as mesmas.
"use strict";
const path = require("path");
const Core = require(path.join(__dirname, "..", "js", "plugins", "AparaDuelCore.js"));

let checks = 0;
const failures = [];
function check(condition, description) {
    checks++;
    if (!condition) failures.push(description);
}
const approx = (a, b) => Math.abs(a - b) < 0.0005;
const roster = () => Core.createRoster();
const boss = (id) => roster()[id - 1];

function baseProfile() {
    const p = boss(1);
    p.perfectWindow = 0.07; p.goodWindow = 0.18; p.windups = [1.1, 0.9, 1.25, 0.82]; p.feintChance = 0; p.falseCues = 0;
    return p;
}
function noFeint(profile) {
    const copy = Object.assign({}, profile, { windups: profile.windups.slice() });
    copy.feintChance = 0; copy.rhythmJitter = 0;
    copy.stances = profile.stances.map(st => Object.assign({}, st, { windups: st.windups.slice(), feintChance: 0 }));
    copy.phases = profile.phases.map(ph => Object.assign({}, ph, { comboChance: 0 }));
    return copy;
}
function skip(duel) {
    prepare(duel);
    duel.tick(duel.timeToImpact() + 0.001);
    duel.playerHp = 100;
    if (duel.phase === Core.Phase.FINISHED) duel.phase = Core.Phase.RECOVERY;
}
const newDuel = (profile, seed) => new Core.Combat(new Core.Settings(), profile || baseProfile(), new Core.SeededRandom(seed || 7));
function prepare(duel) {
    if (duel.phase !== Core.Phase.WINDUP) duel.tick(Math.max(0, duel.phaseEnd - duel.clock) + 0.00001);
}
function parry(duel, lead) {
    prepare(duel);
    duel.tick(duel.timeToImpact() - lead);
    check(duel.press(), "Primeiro clique deve ser aceito");
    duel.tick(lead + 0.00001);
}
function feintDuel(profile, seed) {
    const duel = newDuel(profile, seed);
    for (let i = 0; i < 40; i++) {
        prepare(duel);
        if (duel.isFeint) return duel;
        duel.tick(duel.timeToImpact() + 0.001);
        duel.playerHp = 100;
        duel.phase = Core.Phase.RECOVERY;
    }
    return duel;
}
function record(duel) {
    const events = [];
    duel.on("feintStarted", () => events.push("finta"));
    duel.on("attackStarted", () => events.push("golpe"));
    duel.on("cue", (feint) => events.push(feint ? "sinal_falso" : "sinal"));
    duel.on("impact", () => events.push("contato"));
    return events;
}

// Limites de timing
for (const lead of [0.001, 0.07, 0.0701, 0.18, 0.1801, 0.5]) {
    const duel = newDuel();
    parry(duel, lead);
    if (lead <= 0.07) check(duel.perfectCount === 1 && duel.bossHp === 92, "Limite perfeito: " + lead);
    else if (lead <= 0.18) check(duel.goodCount === 1 && duel.playerHp === 100, "Limite bom: " + lead);
    else check(duel.badCount === 1 && duel.playerHp === 75, "Limite ruim: " + lead);
}

// Morte
{
    const duel = newDuel();
    for (let i = 0; i < 4; i++) { prepare(duel); duel.tick(duel.timeToImpact() + 0.001); }
    check(duel.playerHp === 0 && duel.phase === Core.Phase.FINISHED, "Quatro erros derrotam o jogador");
    duel.tick(10);
    check(duel.badCount === 4, "Duelo encerrado não aplica novos danos");
}

// Postura, segunda fase e vitória
{
    const duel = newDuel();
    for (let i = 0; i < 4; i++) parry(duel, 0.04);
    check(duel.bossHp === 38 && duel.bossStability === 0, "Quatro perfeitos quebram postura (62 de dano)");
    prepare(duel);
    check(duel.bossStability === 100 && duel.secondPhase, "Nova preparação restaura postura e ativa segunda fase");
    check(approx(duel.windupDuration, 1.1 * 0.9), "Segunda fase prepara 10% mais rápido");
    for (let i = 0; i < 4; i++) parry(duel, 0.04);
    check(duel.bossHp === 0 && duel.playerHp === 100 && duel.phase === Core.Phase.FINISHED, "Oito perfeitos vencem");
}

// Bloqueio bom
{
    const duel = newDuel();
    for (let i = 0; i < 5; i++) parry(duel, 0.12);
    check(duel.playerHp === 100 && duel.bossHp === 100 && duel.bossStability === 100, "Parry bom bloqueia sem danos");
}

// Uma tentativa por golpe
{
    const duel = newDuel();
    prepare(duel);
    duel.tick(duel.timeToImpact() - 0.7);
    check(duel.press(), "Primeira tentativa cedo é registrada");
    duel.tick(0.66);
    check(!duel.press(), "Spam não troca um timing cedo por perfeito");
    duel.tick(0.05);
    check(duel.badCount === 1 && duel.playerHp === 75, "Cada ataque resolve uma vez");
}

// Clique tardio e reinício
{
    const duel = newDuel();
    prepare(duel);
    duel.tick(duel.timeToImpact() + 0.01);
    duel.press();
    check(duel.badCount === 1 && duel.perfectCount === 0, "Clique após contato não recebe crédito");
    duel.reset();
    check(duel.playerHp === 100 && duel.bossHp === 100 && duel.phase === Core.Phase.READY, "Reiniciar limpa tudo");
}

// Trilha e estilos
{
    const bosses = roster();
    const s = new Core.Settings();
    check(bosses.length === 8, "Sete mestres e o Boss Final");
    const arenas = new Set();
    bosses.forEach((b, i) => {
        check(b.id === i + 1, b.name + " id em ordem");
        check(b.intro.length === 2 && b.outro.length === 1 && b.intro.concat(b.outro).every(l => l.includes("|")), b.name + " falas com falante");
        check(b.windups.every(w => w - b.rhythmJitter >= s.attackLead + 0.1), b.name + " preparação cabe a partida");
        check(b.perfectWindow < b.goodWindow, b.name + " janela perfeita dentro da boa");
        check((b.id === 8) === b.provisional, b.name + " só o Boss Final é provisório");
        for (const st of b.stances) {
            check(st.perfectWindow < st.goodWindow && st.windups.every(w => w >= s.attackLead + 0.1), b.name + " postura " + st.name + " válida");
            if (st.feintChance > 0) check(st.feintDelayMin / st.falseCues >= s.cueLead - 0.0001 && st.feintDelayMin > st.goodWindow, b.name + " postura " + st.name + " finta válida");
        }
        check(!arenas.has(b.arenaAsset) && b.arenaAsset.length > 0, b.name + " cenário próprio");
        arenas.add(b.arenaAsset);
        check(b.svBattler.length > 0, b.name + " tem battler RTP");
        if (b.feintChance > 0) {
            check(b.falseCues >= 1 && b.falseCues <= 3, b.name + " de um a três instantes falsos");
            check(b.feintDelayMin / b.falseCues >= s.cueLead - 0.0001, b.name + " instantes falsos afastados um sinal");
            check(b.feintDelayMin > b.goodWindow, b.name + " clique no instante falso nunca cabe na janela boa");
        }
        if (i > 0 && i < 7) {
            const prev = bosses[i - 1];
            check(b.perfectWindow < prev.perfectWindow && b.goodWindow < prev.goodWindow, b.name + " mais exigente que " + prev.name);
            check(b.feintChance >= prev.feintChance, b.name + " finta pelo menos tanto quanto " + prev.name);
        }
    });
    check(bosses[0].falseCues === 0 && bosses[1].rhythmJitter > 0 && bosses[3].falseCues === 2, "Gorou sem finta, Jax sincopado, Vance dupla");
    check(bosses[4].cueVisibility < 1 && bosses[5].falseCues === 3 && bosses[6].mimicParry && bosses[6].mirrorHero, "Kaelen cegante, Eleonor tripla, Sombra mímica");
    check(approx(bosses[0].perfectWindow, 0.09) && approx(bosses[6].goodWindow, 0.14), "Janelas de Gorou e Sombra");
}

// Janelas por mestre (postura inicial no Boss Final)
for (const b of roster()) {
    const p = noFeint(b);
    const perfect = p.stances.length ? p.stances[0].perfectWindow : p.perfectWindow;
    const good = p.stances.length ? p.stances[0].goodWindow : p.goodWindow;
    let duel = newDuel(p); parry(duel, perfect); check(duel.perfectCount === 1, b.name + " limite perfeito inclusivo");
    duel = newDuel(p); parry(duel, perfect + 0.005); check(duel.goodCount === 1, b.name + " logo após o perfeito é bom");
    duel = newDuel(p); parry(duel, good + 0.005); check(duel.badCount === 1, b.name + " fora da boa é ruim");
}

// Boss Final: fases
{
    const final = noFeint(boss(8));
    const duel = newDuel(final, 21);
    const phases = [];
    duel.on("phaseChanged", (i, name) => phases.push(i + ":" + name));
    prepare(duel);
    check(duel.phaseIndex === 0 && approx(duel.windupDuration, final.stances[0].windups[0]), "Boss Final começa na primeira fase e postura");
    for (let i = 0; i < 4; i++) parry(duel, 0.03);
    check(duel.bossHp === 38 && duel.phaseIndex === 0, "A fase só muda na próxima preparação");
    prepare(duel);
    check(duel.phaseIndex === 1 && phases[0] === "1:" + final.phases[1].name && !duel.secondPhase, "Vida a 38% entra na segunda fase");
    parry(duel, 0.03);
    prepare(duel);
    check(duel.phaseIndex === 2 && phases.length === 2, "Vida a 30% entra na terceira fase");
    check(approx(duel.windupDuration, duel.rules().windups[(duel.attacks - 1) % duel.rules().windups.length] * final.phases[2].speedMultiplier), "Terceira fase é mais rápida");
    duel.reset();
    check(duel.phaseIndex === 0 && duel.stanceIndex === 0 && !duel.inCombo(), "Reiniciar volta ao início");
}

// Boss Final: troca de postura
{
    const final = noFeint(boss(8));
    const duel = newDuel(final, 21);
    const stances = [];
    duel.on("stanceChanged", (i, name) => stances.push(i + ":" + name));
    const every = final.phases[0].stanceSwitchEvery;
    for (let i = 0; i < every; i++) { prepare(duel); check(duel.stanceIndex === 0, "Primeiros golpes na postura Alta"); parry(duel, 0.1); }
    prepare(duel);
    check(duel.stanceIndex === 1 && stances[0] === "1:" + final.stances[1].name, "Troca de postura depois de " + every + " golpes");
    check(approx(duel.perfectWindow(), final.stances[1].perfectWindow) && approx(duel.goodWindow(), final.stances[1].goodWindow), "Janelas seguem a nova postura");
    duel.tick(duel.timeToImpact() - 0.16); duel.press(); duel.tick(0.161);
    check(duel.badCount === 1, "160 ms é ruim na postura Baixa");
    for (let i = 0; i < every - 1; i++) skip(duel);
    prepare(duel);
    check(duel.stanceIndex === 0 && stances.length === 2, "A postura volta ao início depois da última");
}

// Boss Final: golpes compostos
{
    const final = noFeint(boss(8));
    final.phases[0].comboChance = 1; final.phases[0].comboStrikes = 3; final.phases[0].stanceSwitchEvery = 0;
    const duel = newDuel(final, 21);
    let announced = 0;
    duel.on("comboStarted", (n) => { announced = n; });
    prepare(duel);
    check(announced === 3 && duel.comboRemaining === 2 && duel.comboStrike === 0 && !duel.isFeint, "Composto de três anunciado, sem finta");
    duel.tick(duel.timeToImpact() - 0.1); check(duel.press(), "Primeiro contato aceita tentativa"); duel.tick(0.101);
    check(duel.goodCount === 1 && approx(duel.phaseEnd - duel.clock, final.comboGap), "Depois do primeiro contato vem a pausa curta");
    prepare(duel);
    check(duel.comboStrike === 1 && approx(duel.windupDuration, final.comboWindup), "Segundo contato usa a preparação curta");
    duel.tick(duel.timeToImpact() - 0.1); check(duel.press(), "Segundo contato aceita nova tentativa"); duel.tick(0.101);
    prepare(duel);
    check(duel.comboStrike === 2 && duel.comboRemaining === 0, "Terceiro contato é o último");
    duel.tick(duel.timeToImpact() + 0.001);
    check(duel.badCount === 1 && approx(duel.phaseEnd - duel.clock, duel.settings.recovery), "Deixar passar o último custa vida e volta à recuperação normal");
    prepare(duel);
    duel.bossStability = 25;
    duel.tick(duel.timeToImpact() - 0.03); duel.press(); duel.tick(0.031);
    check(duel.perfectCount === 1 && duel.comboRemaining === 0 && approx(duel.phaseEnd - duel.clock, duel.settings.breakRecovery), "Quebrar a postura corta o composto");
}

// Gorou nunca finta
{
    const duel = newDuel(boss(1), 123);
    for (let i = 0; i < 30; i++) { prepare(duel); check(!duel.isFeint && duel.fakeStrikeAts.length === 0, "Gorou nunca finta"); parry(duel, 0.1); }
    check(duel.feints === 0 && duel.playerHp === 100, "Trinta golpes sem finta");
}

// Finta simples da Sombra: ordem, punição e perfeito
{
    const sombra = boss(7);
    let duel = feintDuel(sombra, 3);
    check(duel.isFeint, "Sombra finta com a semente");
    const delay = duel.strikeAt - duel.fakeStrikeAt;
    check(delay >= sombra.feintDelayMin - 0.0001 && delay <= sombra.feintDelayMax + 0.0001, "Atraso dentro da faixa da Sombra: " + delay);
    const events = record(duel);
    duel.tick(duel.timeToImpact() + 0.001);
    check(events.join(",") === "finta,sinal_falso,golpe,sinal,contato", "Ordem dos sinais na finta simples: " + events.join(","));

    duel = feintDuel(sombra, 3);
    duel.tick(duel.fakeStrikeAt - duel.settings.cueLead - duel.clock + 0.001);
    check(duel.fakeCuePlayed && duel.press(), "Clique no sinal falso é aceito");
    duel.tick(duel.timeToImpact() - 0.02);
    check(!duel.press(), "Sem segunda tentativa");
    duel.tick(0.05);
    check(duel.badCount === 1 && duel.playerHp === 75, "Cair na finta custa vida");

    duel = feintDuel(sombra, 3);
    duel.tick(duel.timeToImpact() - 0.03);
    check(duel.press(), "Esperar o contato real permite o parry");
    duel.tick(0.031);
    check(duel.perfectCount === 1 && duel.bossHp === 92, "Perfeito na finta");
}

// Fintas dupla e tripla
for (const id of [4, 6]) {
    const b = boss(id);
    const duel = feintDuel(b, 11);
    check(duel.isFeint && duel.fakeStrikeAts.length === b.falseCues, b.name + " finta com " + b.falseCues + " instantes");
    for (let i = 1; i < duel.fakeStrikeAts.length; i++) {
        check(duel.fakeStrikeAts[i] - duel.fakeStrikeAts[i - 1] >= duel.settings.cueLead - 0.0001, b.name + " instantes separados por um sinal");
    }
    const events = record(duel);
    duel.tick(duel.timeToImpact() + 0.001);
    const fakes = events.filter(e => e === "sinal_falso").length;
    check(fakes === b.falseCues && events.lastIndexOf("sinal_falso") < events.indexOf("sinal") && events[events.length - 1] === "contato",
        b.name + " sinais falsos antes do real: " + events.join(","));
}

// Sincopado
{
    const jax = boss(2);
    const duel = newDuel(jax, 5);
    let varied = false;
    for (let i = 0; i < 30; i++) {
        prepare(duel);
        const expected = jax.windups[i % jax.windups.length];
        check(Math.abs(duel.windupDuration - expected) <= jax.rhythmJitter + 0.0001, "Jax dentro do compasso");
        if (Math.abs(duel.windupDuration - expected) > 0.01) varied = true;
        duel.tick(duel.timeToImpact() + 0.001); duel.playerHp = 100; duel.phase = Core.Phase.RECOVERY;
    }
    check(varied, "Jax realmente sai do compasso");
}

// A tela conta até o próximo instante, falso ou real
{
    const duel = feintDuel(boss(4), 11);
    check(approx(duel.timeToNextInstant(), duel.fakeStrikeAts[0] - duel.clock), "Conta até o primeiro instante falso");
    duel.tick(duel.fakeStrikeAts[0] - duel.clock + 0.001);
    check(approx(duel.timeToNextInstant(), duel.fakeStrikeAts[1] - duel.clock), "Depois, até o segundo");
    duel.tick(duel.fakeStrikeAts[1] - duel.clock + 0.001);
    check(approx(duel.timeToNextInstant(), duel.timeToImpact()), "Por fim, até o real");
}

// Trilha
{
    const trail = new Core.Campaign(roster());
    check(trail.stage() === 1 && trail.current().name === "Gorou", "Começa em Gorou");
    for (let i = 0; i < 6; i++) check(trail.advance(), "Avança");
    check(trail.current().name === "Sombra" && trail.advance() && trail.current().provisional, "Depois da Sombra vem o Boss Final provisório");
    check(!trail.advance() && trail.completed, "Termina no Boss Final");
    trail.reset();
    check(trail.stage() === 1 && !trail.completed, "Reinicia");

    const gems = new Core.Campaign(roster());
    check(gems.gems() === 0 && !gems.allGems(), "Trilha nova sem gemas");
    gems.markCleared(0); gems.markCleared(0);
    check(gems.gems() === 1 && gems.isCleared(0) && !gems.isCleared(1), "Vencer Gorou devolve uma gema, uma vez só");
    gems.markCleared(7);
    check(gems.gems() === 1 && !gems.holdsGem(7) && gems.holdsGem(6), "O Boss Final não guarda gema; a Sombra guarda");
    for (let i = 1; i < 7; i++) gems.markCleared(i);
    check(gems.gems() === 7 && gems.allGems(), "Sete mestres vencidos, sete gemas");
    gems.markCleared(99); gems.markCleared(-1);
    check(gems.gems() === 7, "Índices fora da trilha são ignorados");
    gems.reset();
    check(gems.gems() === 7 && gems.stage() === 1, "Reiniciar a posição não apaga as gemas");
}

// Determinismo: mesma semente, mesma sequência
{
    const a = newDuel(boss(7), 99), b = newDuel(boss(7), 99);
    let same = true;
    for (let i = 0; i < 10; i++) {
        prepare(a); prepare(b);
        if (a.isFeint !== b.isFeint || !approx(a.strikeAt, b.strikeAt)) same = false;
        a.tick(a.timeToImpact() + 0.001); b.tick(b.timeToImpact() + 0.001);
        a.playerHp = b.playerHp = 100; a.phase = b.phase = Core.Phase.RECOVERY;
    }
    check(same, "Mesma semente produz a mesma sequência");
}

for (const f of failures) console.error("FALHA: " + f);
console.log("APARA Core (RPG Maker): " + checks + " verificações, " + failures.length + " falhas");
process.exit(failures.length > 0 ? 1 : 0);

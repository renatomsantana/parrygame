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
    return copy;
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
    check(bosses.length === 7, "Sete mestres");
    const arenas = new Set();
    bosses.forEach((b, i) => {
        check(b.id === i + 1, b.name + " id em ordem");
        check(b.intro.length === 2 && b.outro.length === 1 && b.intro.concat(b.outro).every(l => l.includes("|")), b.name + " falas com falante");
        check(b.windups.every(w => w - b.rhythmJitter >= s.attackLead + 0.1), b.name + " preparação cabe a partida");
        check(b.perfectWindow < b.goodWindow, b.name + " janela perfeita dentro da boa");
        check(!arenas.has(b.arenaAsset) && b.arenaAsset.length > 0, b.name + " cenário próprio");
        arenas.add(b.arenaAsset);
        check(b.svBattler.length > 0, b.name + " tem battler RTP");
        if (b.feintChance > 0) {
            check(b.falseCues >= 1 && b.falseCues <= 3, b.name + " de um a três instantes falsos");
            check(b.feintDelayMin / b.falseCues >= s.cueLead - 0.0001, b.name + " instantes falsos afastados um sinal");
            check(b.feintDelayMin > b.goodWindow, b.name + " clique no instante falso nunca cabe na janela boa");
        }
        if (i > 0) {
            const prev = bosses[i - 1];
            check(b.perfectWindow < prev.perfectWindow && b.goodWindow < prev.goodWindow, b.name + " mais exigente que " + prev.name);
            check(b.feintChance >= prev.feintChance, b.name + " finta pelo menos tanto quanto " + prev.name);
        }
    });
    check(bosses[0].falseCues === 0 && bosses[1].rhythmJitter > 0 && bosses[3].falseCues === 2, "Gorou sem finta, Jax sincopado, Vance dupla");
    check(bosses[4].cueVisibility < 1 && bosses[5].falseCues === 3 && bosses[6].mimicParry && bosses[6].mirrorHero, "Kaelen cegante, Eleonor tripla, Sombra mímica");
    check(approx(bosses[0].perfectWindow, 0.09) && approx(bosses[6].goodWindow, 0.14), "Janelas de Gorou e Sombra");
}

// Janelas por mestre
for (const b of roster()) {
    const p = noFeint(b);
    let duel = newDuel(p); parry(duel, p.perfectWindow); check(duel.perfectCount === 1, b.name + " limite perfeito inclusivo");
    duel = newDuel(p); parry(duel, p.perfectWindow + 0.005); check(duel.goodCount === 1, b.name + " logo após o perfeito é bom");
    duel = newDuel(p); parry(duel, p.goodWindow + 0.005); check(duel.badCount === 1, b.name + " fora da boa é ruim");
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
    check(trail.current().name === "Sombra" && !trail.advance() && trail.completed, "Termina na Sombra");
    trail.reset();
    check(trail.stage() === 1 && !trail.completed, "Reinicia");
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

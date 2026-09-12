using System;
using System.Collections.Generic;

namespace Apara.Core
{
    /// <summary>
    /// Verificações das regras, dos mestres, dos estilos de finta, do Boss Final
    /// e da trilha. Não dependem do Unity: rodam no Test Runner (EditMode) e no
    /// harness de console em Tools/.
    /// </summary>
    public class CoreSelfTest
    {
        public int Checks;
        public List<string> Failures = new List<string>();

        public static CoreSelfTest Run()
        {
            CoreSelfTest t = new CoreSelfTest();
            t.TestTimingLimits();
            t.TestDeath();
            t.TestPostureAndVictory();
            t.TestGoodBlock();
            t.TestOneAttemptPerAttack();
            t.TestLatePressAndReset();
            t.TestRoster();
            t.TestBossWindows();
            t.TestGorouNeverFeints();
            t.TestFeintSequence();
            t.TestFeintPunishesEarlyClick();
            t.TestPerfectOnFeint();
            t.TestMultipleCues();
            t.TestSyncopation();
            t.TestNextInstantHidesFeint();
            t.TestFinalBossPhases();
            t.TestFinalBossStances();
            t.TestFinalBossCombos();
            t.TestCampaign();
            return t;
        }

        /// <summary>Verifica art/frames.json (texto já lido pelo chamador).</summary>
        public static CoreSelfTest RunSheet(string framesJson)
        {
            CoreSelfTest t = new CoreSelfTest();
            t.TestSheet(framesJson);
            return t;
        }

        private void Check(bool condition, string description)
        {
            Checks++;
            if (!condition)
            {
                Failures.Add(description);
            }
        }

        private static bool Approx(float a, float b)
        {
            return Math.Abs(a - b) < 0.0005f;
        }

        private static BossProfile BaseProfile()
        {
            BossProfile p = new BossProfile();
            p.PerfectWindow = 0.070f;
            p.GoodWindow = 0.180f;
            p.Windups = new float[] { 1.10f, 0.90f, 1.25f, 0.82f };
            p.FeintChance = 0f;
            return p;
        }

        /// <summary>Cópia sem sorteios: sem fintas, sem jitter, sem compostos.</summary>
        private static BossProfile Deterministic(BossProfile profile)
        {
            BossProfile copy = profile.Clone();
            copy.FeintChance = 0f;
            copy.RhythmJitter = 0f;
            foreach (Stance s in copy.Stances) s.FeintChance = 0f;
            foreach (PhaseRule r in copy.Phases) r.ComboChance = 0f;
            return copy;
        }

        private static BossProfile Boss(int id)
        {
            return BossRoster.Create()[id - 1];
        }

        private static CombatCore NewDuel(BossProfile profile, int seed)
        {
            return new CombatCore(new CombatSettings(), profile ?? BaseProfile(), new SeededRandom(seed));
        }

        private static CombatCore NewDuel()
        {
            return NewDuel(null, 7);
        }

        private static void Prepare(CombatCore duel)
        {
            if (duel.CurrentPhase != CombatCore.Phase.Windup)
            {
                duel.Tick(Math.Max(0f, duel.PhaseEnd - duel.Clock) + 0.00001f);
            }
        }

        private void Parry(CombatCore duel, float lead)
        {
            Prepare(duel);
            duel.Tick(duel.TimeToImpact() - lead);
            Check(duel.Press(), "Primeiro clique deve ser aceito");
            duel.Tick(lead + 0.00001f);
        }

        /// <summary>Deixa o golpe passar sem custo, para avançar a sequência.</summary>
        private static void Skip(CombatCore duel)
        {
            Prepare(duel);
            duel.Tick(duel.TimeToImpact() + 0.001f);
            duel.PlayerHp = 100;
            if (duel.CurrentPhase == CombatCore.Phase.Finished) duel.CurrentPhase = CombatCore.Phase.Recovery;
        }

        /// <summary>Avança até a próxima preparação que seja finta (até vinte golpes).</summary>
        private static CombatCore FeintDuel(BossProfile boss, int seed)
        {
            CombatCore duel = NewDuel(boss, seed);
            for (int i = 0; i < 20; i++)
            {
                Prepare(duel);
                if (duel.IsFeint) return duel;
                Skip(duel);
            }
            return duel;
        }

        private static List<string> Record(CombatCore duel)
        {
            List<string> events = new List<string>();
            duel.OnFeintStarted += delegate { events.Add("finta"); };
            duel.OnAttackStarted += delegate { events.Add("golpe"); };
            duel.OnCue += delegate(bool feint) { events.Add(feint ? "sinal_falso" : "sinal"); };
            duel.OnImpact += delegate(string r, float l, bool b) { events.Add("contato"); };
            return events;
        }

        private void CheckStance(string owner, Stance s, CombatSettings settings)
        {
            foreach (float windup in s.Windups) Check(windup >= settings.AttackLead + 0.1f, owner + ": preparação cabe a partida do golpe");
            Check(s.PerfectWindow < s.GoodWindow, owner + ": janela perfeita dentro da boa");
            if (s.FeintChance > 0f)
            {
                Check(s.FalseCues >= 1 && s.FalseCues <= 3, owner + ": de um a três instantes falsos");
                Check(s.FeintDelayMin > 0f && s.FeintDelayMax >= s.FeintDelayMin, owner + ": faixa de atraso válida");
                Check(s.FeintDelayMin / s.FalseCues >= settings.CueLead - 0.0001f, owner + ": instantes falsos afastados pelo menos um sinal");
                Check(s.FeintDelayMin > s.GoodWindow, owner + ": o clique no instante falso nunca cabe na janela boa");
            }
        }

        private void TestTimingLimits()
        {
            float[] leads = { 0.001f, 0.070f, 0.0701f, 0.180f, 0.1801f, 0.50f };
            foreach (float lead in leads)
            {
                CombatCore duel = NewDuel();
                Parry(duel, lead);
                if (lead <= 0.070f)
                    Check(duel.PerfectCount == 1 && duel.BossHp == 92, "Limite perfeito: " + lead);
                else if (lead <= 0.180f)
                    Check(duel.GoodCount == 1 && duel.PlayerHp == 100, "Limite bom: " + lead);
                else
                    Check(duel.BadCount == 1 && duel.PlayerHp == 75, "Limite ruim: " + lead);
            }
        }

        private void TestDeath()
        {
            CombatCore duel = NewDuel();
            for (int i = 0; i < 4; i++)
            {
                Prepare(duel);
                duel.Tick(duel.TimeToImpact() + 0.001f);
            }
            Check(duel.PlayerHp == 0 && duel.CurrentPhase == CombatCore.Phase.Finished, "Quatro erros devem derrotar o jogador");
            duel.Tick(10f);
            Check(duel.PlayerHp == 0 && duel.BadCount == 4, "Duelo encerrado não aplica novos danos");
        }

        private void TestPostureAndVictory()
        {
            CombatCore duel = NewDuel();
            for (int i = 0; i < 4; i++) Parry(duel, 0.040f);
            Check(duel.BossHp == 38 && duel.BossStability == 0, "Quatro perfeitos quebram postura e causam 62 de dano total");
            Prepare(duel);
            Check(duel.BossStability == 100 && duel.SecondPhase, "Nova preparação restaura postura e ativa segunda fase");
            Check(Approx(duel.WindupDuration, 1.10f * 0.9f), "Segunda fase prepara 10% mais rápido");
            for (int i = 0; i < 4; i++) Parry(duel, 0.040f);
            Check(duel.BossHp == 0 && duel.PlayerHp == 100, "Oito perfeitos vencem sem perder vida");
            Check(duel.CurrentPhase == CombatCore.Phase.Finished, "Boss sem vida encerra o duelo");
        }

        private void TestGoodBlock()
        {
            CombatCore duel = NewDuel();
            for (int i = 0; i < 5; i++) Parry(duel, 0.120f);
            Check(duel.PlayerHp == 100, "Parry bom bloqueia todo o dano");
            Check(duel.BossHp == 100 && duel.BossStability == 100, "Parry bom não causa dano ao boss");
        }

        private void TestOneAttemptPerAttack()
        {
            CombatCore duel = NewDuel();
            Prepare(duel);
            duel.Tick(duel.TimeToImpact() - 0.70f);
            Check(duel.Press(), "Primeira tentativa cedo é registrada");
            duel.Tick(0.66f);
            Check(!duel.Press(), "Spam não troca um timing cedo por perfeito");
            duel.Tick(0.05f);
            Check(duel.BadCount == 1 && duel.PlayerHp == 75, "Cada ataque resolve uma vez");
            duel.Tick(0.10f);
            Check(duel.PlayerHp == 75, "Recuperação não repete o dano");
        }

        private void TestLatePressAndReset()
        {
            CombatCore duel = NewDuel();
            Prepare(duel);
            duel.Tick(duel.TimeToImpact() + 0.01f);
            duel.Press();
            Check(duel.BadCount == 1 && duel.PerfectCount == 0, "Clique após contato não recebe crédito retroativo");
            duel.Reset();
            Check(duel.PlayerHp == 100 && duel.BossHp == 100, "Reiniciar restaura ambas as vidas");
            Check(duel.BadCount == 0 && duel.PerfectCount == 0 && duel.CurrentPhase == CombatCore.Phase.Ready, "Reiniciar limpa placar e estado");
        }

        private void TestRoster()
        {
            BossProfile[] bosses = BossRoster.Create();
            CombatSettings settings = new CombatSettings();
            Check(bosses.Length == 8, "A trilha tem sete mestres e o Boss Final");
            HashSet<string> arenas = new HashSet<string>();
            for (int i = 0; i < bosses.Length; i++)
            {
                BossProfile boss = bosses[i];
                Check(boss.Id == i + 1, "Mestre " + (i + 1) + " tem id em ordem");
                Check(boss.Intro.Length == 2 && boss.Outro.Length == 1, boss.Name + " tem duas falas antes e uma depois");
                foreach (string line in boss.Intro) Check(line.Contains("|"), "Fala de " + boss.Name + " traz o nome do falante");
                foreach (string line in boss.Outro) Check(line.Contains("|"), "Fala de " + boss.Name + " traz o nome do falante");
                foreach (float windup in boss.Windups) Check(windup - boss.RhythmJitter >= settings.AttackLead + 0.1f, boss.Name + ": preparação cabe a partida do golpe");
                Check(boss.ArenaAsset.Length > 0 && arenas.Add(boss.ArenaAsset), boss.Name + " tem cenário próprio");
                Check(boss.Venue.Length > 0 && boss.Special.Length > 0, boss.Name + " descreve cenário e mecânica");
                CheckStance(boss.Name, boss.DefaultStance(), settings);
                foreach (Stance s in boss.Stances) CheckStance(boss.Name + " (" + s.Name + ")", s, settings);
                if (i > 0 && i < 7)
                {
                    BossProfile previous = bosses[i - 1];
                    Check(boss.PerfectWindow < previous.PerfectWindow && boss.GoodWindow < previous.GoodWindow,
                        boss.Name + " é mais exigente que " + previous.Name);
                    Check(boss.FeintChance >= previous.FeintChance, boss.Name + " finta pelo menos tanto quanto " + previous.Name);
                }
                Check((boss.Id == 8) == boss.Provisional, boss.Name + ": só o Boss Final é provisório");
            }
            Check(bosses[0].FeintChance == 0f && bosses[0].FalseCues == 0, "Gorou não finta");
            Check(bosses[1].RhythmJitter > 0f, "Neon Jax é sincopado");
            Check(bosses[2].FeintDelayMin >= 0.35f, "Cavan tem a finta pesada, com atraso longo");
            Check(bosses[3].FalseCues == 2, "Vance tem finta dupla");
            Check(bosses[4].CueVisibility < 1f, "Kaelen esconde parte do sinal");
            Check(bosses[5].FalseCues == 3, "Eleonor tem finta tripla");
            Check(bosses[6].MimicParry && bosses[6].MirrorHero && bosses[6].SheetAsset == "hero", "Sombra imita o parry com a prancha de Ren");
            Check(Approx(bosses[6].FeintChance, 0.75f), "Sombra finta em três de quatro golpes");
            Check(bosses[6].FeintDelayMin >= settings.InputCooldown - 0.03f && bosses[6].FeintDelayMax <= settings.InputCooldown + 0.05f,
                "Sombra ataca no intervalo de cooldown");
            Check(Approx(bosses[0].PerfectWindow, 0.09f) && Approx(bosses[0].GoodWindow, 0.22f), "Gorou: 90 / 220 ms");
            Check(Approx(bosses[6].PerfectWindow, 0.05f) && Approx(bosses[6].GoodWindow, 0.14f), "Sombra: 50 / 140 ms");

            BossProfile final = bosses[7];
            Check(final.Stances.Length >= 2, "Boss Final troca de postura: pelo menos duas posturas");
            Check(final.Phases.Length >= 3 && Approx(final.Phases[0].HpFraction, 1f), "Boss Final tem fases múltiplas a partir da vida cheia");
            for (int i = 1; i < final.Phases.Length; i++)
            {
                Check(final.Phases[i].HpFraction < final.Phases[i - 1].HpFraction, "Fases do Boss Final em ordem decrescente de vida");
            }
            bool combos = false;
            foreach (PhaseRule r in final.Phases) if (r.ComboStrikes > 1 && r.ComboChance > 0f) combos = true;
            Check(combos, "Boss Final tem ataques compostos em alguma fase");
            Check(final.ComboWindup >= settings.AttackLead + 0.1f, "Preparação do golpe composto cabe a partida");
            Check(final.ComboGap < settings.Recovery, "Golpes compostos vêm mais rápido que a recuperação normal");
            float hardestPerfect = 1f, hardestGood = 1f;
            foreach (Stance s in final.Stances)
            {
                hardestPerfect = Math.Min(hardestPerfect, s.PerfectWindow);
                hardestGood = Math.Min(hardestGood, s.GoodWindow);
            }
            Check(hardestPerfect <= bosses[6].PerfectWindow && hardestGood <= bosses[6].GoodWindow, "A postura mais dura do Boss Final não é mais fácil que a Sombra");
        }

        private void TestBossWindows()
        {
            foreach (BossProfile boss in BossRoster.Create())
            {
                BossProfile profile = Deterministic(boss);
                float perfect = profile.Stances.Length > 0 ? profile.Stances[0].PerfectWindow : profile.PerfectWindow;
                float good = profile.Stances.Length > 0 ? profile.Stances[0].GoodWindow : profile.GoodWindow;
                CombatCore duel = NewDuel(profile, 7);
                Parry(duel, perfect);
                Check(duel.PerfectCount == 1, boss.Name + ": limite perfeito inclusivo");
                duel = NewDuel(profile, 7);
                Parry(duel, perfect + 0.005f);
                Check(duel.GoodCount == 1, boss.Name + ": logo após o perfeito é bom");
                duel = NewDuel(profile, 7);
                Parry(duel, good + 0.005f);
                Check(duel.BadCount == 1, boss.Name + ": fora da janela boa é ruim");
            }
        }

        private void TestGorouNeverFeints()
        {
            CombatCore duel = NewDuel(Boss(1), 123);
            for (int i = 0; i < 30; i++)
            {
                Prepare(duel);
                Check(!duel.IsFeint && Approx(duel.StrikeAt, duel.FakeStrikeAt) && duel.FakeStrikeAts.Length == 0, "Gorou nunca finta");
                Parry(duel, 0.100f);
            }
            Check(duel.Feints == 0 && duel.PlayerHp == 100, "Trinta golpes de Gorou sem finta");
        }

        private void TestFeintSequence()
        {
            BossProfile sombra = Boss(7);
            CombatCore duel = FeintDuel(sombra, 3);
            Check(duel.IsFeint, "Com a semente, a Sombra finta em até vinte golpes");
            float delay = duel.StrikeAt - duel.FakeStrikeAt;
            Check(delay >= sombra.FeintDelayMin - 0.0001f && delay <= sombra.FeintDelayMax + 0.0001f,
                "Atraso da finta dentro da faixa da Sombra: " + delay);
            List<string> events = Record(duel);
            duel.Tick(duel.TimeToImpact() + 0.001f);
            string sequence = string.Join(",", events.ToArray());
            Check(sequence == "finta,sinal_falso,golpe,sinal,contato", "Ordem dos sinais na finta simples: " + sequence);
        }

        private void TestFeintPunishesEarlyClick()
        {
            CombatCore duel = FeintDuel(Boss(7), 3);
            // Clique no sinal falso: a tentativa é gasta antes do contato real.
            duel.Tick(duel.FakeStrikeAt - duel.Settings.CueLead - duel.Clock + 0.001f);
            Check(duel.FakeCuePlayed, "Sinal falso já tocou");
            Check(duel.Press(), "Clique no sinal falso é aceito como tentativa");
            duel.Tick(duel.TimeToImpact() - 0.02f);
            Check(!duel.Press(), "Não há segunda tentativa no golpe real");
            duel.Tick(0.05f);
            Check(duel.BadCount == 1 && duel.PlayerHp == 75, "Cair na finta custa vida");
        }

        private void TestPerfectOnFeint()
        {
            CombatCore duel = FeintDuel(Boss(7), 3);
            duel.Tick(duel.TimeToImpact() - 0.030f);
            Check(duel.Press(), "Esperar o contato real ainda permite o parry");
            duel.Tick(0.031f);
            Check(duel.PerfectCount == 1 && duel.BossHp == 92, "Perfeito na finta usa a janela da Sombra");
        }

        private void TestMultipleCues()
        {
            int[] ids = { 4, 6 };
            foreach (int id in ids)
            {
                BossProfile boss = Boss(id);
                CombatCore duel = FeintDuel(boss, 11);
                Check(duel.IsFeint && duel.FakeStrikeAts.Length == boss.FalseCues, boss.Name + ": finta com " + boss.FalseCues + " instantes falsos");
                for (int i = 1; i < duel.FakeStrikeAts.Length; i++)
                {
                    float gap = duel.FakeStrikeAts[i] - duel.FakeStrikeAts[i - 1];
                    Check(gap >= duel.Settings.CueLead - 0.0001f, boss.Name + ": instantes falsos separados por um sinal inteiro: " + gap);
                }
                Check(duel.StrikeAt - duel.FakeStrikeAts[duel.FakeStrikeAts.Length - 1] >= duel.Settings.CueLead - 0.0001f,
                    boss.Name + ": o contato real vem um sinal depois do último falso");
                List<string> events = Record(duel);
                duel.Tick(duel.TimeToImpact() + 0.001f);
                int fakes = 0, lastFake = -1, realCue = -1;
                for (int i = 0; i < events.Count; i++)
                {
                    if (events[i] == "sinal_falso") { fakes++; lastFake = i; }
                    if (events[i] == "sinal") realCue = i;
                }
                Check(fakes == boss.FalseCues, boss.Name + ": um sinal falso por instante: " + string.Join(",", events.ToArray()));
                Check(realCue > lastFake && events[events.Count - 1] == "contato", boss.Name + ": sinal real depois dos falsos e contato por último");
                Check(events[0] == "finta" && events[1] == "sinal_falso", boss.Name + ": a primeira partida é falsa");
            }
        }

        private void TestSyncopation()
        {
            BossProfile jax = Boss(2);
            CombatCore duel = NewDuel(jax, 5);
            bool varied = false;
            for (int i = 0; i < 30; i++)
            {
                Prepare(duel);
                float expected = jax.Windups[i % jax.Windups.Length];
                float actual = duel.WindupDuration;
                Check(actual >= expected - jax.RhythmJitter - 0.0001f && actual <= expected + jax.RhythmJitter + 0.0001f,
                    "Neon Jax fica dentro do compasso ± " + jax.RhythmJitter);
                if (Math.Abs(actual - expected) > 0.01f) varied = true;
                Skip(duel);
            }
            Check(varied, "Neon Jax realmente sai do compasso");
            CombatCore gorou = NewDuel(Boss(1), 5);
            Prepare(gorou);
            Check(Approx(gorou.WindupDuration, 66f / 60f), "Gorou mantém o compasso exato");
        }

        private void TestNextInstantHidesFeint()
        {
            CombatCore duel = FeintDuel(Boss(4), 11);
            Check(Approx(duel.TimeToNextInstant(), duel.FakeStrikeAts[0] - duel.Clock), "A interface conta até o primeiro instante falso");
            duel.Tick(duel.FakeStrikeAts[0] - duel.Clock + 0.001f);
            Check(Approx(duel.TimeToNextInstant(), duel.FakeStrikeAts[1] - duel.Clock), "Depois do primeiro, conta até o segundo");
            duel.Tick(duel.FakeStrikeAts[1] - duel.Clock + 0.001f);
            Check(Approx(duel.TimeToNextInstant(), duel.TimeToImpact()), "Depois do último falso, conta até o real");
            CombatCore plain = NewDuel(Boss(1), 5);
            Prepare(plain);
            Check(Approx(plain.TimeToNextInstant(), plain.TimeToImpact()), "Sem finta, os dois relógios coincidem");
            Check(plain.TimeToNextInstant() >= 0f, "Fora da preparação devolve -1; dentro, nunca negativo");
        }

        private void TestFinalBossPhases()
        {
            BossProfile final = Deterministic(Boss(8));
            CombatCore duel = NewDuel(final, 21);
            List<string> phases = new List<string>();
            duel.OnPhaseChanged += delegate(int index, string name) { phases.Add(index + ":" + name); };
            Prepare(duel);
            Check(duel.PhaseIndex == 0 && duel.CurrentRule().Name == final.Phases[0].Name, "Boss Final começa na primeira fase");
            Check(Approx(duel.WindupDuration, final.Stances[0].Windups[0]), "Primeira fase usa a preparação da primeira postura sem acelerar");
            // Quatro perfeitos: 62 de dano, vida 38, abaixo de 66%.
            for (int i = 0; i < 4; i++) Parry(duel, 0.030f);
            Check(duel.BossHp == 38 && duel.PhaseIndex == 0, "A fase só muda na próxima preparação");
            Prepare(duel);
            Check(duel.PhaseIndex == 1 && phases.Count == 1 && phases[0] == "1:" + final.Phases[1].Name, "Vida a 38% entra na segunda fase");
            Check(!duel.SecondPhase, "Mestre com fases próprias não usa a regra comum da segunda fase");
            // Mais um perfeito: 30, abaixo de 34%.
            Parry(duel, 0.030f);
            Prepare(duel);
            Check(duel.PhaseIndex == 2 && phases.Count == 2, "Vida a 30% entra na terceira fase");
            Check(Approx(duel.WindupDuration, duel.Rules().Windups[(duel.Attacks - 1) % duel.Rules().Windups.Length] * final.Phases[2].SpeedMultiplier),
                "Terceira fase multiplica a preparação pela velocidade da fase");
            duel.Reset();
            Check(duel.PhaseIndex == 0 && duel.StanceIndex == 0 && !duel.InCombo(), "Reiniciar volta à primeira fase e postura");
        }

        private void TestFinalBossStances()
        {
            BossProfile final = Deterministic(Boss(8));
            CombatCore duel = NewDuel(final, 21);
            List<string> stances = new List<string>();
            duel.OnStanceChanged += delegate(int index, string name) { stances.Add(index + ":" + name); };
            int every = final.Phases[0].StanceSwitchEvery;
            for (int i = 0; i < every; i++)
            {
                Prepare(duel);
                Check(duel.StanceIndex == 0 && Approx(duel.PerfectWindow(), final.Stances[0].PerfectWindow), "Primeiros golpes na postura " + final.Stances[0].Name);
                Parry(duel, 0.100f);
            }
            Prepare(duel);
            Check(duel.StanceIndex == 1 && stances.Count == 1 && stances[0] == "1:" + final.Stances[1].Name, "Troca de postura depois de " + every + " golpes");
            Check(Approx(duel.PerfectWindow(), final.Stances[1].PerfectWindow) && Approx(duel.GoodWindow(), final.Stances[1].GoodWindow),
                "Janelas seguem a nova postura");
            // Na postura Baixa, o timing que era bom na Alta vira ruim.
            duel.Tick(duel.TimeToImpact() - 0.160f);
            duel.Press();
            duel.Tick(0.161f);
            Check(duel.BadCount == 1, "160 ms é ruim na postura " + final.Stances[1].Name);
            for (int i = 0; i < every - 1; i++) Skip(duel);
            Prepare(duel);
            Check(duel.StanceIndex == 0 && stances.Count == 2, "A postura volta ao início depois da última");
        }

        private void TestFinalBossCombos()
        {
            BossProfile final = Deterministic(Boss(8));
            final.Phases[0].ComboChance = 1f;
            final.Phases[0].ComboStrikes = 3;
            final.Phases[0].StanceSwitchEvery = 0;
            CombatCore duel = NewDuel(final, 21);
            int announced = 0;
            duel.OnComboStarted += delegate(int strikes) { announced = strikes; };
            Prepare(duel);
            Check(announced == 3 && duel.ComboRemaining == 2 && duel.ComboStrike == 0, "Golpe composto de três contatos anunciado na preparação");
            Check(!duel.IsFeint, "Sem finta no golpe composto determinístico");
            // Primeiro contato: bom.
            duel.Tick(duel.TimeToImpact() - 0.100f);
            Check(duel.Press(), "Primeiro contato aceita tentativa");
            duel.Tick(0.101f);
            Check(duel.GoodCount == 1 && Approx(duel.PhaseEnd - duel.Clock, final.ComboGap), "Depois do primeiro contato vem só a pausa curta do composto");
            Prepare(duel);
            Check(duel.ComboStrike == 1 && duel.ComboRemaining == 1 && Approx(duel.WindupDuration, final.ComboWindup), "Segundo contato usa a preparação curta");
            duel.Tick(duel.TimeToImpact() - 0.100f);
            Check(duel.Press(), "Segundo contato aceita nova tentativa");
            duel.Tick(0.101f);
            Check(duel.GoodCount == 2, "Cada contato do composto resolve sozinho");
            Prepare(duel);
            Check(duel.ComboStrike == 2 && duel.ComboRemaining == 0, "Terceiro contato é o último");
            duel.Tick(duel.TimeToImpact() + 0.001f);
            Check(duel.BadCount == 1 && duel.PlayerHp == 75 && Approx(duel.PhaseEnd - duel.Clock, duel.Settings.Recovery),
                "Deixar passar o último contato custa vida e volta à recuperação normal");
            Prepare(duel);
            Check(duel.ComboStrike == 0 && duel.ComboRemaining == 2, "O golpe seguinte abre um composto novo");
            // Quebra de postura interrompe o composto.
            duel.BossStability = 25;
            duel.Tick(duel.TimeToImpact() - 0.030f);
            duel.Press();
            duel.Tick(0.031f);
            Check(duel.PerfectCount == 1 && duel.BossStability == 0 && duel.ComboRemaining == 0
                && Approx(duel.PhaseEnd - duel.Clock, duel.Settings.BreakRecovery), "Quebrar a postura corta o composto e abre a recuperação longa");
        }

        private void TestCampaign()
        {
            Campaign trail = new Campaign(BossRoster.Create());
            Check(trail.Stage == 1 && trail.Total == 8 && trail.Current.Name == "Gorou", "Trilha começa em Gorou");
            for (int i = 0; i < 6; i++) Check(trail.Advance(), "Avança para o mestre " + (i + 2));
            Check(trail.Current.Name == "Sombra" && !trail.Completed, "Sétimo mestre é a Sombra");
            Check(trail.Advance() && trail.Current.Id == 8 && trail.Current.Provisional, "Depois da Sombra vem o Boss Final provisório");
            Check(!trail.Advance() && trail.Completed, "Depois do Boss Final a trilha termina");
            trail.Reset();
            Check(trail.Stage == 1 && !trail.Completed, "Reiniciar volta ao primeiro mestre");

            // Gemas: uma por mestre dos sete; o Boss Final não guarda gema.
            Campaign gems = new Campaign(BossRoster.Create());
            Check(gems.Gems == 0 && !gems.AllGems, "Trilha nova sem gemas");
            gems.MarkCleared(0);
            gems.MarkCleared(0);
            Check(gems.Gems == 1 && gems.IsCleared(0) && !gems.IsCleared(1), "Vencer Gorou devolve uma gema, uma vez só");
            gems.MarkCleared(7);
            Check(gems.Gems == 1 && !gems.HoldsGem(7) && gems.HoldsGem(6), "O Boss Final não guarda gema; a Sombra guarda");
            for (int i = 1; i < 7; i++) gems.MarkCleared(i);
            Check(gems.Gems == 7 && gems.AllGems, "Sete mestres vencidos, sete gemas");
            gems.MarkCleared(99);
            gems.MarkCleared(-1);
            Check(gems.Gems == 7, "Índices fora da trilha são ignorados");
            gems.Reset();
            Check(gems.Gems == 7 && gems.Stage == 1, "Reiniciar a posição não apaga as gemas");
        }

        private void TestSheet(string json)
        {
            Dictionary<string, SheetData> sheets = SpriteSheetData.Parse(json);
            Check(sheets.ContainsKey("hero") && sheets.ContainsKey("boss"), "frames.json traz herói e boss");
            string[] heroAnimations = { "idle", "parry", "attack", "hurt", "death" };
            string[] bossAnimations = { "idle", "windup", "attack", "hurt", "death" };
            foreach (string name in heroAnimations) Check(sheets["hero"].Has(name), "Herói tem " + name);
            foreach (string name in bossAnimations) Check(sheets["boss"].Has(name), "Boss tem " + name);
            Check(sheets["hero"].Image == "hero_orange.png" && sheets["boss"].Image == "boss.png", "Imagens das pranchas");
            foreach (BossProfile boss in BossRoster.Create())
            {
                Check(sheets.ContainsKey(boss.SheetAsset), boss.Name + " aponta para uma prancha conhecida");
            }
            foreach (KeyValuePair<string, SheetData> sheet in sheets)
            {
                foreach (KeyValuePair<string, AnimationData> animation in sheet.Value.Animations)
                {
                    Check(animation.Value.Fps > 0f && animation.Value.Frames.Length > 0, sheet.Key + "." + animation.Key + " tem fps e frames");
                    foreach (FrameRect frame in animation.Value.Frames)
                    {
                        Check(frame.Width > 0 && frame.Height > 0 && frame.X + frame.Width <= 1536 && frame.Y + frame.Height <= 1024,
                            sheet.Key + "." + animation.Key + " cabe na prancha");
                        Check(frame.AnchorX >= 0 && frame.AnchorX <= frame.Width && frame.AnchorY >= 0 && frame.AnchorY <= frame.Height,
                            sheet.Key + "." + animation.Key + " tem âncora dentro do recorte");
                    }
                }
            }
            Check(sheets["hero"].Animations["idle"].Loop && !sheets["hero"].Animations["parry"].Loop, "Só a guarda repete");
            Check(sheets["boss"].Animations["attack"].Frames.Length == 6, "Ataque do boss tem seis frames");
        }
    }
}

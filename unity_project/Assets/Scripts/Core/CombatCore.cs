using System;
using System.Collections.Generic;

namespace Apara.Core
{
    /// <summary>Fonte de sorteio substituível: os testes usam semente fixa.</summary>
    public interface IRandomSource
    {
        /// <summary>Valor uniforme em [0, 1).</summary>
        float Next();
    }

    public class SeededRandom : IRandomSource
    {
        private readonly Random random;

        public SeededRandom(int seed)
        {
            random = new Random(seed);
        }

        public float Next()
        {
            return (float)random.NextDouble();
        }
    }

    /// <summary>
    /// Regra pura do duelo. O relógio só avança quando o jogo está ativo.
    /// Cada golpe aceita uma tentativa; cliques extras não trocam o timing.
    /// Janelas e ritmo vêm da postura atual do mestre. Uma finta mostra a
    /// partida e o sinal em um ou mais instantes falsos; o contato real chega
    /// depois. O Boss Final acrescenta fases por vida, troca de postura e
    /// golpes compostos: vários contatos seguidos, cada um com sua tentativa.
    /// </summary>
    public class CombatCore
    {
        public enum Phase { Ready, Windup, Recovery, Finished }

        public const string ResultPerfect = "perfeito";
        public const string ResultGood = "bom";
        public const string ResultBad = "ruim";

        /// <summary>Preparação começou: duração até o primeiro instante mostrado e se é finta.</summary>
        public event Action<float, bool> OnWindupStarted;
        /// <summary>Partida falsa (uma por instante falso).</summary>
        public event Action OnFeintStarted;
        /// <summary>Partida real do golpe.</summary>
        public event Action OnAttackStarted;
        /// <summary>Sinal: true nos instantes falsos, false no real.</summary>
        public event Action<bool> OnCue;
        public event Action OnParryPressed;
        public event Action<string, float, bool> OnImpact;
        public event Action<bool> OnFinished;
        /// <summary>O mestre trocou de postura: índice e nome.</summary>
        public event Action<int, string> OnStanceChanged;
        /// <summary>O mestre entrou numa nova fase: índice e nome.</summary>
        public event Action<int, string> OnPhaseChanged;
        /// <summary>Um golpe composto começou: quantos contatos vêm em sequência.</summary>
        public event Action<int> OnComboStarted;

        private enum Cue { FakeLaunch, FakeCue, Launch, RealCue }

        private struct Scheduled
        {
            public float Time;
            public Cue Kind;
        }

        public CombatSettings Settings;
        public BossProfile Boss;
        public IRandomSource Random;

        public Phase CurrentPhase = Phase.Ready;
        public float Clock;
        public float PhaseEnd;
        public float StrikeAt;
        /// <summary>Primeiro instante falso, ou o contato real quando não há finta.</summary>
        public float FakeStrikeAt;
        /// <summary>Todos os instantes falsos do golpe atual, em ordem.</summary>
        public float[] FakeStrikeAts = new float[0];
        public float WindupDuration = 1f;
        public bool IsFeint;
        public float LastPress = -100f;
        public bool Attempted;
        public bool AttackLaunched;
        public bool FeintLaunched;
        public bool CuePlayed;
        public bool FakeCuePlayed;
        public int PlayerHp = 100;
        public int BossHp = 100;
        public int BossStability = 100;
        public int Attacks;
        public int Feints;
        public int PerfectCount;
        public int GoodCount;
        public int BadCount;
        /// <summary>Regra comum: com metade da vida o mestre prepara 10% mais rápido (mestres sem fases próprias).</summary>
        public bool SecondPhase;

        // Boss Final.
        public int StanceIndex;
        public int PhaseIndex;
        /// <summary>Contatos que ainda faltam depois do atual, dentro de um golpe composto.</summary>
        public int ComboRemaining;
        /// <summary>Posição do golpe atual no composto (0 = primeiro ou golpe simples).</summary>
        public int ComboStrike;

        private Stance defaultStance;
        private readonly List<Scheduled> schedule = new List<Scheduled>();
        private int scheduleIndex;

        public CombatCore(CombatSettings settings, BossProfile profile, IRandomSource random)
        {
            Settings = settings;
            Random = random ?? new SeededRandom(Environment.TickCount);
            SetBoss(profile ?? new BossProfile());
        }

        public void SetBoss(BossProfile profile)
        {
            Boss = profile;
            defaultStance = profile.DefaultStance();
            Reset();
        }

        public void Reset()
        {
            Clock = 0f;
            CurrentPhase = Phase.Ready;
            PhaseEnd = 0.65f;
            StrikeAt = 0f;
            FakeStrikeAt = 0f;
            FakeStrikeAts = new float[0];
            IsFeint = false;
            LastPress = -100f;
            Attempted = false;
            AttackLaunched = false;
            FeintLaunched = false;
            CuePlayed = false;
            FakeCuePlayed = false;
            PlayerHp = Settings.PlayerHealth;
            BossHp = Settings.BossHealth;
            BossStability = Settings.BossPosture;
            Attacks = 0;
            Feints = 0;
            PerfectCount = 0;
            GoodCount = 0;
            BadCount = 0;
            SecondPhase = false;
            StanceIndex = 0;
            PhaseIndex = 0;
            ComboRemaining = 0;
            ComboStrike = 0;
            schedule.Clear();
            scheduleIndex = 0;
        }

        /// <summary>Postura em vigor: a do mestre, ou a derivada do perfil quando ele não troca.</summary>
        public Stance Rules()
        {
            if (Boss.Stances.Length > 0)
            {
                return Boss.Stances[Math.Min(StanceIndex, Boss.Stances.Length - 1)];
            }
            return defaultStance;
        }

        /// <summary>Fase em vigor, ou null para mestres sem fases próprias.</summary>
        public PhaseRule CurrentRule()
        {
            if (Boss.Phases.Length == 0)
            {
                return null;
            }
            return Boss.Phases[Math.Min(PhaseIndex, Boss.Phases.Length - 1)];
        }

        public float PerfectWindow()
        {
            return Rules().PerfectWindow;
        }

        public float GoodWindow()
        {
            return Rules().GoodWindow;
        }

        public bool InCombo()
        {
            return ComboRemaining > 0 || ComboStrike > 0;
        }

        public void Tick(float delta)
        {
            if (CurrentPhase == Phase.Finished)
            {
                return;
            }
            Clock += Math.Max(0f, delta);
            if (CurrentPhase == Phase.Ready || CurrentPhase == Phase.Recovery)
            {
                if (Clock >= PhaseEnd)
                {
                    BeginAttack();
                }
                return;
            }
            if (CurrentPhase != Phase.Windup)
            {
                return;
            }
            // Os eventos disparam na ordem do tempo, mesmo num passo grande.
            while (scheduleIndex < schedule.Count && Clock >= schedule[scheduleIndex].Time)
            {
                Fire(schedule[scheduleIndex].Kind);
                scheduleIndex++;
            }
            if (Clock >= StrikeAt)
            {
                Resolve();
            }
        }

        private void Fire(Cue kind)
        {
            switch (kind)
            {
                case Cue.FakeLaunch:
                    FeintLaunched = true;
                    if (OnFeintStarted != null) OnFeintStarted();
                    break;
                case Cue.FakeCue:
                    FakeCuePlayed = true;
                    if (OnCue != null) OnCue(true);
                    break;
                case Cue.Launch:
                    AttackLaunched = true;
                    if (OnAttackStarted != null) OnAttackStarted();
                    break;
                case Cue.RealCue:
                    CuePlayed = true;
                    if (OnCue != null) OnCue(false);
                    break;
            }
        }

        public bool Press()
        {
            if (CurrentPhase == Phase.Finished)
            {
                return false;
            }
            if (Clock - LastPress < Settings.InputCooldown)
            {
                return false;
            }
            if (CurrentPhase == Phase.Windup && Attempted)
            {
                return false;
            }
            LastPress = Clock;
            if (CurrentPhase == Phase.Windup)
            {
                Attempted = true;
            }
            if (OnParryPressed != null) OnParryPressed();
            return true;
        }

        /// <summary>Segundos até o contato real, ou -1 fora da preparação.</summary>
        public float TimeToImpact()
        {
            if (CurrentPhase != Phase.Windup)
            {
                return -1f;
            }
            return Math.Max(0f, StrikeAt - Clock);
        }

        /// <summary>
        /// Segundos até o próximo instante mostrado (falso ou real), ou -1 fora da
        /// preparação. É o que a interface deve usar: assim o sinal na tela não
        /// entrega a finta.
        /// </summary>
        public float TimeToNextInstant()
        {
            if (CurrentPhase != Phase.Windup)
            {
                return -1f;
            }
            float next = StrikeAt;
            for (int i = 0; i < FakeStrikeAts.Length; i++)
            {
                if (FakeStrikeAts[i] >= Clock && FakeStrikeAts[i] < next)
                {
                    next = FakeStrikeAts[i];
                }
            }
            return Math.Max(0f, next - Clock);
        }

        private void UpdatePhase()
        {
            // A fase mais avançada cujo limite de vida já foi alcançado. Nunca volta.
            int target = PhaseIndex;
            for (int i = PhaseIndex + 1; i < Boss.Phases.Length; i++)
            {
                if (BossHp <= Boss.Phases[i].HpFraction * Settings.BossHealth)
                {
                    target = i;
                }
            }
            if (target != PhaseIndex)
            {
                PhaseIndex = target;
                if (OnPhaseChanged != null) OnPhaseChanged(PhaseIndex, Boss.Phases[PhaseIndex].Name);
            }
        }

        private void BeginAttack()
        {
            CurrentPhase = Phase.Windup;
            if (BossStability == 0)
            {
                BossStability = Settings.BossPosture;
            }
            Attempted = false;
            AttackLaunched = false;
            FeintLaunched = false;
            CuePlayed = false;
            FakeCuePlayed = false;

            bool continuing = ComboRemaining > 0;
            if (continuing)
            {
                ComboRemaining -= 1;
                ComboStrike += 1;
            }
            else
            {
                ComboStrike = 0;
                UpdatePhase();
            }
            PhaseRule rule = CurrentRule();
            if (!continuing && rule != null && rule.StanceSwitchEvery > 0 && Boss.Stances.Length > 1
                && Attacks > 0 && Attacks % rule.StanceSwitchEvery == 0)
            {
                StanceIndex = (StanceIndex + 1) % Boss.Stances.Length;
                if (OnStanceChanged != null) OnStanceChanged(StanceIndex, Boss.Stances[StanceIndex].Name);
            }
            Stance stance = Rules();

            float duration;
            if (continuing)
            {
                // Golpe seguinte de um composto: preparação curta e sem finta.
                duration = Boss.ComboWindup;
            }
            else
            {
                duration = stance.Windups[Attacks % stance.Windups.Length];
                if (Boss.RhythmJitter > 0f)
                {
                    // Ritmo sincopado: a preparação sai do compasso para um lado ou outro.
                    duration += (Random.Next() * 2f - 1f) * Boss.RhythmJitter;
                }
            }
            if (rule != null)
            {
                duration *= rule.SpeedMultiplier;
            }
            else
            {
                SecondPhase = BossHp <= Settings.BossHealth / 2;
                if (SecondPhase)
                {
                    duration *= Settings.PhaseTwoSpeed;
                }
            }
            WindupDuration = Math.Max(duration, Settings.AttackLead + 0.1f);

            float delay = 0f;
            int cues = 0;
            IsFeint = false;
            if (!continuing)
            {
                // O sorteio acontece sempre, para a sequência ser reproduzível por semente.
                float roll = Random.Next();
                IsFeint = stance.FeintChance > 0f && stance.FalseCues > 0 && roll < stance.FeintChance;
                if (IsFeint)
                {
                    float min = stance.FeintDelayMax > 0f ? stance.FeintDelayMin : Settings.FeintDelayMin;
                    float max = stance.FeintDelayMax > 0f ? stance.FeintDelayMax : Settings.FeintDelayMax;
                    delay = min + Random.Next() * (max - min);
                    cues = stance.FalseCues;
                    Feints += 1;
                }
                if (rule != null && rule.ComboStrikes > 1 && rule.ComboChance > 0f && Random.Next() < rule.ComboChance)
                {
                    ComboRemaining = rule.ComboStrikes - 1;
                    if (OnComboStarted != null) OnComboStarted(rule.ComboStrikes);
                }
            }
            float first = Clock + WindupDuration;
            StrikeAt = first + delay;
            FakeStrikeAts = new float[cues];
            for (int i = 0; i < cues; i++)
            {
                FakeStrikeAts[i] = first + delay * i / cues;
            }
            FakeStrikeAt = cues > 0 ? FakeStrikeAts[0] : StrikeAt;
            Attacks += 1;
            BuildSchedule();
            if (OnWindupStarted != null) OnWindupStarted(WindupDuration, IsFeint);
        }

        private void BuildSchedule()
        {
            schedule.Clear();
            scheduleIndex = 0;
            for (int i = 0; i < FakeStrikeAts.Length; i++)
            {
                Add(FakeStrikeAts[i] - Settings.AttackLead, Cue.FakeLaunch);
                Add(FakeStrikeAts[i] - Settings.CueLead, Cue.FakeCue);
            }
            Add(StrikeAt - Settings.AttackLead, Cue.Launch);
            Add(StrikeAt - Settings.CueLead, Cue.RealCue);
            // Ordenação estável por tempo: eventos simultâneos mantêm a ordem de inserção.
            for (int i = 1; i < schedule.Count; i++)
            {
                Scheduled item = schedule[i];
                int j = i - 1;
                while (j >= 0 && schedule[j].Time > item.Time)
                {
                    schedule[j + 1] = schedule[j];
                    j--;
                }
                schedule[j + 1] = item;
            }
        }

        private void Add(float time, Cue kind)
        {
            Scheduled item = new Scheduled();
            item.Time = time;
            item.Kind = kind;
            schedule.Add(item);
        }

        private void Resolve()
        {
            // Consumir o golpe antes dos eventos impede dano duplicado/reentrância.
            CurrentPhase = Phase.Recovery;
            PhaseEnd = Clock + (ComboRemaining > 0 ? Boss.ComboGap : Settings.Recovery);
            Stance stance = Rules();
            float lead = Attempted ? StrikeAt - LastPress : -1f;
            string result = ResultBad;
            bool broke = false;
            // Só aceita input anterior ao impacto. Não há crédito retroativo.
            if (Attempted && lead >= 0f && lead <= stance.PerfectWindow + 0.000001f)
            {
                result = ResultPerfect;
                PerfectCount += 1;
                BossHp = Math.Max(0, BossHp - Settings.PerfectHealthDamage);
                BossStability = Math.Max(0, BossStability - Settings.PerfectPostureDamage);
                if (BossStability == 0)
                {
                    broke = true;
                    BossHp = Math.Max(0, BossHp - Settings.PostureBreakDamage);
                    PhaseEnd = Clock + Settings.BreakRecovery;
                    // A quebra de postura interrompe o golpe composto.
                    ComboRemaining = 0;
                }
            }
            else if (Attempted && lead >= 0f && lead <= stance.GoodWindow + 0.000001f)
            {
                result = ResultGood;
                GoodCount += 1;
            }
            else
            {
                BadCount += 1;
                PlayerHp = Math.Max(0, PlayerHp - Settings.BadDamage);
            }
            if (OnImpact != null) OnImpact(result, lead, broke);
            if (BossHp <= 0 || PlayerHp <= 0)
            {
                CurrentPhase = Phase.Finished;
                ComboRemaining = 0;
                if (OnFinished != null) OnFinished(BossHp <= 0);
            }
        }
    }
}

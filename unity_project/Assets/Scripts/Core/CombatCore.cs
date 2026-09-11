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
    /// Janelas e ritmo vêm do mestre atual. Uma finta mostra a partida e o
    /// sinal em um ou mais instantes falsos; o contato real chega depois.
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
        public bool SecondPhase;

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
            schedule.Clear();
            scheduleIndex = 0;
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
            float duration = Boss.Windups[Attacks % Boss.Windups.Length];
            if (Boss.RhythmJitter > 0f)
            {
                // Ritmo sincopado: a preparação sai do compasso para um lado ou outro.
                duration += (Random.Next() * 2f - 1f) * Boss.RhythmJitter;
            }
            SecondPhase = BossHp <= Settings.BossHealth / 2;
            if (SecondPhase)
            {
                duration *= Settings.PhaseTwoSpeed;
            }
            WindupDuration = Math.Max(duration, Settings.AttackLead + 0.1f);
            // O sorteio acontece sempre, para a sequência ser reproduzível por semente.
            float roll = Random.Next();
            IsFeint = Boss.FeintChance > 0f && Boss.FalseCues > 0 && roll < Boss.FeintChance;
            float delay = 0f;
            int cues = 0;
            if (IsFeint)
            {
                float min = Boss.FeintDelayMax > 0f ? Boss.FeintDelayMin : Settings.FeintDelayMin;
                float max = Boss.FeintDelayMax > 0f ? Boss.FeintDelayMax : Settings.FeintDelayMax;
                delay = min + Random.Next() * (max - min);
                cues = Boss.FalseCues;
                Feints += 1;
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
            PhaseEnd = Clock + Settings.Recovery;
            float lead = Attempted ? StrikeAt - LastPress : -1f;
            string result = ResultBad;
            bool broke = false;
            // Só aceita input anterior ao impacto. Não há crédito retroativo.
            if (Attempted && lead >= 0f && lead <= Boss.PerfectWindow + 0.000001f)
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
                }
            }
            else if (Attempted && lead >= 0f && lead <= Boss.GoodWindow + 0.000001f)
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
                if (OnFinished != null) OnFinished(BossHp <= 0);
            }
        }
    }
}

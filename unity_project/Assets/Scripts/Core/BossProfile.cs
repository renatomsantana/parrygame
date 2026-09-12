namespace Apara.Core
{
    /// <summary>Cor sem depender de UnityEngine; a camada de jogo converte.</summary>
    [System.Serializable]
    public struct Rgb
    {
        public float R;
        public float G;
        public float B;

        public Rgb(float r, float g, float b)
        {
            R = r;
            G = g;
            B = b;
        }

        public static readonly Rgb White = new Rgb(1f, 1f, 1f);
    }

    /// <summary>
    /// Uma postura de combate: janelas, ritmo e estilo de finta. Os mestres 1 a 7
    /// têm uma só (derivada do perfil); o Boss Final troca entre várias.
    /// </summary>
    [System.Serializable]
    public class Stance
    {
        public string Name = "";
        public float PerfectWindow = 0.070f;
        public float GoodWindow = 0.180f;
        public float[] Windups = new float[] { 1.10f, 0.90f, 1.25f };
        public float FeintChance = 0f;
        public int FalseCues = 1;
        public float FeintDelayMin = 0f;
        public float FeintDelayMax = 0f;
        public bool MimicParry = false;

        public Stance Clone()
        {
            Stance copy = (Stance)MemberwiseClone();
            copy.Windups = (float[])Windups.Clone();
            return copy;
        }
    }

    /// <summary>
    /// Uma fase do Boss Final, ativa quando a vida cai até a fração indicada.
    /// Define velocidade, golpes compostos e a cadência da troca de postura.
    /// </summary>
    [System.Serializable]
    public class PhaseRule
    {
        public string Name = "";
        /// <summary>A fase entra quando a vida do mestre é menor ou igual a esta fração do máximo.</summary>
        public float HpFraction = 1f;
        public float SpeedMultiplier = 1f;
        /// <summary>Chance de a preparação abrir um golpe composto (0 a 1).</summary>
        public float ComboChance = 0f;
        /// <summary>Golpes por composto; 1 significa sem compostos.</summary>
        public int ComboStrikes = 1;
        /// <summary>Troca de postura a cada N golpes; 0 nunca troca.</summary>
        public int StanceSwitchEvery = 0;

        public PhaseRule Clone()
        {
            return (PhaseRule)MemberwiseClone();
        }
    }

    /// <summary>
    /// Um mestre da trilha: janelas, ritmo, estilo de finta, cenário, visual e
    /// falas ("Nome|Texto"). Os campos de arte são nomes de recurso: a camada de
    /// jogo carrega o que existir e cai no padrão quando a arte ainda não chegou.
    /// </summary>
    [System.Serializable]
    public class BossProfile
    {
        public int Id = 1;
        public string Name = "Mestre";
        public string Title = "";
        public string Venue = "";
        public string Special = "";
        /// <summary>Definição provisória, à espera do roteiro (o Boss Final).</summary>
        public bool Provisional = false;

        // Timing, em segundos antes do contato.
        public float PerfectWindow = 0.070f;
        public float GoodWindow = 0.180f;
        public float[] Windups = new float[] { 1.10f, 0.90f, 1.25f };

        // Estilo de finta.
        /// <summary>Chance de cada preparação ser finta (0 a 1).</summary>
        public float FeintChance = 0f;
        /// <summary>Quantos instantes falsos antes do contato real (1 = finta simples, 2 = dupla, 3 = tripla).</summary>
        public int FalseCues = 1;
        /// <summary>Atraso total entre o primeiro instante falso e o contato real. Zero usa o valor comum.</summary>
        public float FeintDelayMin = 0f;
        public float FeintDelayMax = 0f;
        /// <summary>Variação aleatória da preparação (± segundos): o ritmo sincopado do Neon Jax.</summary>
        public float RhythmJitter = 0f;
        /// <summary>Força do sinal (0 a 1): os cortes cegantes de Kaelen escondem parte dele.</summary>
        public float CueVisibility = 1f;
        /// <summary>A finta imita o gesto de parry em vez da partida do golpe: a mímica da Sombra.</summary>
        public bool MimicParry = false;

        // Boss Final: posturas, fases e golpes compostos. Vazios nos mestres comuns.
        public Stance[] Stances = new Stance[0];
        public PhaseRule[] Phases = new PhaseRule[0];
        /// <summary>Preparação curta dos golpes seguintes de um composto.</summary>
        public float ComboWindup = 0.50f;
        /// <summary>Pausa entre golpes de um composto, no lugar da recuperação normal.</summary>
        public float ComboGap = 0.10f;

        // Cenário e visual. Nomes de recurso; a arte de cada um pode chegar depois.
        public string ArenaAsset = "arena";
        public string SheetAsset = "boss";
        public bool MirrorHero = false;
        public Rgb Tint = Rgb.White;
        public Rgb ArenaTint = Rgb.White;
        public Rgb HudColor = new Rgb(0.925f, 0.447f, 0.522f);

        public string[] Intro = new string[0];
        public string[] Outro = new string[0];

        /// <summary>Postura derivada dos campos do próprio perfil (mestres sem posturas próprias).</summary>
        public Stance DefaultStance()
        {
            Stance s = new Stance();
            s.Name = "";
            s.PerfectWindow = PerfectWindow;
            s.GoodWindow = GoodWindow;
            s.Windups = (float[])Windups.Clone();
            s.FeintChance = FeintChance;
            s.FalseCues = FalseCues;
            s.FeintDelayMin = FeintDelayMin;
            s.FeintDelayMax = FeintDelayMax;
            s.MimicParry = MimicParry;
            return s;
        }

        public BossProfile Clone()
        {
            BossProfile copy = (BossProfile)MemberwiseClone();
            copy.Windups = (float[])Windups.Clone();
            copy.Intro = (string[])Intro.Clone();
            copy.Outro = (string[])Outro.Clone();
            copy.Stances = new Stance[Stances.Length];
            for (int i = 0; i < Stances.Length; i++) copy.Stances[i] = Stances[i].Clone();
            copy.Phases = new PhaseRule[Phases.Length];
            for (int i = 0; i < Phases.Length; i++) copy.Phases[i] = Phases[i].Clone();
            return copy;
        }
    }
}

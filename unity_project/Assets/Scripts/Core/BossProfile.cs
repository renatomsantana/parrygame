namespace Apara.Core
{
    /// <summary>Cor sem depender de UnityEngine; a camada de jogo converte.</summary>
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
    /// Um mestre da trilha: janelas, ritmo, estilo de finta, cenário, visual e
    /// falas ("Nome|Texto"). Os campos de arte são nomes de recurso: a camada de
    /// jogo carrega o que existir e cai no padrão quando a arte ainda não chegou.
    /// </summary>
    public class BossProfile
    {
        public int Id = 1;
        public string Name = "Mestre";
        public string Title = "";
        public string Venue = "";
        public string Special = "";

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

        // Cenário e visual. Nomes de recurso; a arte de cada um pode chegar depois.
        public string ArenaAsset = "arena";
        public string SheetAsset = "boss";
        public bool MirrorHero = false;
        public Rgb Tint = Rgb.White;
        public Rgb ArenaTint = Rgb.White;
        public Rgb HudColor = new Rgb(0.925f, 0.447f, 0.522f);

        public string[] Intro = new string[0];
        public string[] Outro = new string[0];

        public BossProfile Clone()
        {
            BossProfile copy = (BossProfile)MemberwiseClone();
            copy.Windups = (float[])Windups.Clone();
            copy.Intro = (string[])Intro.Clone();
            copy.Outro = (string[])Outro.Clone();
            return copy;
        }
    }
}

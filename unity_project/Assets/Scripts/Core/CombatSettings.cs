namespace Apara.Core
{
    /// <summary>Valores comuns a todos os duelos. Janelas e ritmo ficam em BossProfile.</summary>
    public class CombatSettings
    {
        public int PlayerHealth = 100;
        public int BadDamage = 25;
        public int BossHealth = 100;
        public int BossPosture = 100;
        public int PerfectHealthDamage = 8;
        public int PerfectPostureDamage = 25;
        public int PostureBreakDamage = 30;

        public float InputCooldown = 0.320f;
        public float AttackLead = 0.220f;
        public float CueLead = 0.180f;
        public float Recovery = 0.800f;
        public float BreakRecovery = 1.500f;
        public float PhaseTwoSpeed = 0.90f;

        public float FeintDelayMin = 0.200f;
        public float FeintDelayMax = 0.400f;

        public float GoodHitstop = 0.045f;
        public float PerfectHitstop = 0.090f;
        public float BadHitstop = 0.045f;
        public float BreakHitstop = 0.160f;
    }
}

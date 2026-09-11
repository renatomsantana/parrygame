namespace Apara.Core
{
    /// <summary>Posição do jogador na trilha dos mestres. Regra pura.</summary>
    public class Campaign
    {
        public BossProfile[] Bosses;
        public int Index;
        public bool Completed;

        public Campaign(BossProfile[] roster)
        {
            Bosses = roster;
            Reset();
        }

        public void Reset()
        {
            Index = 0;
            Completed = false;
        }

        public BossProfile Current
        {
            get { return Bosses[Index]; }
        }

        public int Stage
        {
            get { return Index + 1; }
        }

        public int Total
        {
            get { return Bosses.Length; }
        }

        /// <summary>Devolve true se ainda há um mestre à frente; false ao terminar a trilha.</summary>
        public bool Advance()
        {
            if (Index + 1 < Bosses.Length)
            {
                Index += 1;
                return true;
            }
            Completed = true;
            return false;
        }
    }
}

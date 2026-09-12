namespace Apara.Core
{
    /// <summary>
    /// Posição do jogador na trilha e as gemas recuperadas. Regra pura.
    /// Cada um dos sete mestres roubou uma gema da empunhadura sagrada; vencer
    /// um mestre devolve a gema. O Boss Final não guarda gema.
    /// </summary>
    public class Campaign
    {
        public const int GemCount = 7;

        public BossProfile[] Bosses;
        public int Index;
        public bool Completed;
        /// <summary>Máscara de bits dos mestres já vencidos (bit i = mestre i).</summary>
        public int ClearedMask;

        public Campaign(BossProfile[] roster)
        {
            Bosses = roster;
            Reset();
        }

        /// <summary>Volta ao primeiro mestre. As vitórias registradas ficam.</summary>
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

        public void MarkCleared(int index)
        {
            if (index >= 0 && index < Bosses.Length)
            {
                ClearedMask |= 1 << index;
            }
        }

        public bool IsCleared(int index)
        {
            return (ClearedMask & (1 << index)) != 0;
        }

        /// <summary>Gemas recuperadas: mestres vencidos entre os sete primeiros.</summary>
        public int Gems
        {
            get
            {
                int gems = 0;
                int limit = System.Math.Min(GemCount, Bosses.Length);
                for (int i = 0; i < limit; i++)
                {
                    if (IsCleared(i)) gems++;
                }
                return gems;
            }
        }

        /// <summary>Verdadeiro quando o mestre indicado devolve uma gema ao ser vencido.</summary>
        public bool HoldsGem(int index)
        {
            return index >= 0 && index < System.Math.Min(GemCount, Bosses.Length);
        }

        public bool AllGems
        {
            get { return Gems == System.Math.Min(GemCount, Bosses.Length); }
        }
    }
}

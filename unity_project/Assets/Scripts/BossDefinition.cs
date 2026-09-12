using Apara.Core;
using UnityEngine;

namespace Apara
{
    /// <summary>
    /// Um mestre como asset, editável no Inspector. A ponte carrega todos os
    /// assets em Assets/Resources/Bosses; se não houver nenhum, usa o elenco em
    /// código (BossRoster). O menu APARA → Exportar mestres cria os assets a
    /// partir do código, uma vez; depois o Inspector manda.
    /// </summary>
    [CreateAssetMenu(fileName = "Mestre", menuName = "APARA/Mestre")]
    public class BossDefinition : ScriptableObject
    {
        public BossProfile Profile = new BossProfile();
    }
}

using UnityEngine;

namespace Apara
{
    /// <summary>
    /// Monta o jogo em qualquer cena aberta ao entrar em Play: não há cena
    /// salva para manter, tudo nasce em código a partir de Assets/Resources/Art.
    /// </summary>
    public static class AparaBootstrap
    {
        [RuntimeInitializeOnLoadMethod(RuntimeInitializeLoadType.AfterSceneLoad)]
        private static void Boot()
        {
            if (Object.FindFirstObjectByType<AparaCombatBridge>() != null)
            {
                return;
            }
            GameObject root = new GameObject("APARA");
            root.AddComponent<AparaCombatBridge>();
        }
    }
}

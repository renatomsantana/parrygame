using System.IO;
using Apara.Core;
using UnityEditor;
using UnityEngine;

namespace Apara.EditorTools
{
    /// <summary>Cria um asset por mestre a partir de BossRoster, para ajustar no Inspector.</summary>
    public static class BossAssetsExporter
    {
        private const string Folder = "Assets/Resources/Bosses";

        [MenuItem("APARA/Exportar mestres para Resources/Bosses")]
        public static void Export()
        {
            Directory.CreateDirectory(Folder);
            foreach (BossProfile profile in BossRoster.Create())
            {
                string path = Folder + "/" + profile.Id.ToString("00") + "_" + Slug(profile.Name) + ".asset";
                BossDefinition asset = AssetDatabase.LoadAssetAtPath<BossDefinition>(path);
                if (asset == null)
                {
                    asset = ScriptableObject.CreateInstance<BossDefinition>();
                    asset.Profile = profile;
                    AssetDatabase.CreateAsset(asset, path);
                }
                else
                {
                    Debug.Log("APARA: " + path + " já existe; mantido como está.");
                }
            }
            AssetDatabase.SaveAssets();
            AssetDatabase.Refresh();
            Debug.Log("APARA: mestres exportados para " + Folder + ". A partir de agora a ponte usa os assets.");
        }

        private static string Slug(string name)
        {
            string slug = name.ToLowerInvariant().Replace(" ", "_");
            slug = slug.Replace("ã", "a").Replace("é", "e").Replace("ê", "e").Replace("ç", "c").Replace("ó", "o");
            return slug;
        }
    }
}

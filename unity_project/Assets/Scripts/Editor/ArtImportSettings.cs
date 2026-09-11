using Apara.Core;
using UnityEditor;
using UnityEngine;

namespace Apara.EditorTools
{
    /// <summary>Pixel art sem filtro nem compressão para tudo em Assets/Resources/Art.</summary>
    public class ArtImportSettings : AssetPostprocessor
    {
        private void OnPreprocessTexture()
        {
            if (!assetPath.Replace('\\', '/').Contains("/Resources/Art/"))
            {
                return;
            }
            TextureImporter importer = (TextureImporter)assetImporter;
            importer.textureType = TextureImporterType.Sprite;
            importer.spriteImportMode = SpriteImportMode.Single;
            importer.spritePixelsPerUnit = 1f;
            importer.filterMode = FilterMode.Point;
            importer.mipmapEnabled = false;
            importer.textureCompression = TextureImporterCompression.Uncompressed;
            importer.alphaIsTransparency = true;
            importer.npotScale = TextureImporterNPOTScale.None;
            importer.maxTextureSize = 2048;
            importer.isReadable = false;
        }
    }

    public static class AparaMenu
    {
        [MenuItem("APARA/Rodar testes do núcleo")]
        private static void RunCoreTests()
        {
            CoreSelfTest core = CoreSelfTest.Run();
            TextAsset frames = Resources.Load<TextAsset>("Art/frames");
            CoreSelfTest sheet = CoreSelfTest.RunSheet(frames.text);
            int checks = core.Checks + sheet.Checks;
            int failures = core.Failures.Count + sheet.Failures.Count;
            foreach (string failure in core.Failures) Debug.LogError("FALHA: " + failure);
            foreach (string failure in sheet.Failures) Debug.LogError("FALHA: " + failure);
            Debug.Log("APARA Core: " + checks + " verificações, " + failures + " falhas");
        }
    }
}

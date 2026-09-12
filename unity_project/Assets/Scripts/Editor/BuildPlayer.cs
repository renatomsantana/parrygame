using System.IO;
using UnityEditor;
using UnityEngine;

namespace Apara.EditorTools
{
    /// <summary>
    /// Build sem abrir a interface: Tools/Build.ps1 chama este método em batchmode.
    /// Também disponível no menu APARA.
    /// </summary>
    public static class BuildPlayer
    {
        private const string ScenePath = "Assets/Scenes/Main.unity";

        [MenuItem("APARA/Gerar build Windows")]
        public static void Windows()
        {
            string output = Path.Combine("Builds", "Windows", "APARA.exe");
            Directory.CreateDirectory(Path.GetDirectoryName(output));
            BuildPlayerOptions options = new BuildPlayerOptions
            {
                scenes = new[] { ScenePath },
                locationPathName = output,
                target = BuildTarget.StandaloneWindows64,
                options = BuildOptions.None
            };
            var report = BuildPipeline.BuildPlayer(options);
            Debug.Log("APARA build: " + report.summary.result + " → " + output);
            if (report.summary.result != UnityEditor.Build.Reporting.BuildResult.Succeeded && Application.isBatchMode)
            {
                EditorApplication.Exit(1);
            }
        }
    }
}

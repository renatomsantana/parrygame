using System;
using System.IO;
using Apara.Core;

/// <summary>
/// Roda CoreSelfTest fora do Unity. Compilado por RunCoreTests.ps1 com o csc.exe
/// que acompanha o Windows; por isso o núcleo evita sintaxe posterior ao C# 5.
/// </summary>
public static class Program
{
    public static int Main(string[] args)
    {
        CoreSelfTest core = CoreSelfTest.Run();
        int checks = core.Checks;
        int failures = core.Failures.Count;
        foreach (string failure in core.Failures) Console.Error.WriteLine("FALHA: " + failure);

        string framesPath = args.Length > 0 ? args[0] : Path.Combine("Assets", Path.Combine("Resources", Path.Combine("Art", "frames.json")));
        if (File.Exists(framesPath))
        {
            CoreSelfTest sheet = CoreSelfTest.RunSheet(File.ReadAllText(framesPath));
            checks += sheet.Checks;
            failures += sheet.Failures.Count;
            foreach (string failure in sheet.Failures) Console.Error.WriteLine("FALHA: " + failure);
        }
        else
        {
            Console.Error.WriteLine("frames.json não encontrado em " + framesPath + "; verificação da prancha pulada.");
        }

        Console.WriteLine(string.Format("APARA Core: {0} verificações, {1} falhas", checks, failures));
        return failures > 0 ? 1 : 0;
    }
}

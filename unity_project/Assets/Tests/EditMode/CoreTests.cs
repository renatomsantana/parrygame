using System.IO;
using Apara.Core;
using NUnit.Framework;
using UnityEngine;

namespace Apara.Tests
{
    /// <summary>Test Runner (Window > General > Test Runner > EditMode).</summary>
    public class CoreTests
    {
        [Test]
        public void RegrasMestresFintasETrilha()
        {
            CoreSelfTest result = CoreSelfTest.Run();
            Assert.That(result.Checks, Is.GreaterThan(200));
            Assert.That(result.Failures, Is.Empty, string.Join("\n", result.Failures));
        }

        [Test]
        public void PranchaDeFrames()
        {
            string path = Path.Combine(Application.dataPath, "Resources/Art/frames.json");
            CoreSelfTest result = CoreSelfTest.RunSheet(File.ReadAllText(path));
            Assert.That(result.Failures, Is.Empty, string.Join("\n", result.Failures));
        }
    }
}

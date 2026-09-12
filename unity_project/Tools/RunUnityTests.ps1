# Roda os testes EditMode no Unity em batchmode e grava Builds/tests.xml.
# Uso: powershell -ExecutionPolicy Bypass -File Tools/RunUnityTests.ps1 [-UnityExe caminho\Unity.exe]
param([string]$UnityExe = "")
$ErrorActionPreference = "Stop"
$root = Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)
. (Join-Path $root "Tools\FindUnity.ps1")
$unity = Find-Unity $root $UnityExe
$results = Join-Path $root "Builds\tests.xml"
New-Item -ItemType Directory -Force (Split-Path $results) | Out-Null
& $unity -batchmode -nographics -projectPath $root -runTests -testPlatform EditMode -testResults $results -logFile (Join-Path $root "Builds\tests.log")
$code = $LASTEXITCODE
if (Test-Path $results) {
    [xml]$xml = Get-Content $results
    $run = $xml.'test-run'
    Write-Host ("Unity EditMode: {0} testes, {1} falhas" -f $run.total, $run.failed)
}
exit $code

# Gera Builds/Windows/APARA.exe sem abrir a interface do Unity.
# Uso: powershell -ExecutionPolicy Bypass -File Tools/Build.ps1 [-UnityExe caminho\Unity.exe]
# Sem -UnityExe, procura a versão de ProjectSettings/ProjectVersion.txt no Unity Hub.
param([string]$UnityExe = "")
$ErrorActionPreference = "Stop"
$root = Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)
. (Join-Path $root "Tools\FindUnity.ps1")
$unity = Find-Unity $root $UnityExe
$log = Join-Path $root "Builds\build.log"
New-Item -ItemType Directory -Force (Split-Path $log) | Out-Null
& $unity -batchmode -nographics -quit -projectPath $root -executeMethod Apara.EditorTools.BuildPlayer.Windows -logFile $log
if ($LASTEXITCODE -ne 0) { Write-Host "Build falhou; veja $log"; exit $LASTEXITCODE }
Write-Host "Build em $(Join-Path $root 'Builds\Windows\APARA.exe')"

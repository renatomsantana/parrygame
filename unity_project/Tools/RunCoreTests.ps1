# Compila Assets/Scripts/Core com o csc.exe do .NET Framework (presente em todo
# Windows) e roda o harness. Não precisa de Unity nem de .NET SDK.
# Uso: powershell -ExecutionPolicy Bypass -File Tools/RunCoreTests.ps1
$ErrorActionPreference = "Stop"
$root = Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)
$csc = Join-Path $env:WINDIR "Microsoft.NET\Framework64\v4.0.30319\csc.exe"
if (-not (Test-Path $csc)) { $csc = Join-Path $env:WINDIR "Microsoft.NET\Framework\v4.0.30319\csc.exe" }
$out = Join-Path $root "Tools\CoreTestHarness\bin"
New-Item -ItemType Directory -Force $out | Out-Null
$sources = @(Get-ChildItem (Join-Path $root "Assets\Scripts\Core") -Filter *.cs | ForEach-Object { $_.FullName })
$sources += Join-Path $root "Tools\CoreTestHarness\Program.cs"
$exe = Join-Path $out "AparaCoreTests.exe"
& $csc /nologo /target:exe /langversion:5 /warn:4 /out:$exe $sources
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
& $exe (Join-Path $root "Assets\Resources\Art\frames.json")
exit $LASTEXITCODE

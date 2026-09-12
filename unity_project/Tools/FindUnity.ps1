# Localiza o Unity.exe: o caminho passado, ou a versão do projeto instalada pelo Unity Hub.
function Find-Unity([string]$root, [string]$given) {
    if ($given -and (Test-Path $given)) { return $given }
    $versionLine = Get-Content (Join-Path $root "ProjectSettings\ProjectVersion.txt") | Select-Object -First 1
    $version = ($versionLine -split ":\s*")[1]
    $candidates = @(
        "C:\Program Files\Unity\Hub\Editor\$version\Editor\Unity.exe",
        "D:\Unity\Hub\Editor\$version\Editor\Unity.exe",
        "$env:LOCALAPPDATA\Programs\Unity\Hub\Editor\$version\Editor\Unity.exe"
    )
    foreach ($c in $candidates) { if (Test-Path $c) { return $c } }
    $hub = "C:\Program Files\Unity\Hub\Editor"
    if (Test-Path $hub) {
        $any = Get-ChildItem $hub -Directory | Sort-Object Name -Descending | Select-Object -First 1
        if ($any) {
            $exe = Join-Path $any.FullName "Editor\Unity.exe"
            if (Test-Path $exe) { Write-Host "Aviso: usando Unity $($any.Name), o projeto pede $version"; return $exe }
        }
    }
    throw "Unity $version não encontrado. Instale pelo Unity Hub ou passe -UnityExe."
}

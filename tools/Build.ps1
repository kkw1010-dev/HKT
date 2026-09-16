<#
.SYNOPSIS
  Compile every CIGAR module, patch the Korean text into the .pex files, and
  optionally deploy to the MO2 mod folder and run the deployment checks.

.DESCRIPTION
  Fails on the first compile error, on any unmapped, unused or leftover text
  placeholder, and (with -Deploy) on any failed deployment check. Skyrim must be
  closed when deploying; MO2 may stay open (the profile files are only read).

.EXAMPLE
  powershell -ExecutionPolicy Bypass -File tools\Build.ps1 -Deploy
#>
param(
    [switch]$Deploy
)

$ErrorActionPreference = 'Stop'
$Repo      = Split-Path -Parent $PSScriptRoot
$Mods      = 'C:\TAKEALOOK\mods'
$Compiler  = Join-Path $Mods 'Creation Kit\Root\Papyrus Compiler\PapyrusCompiler.exe'
$Vanilla   = Join-Path $Mods 'Papyrus Compiler\source\scripts'
$Flags     = Join-Path $Vanilla 'TESV_Papyrus_Flags.flg'
$ModFolder = Join-Path $Mods 'CIGAR'
$BuildDir  = Join-Path $Repo 'build\Scripts'
$Strings   = Join-Path $Repo 'strings.ko.json'

$ModuleSources = Get-ChildItem (Join-Path $Repo 'modules') -Directory |
    ForEach-Object { Join-Path $_.FullName 'Source\Scripts' }

# Precedence: CIGAR sources, stubs, SKSE (extends vanilla classes), framework APIs, vanilla.
$Imports = @($ModuleSources) + @(
    (Join-Path $Repo 'stubs'),
    (Join-Path $Mods 'Skyrim Script Extender (SKSE64)\Scripts\Source'),
    (Join-Path $Mods '[NoDelete] 0007 SkyPrompt NEW\Scripts\Source'),
    (Join-Path $Mods "powerofthree's Papyrus Extender\Source\Scripts"),
    (Join-Path $Mods 'PapyrusUtil SE - Modders Scripting Utility Functions\Source\Scripts'),
    $Vanilla
)

foreach ($p in @($Compiler, $Flags, $Strings) + $Imports) {
    if (-not (Test-Path -LiteralPath $p)) { throw "Missing build input: $p" }
}

New-Item -ItemType Directory -Force $BuildDir | Out-Null
Get-ChildItem $BuildDir -Filter *.pex | Remove-Item -Force

$importArg = $Imports -join ';'
$built = @()
foreach ($srcDir in $ModuleSources) {
    foreach ($psc in Get-ChildItem $srcDir -Filter *.psc) {
        Write-Host "compile $($psc.Name)"
        # The compiler reports on stderr; judge by exit code, not by stderr output.
        $ErrorActionPreference = 'Continue'
        $out = & $Compiler $psc.FullName "-i=$importArg" "-o=$BuildDir" "-f=$Flags" 2>&1
        $ErrorActionPreference = 'Stop'
        if ($LASTEXITCODE -ne 0) {
            $out | Write-Host
            throw "Compile failed: $($psc.Name)"
        }
        $built += Join-Path $BuildDir ($psc.BaseName + '.pex')
    }
}

$patcher = Join-Path $PSScriptRoot 'patch_pex_strings.py'
& python $patcher $Strings @built
if ($LASTEXITCODE -ne 0) { throw 'Text patch failed.' }
& python $patcher --verify $Strings @built
if ($LASTEXITCODE -ne 0) { throw 'Text verify failed.' }

if (-not $Deploy) {
    Write-Host "build OK (not deployed): $BuildDir"
    exit 0
}

# Deploying only copies files into the mod folder; a running game would keep the old
# scripts loaded and SI could overwrite settings.json on exit.
if (Get-Process -Name SkyrimSE -ErrorAction SilentlyContinue) {
    throw 'Close Skyrim before deploying.'
}

$deployScripts = Join-Path $ModFolder 'Scripts'
$deploySource  = Join-Path $ModFolder 'Source\Scripts'
New-Item -ItemType Directory -Force $deployScripts, $deploySource | Out-Null
Get-ChildItem $deployScripts -Filter 'CIGAR_*.pex' | Remove-Item -Force
Get-ChildItem $deploySource -Filter 'CIGAR_*.psc' | Remove-Item -Force
Copy-Item $built $deployScripts
foreach ($srcDir in $ModuleSources) {
    Copy-Item (Join-Path $srcDir '*.psc') $deploySource
}
Copy-Item (Join-Path $Repo 'plugin\CIGAR.esp') $ModFolder

& python (Join-Path $PSScriptRoot 'sync_si_settings.py')
if ($LASTEXITCODE -ne 0) { throw 'SI settings override failed.' }

& python (Join-Path $PSScriptRoot 'verify_deploy.py')
if ($LASTEXITCODE -ne 0) { throw 'Deployment checks failed.' }
Write-Host 'deploy OK'

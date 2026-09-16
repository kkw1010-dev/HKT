<#
.SYNOPSIS
  Compile every SI-Extensions module, patch its Korean text into the .pex files,
  and optionally deploy to the MO2 mod folder and run the deployment checks.

.DESCRIPTION
  Fails on the first compile error, on any unpatched or missing text placeholder,
  and (with -Deploy) on any failed deployment check. Skyrim must be closed when
  deploying; MO2 may stay open (the profile files are only read).

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
$ModFolder = Join-Path $Mods 'SI-Extensions'
$BuildDir  = Join-Path $Repo 'build\Scripts'

# Precedence: stubs, then SKSE (extends vanilla classes), then framework APIs, then vanilla.
$Imports = @(
    (Join-Path $Repo 'stubs'),
    (Join-Path $Mods 'Skyrim Script Extender (SKSE64)\Scripts\Source'),
    (Join-Path $Mods '[NoDelete] 0007 SkyPrompt NEW\Scripts\Source'),
    (Join-Path $Mods "powerofthree's Papyrus Extender\Source\Scripts"),
    (Join-Path $Mods 'PapyrusUtil SE - Modders Scripting Utility Functions\Source\Scripts'),
    $Vanilla
)

foreach ($p in @($Compiler, $Flags) + $Imports) {
    if (-not (Test-Path -LiteralPath $p)) { throw "Missing build input: $p" }
}

New-Item -ItemType Directory -Force $BuildDir | Out-Null
Get-ChildItem $BuildDir -Filter *.pex | Remove-Item -Force

foreach ($module in Get-ChildItem (Join-Path $Repo 'modules') -Directory) {
    $srcDir = Join-Path $module.FullName 'Source\Scripts'
    $sources = Get-ChildItem $srcDir -Filter *.psc
    foreach ($psc in $sources) {
        $importArg = (@($srcDir) + $Imports) -join ';'
        Write-Host "compile $($module.Name)/$($psc.Name)"
        # The compiler reports on stderr; judge by exit code, not by stderr output.
        $ErrorActionPreference = 'Continue'
        $out = & $Compiler $psc.FullName "-i=$importArg" "-o=$BuildDir" "-f=$Flags" 2>&1
        $ErrorActionPreference = 'Stop'
        if ($LASTEXITCODE -ne 0) {
            $out | Write-Host
            throw "Compile failed: $($psc.Name)"
        }
    }

    $strings = Join-Path $module.FullName 'strings.ko.json'
    if (Test-Path $strings) {
        $patched = 0
        foreach ($psc in $sources) {
            $pex = Join-Path $BuildDir ($psc.BaseName + '.pex')
            $bytes = [IO.File]::ReadAllBytes($pex)
            if ([Text.Encoding]::ASCII.GetString($bytes).Contains('@SIX:')) {
                & python (Join-Path $PSScriptRoot 'patch_pex_strings.py') $pex $strings
                if ($LASTEXITCODE -ne 0) { throw "Text patch failed: $pex" }
                & python (Join-Path $PSScriptRoot 'patch_pex_strings.py') --verify $pex $strings
                if ($LASTEXITCODE -ne 0) { throw "Text verify failed: $pex" }
                $patched++
            }
        }
        if ($patched -eq 0) { throw "$strings exists but no compiled script of $($module.Name) uses a placeholder" }
    }
}

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
Get-ChildItem $deployScripts -Filter 'SIX_*.pex' | Remove-Item -Force
Copy-Item (Join-Path $BuildDir 'SIX_*.pex') $deployScripts
foreach ($module in Get-ChildItem (Join-Path $Repo 'modules') -Directory) {
    Copy-Item (Join-Path $module.FullName 'Source\Scripts\*.psc') $deploySource
}

& python (Join-Path $PSScriptRoot 'sync_si_settings.py')
if ($LASTEXITCODE -ne 0) { throw 'SI settings override failed.' }

& python (Join-Path $PSScriptRoot 'verify_deploy.py')
if ($LASTEXITCODE -ne 0) { throw 'Deployment checks failed.' }
Write-Host 'deploy OK'

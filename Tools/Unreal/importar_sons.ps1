<#
.SYNOPSIS
    Importa Audio/*.wav para /Game/Zombie/Sons sem abrir o editor (Tools/Unreal/importar_sons.py).
.EXAMPLE
    .\Tools\Unreal\importar_sons.ps1
.NOTES
    Código de saída: 0 deu certo; 1 erro. Feche o editor antes (ele segura os pacotes). Os .uasset criados vão no Git LFS.
#>
param(
    [string]$Uproject = 'Zombie.uproject',
    [string]$EngineVersion,
    [int]$TimeoutSeconds = 600
)
$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'Find-Unreal.ps1')

try { $ctx = Resolve-UnrealContext $Uproject $EngineVersion } catch { Write-Error $_.Exception.Message; exit 1 }

$exe = Join-Path $ctx.Root 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
$script = (Join-Path $PSScriptRoot 'importar_sons.py').Replace('\', '/')
$projDir = Split-Path $ctx.Uproject
$log = Join-Path $projDir 'Saved\Logs\importar-sons.log'

# Python embutido não pode gravar .pyc na instalação da engine (também travado dentro do .py).
$env:PYTHONDONTWRITEBYTECODE = '1'
$argList = @("`"$($ctx.Uproject)`"", '-run=PythonScript', "-Script=`"$script`"", '-unattended', '-nullrhi', '-nosplash', "-abslog=`"$log`"")
Write-Host "Importando os sons de Audio/ (limite ${TimeoutSeconds}s)"
$p = Start-Process $exe -ArgumentList $argList -PassThru -WindowStyle Hidden
if (-not $p.WaitForExit($TimeoutSeconds * 1000)) { $p.Kill(); Write-Host "TIMEOUT. Log: $log"; exit 1 }
if ($p.ExitCode -ne 0) { Write-Host "FALHOU (código $($p.ExitCode)). Log: $log"; exit 1 }
$ok = Select-String -LiteralPath $log -Pattern 'importar_sons OK:' -SimpleMatch | Select-Object -Last 1
if (-not $ok) { Write-Host "Sem a linha de confirmação no log: $log"; exit 1 }
Write-Host $ok.Line
exit 0

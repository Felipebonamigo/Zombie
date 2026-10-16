<#
.SYNOPSIS
    Importa as texturas PBR do Poly Haven (CC0) em /Game/Externo/PolyHaven/Superficies e cria M_ZMBSuperficie e as instâncias
    (Tools/Unreal/criar_superficies.py). Roda com -ExecutePythonScript, com o editor fechado.
.EXAMPLE
    .\Tools\Unreal\criar_superficies.ps1
.NOTES
    Código de saída: 0 deu certo; 1 erro. Feche o editor antes (ele segura os pacotes). O material-pai vai no Git LFS; texturas e instâncias ficam em Content/Externo/ (fora do git).
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
$script = (Join-Path $PSScriptRoot 'criar_superficies.py').Replace('\', '/')
$projDir = Split-Path $ctx.Uproject
$log = Join-Path $projDir 'Saved\Logs\criar-superficies.log'

# Python embutido não pode gravar .pyc na instalação da engine (também travado dentro do .py).
$env:PYTHONDONTWRITEBYTECODE = '1'
$argList = @("`"$($ctx.Uproject)`"", "-ExecutePythonScript=`"$script`"", '-unattended', '-nopause', '-nosplash', '-nosound', "-abslog=`"$log`"")
Write-Host "Criando as superfícies PBR (asfalto, calçada, fachada) (limite ${TimeoutSeconds}s)"
$p = Start-Process $exe -ArgumentList $argList -PassThru -WindowStyle Hidden
if (-not $p.WaitForExit($TimeoutSeconds * 1000)) { $p.Kill(); Write-Host "TIMEOUT. Log: $log"; exit 1 }
$ok = Select-String -LiteralPath $log -Pattern 'criar_superficies OK' -SimpleMatch | Select-Object -Last 1
if (-not $ok) { Write-Host "Sem a linha de confirmação no log: $log"; exit 1 }
Write-Host $ok.Line
exit 0

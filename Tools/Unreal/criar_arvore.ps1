<#
.SYNOPSIS
    Importa a island_tree_02 (Poly Haven, CC0) em /Game/Externo/PolyHaven e cria M_ZMBFolha e M_ZMBTronco (Tools/Unreal/criar_arvore.py).
    Roda com -ExecutePythonScript (sem -nullrhi): o gerador de LODs precisa dos subsistemas do editor.
.EXAMPLE
    .\Tools\Unreal\criar_materiais.ps1
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
$script = (Join-Path $PSScriptRoot 'criar_arvore.py').Replace('\', '/')
$projDir = Split-Path $ctx.Uproject
$log = Join-Path $projDir 'Saved\Logs\criar-arvore.log'

# Python embutido não pode gravar .pyc na instalação da engine (também travado dentro do .py).
$env:PYTHONDONTWRITEBYTECODE = '1'
$argList = @("`"$($ctx.Uproject)`"", "-ExecutePythonScript=`"$script`"", '-unattended', '-nopause', '-nosplash', '-nosound', "-abslog=`"$log`"")
Write-Host "Criando a árvore folhuda e os materiais dela (limite ${TimeoutSeconds}s)"
$p = Start-Process $exe -ArgumentList $argList -PassThru -WindowStyle Hidden
if (-not $p.WaitForExit($TimeoutSeconds * 1000)) { $p.Kill(); Write-Host "TIMEOUT. Log: $log"; exit 1 }
$ok = Select-String -LiteralPath $log -Pattern 'criar_arvore OK' -SimpleMatch | Select-Object -Last 1
if (-not $ok) { Write-Host "Sem a linha de confirmação no log: $log"; exit 1 }
Write-Host $ok.Line
exit 0

<#
.SYNOPSIS
    Abre o Zombie (AO VIVO) em modo jogo (sem o editor), com as opções do protótipo; também tira print e fecha sozinho.
.EXAMPLE
    .\Tools\Unreal\jogar.ps1                              # joga numa janela 1600x900 (feche a janela para sair)
    .\Tools\Unreal\jogar.ps1 -Shot 8                      # print aos 8 s (dia) e fecha
    .\Tools\Unreal\jogar.ps1 -Shot 8 -Hour 21 -Cam 3      # print de noite, no modo celular
    .\Tools\Unreal\jogar.ps1 -Shot 20 -Horde -Cam 1       # print no meio de uma horda, câmera média
    .\Tools\Unreal\jogar.ps1 -Twitch nomedocanal          # lê o chat de verdade da Twitch
.NOTES
    Compile antes (build.ps1). Código de saída: 0 o jogo fechou sem erro (e com -Shot, o print existe); 1 falhou.
    O print fica em Saved\Screenshots\<plataforma>\zmb-shot.png; o caminho sai no fim. Log: Saved\Logs\jogar.log.
    Opções do jogo: Game/ZMBGameMode.h (-ZMBSeed, -ZMBHour, -ZMBCam, -ZMBHorde, -ZMBShot) e Stream/ZMBTwitchChat.h.
#>
param(
    [string]$Uproject = 'Zombie.uproject',
    [int]$Seed = 2026,
    [double]$Hour = -1,
    [ValidateRange(-1, 3)][int]$Cam = -1,
    [switch]$Horde,
    [ValidateRange(0, 7)][int]$Lookdev = 0,   # cena de aparência: ângulo fixo 1 a 7 (5 poste, 6 praça, 7 telhado), 17,5 h, relógio parado, sem horda
    [double]$Shot = 0,
    [string]$Twitch,
    [int]$ResX = 1600,
    [int]$ResY = 900,
    [string[]]$Exec,
    [string]$EngineVersion,
    [int]$TimeoutSeconds = 300
)
$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'Find-Unreal.ps1')

try {
    $ctx = Resolve-UnrealContext $Uproject $EngineVersion
} catch {
    Write-Error $_.Exception.Message
    exit 1
}

$exe = Join-Path $ctx.Root 'Engine\Binaries\Win64\UnrealEditor.exe'
$projDir = Split-Path $ctx.Uproject
$log = Join-Path $projDir 'Saved\Logs\jogar.log'
$argList = @("`"$($ctx.Uproject)`"", '-game', '-windowed', "-ResX=$ResX", "-ResY=$ResY", "-ZMBSeed=$Seed", "-abslog=`"$log`"")
if ($Hour -ge 0) { $argList += "-ZMBHour=$($Hour.ToString([Globalization.CultureInfo]::InvariantCulture))" }
if ($Cam -ge 0) { $argList += "-ZMBCam=$Cam" }
if ($Horde) { $argList += '-ZMBHorde' }
if ($Lookdev -gt 0) { $argList += "-ZMBLookdev=$Lookdev" }
if ($Twitch) { $argList += "-ZMBTwitch=$Twitch" }
if ($Shot -gt 0) { $argList += "-ZMBShot=$($Shot.ToString([Globalization.CultureInfo]::InvariantCulture))" }
if ($Exec) { $argList += "-ExecCmds=`"$($Exec -join ',')`"" }

# print anterior com o mesmo nome atrapalharia a conferência
$shots = Join-Path $projDir 'Saved\Screenshots'
if ($Shot -gt 0 -and (Test-Path -LiteralPath $shots)) {
    Get-ChildItem -LiteralPath $shots -Recurse -Filter 'zmb-shot*.png' | Remove-Item -Force
}

Write-Host "Abrindo o jogo: $($argList -join ' ')"
$p = Start-Process $exe -ArgumentList $argList -PassThru
if ($Shot -le 0) {
    $p.WaitForExit()
    exit 0
}
if (-not $p.WaitForExit($TimeoutSeconds * 1000)) {
    $p.Kill()
    Write-Host "TIMEOUT depois de ${TimeoutSeconds}s. Log: $log"
    exit 1
}
$found = if (Test-Path -LiteralPath $shots) { Get-ChildItem -LiteralPath $shots -Recurse -Filter 'zmb-shot*.png' | Select-Object -First 1 } else { $null }
if (-not $found) {
    Write-Host "O jogo fechou sem print. Veja o log: $log"
    exit 1
}
Write-Host "PRINT: $($found.FullName)"
$perf = Select-String -LiteralPath $log -Pattern 'ZMBPerf:' -SimpleMatch | Select-Object -Last 2
$perf | ForEach-Object { Write-Host $_.Line }
$errors = Select-String -LiteralPath $log -Pattern 'Error:|Fatal' -SimpleMatch:$false | Where-Object { $_.Line -notmatch 'LogPython|GameFeatures|LoadErrors' } | Select-Object -First 10
if ($errors) {
    Write-Host '--- erros no log ---'
    $errors | ForEach-Object { Write-Host $_.Line }
}
exit 0

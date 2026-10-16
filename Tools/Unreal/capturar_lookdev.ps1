<#
.SYNOPSIS
    Tira as quatro capturas da cena de aparência (jogar.ps1 -Lookdev 1..4) e guarda em docs/img/realismo/.
.EXAMPLE
    .\Tools\Unreal\capturar_lookdev.ps1 -Passo 0-base -Quando antes
    .\Tools\Unreal\capturar_lookdev.ps1 -Passo 1-luz -Quando depois -Hour 21     # a noite, ângulos com sufixo -noite
.NOTES
    Nome: AAAA-MM-DD-<passo>-<antes|depois>-<angulo>[-noite].png. Imprime a linha ZMBPerf (média e pior quadro, sem
    vsync nem limite de fps) de cada ângulo. Compile antes (build.ps1).
#>
param(
    [Parameter(Mandatory)][string]$Passo,
    [Parameter(Mandatory)][ValidateSet('antes', 'depois')][string]$Quando,
    [double]$Hour = -1,
    [double]$Shot = 10,
    [int[]]$Angulos = @(1, 2, 3, 4),   # 5 = poste de perto, 6 = praça de perto
    [int]$Seed = 2026
)
$ErrorActionPreference = 'Stop'
$root = Split-Path (Split-Path $PSScriptRoot)
$out = Join-Path $root 'docs\img\realismo'
New-Item -ItemType Directory -Force $out | Out-Null
$nomes = @('rua', 'orla', 'mar', 'leste', 'poste', 'praca', 'telhado')
$sufixo = if ($Hour -ge 0 -and ($Hour -ge 19.5 -or $Hour -lt 5.5)) { '-noite' } else { '' }
foreach ($a in $Angulos) {
    $args = @{ Lookdev = $a; Seed = $Seed; Cam = 3; Shot = $Shot; Exec = @('r.VSync 0', 't.MaxFPS 0') }
    if ($Hour -ge 0) { $args.Hour = $Hour }
    & (Join-Path $PSScriptRoot 'jogar.ps1') @args
    $print = Get-ChildItem (Join-Path $root 'Saved\Screenshots') -Recurse -Filter 'zmb-shot*.png' | Select-Object -First 1
    if (-not $print) { Write-Error "sem print no ângulo $a"; exit 1 }
    $dest = Join-Path $out ("{0}-{1}-{2}-{3}{4}.png" -f (Get-Date -Format 'yyyy-MM-dd'), $Passo, $Quando, $nomes[$a - 1], $sufixo)
    Copy-Item $print.FullName $dest -Force
    Write-Host "SALVO: $dest"
}

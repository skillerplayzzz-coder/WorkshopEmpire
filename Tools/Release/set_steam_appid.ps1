# Phase 20: puts the game's own Steam App ID (from Steamworks) into every place that needs it.
#   powershell -File Tools\Release\set_steam_appid.ps1 -AppId 1234567 [-DepotId 1234568]
# - Config/DefaultEngine.ini [OnlineSubsystemSteam] SteamDevAppId (the engine reads it in every build type)
#   and bRelaunchInSteam (a release started outside Steam restarts through Steam; off for the test ID 480)
# - Tools/Release/steam/*.vdf (upload scripts for steamcmd, depot = App ID + 1 unless given)
# Run package_shipping.ps1 afterwards: the ID is baked into the build.
param(
    [Parameter(Mandatory = $true)][int]$AppId,
    [int]$DepotId = 0
)
$ErrorActionPreference = 'Stop'
$root = Resolve-Path (Join-Path $PSScriptRoot '..\..')
if ($DepotId -le 0) { $DepotId = $AppId + 1 }

$ini = Join-Path $root 'Config\DefaultEngine.ini'
$text = [IO.File]::ReadAllText($ini)
$text = [Text.RegularExpressions.Regex]::Replace($text, '(?m)^SteamDevAppId=\d+', "SteamDevAppId=$AppId")
$relaunch = if ($AppId -eq 480) { 'false' } else { 'true' }
if ($text -match '(?m)^bRelaunchInSteam=') {
    $text = [Text.RegularExpressions.Regex]::Replace($text, '(?m)^bRelaunchInSteam=\w+', "bRelaunchInSteam=$relaunch")
} else {
    $text = [Text.RegularExpressions.Regex]::Replace($text, '(?m)^(SteamDevAppId=\d+)', "`$1`r`nbRelaunchInSteam=$relaunch")
}
[IO.File]::WriteAllText($ini, $text)

$steam = Join-Path $PSScriptRoot 'steam'
foreach ($template in Get-ChildItem $steam -Filter '*.vdf.template') {
    $out = Join-Path $steam ($template.Name -replace '\.template$', '')
    $vdf = [IO.File]::ReadAllText($template.FullName).Replace('{APP_ID}', "$AppId").Replace('{DEPOT_ID}', "$DepotId")
    [IO.File]::WriteAllText($out, $vdf)
}
Write-Output "Steam App ID $AppId (depot $DepotId) set: Config/DefaultEngine.ini, Tools/Release/steam/*.vdf. bRelaunchInSteam=$relaunch"

# Phase 20: the release build (Shipping: no console, no dev automation, optimised).
#   powershell -File Tools\Release\package_shipping.ps1 [-Name Release] [-Engine F:\UE_5.7]
# Output: <workspace>\Builds\<Name>\Windows (outside the repository; Builds/ is never committed).
param(
    [string]$Name = 'Release',
    [string]$Engine = 'F:\UE_5.7'
)
$ErrorActionPreference = 'Stop'
$root = Resolve-Path (Join-Path $PSScriptRoot '..\..')
$project = Join-Path $root 'WorkshopEmpire.uproject'
$archive = Join-Path (Split-Path $root -Parent) "Builds\$Name"
& "$Engine\Engine\Build\BatchFiles\RunUAT.bat" BuildCookRun "-project=$project" -noP4 -platform=Win64 -clientconfig=Shipping `
    -build -cook -stage -pak -iostore -compressed -prereqs -archive "-archivedirectory=$archive" -utf8output -nocompileeditor
if ($LASTEXITCODE -ne 0) { throw "BuildCookRun failed ($LASTEXITCODE)" }
Write-Output "Shipping build: $archive\Windows\WorkshopEmpire.exe"

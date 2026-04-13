param(
    [Parameter(Mandatory = $true)]
    [string]$ProjectRoot
)

$copyScript = "D:\\codex\\projectB\\firmware\\copy_to_cubeide_project.ps1"
$patchScript = "D:\\codex\\projectB\\firmware\\patch_cubeide_main.ps1"

if (-not (Test-Path $copyScript) -or -not (Test-Path $patchScript)) {
    Write-Error "Setup scripts missing under D:\\codex\\projectB\\firmware"
    exit 1
}

powershell -ExecutionPolicy Bypass -File $copyScript -ProjectRoot $ProjectRoot
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

if (-not (Test-Path (Join-Path $ProjectRoot "Core\\Src\\main.c"))) {
    Write-Host "main.c not found yet. Please open .ioc in CubeIDE and click Generate Code first."
    Write-Host "Template files have been copied already."
    exit 0
}

powershell -ExecutionPolicy Bypass -File $patchScript -ProjectRoot $ProjectRoot
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

Write-Host "ProjectB STM32 MVP setup done for: $ProjectRoot"

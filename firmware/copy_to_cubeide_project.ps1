param(
    [Parameter(Mandatory = $true)]
    [string]$ProjectRoot
)

$srcBase = "D:\codex\projectB\firmware\stm32_mvp_template"
$incDir = Join-Path $ProjectRoot "Core\Inc"
$srcDir = Join-Path $ProjectRoot "Core\Src"

if (-not (Test-Path $incDir) -or -not (Test-Path $srcDir)) {
    Write-Error "Target project path invalid. Expected Core\\Inc and Core\\Src under: $ProjectRoot"
    exit 1
}

Copy-Item -LiteralPath (Join-Path $srcBase "curtain_ctrl.h") -Destination (Join-Path $incDir "curtain_ctrl.h") -Force
Copy-Item -LiteralPath (Join-Path $srcBase "curtain_port.h") -Destination (Join-Path $incDir "curtain_port.h") -Force
Copy-Item -LiteralPath (Join-Path $srcBase "app_entry.h") -Destination (Join-Path $incDir "app_entry.h") -Force
Copy-Item -LiteralPath (Join-Path $srcBase "curtain_ctrl.c") -Destination (Join-Path $srcDir "curtain_ctrl.c") -Force
Copy-Item -LiteralPath (Join-Path $srcBase "app_entry.c") -Destination (Join-Path $srcDir "app_entry.c") -Force
Copy-Item -LiteralPath (Join-Path $srcBase "example_main_loop.c") -Destination (Join-Path $srcDir "example_main_loop.c") -Force

Write-Host "Copied STM32 MVP template files to: $ProjectRoot"

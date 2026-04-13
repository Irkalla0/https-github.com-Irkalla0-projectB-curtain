param(
    [Parameter(Mandatory = $true)]
    [string]$ProjectRoot
)

$mainPath = Join-Path $ProjectRoot "Core\\Src\\main.c"
if (-not (Test-Path $mainPath)) {
    Write-Error "main.c not found: $mainPath"
    exit 1
}

$content = Get-Content -Path $mainPath -Raw -Encoding UTF8

if ($content -notmatch '#include\\s+\"app_entry.h\"') {
    $content = $content -replace '(\\/\\* USER CODE END Includes \\*\\/)', "#include ""app_entry.h""`r`n`$1"
}

if ($content -notmatch 'App_Init\\s*\\(\\s*\\)') {
    $content = $content -replace '(\\/\\* USER CODE END 2 \\*\\/)', "  App_Init();`r`n  `$1"
}

if ($content -notmatch 'App_Run10msScheduler\\s*\\(\\s*\\)') {
    $content = $content -replace '(\\/\\* USER CODE BEGIN WHILE \\*\\/)', "`$1`r`n  App_Run10msScheduler();"
}

Set-Content -Path $mainPath -Value $content -Encoding UTF8
Write-Host "Patched main.c at: $mainPath"

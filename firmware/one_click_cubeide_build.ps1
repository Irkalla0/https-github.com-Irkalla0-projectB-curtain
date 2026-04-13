param(
    [string]$ProjectRoot = "D:\codex\projectB\firmware\cubeide_project\CurtainMVP_F103",
    [string]$Workspace = "D:\codex\projectB\firmware\cubeide_workspace"
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Get-CubeIDEConsoleExe {
    $cmd = Get-Command stm32cubeidec.exe -ErrorAction SilentlyContinue
    if ($cmd) {
        return $cmd.Source
    }

    $keys = @(
        "HKLM:\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\*",
        "HKLM:\SOFTWARE\WOW6432Node\Microsoft\Windows\CurrentVersion\Uninstall\*",
        "HKCU:\Software\Microsoft\Windows\CurrentVersion\Uninstall\*"
    )

    foreach ($k in $keys) {
        $apps = Get-ItemProperty $k -ErrorAction SilentlyContinue |
            Where-Object {
                ($_.PSObject.Properties.Name -contains "DisplayName") -and
                ($_.DisplayName -like "*STM32CubeIDE*")
            }
        foreach ($app in $apps) {
            $base = ($app.InstallLocation -replace '"', '').Trim()
            if (-not [string]::IsNullOrWhiteSpace($base)) {
                $candidate = Join-Path $base "STM32CubeIDE\stm32cubeidec.exe"
                if (Test-Path $candidate) {
                    return $candidate
                }
            }
        }
    }

    throw "Cannot find stm32cubeidec.exe. Please confirm STM32CubeIDE is installed."
}

function Get-CubeIDEGuiExe([string]$ConsoleExe) {
    $candidate = $ConsoleExe -replace "stm32cubeidec\.exe$", "stm32cubeide.exe"
    if (Test-Path $candidate) {
        return $candidate
    }
    throw "Cannot find stm32cubeide.exe near: $ConsoleExe"
}

$projectDir = Join-Path $ProjectRoot "STM32CubeIDE"
$iocPath = Join-Path $ProjectRoot "CurtainMVP_F103.ioc"
$projectFile = Join-Path $projectDir ".project"
$setupScript = Join-Path (Split-Path -Parent $PSCommandPath) "setup_projectb_stm32.ps1"

if (-not (Test-Path $projectFile)) {
    throw "Project file not found: $projectFile"
}
if (-not (Test-Path $iocPath)) {
    throw "IOC file not found: $iocPath"
}

New-Item -ItemType Directory -Path $Workspace -Force | Out-Null

if (Test-Path $setupScript) {
    Write-Host "[0/3] Syncing template + patches..."
    & powershell -ExecutionPolicy Bypass -File $setupScript -ProjectRoot $ProjectRoot
    if ($LASTEXITCODE -ne 0) {
        throw "Setup script failed with exit code: $LASTEXITCODE"
    }
}

$debugDir = Join-Path $projectDir "Debug"
if (Test-Path $debugDir) {
    Remove-Item -LiteralPath $debugDir -Recurse -Force
}

$cubeidec = Get-CubeIDEConsoleExe
$cubeide = Get-CubeIDEGuiExe -ConsoleExe $cubeidec

Write-Host "[1/3] Import + build (headless)..."
& $cubeidec `
    -nosplash `
    -application org.eclipse.cdt.managedbuilder.core.headlessbuild `
    -data $Workspace `
    -import $projectDir `
    -build "CurtainMVP_F103/Debug"

$buildOk = $true
if ($LASTEXITCODE -ne 0) {
    $workspaceLock = Join-Path $Workspace ".metadata\\.lock"
    if (Test-Path $workspaceLock) {
        Write-Warning "Workspace is currently open. Skipping headless build."
        $buildOk = $false
    } else {
        throw "Headless build failed with exit code: $LASTEXITCODE"
    }
}

if ($buildOk) {
    Write-Host "[2/3] Build done."
} else {
    Write-Host "[2/3] Build skipped (workspace in use)."
}
Write-Host "[3/3] Opening STM32CubeIDE with imported workspace..."
Start-Process -FilePath $cubeide -ArgumentList @("-data", $Workspace, "--launcher.openFile", $iocPath) | Out-Null

Write-Host "Done. Project is imported and built. You can click Run/Debug directly."

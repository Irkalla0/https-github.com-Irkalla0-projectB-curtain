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

# 1) include app entry
if ($content -notmatch '#include\s+\"app_entry.h\"') {
    $content = $content -replace '(\/\* USER CODE END Includes \*\/)', "#include ""app_entry.h""`r`n`$1"
}

# 2) ensure TIM handle
if ($content -notmatch 'TIM_HandleTypeDef\s+htim1\s*;') {
    $content = $content -replace 'UART_HandleTypeDef\s+huart1\s*;', "TIM_HandleTypeDef htim1;`r`nUART_HandleTypeDef huart1;"
}

# 3) ensure TIM init prototype
if ($content -notmatch 'static void MX_TIM1_Init\(void\)\s*;') {
    $content = $content -replace 'static void MX_USART1_UART_Init\(void\)\s*;', "static void MX_TIM1_Init(void);`r`nstatic void MX_USART1_UART_Init(void);"
}

# 4) ensure app init call
if ($content -notmatch 'App_Init\s*\(\s*\)\s*;') {
    $content = $content -replace '(\/\* USER CODE END 2 \*\/)', "  App_Init();`r`n`$1"
}

# 5) ensure scheduler call
if ($content -notmatch 'App_Run10msScheduler\s*\(\s*\)\s*;') {
    $content = $content -replace '(\/\* USER CODE BEGIN WHILE \*\/)', "`$1`r`n    App_Run10msScheduler();"
}

# 6) ensure MX_TIM1_Init call during peripheral init
if ($content -notmatch 'MX_TIM1_Init\s*\(\s*\)\s*;') {
    $content = $content -replace '(MX_GPIO_Init\(\);\s*\r?\n)', "`$1  MX_TIM1_Init();`r`n"
}

# 7) keep key/limit inputs pull-up for active-low wiring
$content = $content -replace '(GPIO_InitStruct\.Pin = GPIO_PIN_0\|GPIO_PIN_1\|GPIO_PIN_4\|GPIO_PIN_5\s*\r?\n\s*\|GPIO_PIN_6;\s*\r?\n\s*GPIO_InitStruct\.Mode = GPIO_MODE_INPUT;\s*\r?\n\s*)GPIO_InitStruct\.Pull = GPIO_NOPULL;', '$1GPIO_InitStruct.Pull = GPIO_PULLUP;'

# 8) add TIM1 init body when missing (safe idempotent)
if ($content -notmatch 'static void MX_TIM1_Init\(void\)\s*\{') {
    $timBlock = @"
/**
  * @brief TIM1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM1_Init(void)
{
  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  __HAL_RCC_TIM1_CLK_ENABLE();

  htim1.Instance = TIM1;
  htim1.Init.Prescaler = 7;
  htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim1.Init.Period = 99;
  htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim1.Init.RepetitionCounter = 0;
  htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim1) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim1, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim1) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
}

"@
    $content = $content -replace '(\/\*\*\s*\r?\n\s*\* @brief USART1 Initialization Function)', "$timBlock`$1"
}

Set-Content -Path $mainPath -Value $content -Encoding UTF8

# 9) ensure TIM HAL module enabled
$halConfPath = Join-Path $ProjectRoot "Core\\Inc\\stm32f1xx_hal_conf.h"
if (Test-Path $halConfPath) {
    $halConf = Get-Content -Path $halConfPath -Raw -Encoding UTF8
    if ($halConf -match '/\*#define HAL_TIM_MODULE_ENABLED\s+\*/') {
        $halConf = $halConf -replace '/\*#define HAL_TIM_MODULE_ENABLED\s+\*/', '#define HAL_TIM_MODULE_ENABLED'
        Set-Content -Path $halConfPath -Value $halConf -Encoding UTF8
    }
}

Write-Host "Patched main.c (+TIM1 +app hooks) and hal_conf at: $ProjectRoot"

# 10) ensure CubeIDE source list contains app_entry.c and curtain_ctrl.c
$mxPath = Join-Path $ProjectRoot ".mxproject"
if (Test-Path $mxPath) {
    $mx = Get-Content -Path $mxPath -Raw -Encoding UTF8

    if ($mx -notmatch 'Core\\Src\\app_entry\.c') {
        $mx = $mx -replace 'SourceFiles=Core\\Src\\main\.c;Core\\Src\\stm32f1xx_it\.c;Core\\Src\\stm32f1xx_hal_msp\.c;',
            'SourceFiles=Core\\Src\\main.c;Core\\Src\\stm32f1xx_it.c;Core\\Src\\stm32f1xx_hal_msp.c;Core\\Src\\app_entry.c;Core\\Src\\curtain_ctrl.c;'
    }

    if ($mx -match 'SourceFileListSize=3') {
        $mx = $mx -replace 'SourceFileListSize=3', 'SourceFileListSize=5'
    }

    if ($mx -notmatch 'SourceFiles#3=\.\.\\Core\\Src\\app_entry\.c') {
        $mx = $mx -replace 'SourceFiles#2=\.\.\\Core\\Src\\main\.c',
            "SourceFiles#2=..\Core\Src\main.c`r`nSourceFiles#3=..\Core\Src\app_entry.c`r`nSourceFiles#4=..\Core\Src\curtain_ctrl.c"
    }

    Set-Content -Path $mxPath -Value $mx -Encoding UTF8
}

Write-Host "Patched .mxproject source list at: $ProjectRoot"

# 11) ensure HAL TIM driver files exist and are listed in .mxproject
$dstTimInc = Join-Path $ProjectRoot "Drivers\\STM32F1xx_HAL_Driver\\Inc"
$dstTimSrc = Join-Path $ProjectRoot "Drivers\\STM32F1xx_HAL_Driver\\Src"
$fwRootCandidates = @(
    "C:\\Users\\$env:USERNAME\\STM32Cube\\Repository\\STM32Cube_FW_F1_V1.8.6\\Drivers\\STM32F1xx_HAL_Driver",
    "C:\\Users\\$env:USERNAME\\STM32Cube\\Repository\\STM32Cube_FW_F1_V1.8.5\\Drivers\\STM32F1xx_HAL_Driver"
)

$fwHalRoot = $null
foreach ($c in $fwRootCandidates) {
    if (Test-Path (Join-Path $c "Inc\\stm32f1xx_hal_tim.h")) {
        $fwHalRoot = $c
        break
    }
}

if ($fwHalRoot) {
    Copy-Item -LiteralPath (Join-Path $fwHalRoot "Inc\\stm32f1xx_hal_tim.h") -Destination (Join-Path $dstTimInc "stm32f1xx_hal_tim.h") -Force
    Copy-Item -LiteralPath (Join-Path $fwHalRoot "Inc\\stm32f1xx_hal_tim_ex.h") -Destination (Join-Path $dstTimInc "stm32f1xx_hal_tim_ex.h") -Force
    Copy-Item -LiteralPath (Join-Path $fwHalRoot "Src\\stm32f1xx_hal_tim.c") -Destination (Join-Path $dstTimSrc "stm32f1xx_hal_tim.c") -Force
    Copy-Item -LiteralPath (Join-Path $fwHalRoot "Src\\stm32f1xx_hal_tim_ex.c") -Destination (Join-Path $dstTimSrc "stm32f1xx_hal_tim_ex.c") -Force
}

if (Test-Path $mxPath) {
    $mx = Get-Content -Path $mxPath -Raw -Encoding UTF8

    if ($mx -notmatch 'Drivers\\STM32F1xx_HAL_Driver\\Src\\stm32f1xx_hal_tim\.c') {
        $mx = $mx -replace 'Drivers\\STM32F1xx_HAL_Driver\\Src\\stm32f1xx_hal_exti\.c;',
            'Drivers\\STM32F1xx_HAL_Driver\\Src\\stm32f1xx_hal_exti.c;Drivers\\STM32F1xx_HAL_Driver\\Src\\stm32f1xx_hal_tim.c;Drivers\\STM32F1xx_HAL_Driver\\Src\\stm32f1xx_hal_tim_ex.c;'
    }

    if ($mx -notmatch 'Drivers\\STM32F1xx_HAL_Driver\\Inc\\stm32f1xx_hal_tim\.h') {
        $mx = $mx -replace 'Drivers\\STM32F1xx_HAL_Driver\\Inc\\stm32f1xx_hal_exti\.h;',
            'Drivers\\STM32F1xx_HAL_Driver\\Inc\\stm32f1xx_hal_exti.h;Drivers\\STM32F1xx_HAL_Driver\\Inc\\stm32f1xx_hal_tim.h;Drivers\\STM32F1xx_HAL_Driver\\Inc\\stm32f1xx_hal_tim_ex.h;'
    }

    Set-Content -Path $mxPath -Value $mx -Encoding UTF8
}

Write-Host "Ensured TIM HAL driver files and .mxproject entries at: $ProjectRoot"

# 12) ensure STM32CubeIDE linked resources include custom source files
$eclipseProjectPath = Join-Path $ProjectRoot "STM32CubeIDE\\.project"
if (Test-Path $eclipseProjectPath) {
    $proj = Get-Content -Path $eclipseProjectPath -Raw -Encoding UTF8

    $linksToAdd = @()
    if ($proj -notmatch '<name>Drivers/STM32F1xx_HAL_Driver/stm32f1xx_hal_tim\.c</name>') {
        $linksToAdd += @"
		<link>
			<name>Drivers/STM32F1xx_HAL_Driver/stm32f1xx_hal_tim.c</name>
			<type>1</type>
			<locationURI>PARENT-1-PROJECT_LOC/Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_tim.c</locationURI>
		</link>
"@
    }
    if ($proj -notmatch '<name>Drivers/STM32F1xx_HAL_Driver/stm32f1xx_hal_tim_ex\.c</name>') {
        $linksToAdd += @"
		<link>
			<name>Drivers/STM32F1xx_HAL_Driver/stm32f1xx_hal_tim_ex.c</name>
			<type>1</type>
			<locationURI>PARENT-1-PROJECT_LOC/Drivers/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_tim_ex.c</locationURI>
		</link>
"@
    }
    if ($proj -notmatch '<name>Application/User/Core/app_entry\.c</name>') {
        $linksToAdd += @"
		<link>
			<name>Application/User/Core/app_entry.c</name>
			<type>1</type>
			<locationURI>PARENT-1-PROJECT_LOC/Core/Src/app_entry.c</locationURI>
		</link>
"@
    }
    if ($proj -notmatch '<name>Application/User/Core/curtain_ctrl\.c</name>') {
        $linksToAdd += @"
		<link>
			<name>Application/User/Core/curtain_ctrl.c</name>
			<type>1</type>
			<locationURI>PARENT-1-PROJECT_LOC/Core/Src/curtain_ctrl.c</locationURI>
		</link>
"@
    }

    if ($linksToAdd.Count -gt 0) {
        $insert = ($linksToAdd -join "`r`n")
        $proj = $proj -replace '</linkedResources>', "$insert`r`n	</linkedResources>"
        Set-Content -Path $eclipseProjectPath -Value $proj -Encoding UTF8
    }
}

Write-Host "Ensured STM32CubeIDE linked resources at: $ProjectRoot"

# Next Steps (After Generate)

## Fastest Way (One Command)
```powershell
powershell -ExecutionPolicy Bypass -File D:\codex\projectB\firmware\one_click_cubeide_build.ps1
```
This command will auto-import project, headless build, and open CubeIDE workspace.

1. Open this file in STM32CubeIDE:
`D:\codex\projectB\firmware\cubeide_project\CurtainMVP_F103\CurtainMVP_F103.ioc`

2. Click **Generate Code**.

3. Run this command in PowerShell:
```powershell
powershell -ExecutionPolicy Bypass -File D:\codex\projectB\firmware\setup_projectb_stm32.ps1 -ProjectRoot D:\codex\projectB\firmware\cubeide_project\CurtainMVP_F103
```

4. In CubeIDE, click **Build** and then **Run/Debug** (flash to board).

5. Open UART monitor at `115200 8N1`.
Expected boot log: `boot ok`

6. Key test (active-low):
- `PA4` = open
- `PA5` = stop
- `PA6` = close

## Current Bring-up Defaults
- Limit switches are optional for now.
- Timeout performs safe stop (not latched fault).
- After wiring real limit switches, set `CURTAIN_USE_LIMIT_SWITCH` to `1`.

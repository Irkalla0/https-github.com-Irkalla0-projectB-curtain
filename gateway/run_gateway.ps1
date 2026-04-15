param(
    [string]$Config = "D:\codex\projectB\gateway\config.yaml"
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

python -m pip install -r D:\codex\projectB\gateway\requirements.txt
python D:\codex\projectB\gateway\main.py --config $Config

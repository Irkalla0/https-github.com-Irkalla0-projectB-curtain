#!/usr/bin/env bash
set -euo pipefail

CONFIG_PATH="${1:-./gateway/config.yaml}"

python3 -m pip install -r ./gateway/requirements.txt
python3 ./gateway/main.py --config "${CONFIG_PATH}"

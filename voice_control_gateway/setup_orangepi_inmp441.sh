#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
MODEL_URL="https://alphacephei.com/vosk/models/vosk-model-small-cn-0.22.zip"
MODEL_ZIP="${SCRIPT_DIR}/models/vosk-model-small-cn-0.22.zip"
MODEL_DIR="${SCRIPT_DIR}/models/vosk-model-small-cn-0.22"

echo "[1/4] install system deps"
sudo apt update
sudo apt install -y python3 python3-pip python3-dev portaudio19-dev libasound2-dev unzip wget

echo "[2/4] install python deps"
python3 -m pip install --upgrade pip
python3 -m pip install -r "${SCRIPT_DIR}/requirements.txt"

echo "[3/4] download vosk model (if missing)"
mkdir -p "${SCRIPT_DIR}/models"
if [[ ! -d "${MODEL_DIR}" ]]; then
  wget -O "${MODEL_ZIP}" "${MODEL_URL}"
  unzip -o "${MODEL_ZIP}" -d "${SCRIPT_DIR}/models"
else
  echo "model already exists: ${MODEL_DIR}"
fi

echo "[4/4] done"
echo "next steps:"
echo "  1) cp config.example.yaml config.yaml"
echo "  2) python3 voice_control.py --list-devices"
echo "  3) python3 test_command_parser.py"
echo "  4) python3 voice_control.py --config config.yaml"

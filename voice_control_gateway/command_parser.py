from __future__ import annotations

from dataclasses import dataclass
import re
from typing import Optional


@dataclass(frozen=True)
class VoiceCommand:
    kind: str  # "target" or "stop"
    target_pos: Optional[int] = None
    raw_text: str = ""


_PUNCTUATION_RE = re.compile(r"[，。！？、,.!?\"'“”‘’（）()【】\[\]\s]+")
_ARABIC_NUMBER_RE = re.compile(r"(\d{1,3})")
_CHINESE_NUMBER_PREFIX_RE = re.compile(r"^[零〇一二两三四五六七八九十百]+")

_STOP_KEYWORDS = ("停止", "停下", "暂停", "急停", "别动")
_OPEN_KEYWORDS = ("打开窗帘", "开窗帘", "打开", "拉开")
_CLOSE_KEYWORDS = ("关闭窗帘", "关窗帘", "关闭", "拉上", "合上")
_TARGET_TRIGGERS = ("设置到", "设到", "调到", "开到", "到")

_SPECIAL_TARGETS = {
    "半开": 50,
    "一半": 50,
    "中间": 50,
    "全开": 100,
    "全关": 0,
}

_CH_DIGITS = {
    "零": 0,
    "〇": 0,
    "一": 1,
    "二": 2,
    "两": 2,
    "三": 3,
    "四": 4,
    "五": 5,
    "六": 6,
    "七": 7,
    "八": 8,
    "九": 9,
}

_CH_UNITS = {
    "十": 10,
    "百": 100,
}


def _normalize(text: str) -> str:
    return _PUNCTUATION_RE.sub("", text).lower()


def _clamp_percent(value: int) -> int:
    return max(0, min(100, value))


def _chinese_number_to_int(text: str) -> Optional[int]:
    if not text:
        return None
    for ch in text:
        if ch not in _CH_DIGITS and ch not in _CH_UNITS:
            return None

    total = 0
    current_digit = 0
    for ch in text:
        if ch in _CH_DIGITS:
            current_digit = _CH_DIGITS[ch]
            continue
        unit = _CH_UNITS[ch]
        if current_digit == 0:
            current_digit = 1
        total += current_digit * unit
        current_digit = 0
    total += current_digit
    return total


def _parse_number_prefix(text: str) -> Optional[int]:
    if not text:
        return None

    digit_match = _ARABIC_NUMBER_RE.match(text)
    if digit_match:
        return int(digit_match.group(1))

    chinese_match = _CHINESE_NUMBER_PREFIX_RE.match(text)
    if chinese_match:
        return _chinese_number_to_int(chinese_match.group(0))
    return None


def _extract_target_pos(text: str) -> Optional[int]:
    for key, value in _SPECIAL_TARGETS.items():
        if key in text:
            return value

    if "百分之" in text:
        tail = text.split("百分之", 1)[1]
        value = _parse_number_prefix(tail)
        if value is not None:
            return _clamp_percent(value)

    for trigger in _TARGET_TRIGGERS:
        idx = text.find(trigger)
        if idx >= 0:
            tail = text[idx + len(trigger) :]
            value = _parse_number_prefix(tail)
            if value is not None:
                return _clamp_percent(value)

    if "窗帘" in text:
        suffix = text.split("窗帘", 1)[1]
        value = _parse_number_prefix(suffix)
        if value is not None:
            return _clamp_percent(value)

    if any(hint in text for hint in ("窗帘", "开度", "百分比")):
        digit_match = _ARABIC_NUMBER_RE.search(text)
        if digit_match:
            return _clamp_percent(int(digit_match.group(1)))
    return None


def parse_voice_command(raw_text: str) -> Optional[VoiceCommand]:
    text = _normalize(raw_text)
    if not text:
        return None

    # Stop must take highest priority to avoid accidental movement.
    if any(k in text for k in _STOP_KEYWORDS):
        return VoiceCommand(kind="stop", raw_text=raw_text)

    target = _extract_target_pos(text)
    if target is not None:
        return VoiceCommand(kind="target", target_pos=target, raw_text=raw_text)

    if any(k in text for k in _OPEN_KEYWORDS):
        return VoiceCommand(kind="target", target_pos=100, raw_text=raw_text)

    if any(k in text for k in _CLOSE_KEYWORDS):
        return VoiceCommand(kind="target", target_pos=0, raw_text=raw_text)

    return None

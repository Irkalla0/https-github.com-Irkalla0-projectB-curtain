from __future__ import annotations

from dataclasses import dataclass
from typing import Optional
import re


@dataclass(frozen=True)
class VoiceCommand:
    kind: str  # "target" or "stop"
    target_pos: Optional[int] = None
    raw_text: str = ""


_PUNCTUATION = re.compile(r"[，。！？、,.!?；;：:\"'（）()【】\[\]\s]+")
_PERCENT_RE = re.compile(r"(\d{1,3})\s*%?")


def _normalize(text: str) -> str:
    return _PUNCTUATION.sub("", text).lower()


def _simple_chinese_to_int(text: str) -> Optional[int]:
    # Supports 0-100 style phrases: "五十", "一百", "九十五", "十", "零"
    digit = {
        "零": 0,
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
    if text == "十":
        return 10
    if text == "一百":
        return 100
    if text in digit:
        return digit[text]

    if "百" in text:
        left, _, right = text.partition("百")
        if left not in ("一", ""):
            return None
        if right == "":
            return 100
        if right in digit:
            return 100 + digit[right]
        return None

    if "十" in text:
        left, _, right = text.partition("十")
        tens = 1 if left == "" else digit.get(left)
        if tens is None:
            return None
        ones = 0 if right == "" else digit.get(right)
        if ones is None:
            return None
        return tens * 10 + ones

    return None


def _extract_target_pos(text: str) -> Optional[int]:
    digit_match = _PERCENT_RE.search(text)
    if digit_match:
        value = int(digit_match.group(1))
        return max(0, min(100, value))

    # e.g. "窗帘到五十"
    for trigger in ("到", "开到", "调到"):
        idx = text.find(trigger)
        if idx >= 0:
            tail = text[idx + len(trigger) :]
            number = _simple_chinese_to_int(tail)
            if number is not None:
                return max(0, min(100, number))

    special = {
        "半开": 50,
        "一半": 50,
        "全开": 100,
        "全关": 0,
    }
    for key, value in special.items():
        if key in text:
            return value
    return None


def parse_voice_command(raw_text: str) -> Optional[VoiceCommand]:
    text = _normalize(raw_text)
    if not text:
        return None

    # Stop has highest priority.
    if any(k in text for k in ("停止", "停下", "暂停", "急停")):
        return VoiceCommand(kind="stop", raw_text=raw_text)

    target = _extract_target_pos(text)
    if target is not None:
        return VoiceCommand(kind="target", target_pos=target, raw_text=raw_text)

    if any(k in text for k in ("打开窗帘", "开窗帘", "打开")):
        return VoiceCommand(kind="target", target_pos=100, raw_text=raw_text)

    if any(k in text for k in ("关闭窗帘", "关窗帘", "关闭")):
        return VoiceCommand(kind="target", target_pos=0, raw_text=raw_text)

    return None


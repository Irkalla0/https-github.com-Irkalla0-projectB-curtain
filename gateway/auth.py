from __future__ import annotations

from dataclasses import dataclass
from typing import Any

import jwt


class AuthError(Exception):
    pass


@dataclass(frozen=True)
class AuthResult:
    subject: str
    scopes: list[str]


def verify_bearer_token(token: str, jwt_secret: str, jwt_algorithms: list[str], static_tokens: list[str]) -> AuthResult:
    token = token.strip()
    if not token:
        raise AuthError("missing token")

    if token in static_tokens:
        return AuthResult(subject="static-token", scopes=["curtain:control"])

    try:
        payload: dict[str, Any] = jwt.decode(token, jwt_secret, algorithms=jwt_algorithms)
    except jwt.PyJWTError as exc:
        raise AuthError(f"invalid jwt: {exc}") from exc

    subject = str(payload.get("sub", "unknown"))
    raw_scope = payload.get("scope", "")
    if isinstance(raw_scope, str):
        scopes = [item for item in raw_scope.split(" ") if item]
    elif isinstance(raw_scope, list):
        scopes = [str(item) for item in raw_scope]
    else:
        scopes = []
    return AuthResult(subject=subject, scopes=scopes)

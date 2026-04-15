from __future__ import annotations

import os

import jwt

from gateway.auth import verify_bearer_token


def test_static_token_auth() -> None:
    result = verify_bearer_token(
        token="projectb-local-token",
        jwt_secret="secret",
        jwt_algorithms=["HS256"],
        static_tokens=["projectb-local-token"],
    )
    assert result.subject == "static-token"


def test_jwt_auth() -> None:
    token = jwt.encode({"sub": "tester", "scope": "curtain:control"}, "secret", algorithm="HS256")
    result = verify_bearer_token(
        token=token,
        jwt_secret="secret",
        jwt_algorithms=["HS256"],
        static_tokens=[],
    )
    assert result.subject == "tester"
    assert "curtain:control" in result.scopes

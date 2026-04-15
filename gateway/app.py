from __future__ import annotations

from typing import Annotated

from fastapi import Depends, FastAPI, Header, HTTPException, status
from pydantic import BaseModel, Field

from gateway.auth import AuthError, AuthResult, verify_bearer_token
from gateway.config import AppConfig
from gateway.service import GatewayService, GatewayServiceError


class TargetRequest(BaseModel):
    target_pos: int = Field(ge=0, le=100)
    trace_id: str = ""


class StopRequest(BaseModel):
    reason: str = "api"
    trace_id: str = ""


class CurtainResponse(BaseModel):
    success: bool
    queued: bool
    seq: int
    state: str
    current_pos: int
    fault_code: int
    reason: str = ""


def _extract_bearer(authorization: str) -> str:
    if not authorization.lower().startswith("bearer "):
        raise AuthError("invalid authorization header")
    return authorization.split(" ", 1)[1].strip()


def build_auth_dependency(cfg: AppConfig):
    def _verify(authorization: Annotated[str, Header(alias="Authorization")]) -> AuthResult:
        try:
            token = _extract_bearer(authorization)
            return verify_bearer_token(
                token=token,
                jwt_secret=cfg.auth.jwt_secret,
                jwt_algorithms=cfg.auth.jwt_algorithms,
                static_tokens=cfg.auth.static_tokens,
            )
        except AuthError as exc:
            raise HTTPException(status_code=status.HTTP_401_UNAUTHORIZED, detail=str(exc)) from exc

    return _verify


def create_app(cfg: AppConfig) -> FastAPI:
    app = FastAPI(title="ProjectB Local Gateway", version="1.0.0")
    service = GatewayService(cfg)
    verify_dep = build_auth_dependency(cfg)

    @app.on_event("startup")
    def _startup() -> None:
        service.start()

    @app.on_event("shutdown")
    def _shutdown() -> None:
        service.stop()

    @app.get("/healthz")
    def healthz() -> dict[str, str]:
        return {"status": "ok"}

    @app.post("/api/v1/curtains/{curtain_id}/target", response_model=CurtainResponse)
    def set_target(curtain_id: str, body: TargetRequest, auth: Annotated[AuthResult, Depends(verify_dep)]) -> CurtainResponse:
        try:
            result = service.send_command(
                actor=auth.subject,
                curtain_id=curtain_id,
                cmd="target",
                target_pos=body.target_pos,
                trace_id=body.trace_id,
            )
        except GatewayServiceError as exc:
            raise HTTPException(status_code=500, detail=str(exc)) from exc
        return CurtainResponse(**result.__dict__)

    @app.post("/api/v1/curtains/{curtain_id}/stop", response_model=CurtainResponse)
    def stop(curtain_id: str, body: StopRequest, auth: Annotated[AuthResult, Depends(verify_dep)]) -> CurtainResponse:
        try:
            result = service.send_command(
                actor=auth.subject,
                curtain_id=curtain_id,
                cmd="stop",
                target_pos=0,
                trace_id=body.trace_id,
            )
        except GatewayServiceError as exc:
            raise HTTPException(status_code=500, detail=str(exc)) from exc
        return CurtainResponse(**result.__dict__)

    @app.get("/api/v1/curtains/{curtain_id}/status")
    def status_view(curtain_id: str, _auth: Annotated[AuthResult, Depends(verify_dep)]) -> dict:
        return service.get_status(curtain_id)

    @app.get("/api/v1/curtains/{curtain_id}/faults")
    def faults_view(curtain_id: str, _auth: Annotated[AuthResult, Depends(verify_dep)]) -> dict:
        return {"items": service.list_faults(curtain_id, limit=100)}

    @app.post("/api/v1/gateway/replay")
    def replay(_auth: Annotated[AuthResult, Depends(verify_dep)]) -> dict:
        replayed = service.replay_queue_once()
        return {"replayed": replayed}

    return app

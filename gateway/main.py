from __future__ import annotations

import argparse
import ssl

import uvicorn

from gateway.app import create_app
from gateway.config import load_config


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="ProjectB local gateway server")
    parser.add_argument("--config", default="gateway/config.yaml", help="config path")
    return parser.parse_args()


def main() -> None:
    args = parse_args()
    cfg = load_config(args.config)
    app = create_app(cfg)

    kwargs = {
        "app": app,
        "host": cfg.server.host,
        "port": cfg.server.port,
        "log_level": "info",
    }

    if cfg.server.tls.enabled:
        kwargs["ssl_certfile"] = cfg.server.tls.certfile
        kwargs["ssl_keyfile"] = cfg.server.tls.keyfile
        if cfg.server.tls.cafile:
            kwargs["ssl_ca_certs"] = cfg.server.tls.cafile
        kwargs["ssl_cert_reqs"] = ssl.CERT_REQUIRED if cfg.server.tls.require_client_cert else ssl.CERT_NONE

    uvicorn.run(**kwargs)


if __name__ == "__main__":
    main()

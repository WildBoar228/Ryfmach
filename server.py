#!/usr/bin/env python3
import os
import sys

BASE_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), "Ryfmach", "python")
if BASE_DIR not in sys.path:
    sys.path.insert(0, BASE_DIR)
os.chdir(BASE_DIR)


def _bind_address() -> str:
    port = os.environ.get("PORT", "20006")
    host = os.environ.get("INSTANCE_HOST", os.environ.get("HOST", "127.0.0.1"))
    args = sys.argv[1:]
    if args:
        target = args[0].strip()
        if target.startswith("--bind="):
            return target.split("=", 1)[1]
        if target.startswith("--bind"):
            return args[1] if len(args) > 1 else f"127.0.0.1:{port}"
        if ":" in target:
            return target
        return f"127.0.0.1:{target}"
    return f"{host}:{port}"


def main() -> None:
    bind = _bind_address()
    sys.argv = [
        "gunicorn",
        "main:app",
        f"--bind={bind}",
        "--workers=2",
        "--timeout=120",
        "--access-logfile=-",
        "--error-logfile=-",
    ]
    from gunicorn.app.wsgiapp import WSGIApplication

    WSGIApplication("%(prog)s [OPTIONS] [APP_MODULE]", prog="gunicorn").run()


if __name__ == "__main__":
    main()

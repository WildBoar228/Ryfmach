import os
from pathlib import Path

from dotenv import load_dotenv


PROJECT_ROOT = Path(__file__).resolve().parent
load_dotenv(PROJECT_ROOT / ".env")


def get_required_path(variable_name: str) -> Path:
    value = os.getenv(variable_name)
    if not value:
        raise RuntimeError(f"{variable_name} is not set")
    return Path(value).expanduser()


def get_app_port() -> int:
    value = os.getenv("RYFMACH_JINJA_PORT")
    if not value:
        raise RuntimeError("RYFMACH_JINJA_PORT is not set")

    try:
        port = int(value)
    except ValueError as error:
        raise RuntimeError("RYFMACH_JINJA_PORT must be an integer") from error

    if not 1 <= port <= 65535:
        raise RuntimeError("RYFMACH_JINJA_PORT must be between 1 and 65535")
    return port


RHYME_LIKES_DB_PATH = get_required_path("RHYME_LIKES_DB_PATH")
SLOUNIK_DB_PATH = get_required_path("SLOUNIK_DB_PATH")
RYFMACH_JINJA_PORT = get_app_port()

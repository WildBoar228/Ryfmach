import os
from pathlib import Path


def get_required_value(variable_name: str) -> str:
    value = os.getenv(variable_name)
    if not value:
        raise RuntimeError(f"{variable_name} is not set")
    return value


def get_required_path(variable_name: str) -> Path:
    return Path(get_required_value(variable_name)).expanduser()


def get_required_port(variable_name: str) -> int:
    value = get_required_value(variable_name)
    try:
        port = int(value)
    except ValueError as error:
        raise RuntimeError(f"{variable_name} must be an integer") from error

    if not 1 <= port <= 65535:
        raise RuntimeError(f"{variable_name} must be between 1 and 65535")
    return port


def get_required_bool(variable_name: str) -> bool:
    value = get_required_value(variable_name).strip().lower()
    if value in {"1", "true", "yes", "on"}:
        return True
    if value in {"0", "false", "no", "off"}:
        return False
    raise RuntimeError(
        f"{variable_name} must be one of: true, false, 1, 0, yes, no, on, off"
    )


RHYME_LIKES_DB_PATH = get_required_path("RHYME_LIKES_DB_PATH")
SLOUNIK_DB_PATH = get_required_path("SLOUNIK_DB_PATH")
FLASK_SECRET_KEY = get_required_value("FLASK_SECRET_KEY")
PUBLIC_BASE_URL = get_required_value("PUBLIC_BASE_URL").rstrip("/")
RYFMACH_IS_TEST_SITE = get_required_bool("RYFMACH_IS_TEST_SITE")
RYFMACH_API_HOST = get_required_value("RYFMACH_API_HOST")
RYFMACH_API_PORT = get_required_port("RYFMACH_API_PORT")
RYFMACH_APP_LOG_PATH = get_required_path("RYFMACH_APP_LOG_PATH")
RYFMACH_JINJA_PORT = get_required_port("RYFMACH_JINJA_PORT")

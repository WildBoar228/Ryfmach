from logging.handlers import RotatingFileHandler
import logging
from pathlib import Path


def new_debug_logger(logger_name: str, log_dir: str = "logs") -> logging.Logger:
    Path("logs").mkdir(exist_ok=True)

    logger = logging.getLogger(logger_name)
    logger.setLevel(logging.DEBUG)
    logger.propagate = False

    file_handler = RotatingFileHandler(
        Path(log_dir) / f"{logger_name}.log",
        maxBytes=1 * 1024 * 1024,  # 1 MB
        backupCount=10,
        encoding="utf-8",
    )

    file_handler.setLevel(logging.DEBUG)
    file_handler.setFormatter(
        logging.Formatter(
            "[%(asctime)s] %(levelname)s in %(module)s: %(message)s"
        )
    )

    logger.addHandler(file_handler)
    return logger

import sqlite3
import threading

from config import RHYME_LIKES_DB_PATH


likes_db_lock = threading.Lock()


def init_db():
    RHYME_LIKES_DB_PATH.parent.mkdir(parents=True, exist_ok=True)
    with sqlite3.connect(RHYME_LIKES_DB_PATH) as con:
        con.execute("""
            CREATE TABLE IF NOT EXISTS rhyme_likes (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                request_word TEXT NOT NULL,
                request_stress INTEGER NOT NULL,
                rhyme_word TEXT NOT NULL,
                rhyme_stress INTEGER NOT NULL,
                score INTEGER NOT NULL DEFAULT 0,
                updated_at TEXT NOT NULL DEFAULT CURRENT_TIMESTAMP,
                UNIQUE(request_word, request_stress, rhyme_word, rhyme_stress)
            )
        """)


def update_score(request_word: str, request_stress: int, rhyme_word: str, rhyme_stress: int, delta: int):
    init_db()
    with likes_db_lock:
        with sqlite3.connect(RHYME_LIKES_DB_PATH) as con:
            word_pair = [(request_word, request_stress), (rhyme_word, rhyme_stress)]
            word_pair.sort()
            (request_word, request_stress), (rhyme_word, rhyme_stress) = word_pair

            con.execute("""
                INSERT OR IGNORE INTO rhyme_likes (
                    request_word,
                    request_stress,
                    rhyme_word,
                    rhyme_stress,
                    score
                )
                VALUES (?, ?, ?, ?, 0)
            """, (request_word, request_stress, rhyme_word, rhyme_stress))

            con.execute("""
                UPDATE rhyme_likes
                SET
                    score = score + ?,
                    updated_at = CURRENT_TIMESTAMP
                WHERE
                    request_word = ?
                    AND request_stress = ?
                    AND rhyme_word = ?
                    AND rhyme_stress = ?
            """, (delta, request_word, request_stress, rhyme_word, rhyme_stress))

            cur = con.execute("""
                SELECT score FROM rhyme_likes
                WHERE
                    request_word = ?
                    AND request_stress = ?
                    AND rhyme_word = ?
                    AND rhyme_stress = ?
            """, (request_word, request_stress, rhyme_word, rhyme_stress))
            row = cur.fetchone()
            
            print(f"Update rhyme score: {word_pair}: {row[0] if row else None}")
            return row[0] if row else None

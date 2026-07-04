from __future__ import annotations

import argparse
import json
import random
import sys
import time
from pathlib import Path

import bel_lang_engine.ryfmach as ryfmach


def read_all_words() -> list:
    return ryfmach.cur.execute("SELECT * FROM words").fetchall()


def word_record_to_dict(rec: list) -> dict:
    return {
        "id": rec[ryfmach.DB_WordsColumns.ID],
        "word": rec[ryfmach.DB_WordsColumns.WORD],
        "initial_id": rec[ryfmach.DB_WordsColumns.INITIAL_ID],
        "part_of_speech": rec[ryfmach.DB_WordsColumns.PART_OF_SPEECH],
        "accent": rec[ryfmach.DB_WordsColumns.ACCENT]
    }


def gen_words(words: list, N: int, filename: str, rng: random.Random):
    dataset = rng.sample(words, N)
    with open(filename, "w", encoding="utf-8") as file:
        for word in dataset:
            file.write(json.dumps(ryfmach.get_word_dict(word)))
            file.write("\n")


def gen_rhyme_queries(words: list, queries_cnt: int, max_rhymes: int, 
                      filename: str, rng: random.Random):
    query_words = rng.sample(words, queries_cnt)

    with open(filename, "w", encoding="utf-8") as file:
        for word in query_words:
            word_dict = ryfmach.get_word_dict(word)
            rhymes = ryfmach.find_rhymes(word_dict["word"], word_dict["accent"],
                                         mistake=-1, cnt_limit=max_rhymes)
            response = {
                "query_word": word_dict,
                "rhymes": rhymes
            }
            file.write(json.dumps(response))
            file.write("\n")


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("--out", default="data", type=Path)
    ap.add_argument("--word-cnt", type=int, default=1000)
    ap.add_argument("--rhyme-queries", type=int, default=100)
    ap.add_argument("--max-rhymes", type=int, default=1000)
    ap.add_argument("--seed", type=int, default=42)
    args = ap.parse_args()

    rng = random.Random(args.seed)
    args.out.parent.mkdir(parents=True, exist_ok=True)

    started = time.time()
    print("Load words from Slounik...")
    words = read_all_words()
    print("Words are ready")

    words_path = args.out / "words.jsonl"
    rhymes_path = args.out / "rhymes.jsonl"

    gen_words(words, args.word_cnt, words_path, rng=rng)
    elapsed = time.time() - started
    size_mb = words_path.stat().st_size / (1024 * 1024)
    print(f"wrote {args.word_cnt} words -> {words_path} ({size_mb:.1f} MB, {elapsed:.1f}s)")

    started = time.time()
    gen_rhyme_queries(
        words,
        queries_cnt=args.rhyme_queries,
        max_rhymes=args.max_rhymes,
        filename=rhymes_path,
        rng=rng)

    elapsed = time.time() - started
    size_mb = rhymes_path.stat().st_size / (1024 * 1024)
    print(f"wrote {args.rhyme_queries} queries ({args.max_rhymes}) \
          -> {rhymes_path} ({size_mb:.1f} MB, {elapsed:.1f}s)")
    return 0


if __name__ == "__main__":
    sys.exit(main())

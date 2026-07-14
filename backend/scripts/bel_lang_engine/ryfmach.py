import sqlite3
import threading
from bel_lang_engine.language import *
import bel_lang_engine.sounds as sounds
import hashlib
from pprint import pprint
import bel_lang_engine.lang_logs as lang_logs
from config import SLOUNIK_DB_PATH

ryfmach_logger = lang_logs.new_debug_logger("ryfmach")


MAX_RHYMES_IN_RESP = 2000
MAX_RHYME_MISTAKE = 3
MIN_RHYMES_ADAPTIVE = 50
MAX_ACCEPTABLE_PENALTY = 13 * 10000

class DB_WordsColumns:
    ID = 0
    WORD = 1
    INITIAL_ID = 2
    PART_OF_SPEECH = 3
    ACCENT = 4
    SOUND_HASH_0 = 5
    SOUND_HASH_1 = 6
    SOUND_HASH_2 = 7
    SOUND_HASH_3 = 8


def get_word_dict(w):
    try:
        db_lock.acquire(True)
        word_data = {}
        word_data["id"] = w[DB_WordsColumns.ID]
        word_data["word"] = w[DB_WordsColumns.WORD]
        word_data["part_of_speech_id"] = w[DB_WordsColumns.PART_OF_SPEECH]
        word_data["accent"] = w[DB_WordsColumns.ACCENT]
        posp = cur.execute('''SELECT name FROM parts_of_speech
                            WHERE id == ?''',
                            (w[DB_WordsColumns.PART_OF_SPEECH], )).fetchone()
        posp = posp[0] if len(posp) else ""
        word_data["part_of_speech"] = posp
        
        if w[2] == w[0]:
            word_data["is_initial"] = True
            word_data["initial_id"] = w[DB_WordsColumns.ID]
        else:
            word_data["is_initial"] = False
            try:
                word_data["initial_id"] = w[DB_WordsColumns.INITIAL_ID]
                word_data["initial_word"], word_data["initial_accent"] = cur.execute(
                    '''SELECT word, accent_index FROM words WHERE id == ?''',
                    (w[DB_WordsColumns.INITIAL_ID],)
                ).fetchone()
            except TypeError:
                # ryfmach_logger.debug(f"not found initial of {word_data["word"]}, initial index is {w[2]}")
                word_data["is_initial"] = True

    except Exception as exc:
        ryfmach_logger.debug(exc)

    finally:
        db_lock.release()
    
    return word_data


def find_word_records_in_table(word: str, table: str, fix_similar_letters = True):
    try:
        db_lock.acquire(True)
        records = []
        
        word_template = word
        if fix_similar_letters:
            replace_letters = {"у": "ў", "ў": "у", "е": "ё", "ё": "е"}

            for letter in replace_letters:
                word_template = word_template.replace(letter, "_")
        
        if word_template.count('_') > 0 and word_template.count('_') <= 5:
            match = cur.execute(f'''SELECT * FROM {table} WHERE word LIKE ?;''',
                                    (word_template,)).fetchall()
            for _, rec in enumerate(match):
                is_same_word = True
                for j in range(len(word)):
                    if (word_template[j] == '_' and
                        rec[1][j] != word[j] and
                        rec[1][j] != replace_letters.get(word[j])):
                            is_same_word = False
                            break
                if is_same_word:
                    records.append(rec)
        else:
            records = cur.execute(f'''
                SELECT * FROM {table}
                WHERE word = ?
            ''', (word,)).fetchall()

    except Exception as exc:
        ryfmach_logger.debug("ERROR find_word_records_in_table: ", exc)
    finally:
        db_lock.release()
    
    return records


def get_word_data_from_db(input_word: str, fix_similar_letters: bool = False):
    words = find_word_records_in_table(input_word, "words", fix_similar_letters)
    words = sorted(words, key=record_sort_key(alphabet_sort_words_key))
    
    word_variants = []
    
    for i in range(len(words)):
        if i == 0 or not (words[i][1] == words[i - 1][1] and
                          words[i][2] == words[i - 1][2] and
                          words[i][4] == words[i - 1][4]):
            word_variants.append(get_word_dict(words[i]))

    last_part_word_variants = get_compound_word_data_from_db(input_word, fix_similar_letters)
    return last_part_word_variants + word_variants


def get_word_by_id(word_id: int):
    word_rec = None
    try:
        db_lock.acquire(True)

        word_rec = cur.execute("""
            SELECT * FROM words
            WHERE id = ?
        """, (word_id,)).fetchone()

    except Exception as exc:
        ryfmach_logger.debug("ERROR get_word_by_id: ", exc)
    finally:
        db_lock.release()
        
    return word_rec


def get_word_forms(initial_id: int):
    word_forms = None
    try:
        db_lock.acquire(True)

        word_forms = cur.execute("""
            SELECT * FROM words
            WHERE initial_id = ?
        """, (initial_id,)).fetchall()

    except Exception as exc:
        ryfmach_logger.debug("ERROR get_word_forms: ", exc)
    finally:
        db_lock.release()

    if word_forms is None:
        return []

    return list(map(lambda r: get_word_dict(r), word_forms))


def get_compound_word_data_from_db(input_word: str, fix_similar_letters: bool = False):
    if input_word.count('-') > 0:
        last_part_start = input_word.rfind('-')
        prefix = input_word[:last_part_start]
        last_part = input_word[last_part_start + 1:]
        last_part_words = get_word_data_from_db(last_part, fix_similar_letters)
        accent_shift = len(input_word) - len(last_part)

        def append_prefix(w):
            w["word"] = prefix + '-' + w["word"]
            w["accent"] = w["accent"] + accent_shift
            # if w.get("initial_word") is not None:
            #     w["initial_word"] = prefix + '-' + w["initial_word"]
            #     w["initial_accent"] = w["initial_accent"] + accent_shift
            return w

        last_part_words = list(map(append_prefix, last_part_words))
        return last_part_words
    else:
        return []


def get_working_part(word: str, accent: int, mistake: int = 0):
    if mistake < 0:
        mistake = 0
    t = get_transcription_sounds(word, accent)

    acc_sound = 0

    # ідэяльная рыфма
    i = 0
    while i < len(t):
        if i > 0 and not is_vowel_sound(t[i]) and (t[i] == t[i - 1] or t[i] in ["дз", "дз'", "дж"] and t[i - 1] in ["д"]):
            t.pop(i - 1)
            continue

        if t[i] == "_ы_":
            t[i] = "_і_"
        
        if "_" in t[i]:
            acc_sound = i
        i += 1

    if acc_sound == len(t) - 1 and len(t) > 1 and mistake < 2:
        t = t[acc_sound - 1:]
    else:
        t = t[acc_sound:]
    
    # добрая рыфма
    if mistake >= 1:
        i = 0
        while i < len(t):            
            if is_vowel_sound(t[i]):
                if non_accent_pair(t[i]) is None:
                    t[i] = "ы"

            if is_consonant_sound(t[i]):
                if i > 0 and i < len(t) - 1:
                    if is_ring(t[i]) and thud_pair(t[i]):
                        t[i] = thud_pair(t[i])
                
                if (i == len(t) - 1 or not is_vowel_sound(t[i + 1])):
                    if is_soft(t[i]) and hard_pair(t[i]):
                        t[i] = hard_pair(t[i])
            
            i += 1
    
    # сярэдняя рыфма
    if mistake >= 2:
        i = 0
        while i < len(t):
            if t[i] in ["й", "ў"]:
                t.pop(i)
                continue

            if i < len(t) - 1:
                if is_sonor(t[i]):
                    t[i] = "л"
                elif is_hiss(t[i]):
                    t[i] = "ш"
                elif is_whistl(t[i]):
                    t[i] = "с"
            
            i += 1
    
        i = 0
        while i < len(t):
            if i > 0 and (t[i] == t[i - 1] or t[i] in ["дз", "дз'", "дж"] and t[i - 1] in ["д"]):
                t.pop(i - 1)
                continue
            i += 1
    
    # слабая рыфма
    if mistake >= 3:
        i = 0
        while i < len(t):
            if is_consonant_sound(t[i]):
                if i == len(t) - 1 or is_vowel_sound(t[i + 1]):
                    if is_sonor(t[i]):
                        t[i] = "л"
                    elif is_hiss(t[i]):
                        t[i] = "ш"
                    elif is_whistl(t[i]):
                        if i == len(t) - 1:
                            t[i] = "с"
                        else:
                            t[i] = "ш"
                    elif is_ring(t[i]):
                        t[i] = "в"
                    elif is_thud(t[i]):
                        t[i] = "ф"
                else:
                    t.pop(i)
                    i -= 1

            i += 1
    
        i = 0
        while i < len(t):
            if i > 0 and t[i] == t[i - 1]:
                t.pop(i - 1)
                continue
            i += 1
    
    return "".join(t)


def get_sound_hash(word: str, accent: int, mistake: int = 0):
    sound_str = get_working_part(word, accent, mistake)
    return int(hashlib.sha1(sound_str.encode('utf-8')).hexdigest(), 16) % 12345678901234277


def alphabet_sort_words_key(w, accent=None):
    global alphabet
    try:
        return [get_alpha_index(c) for c in w]
    except Exception as exc:
        ryfmach_logger.debug(w, exc)


def quality_sort_words_key(compare_with):
    def quality_with_comparation(w, accent):
        global alphabet
        try:
            return (
                sounds.get_rhyme_penalty(
                    w, accent,
                    compare_with["word"], compare_with["accent"]
                ),
                alphabet_sort_words_key(w, accent),
            )
        except Exception as exc:
            ryfmach_logger.debug(compare_with, w, exc)
    return quality_with_comparation


def record_sort_key(word_key):
    def record_key(w_record):
        return (
            word_key(w_record[1], w_record[4]),
            w_record[0] != w_record[2],
        )
    return record_key


def json_sort_key(word_key):
    def json_key(w_record):
        return (
            word_key(w_record["word"], w_record["accent"]),
            w_record["word"] != w_record.get("initial_word"),
        )
    return json_key


def find_rhymes(input_word: str,
                input_accent: int,
                filtered_posp: list[bool] = [True, True, True, True, True, True, True,],
                only_initial: bool = False,
                mistake: int = 0,
                debug_output=True,
                cnt_limit=300,
                words_sort_key=alphabet_sort_words_key):
    
    working_parts = [get_working_part(input_word, input_accent, i)
                     for i in range(0, max(0, mistake) + 1)]
    # ryfmach_logger.debug(working_parts)
    
    sound_hashes = [get_sound_hash(input_word, input_accent, i)
                    for i in range(0, max(0, mistake) + 1)]

    try:
        db_lock.acquire(True)

        enabled_posp_cnt = sum(filtered_posp)
        posp_filter = " AND ("
        if (enabled_posp_cnt > len(filtered_posp) / 2):
            for i in range(len(filtered_posp)):
                if not filtered_posp[i]:
                    if (posp_filter[-1] != '('):
                        posp_filter += " AND "
                    posp_filter += f"part_of_speech != {i + 1}"
        else:
            for i in range(len(filtered_posp)):
                if filtered_posp[i]:
                    if (posp_filter[-1] != '('):
                        posp_filter += " OR "
                    posp_filter += f"part_of_speech == {i + 1}"
        posp_filter += ")"
        if not any(filtered_posp) or all(filtered_posp):
            posp_filter = ""

        is_initial_filter = " AND id == initial_id" if only_initial else ""

        sound_hash_filter = ""
        for i in range(mistake):
            sound_hash_filter += f"sound_hash{i} != ? AND "
        sound_hash_filter += f"sound_hash{max(0, mistake)} == ?"
        
        words = cur.execute(f'''SELECT * FROM words
                                WHERE {sound_hash_filter} AND word != ?
                                {posp_filter}
                                {is_initial_filter}
                                ORDER BY word, initial_id, accent_index
                                LIMIT ?;''',
                                (*sound_hashes, input_word, MAX_RHYMES_IN_RESP)
        ).fetchall()

    except Exception as exc:
        ryfmach_logger.debug("ERROR query: ", exc)
    finally:
        db_lock.release()

    words = list(filter(lambda r:
                        get_working_part(r[1], r[4], mistake) ==
                            working_parts[-1],
                        words))

    words = sorted(words, key=record_sort_key(words_sort_key))
    # ryfmach_logger.debug(f'{len(words)} words (at first), last: {words[-1] if len(words) > 0 else '-'}')

    # with open("output.txt", "w", encoding="utf-8") as file:
    #     file.write(str(words))

    rhymes = []
    for i in range(len(words)):
        word_dict = get_word_dict(words[i])
        if word_dict.get("initial_word") != input_word:
            rhymes.append(word_dict)
    
    if len(rhymes) > cnt_limit:
        rhymes = rhymes[:cnt_limit]
    
    used_initial = set()
    k = 0
    for i in range(len(rhymes)):
        if rhymes[i].get("initial_word"):
            used_key = (rhymes[i]["initial_word"], rhymes[i]["initial_accent"])
        else:
            used_key = (rhymes[i]["word"], rhymes[i]["accent"])
            
        if used_key not in used_initial:
            used_initial.add(used_key)
            rhymes[k] = rhymes[i]
            k += 1
    rhymes = rhymes[:k]
    
    output_str = f'{mistake}  {input_word} ({input_accent}):  '
    if mistake == -1:
        if len(rhymes) < MIN_RHYMES_ADAPTIVE:
            output_str += f'{len(rhymes)}  '

            for mst in range(1, MAX_RHYME_MISTAKE + 1):
                if len(rhymes) < MIN_RHYMES_ADAPTIVE:
                    old_size = len(rhymes)
                    rhymes1 = find_rhymes(input_word, input_accent, filtered_posp,
                                          only_initial, mst, cnt_limit=cnt_limit // 2,
                                          words_sort_key=words_sort_key,
                                          debug_output=debug_output)
                    
                    rhymes += rhymes1
    
                    k = old_size
                    for i in range(k, len(rhymes)):
                        if rhymes[i].get("initial_word") is None: #rhymes[i]["is_initial"]:
                            used_key = (rhymes[i]["word"], rhymes[i]["accent"])
                        else:
                            used_key = (rhymes[i]["initial_word"], rhymes[i]["initial_accent"])
                            
                        if used_key not in used_initial:
                            used_initial.add(used_key)
                            rhymes[k] = rhymes[i]
                            k += 1
                    rhymes = rhymes[:k]

                    output_str += f'+{len(rhymes1)}  '

            output_str += '=  '
            
            rhymes = sorted(rhymes, key=json_sort_key(words_sort_key))

        # check_rhyme = MIN_RHYMES_ADAPTIVE
        # step = (len(rhymes) - check_rhyme + 1) // 2
        # while step > 0 and check_rhyme < len(rhymes):
        #     if sounds.get_rhyme_penalty(input_word, input_accent,
        #                                 rhymes[check_rhyme]["word"],
        #                                 rhymes[check_rhyme]["accent"]) <= MAX_ACCEPTABLE_PENALTY:
        #         check_rhyme += step
        #     step //= 2

        # ryfmach_logger.debug(f"cut righter than {check_rhyme} (initially {len(rhymes)})")
        # rhymes = rhymes[:check_rhyme]
    
    if len(rhymes) > cnt_limit:
        rhymes = rhymes[:cnt_limit]

    output_str += f'{len(rhymes)} rhymes found'
    if debug_output:
        ryfmach_logger.debug(output_str)
    
    return rhymes


def print_best_rhymes(input_word, rhyme_list, k=5):
    output = "Best rhymes to " + sounds.language.add_accent(
        input_word["word"],
        input_word["accent"]
    ) + ":   "

    best_rhymes = rhyme_list[:k] if len(rhyme_list) > k else rhyme_list
    for i, rhyme in enumerate(best_rhymes):
        penalty = sounds.get_rhyme_penalty(
            input_word["word"], input_word["accent"],
            rhyme["word"], rhyme["accent"]
        )
        penalty = round(penalty)
        output += f"{sounds.language.add_accent(rhyme["word"], rhyme["accent"])} ({penalty}),  "
    
    ryfmach_logger.debug(output)


def rhymes_text_list(input_word_request):
    input_word_request["word"] = clean_input_word(input_word_request["word"])

    if not is_belarusian(input_word_request["word"]) or len(input_word_request["word"]) > 40:
        return []
    if input_word_request.get("accent") is None:
        input_word_data = get_word_data_from_db(input_word_request["word"].lower(),
                                                fix_similar_letters=True)
    else:
        input_word_data = [input_word_request]
    
    filtered_posp = input_word_request.get("filtered_posp", [True] * 7)
    only_initial = input_word_request.get("only_initial", False)
    mistake = input_word_request.get("search_mistake", 0)

    rhymes = []
    for i in range(len(input_word_data)):
        if input_word_request["sort_mode"] == "alphabet":
            sort_func = alphabet_sort_words_key
        elif input_word_request["sort_mode"] == "quality":
            sort_func = quality_sort_words_key(input_word_data[i])

        rhymes.append({"word_variant": input_word_data[i],
                       "rhymes_data": find_rhymes(input_word_data[i]["word"],
                                                  input_word_data[i]["accent"],
                                                  filtered_posp=filtered_posp,
                                                  only_initial=only_initial,
                                                  mistake=mistake,
                                                  words_sort_key=sort_func)})
    
    for i in range(len(rhymes)):
        print_best_rhymes(rhymes[i]["word_variant"], rhymes[i]["rhymes_data"])
    
    return rhymes


con = sqlite3.connect(SLOUNIK_DB_PATH, check_same_thread=False)
cur = con.cursor()

db_lock = threading.Lock()


if __name__ == "__main__":
    pprint(get_word_data_from_db("пакецікі"))

    tests = []

    # import random
    # for i in range(30):
    #     resp = cur.execute(
    #                 '''SELECT word, accent_index FROM words WHERE id == ?''',
    #                 (random.randint(1, 1000000),)
    #             ).fetchone()
    #     if resp:
    #         tests.append(resp)

    tests = [("хата", 1),
         ("неба", 1),
         ("дзень", 2),
         ("ксёндз", 2),
         ("шчаўе", 2),
         ("лодка", 1),
         ("кніжка", 2),
         ("касьба", 5),
         ("лічба", 1),
         ("малацьба", 7),
         ("цемя", 1),
         ("здзек", 3),
         ("збіраць", 4),
         ("дзверы", 3),
         ("чацвёрты", 4),
         ("скінуць", 2),
         ("дошцы", 1),
         ("смяешся", 3),
         ("зжаць", 2),
         ("сшытак", 2),
         ("расчасаць", 6),
         ("нарэшце", 3),
         ("нарэжце", 3),
         ("матчын", 1),
         ("кладцы", 2),
         ("разводдзе", 4),
         ("ерась", 0),
         ("верас", 1),
         ("роспач", 1),
         ("пробашч", 2),
         ("бусел", 1),
         ("вузел", 1),]

    # for i, test in enumerate(tests):
    #     print(f'{i}  {test[0]}:    {' '.join(get_transcription_sounds(*test))}')
    #     for j in range(4):
    #         print(f'{get_working_part(*test, mistake=j)} {get_sound_hash(*test, mistake=j)}')
    #     print()

    con.close()

import bel_lang_engine.language as language
# import numpy as np
from pprint import pprint

cost_to_replace = {
    ("", ""): 0
}


def run_first_iter_replace_cost():
    # # delete vowel
    # for s in language.vowel_sound_list:
    #     cost_to_replace[("_" + s + "_", "")] = 30
    #     cost_to_replace[(s, "")] = 10
        
    # replace vowel with vowel
    for s1 in language.vowel_sound_list:
        for s2 in language.vowel_sound_list:
            if s1 == s2:
                cost_to_replace[(s1, s1)] = 0
                cost_to_replace[("_" + s1 + "_", s1)] = 20
                cost_to_replace[("_" + s1 + "_", "_" + s1 + "_")] = 0
                break
            elif s1 + ";" + s2 in ["і;ы", "ы;і"]:
                cost_to_replace[(s1, s2)] = 0
                cost_to_replace[("_" + s1 + "_", s2)] = 0
                cost_to_replace[(s1, "_" + s2 + "_")] = 0
                cost_to_replace[("_" + s1 + "_", "_" + s2 + "_")] = 0
            else:
                cost_to_replace[(s1, s2)] = 1
                cost_to_replace[("_" + s1 + "_", s2)] = 30
                cost_to_replace[(s1, "_" + s2 + "_")] = 30
                cost_to_replace[("_" + s1 + "_", "_" + s2 + "_")] = 10
            
    # # delete consonant
    # for s in language.cons_sound_list:
    #     cost_to_replace[(s, "")] = 2

    # replace consonant with consonant
    for s1 in language.cons_sound_list:
        for s2 in language.cons_sound_list:
            if s1 == s2:
                cost_to_replace[(s1, s1)] = 0
                break
            else:
                # ringing/thud, soft/hard
                diff = (
                    abs(language.cons_data[s1][1] - language.cons_data[s2][1]) + 
                    abs(language.cons_data[s1][2] - language.cons_data[s2][2]) * 2
                ) * 7

                # if have any common group
                if language.cons_data[s1][0] == language.cons_data[s2][0]:
                    diff /= 2
                else:
                    diff += 6

                if language.is_whistl(s1) and language.is_whistl(s2):
                    diff /= 3
                if language.is_hiss(s1) and language.is_hiss(s2):
                    diff /= 3
                
                cost_to_replace[(s1, s2)] = diff

    # replace vowel with consonant
    for v in language.vowel_sound_list:
        for c in language.cons_sound_list:
            if language.is_ring(c):
                cost_to_replace[(v, c)] = 15
                cost_to_replace[("_" + v + "_", c)] = 30
            else:
                cost_to_replace[(v, c)] = 20
                cost_to_replace[("_" + v + "_", c)] = 40

    if __name__ == "__main__":
        pprint(cost_to_replace)
        print(len(cost_to_replace))


def get_replace_cost(s1, s2):
    if cost_to_replace.get((s1, s2)) is not None:
        return cost_to_replace[(s1, s2)]
    if cost_to_replace.get((s2, s1)) is not None:
        return cost_to_replace[(s2, s1)]

    print("NO PAIR: ", (s1, s2))
    return 1000


def calc_transcript_comp_dp(s, t, dp, anc, max_shift=2):
    n = len(s)
    m = len(t)
    dp[0][0] = 0
    
    for i in range(1, n + 1):
        for j in range(max(1, i - max_shift), min(m + 1, i + max_shift + 1)):
            if i >= 1 and i >= j - max_shift and dp[i - 1][j] < dp[i][j]:
                dp[i][j] = dp[i - 1][j]
                anc[i][j] = 1
                
            if j >= 1 and j >= i - max_shift and dp[i][j - 1] < dp[i][j]:
                dp[i][j] = dp[i][j - 1]
                anc[i][j] = 2

            if i >= 1 and j >= 1 and dp[i - 1][j - 1] < dp[i][j]:
                dp[i][j] = dp[i - 1][j - 1]
                anc[i][j] = 3
            
            replace_cost = get_replace_cost(s[i - 1] if i >= 1 else "",
                                            t[j - 1] if j >= 1 else "")

            dp[i][j] += replace_cost
    
    
    # return mapping, dp_mat[n][m]


def get_rhyme_sounds_mapping(word1, accent1, word2, accent2, max_shift=5, use_prev_sounds=0):
    tr1 = language.get_transcription_sounds(word1, accent1)
    tr2 = language.get_transcription_sounds(word2, accent2)

    cut_index1 = language.get_accent_in_transcription(tr1)
    cut_index2 = language.get_accent_in_transcription(tr2)
    cut_index1 = max(0, cut_index1 - use_prev_sounds)
    cut_index2 = max(0, cut_index2 - use_prev_sounds)
    
    cut_tr1 = tr1[cut_index1:]
    cut_tr2 = tr2[cut_index2:]
    n, m = len(cut_tr1), len(cut_tr2)

    dp = [[1000 for _ in range(m + 1)] for __ in range(n + 1)]
    anc = [[0 for _ in range(m + 1)] for __ in range(n + 1)]

    calc_transcript_comp_dp(cut_tr1, cut_tr2, dp, anc, max_shift=max_shift)
    
    if __name__ == "__main__":
        pprint(dp)
        print(f"final error: ", dp[n][m])
    
    pairs = []
    i = n
    j = m
    while i >= 0 and j >= 0 and anc[i][j] != 0:
        pairs.append((i - 1, j - 1))
        if anc[i][j] == 1:
            i -= 1
        elif anc[i][j] == 2:
            j -= 1
        elif anc[i][j] == 3:
            i -= 1
            j -= 1
    if i == 1 and j == 1:
        pairs.append((i - 1, j - 1))

    if __name__ == "__main__":
        print(f"pairs: {pairs}")
    return pairs, dp[n][m]


def get_rhyme_penalty(word1, accent1, word2, accent2, max_shift=5):
    tr1 = language.get_transcription_sounds(word1, accent1)
    tr2 = language.get_transcription_sounds(word2, accent2)
    cut_index1 = language.get_accent_in_transcription(tr1)
    cut_index2 = language.get_accent_in_transcription(tr2)
    
    cut_tr1 = tr1[cut_index1:]
    cut_tr2 = tr2[cut_index2:]
    
    n, m = len(cut_tr1), len(cut_tr2)
    dp = [[1000 for _ in range(m + 1)] for __ in range(n + 1)]
    anc = [[0 for _ in range(m + 1)] for __ in range(n + 1)]
    calc_transcript_comp_dp(cut_tr1, cut_tr2, dp, anc, max_shift)
    suffix_cost = dp[n][m]
    
    if __name__ == "__main__":
        pass
        pprint(dp)
        print(f"suffix dp: {suffix_cost}")
    
    cut_tr1 = tr1[cut_index1 - 1::-1]
    cut_tr2 = tr2[cut_index2 - 1::-1]
    n, m = len(cut_tr1), len(cut_tr2)
    dp = [[1000 for _ in range(m + 1)] for __ in range(n + 1)]
    anc = [[0 for _ in range(m + 1)] for __ in range(n + 1)]
    calc_transcript_comp_dp(cut_tr1, cut_tr2, dp, anc, max_shift)

    prefix_cost = 0
    for i in range(1, min(n, m) + 1):
        minim = dp[i][i]
        for diff in range(-max_shift, max_shift + 1):
            j = i + diff
            if j >= 0 and j <= m and dp[i][j] < minim:
                minim = dp[i][j]
        prefix_cost += minim - i
    
    if __name__ == "__main__":
        pass
        pprint(dp)
        print(f"prefix dp: {prefix_cost}")
    
        print(f"finally {word1}: {suffix_cost}, {prefix_cost} =  {suffix_cost * 10000 + (1000 + prefix_cost)}")

    return suffix_cost * 10000 + (1000 + prefix_cost) # instead of tuple (suf, pref)


run_first_iter_replace_cost()


if __name__ == "__main__":
    # input_word = input()
    # input_accent = int(input())

    # rhymes = (
    #     ryfmach.find_rhymes(input_word, input_accent, mistake=0, cnt_limit=10, filtered_posp=[1,0,0,0,0,0,0]) +
    #     ryfmach.find_rhymes(input_word, input_accent, mistake=1, cnt_limit=10, filtered_posp=[1,0,0,0,0,0,0]) +
    #     ryfmach.find_rhymes(input_word, input_accent, mistake=2, cnt_limit=10, filtered_posp=[1,0,0,0,0,0,0])
    # )
    # print(',  '.join([w["word"] for w in rhymes]))
    # rhymes = sorted(
    #     rhymes,
    #     key=lambda w: get_rhyme_quality(
    #         input_word,
    #         input_accent,
    #         w["word"],
    #         w["accent"],
    #     )
    # )
    # print(',  '.join([w["word"] for w in rhymes]))


    print()
    get_rhyme_penalty(
        "спадчынніца", 2,
        "падчарыца", 1,
    )

    print()
    get_rhyme_penalty(
        "суладжанасці", 3,
        "падчарыца", 1,
    )


    print()
    get_rhyme_penalty(
        "абвыклы", 3,
        "рыфма", 1,
    )

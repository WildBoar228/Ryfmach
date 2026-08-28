from .Algo1 import algo1
from .Algo2 import algo2

def cutHalf(half, connection):
    if half[-1] == "'":
        return [{'type': 2, 'text': half}]
    cursor = connection.cursor()
    cursor.execute("SELECT analysis FROM morph_prefixes WHERE spelling == ?", (half, ))
    razdel = cursor.fetchall()[0][0]
    listik = razdel.split('-')
    ans = []
    for i in range(len(listik)):
        tempDic = dict()
        if i == 0:
            tempDic['type'] = 2
        elif i == len(listik) - 1:
            tempDic['type'] = 6
        else:
            tempDic['type'] = 3
        tempDic['text'] = listik[i]
        ans.append(tempDic) 
    return ans
            

def razbor(word, connection):
    zluchok = 'no'
    cursor = connection.cursor()
    if word[0:3] == 'па-':
        word = word[0:2] + word[3:]
        zluchok = 'ok'
    cursor.execute("SELECT word, analysis FROM morphemics WHERE word == ?", (word, ))
    listikRaz = cursor.fetchall()
    if len(listikRaz) > 0:
        ansNow = algo1(listikRaz[0][0], listikRaz[0][1], connection)
        if zluchok != 'no':
            ansNow['analysis'][0]['text'] = ansNow['analysis'][0]['text'] + '-'
        return algo1(listikRaz[0][0], listikRaz[0][1], connection)
    cursor.execute("SELECT initial_id, word FROM words WHERE word == ?", (word, ))
    listikIDs = cursor.fetchall()
    if len(listikIDs) == 0:
        return dict()
    cursor.execute("SELECT word, part_of_speech FROM words WHERE id == ?", (listikIDs[0][0], ))
    abobaList = cursor.fetchall()
    pachForm = abobaList[0][0]
    part_of_speech = abobaList[0][1]
    cursor.execute("SELECT word, analysis FROM morphemics WHERE word == ?", (pachForm, ))
    listikRaz = cursor.fetchall()
    if len(listikRaz) > 0:
        return algo2(
            word,
            algo1(listikRaz[0][0], listikRaz[0][1], connection),
            listikIDs[0][0],
            connection,
        )
    
    wordNoPost = pachForm
    if part_of_speech == 2:
        postEx = pachForm[len(pachForm) - 2:len(pachForm)]
        if postEx in ['ца', 'ся']:
            if postEx == 'ца':
                wordNoPost = pachForm[0:len(pachForm) - 2] + 'ь'
            else:
                wordNoPost = pachForm[0:len(pachForm) - 2]
            cursor.execute("SELECT word, analysis FROM morphemics WHERE word == ?", (wordNoPost, ))
            listikRaz = cursor.fetchall()
            if len(listikRaz) > 0:
                cursor.execute("SELECT initial_id, word FROM words WHERE word == ?", (wordNoPost, ))
                listikIDs = cursor.fetchall()
                dicNoPost = algo1(listikRaz[0][0], listikRaz[0][1], connection)
                if postEx == 'ца':
                    dicNoPost['analysis'][-1]['text'] = dicNoPost['analysis'][-1]['text'][0:len(dicNoPost['analysis'][-1]['text']) - 1]
                    dicNoPost['analysis'].append({'type': 5, 'text': 'ца'})
                else:
                    dicNoPost['analysis'].append({'type': 5, 'text': 'ся'})
                return algo2(word, dicNoPost, listikIDs[0][0], connection)
            
    cursor.execute("SELECT spelling FROM morph_prefixes WHERE id <= ?", (96, ))
    firstHalfTemp = cursor.fetchall()
    firstHalf = []
    for hsf in firstHalfTemp:
        firstHalf += list(hsf)
    firstHalfPlusApostr = []
    for hsf in firstHalf:
        firstHalfPlusApostr.append(hsf + "'")
    for half in firstHalf + firstHalfPlusApostr:
        if half == pachForm[0:len(half)]:
            dictSecHalf = razbor(pachForm[len(half):], connection)
            if len(dictSecHalf) != 0:
                dictSecHalf['sure'] = False
                dictSecHalf['analysis'] = cutHalf(half, connection) + dictSecHalf['analysis']
                if pachForm == word:
                    return dictSecHalf
                else:
                    return algo2(word, dictSecHalf, listikIDs[0][0], connection)
    
    cursor.execute("SELECT initial_id, word FROM words WHERE word == ?", (word, ))
    listikIDs = cursor.fetchall()
    cursor.execute("SELECT word, part_of_speech FROM words WHERE id == ?", (listikIDs[0][0], ))
    abobaList = cursor.fetchall()
    pachForm = abobaList[0][0]
    part_of_speech = abobaList[0][1]

    cursor.execute("SELECT spelling FROM morph_prefixes WHERE id > ?", (96, ))
    pristavkiTemp = cursor.fetchall()
    pristavki = ['а', 'аба', 'па', 'на', 'вы', 'пера', 'за', 'с', 'раза', 'у']
    for prist in pristavkiTemp:
        pristavki += list(prist)
    pristavkiPlusApostr = []
    for prist in pristavki:
        pristavkiPlusApostr.append(prist + "'")
    for prist in pristavki + pristavkiPlusApostr:
        if prist == pachForm[0:len(prist)]:
            dictNoPr = razbor(pachForm[len(prist):], connection)
            if len(dictNoPr) != 0:
                dictNoPr['sure'] = False
                dictNoPr['analysis'] = [{'type': 1, 'text': prist}] + dictNoPr['analysis']
                if pachForm == word:
                    return dictNoPr
                else:
                    return algo2(word, dictNoPr, listikIDs[0][0], connection)
            
    #крайні выпадак
    cursor.execute("SELECT initial_id, word FROM words WHERE word == ?", (word, ))
    listikIDs = cursor.fetchall()
    cursor.execute("SELECT word, part_of_speech FROM words WHERE id == ?", (listikIDs[0][0], ))
    abobaList = cursor.fetchall()
    pachForm = abobaList[0][0]
    part_of_speech = abobaList[0][1]
    dictRandom = dict()
    dictRandom['sure'] = False
    lenEnd = 0
    for i in pachForm[::-1]:
        if i in ['ё', 'у', 'е', 'ы', 'а', 'о', 'э', 'я', 'і', 'ю']:
            lenEnd += 1
        else:
            break
    dictRandom['analysis'] = [{'type': 2, 'text': pachForm[0:len(pachForm) - lenEnd]}, {'type': 4, 'text': pachForm[len(pachForm) - lenEnd:]}]
    if pachForm == word:
        return dictRandom
    else:
        return algo2(word, dictRandom, listikIDs[0][0], connection)

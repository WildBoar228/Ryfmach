from .Algo1 import algo1

def noteq(word, razbor):
    listiks = []
    for tempMorph in razbor["analysis"]:
        listiks.append(tempMorph['text'])
    return word != "".join(listiks)

#Ці гэта загадны лад (заглушка, калі 2 варыянты)
def zagLad(wordy, formy : list):
    if formy.count(wordy) >= 2:
        return True
    if wordy in formy:
        pos = formy.index(wordy)
        if pos == 5:
            return False
        if pos == 8:
            return True
        for temp in formy[::-1]:
            if temp[len(temp) - 2: len(temp)] == "це":
                if temp == wordy:
                    return True
                else:
                    return False
    return False

#Вылучэнне корня і суфіксаў
def algo4(wordy, kol, dicty, listik):
    i = 0
    while kol != 1 and len(wordy) > 0:
        tempDic = dict()
        if i == 0:
            tempDic['type'] = 2
        else:
            tempDic['type'] = 3
        tempDic['text'] = wordy[0:len(listik[i])]
        wordy = wordy[len(listik[i]):]
        kol -= 1
        i += 1
        dicty.append(tempDic)

    if len(wordy) > 0:
        tempDic = dict()
        if i == 0:
            tempDic['type'] = 2
        else:
            tempDic['type'] = 3
        tempDic['text'] = wordy
        dicty.append(tempDic)
    return dicty

#Вылучэнне канчатка
def algo3(wordy, formy, part_of_speech):
    lenansy = -1
    ansy = ''
    for tword in formy:
        dlinaSame = 0
        for i in range(min(len(wordy), len(tword))):
            if wordy[i] == tword[i]:
                dlinaSame += 1
            else:
                break
        if dlinaSame != len(wordy) and dlinaSame != len(tword):
            tempLen = len(wordy) - dlinaSame
            if tempLen < lenansy or lenansy == -1:
                lenansy = tempLen
                ansy = wordy[len(wordy) - lenansy:len(wordy)]
    if part_of_speech == 1 and lenansy < len(wordy):
        if (wordy[-lenansy - 1] == 'а' or wordy[-lenansy - 1] == 'я') and wordy[-lenansy] == 'м':
            ansy = wordy[-lenansy - 1] + ansy
    if part_of_speech == 3 and lenansy < len(wordy):
        if wordy[-lenansy - 1] == 'ы' or wordy[-lenansy - 1] == 'і':
            ansy = wordy[-lenansy - 1] + ansy
    return ansy


#Разбор слова з вядомым разборам пачатковай формы слова
def algo2(word, razbor : dict, initial_id, connection):
    flagEnding = noteq(word, razbor)
    dictRazbor = {}
    dictRazbor["sure"] = False
    dictRazbor["analysis"] = []

    cursor = connection.cursor()
    cursor.execute("SELECT word, part_of_speech FROM words WHERE initial_id == ?", (initial_id, ))
    formsTemp = cursor.fetchall()
    forms = []
    part_of_speech = formsTemp[0][1]

    lenPrist = 0
    rootToCheckPre = ''
    for tempMorph in razbor["analysis"]:
        if tempMorph["type"] == 1:
            lenPrist += len(tempMorph["text"])
            dictRazbor["analysis"].append(tempMorph)
        if tempMorph["type"] == 2:
            rootToCheckPre = tempMorph['text']
    for forma in formsTemp:
        forms.append(forma[0][lenPrist:])
    wordNoPrist = word[lenPrist:]

    #праверка прыстаўкі (з-са)
    if lenPrist != 0 and wordNoPrist[0] != rootToCheckPre[0] and wordNoPrist[0] == 'а':
        lastPre = dictRazbor['analysis'][-1]['text']
        if lastPre == 'з':
            dictRazbor['analysis'][-1]['text'] = 'са'
        else:
            dictRazbor['analysis'][-1]['text'] = dictRazbor['analysis'][-1]['text'] + 'а'
        wordNoPrist = wordNoPrist[1:]
        for i in range(len(forms)):
            forms[i] = forms[i][1:]

    postZag = 'no'
    suffZag = 'no'
    postSimp = 'no'
    ending = 'no'
    suffPast = 'no'
    suffDeePrym = 'no'
    suffDeePrych = 'no'
    if razbor['analysis'][-1]['type'] == 5 and part_of_speech == 2:
        postSimp = wordNoPrist[len(wordNoPrist) - 2: len(wordNoPrist)]
        wordNoPrist = wordNoPrist[0:len(wordNoPrist) - 2]
        del razbor["analysis"][-1]
    if part_of_speech == 2:
        if flagEnding:
            del razbor["analysis"][-1] #Прыбіраем суфікс ініфінітыва
        mainSuff = razbor["analysis"][-1]
        if wordNoPrist[-1] == 'ы':
            if wordNoPrist[len(wordNoPrist) - 3: len(wordNoPrist)] in ['учы', 'ючы', 'ачы', 'ячы', 'ўшы']:
                suffDeePrych = wordNoPrist[len(wordNoPrist) - 3: len(wordNoPrist)]
                wordNoPristEndFormSuff = wordNoPrist[0:len(wordNoPrist) - 3]
                flagEnding = False
            elif wordNoPrist[len(wordNoPrist) - 2: len(wordNoPrist)] == "шы":
                suffDeePrych = "шы"
                wordNoPristEndFormSuff = wordNoPrist[0:len(wordNoPrist) - 2]
                flagEnding = False
            else:
                ending = wordNoPrist[-1]
                wordNoPristEndFormSuff = wordNoPrist[0:len(wordNoPrist) - 1]
                if wordNoPristEndFormSuff[len(wordNoPristEndFormSuff) - 2: len(wordNoPristEndFormSuff)] in ['ем', 'ім', 'ен']:
                    suffDeePrym = wordNoPristEndFormSuff[len(wordNoPristEndFormSuff) - 2: len(wordNoPristEndFormSuff)]
                    wordNoPristEndFormSuff = wordNoPristEndFormSuff[0:len(wordNoPristEndFormSuff) - 2]
                elif wordNoPristEndFormSuff[len(wordNoPristEndFormSuff) - 1: len(wordNoPristEndFormSuff)] in ['н', 'т', 'л']:
                    suffDeePrym = wordNoPristEndFormSuff[len(wordNoPristEndFormSuff) - 1: len(wordNoPristEndFormSuff)]
                    wordNoPristEndFormSuff = wordNoPristEndFormSuff[0:len(wordNoPristEndFormSuff) - 1]
                    if wordNoPristEndFormSuff[-1] == 'а' and mainSuff['text'] != wordNoPristEndFormSuff[len(wordNoPristEndFormSuff) - len(mainSuff['text']): len(wordNoPristEndFormSuff)]:
                        suffDeePrym = 'ан'
                        wordNoPristEndFormSuff = wordNoPristEndFormSuff[0:len(wordNoPristEndFormSuff) - 1]
        else:
            if wordNoPrist[-1] == 'ў':
                ending = ''
                suffPast = 'ў'
                wordNoPristEndFormSuff = wordNoPrist[0:len(wordNoPrist) - 1]
            elif wordNoPrist[-2] == 'л' and wordNoPrist[-1] in ['а', 'о', 'і']:
                ending = wordNoPrist[-1]
                suffPast = 'л'
                wordNoPristEndFormSuff = wordNoPrist[0:len(wordNoPrist) - 2]
            elif wordNoPrist[-1] == 'й':
                ending = ''
                suffZag = 'й'
                wordNoPristEndFormSuff = wordNoPrist[0:len(wordNoPrist) - 1]
            else:
                if wordNoPrist[len(wordNoPrist)-2:len(wordNoPrist)] == 'це':
                    if zagLad(wordNoPrist, forms):
                        postZag = 'це'
                        if wordNoPrist[-3] == 'й':
                            ending = ''
                            suffZag = 'й'
                            wordNoPrist = wordNoPrist[0:len(wordNoPrist) - 3]
                        else:
                            if wordNoPrist[-3] in ['ё', 'у', 'е', 'ы', 'а', 'о', 'э', 'я', 'і', 'ю']:
                                ending = wordNoPrist[-3]
                                wordNoPrist = wordNoPrist[0:len(wordNoPrist) - 3]
                            else:
                                ending = ''
                                wordNoPrist = wordNoPrist[0:len(wordNoPrist) - 2]
                    else:
                        if wordNoPrist[-3] in ['ё', 'у', 'е', 'ы', 'а', 'о', 'э', 'я', 'і', 'ю']:
                            ending = wordNoPrist[-3] + 'це'
                            wordNoPrist = wordNoPrist[0:len(wordNoPrist) - 3]
                        else:
                            ending = 'це'
                            wordNoPrist = wordNoPrist[0:len(wordNoPrist) - 2]
                if flagEnding and ending == 'no':
                    ending = algo3(wordNoPrist, forms, part_of_speech)
                    if ending == 'лю':
                        ending = 'ю'
                    wordNoPristEndFormSuff = wordNoPrist[0:len(wordNoPrist) - len(ending)]
                else:
                    wordNoPristEndFormSuff = wordNoPrist
    else:
        ending = algo3(wordNoPrist, forms, part_of_speech)
        wordNoPristEndFormSuff = wordNoPrist[0:len(wordNoPrist) - len(ending)]
            

    kol = 0
    listikSuff = []
    for morph in razbor['analysis']:
        if morph['type'] == 2 or morph['type'] == 3:
            kol += 1
            listikSuff.append(morph['text'])
    dictRazbor['analysis'] = algo4(wordNoPristEndFormSuff, kol, dictRazbor['analysis'], listikSuff)

    if suffDeePrych == suffDeePrym == suffPast == suffZag == postZag == 'no':
        #выпраўленне памылкі з канчаткамі суфіксаў
        if flagEnding and part_of_speech == 2:
            dicLastSuf = dictRazbor['analysis'][-1]
            if dicLastSuf['type'] in [3, 2] and len(dicLastSuf['text']) > 1:
                suffy = dicLastSuf['text']
                if suffy[-1] in ['ё', 'у', 'е', 'ы', 'а', 'о', 'э', 'я', 'і', 'ю'] and suffy[-2] in ['ё', 'у', 'е', 'ы', 'а', 'о', 'э', 'я', 'і', 'ю']:
                    dicLastSuf['text'] = suffy[0:len(suffy) - 1]
                    dictRazbor['analysis'][-1] = dicLastSuf
                    ending = suffy[-1] + ending
            while len(ending) == 0 or ending[0] not in ['ё', 'у', 'е', 'ы', 'а', 'о', 'э', 'я', 'і', 'ю']:
                if dicLastSuf['type'] not in [3, 2] or len(dicLastSuf['text']) == 0:
                    break
                suffy = dicLastSuf['text']
                dicLastSuf['text'] = suffy[0:len(suffy) - 1]
                dictRazbor['analysis'][-1] = dicLastSuf
                ending = suffy[-1] + ending
            if dictRazbor['analysis'][-1]['type'] == 3 and len(dictRazbor['analysis'][-1]['text']) == 0:
                del dictRazbor['analysis'][-1]

    #выпраўленне памлкі з корнем у словах гнаць, браць і т. п.
    if part_of_speech == 2:
        rooty = ''
        for morph in razbor['analysis']:
            if morph['type'] == 2:
                rooty = morph['text']
        if len(rooty) == 2:
            ourRoot = ''
            ourSuf = ''
            posR = 0
            for i in range(len(dictRazbor['analysis']) - 1):
                if dictRazbor['analysis'][i]['type'] == 2:
                    ourRoot = dictRazbor['analysis'][i]['text']
                    posR = i
                    ourSuf = dictRazbor['analysis'][i + 1]['text']
            if ourRoot != '' and ourSuf != '' and dictRazbor['analysis'][posR + 1]['type'] == 3:
                if ourSuf[0] == rooty[-1] and ourRoot[-1] in ['ё', 'у', 'е', 'ы', 'а', 'о', 'э', 'я', 'і', 'ю']:
                    ourRoot = ourRoot + ourSuf[0]
                    ourSuf = ourSuf[1:]
                    dictRazbor['analysis'][posR] = {'type': 2, 'text': ourRoot}
                    if ourSuf != '':
                        dictRazbor['analysis'][posR + 1] = {'type': 3, 'text': ourSuf}
                    else:
                        del dictRazbor['analysis'][posR + 1]
 

    if suffPast != 'no':
        tempDic = dict()
        tempDic['type'] = 3
        tempDic['text'] = suffPast
        dictRazbor['analysis'].append(tempDic)
    if suffDeePrych != 'no':
        tempDic = dict()
        tempDic['type'] = 3
        tempDic['text'] = suffDeePrych
        dictRazbor['analysis'].append(tempDic)
    if suffDeePrym != 'no':
        tempDic = dict()
        tempDic['type'] = 3
        tempDic['text'] = suffDeePrym
        dictRazbor['analysis'].append(tempDic)
    if suffZag != 'no':
        tempDic = dict()
        tempDic['type'] = 3
        tempDic['text'] = suffZag
        dictRazbor['analysis'].append(tempDic)
    if ending != 'no':
        tempDic = dict()
        tempDic['type'] = 4
        tempDic['text'] = ending
        dictRazbor['analysis'].append(tempDic)
    if postZag != 'no':
        tempDic = dict()
        tempDic['type'] = 5
        tempDic['text'] = postZag
        dictRazbor['analysis'].append(tempDic)
    if postSimp != 'no':
        tempDic = dict()
        tempDic['type'] = 5
        tempDic['text'] = postSimp
        dictRazbor['analysis'].append(tempDic)
    return dictRazbor

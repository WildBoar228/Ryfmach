import os
import sqlite3

CURRENT_DIR = os.path.dirname(os.path.abspath(__file__))
DB_PATH = os.path.abspath(os.path.join(CURRENT_DIR, "..", "..", "..", "db", "shared", "Slounik5.db"))

#Разбор пачатковай формы слова
def algo1(word, analysis):
    conn = sqlite3.connect(DB_PATH)
    cursor = conn.cursor()
    dictRazbor = {}
    dictRazbor["sure"] = True
    dictRazbor["analysis"] = []
    listikMorph = analysis.split('-')
    listikType = [0] * len(listikMorph)
    listikLetters = [""] * len(listikMorph)

    if listikMorph[-1] == "небудзь" or listikMorph[-1] == "сьці":
        listikType[-1] = 5
        listikLetters[-1] = listikMorph[-1]
        del listikMorph[-1]
    if len(listikMorph) >= 2 and listikMorph[-1] == "ці" and listikMorph[-2] == "сь":
        listikType[-2] = 5
        listikLetters[-2] = "сьці"
        del listikMorph[-1]
        del listikMorph[-1]
        del listikType[-1]
        del listikLetters[-1]
 
    listikWordNoEndPostNullSuf = []
    for i in range(len(listikMorph)):
        if listikMorph[i][0] == '<':
            listikType[i] = 3
            listikLetters[i] = listikMorph[i].strip("<>")
        elif listikMorph[i][0] == '[':
            listikType[i] = 4
            listikLetters[i] = listikMorph[i].strip("[]")
        else:
            listikWordNoEndPostNullSuf.append(listikMorph[i])
    
    listikDefRoot = []
    #cursor.execute("SELECT position FROM root_table WHERE word == ?", (word, ))
    #listikDefRoot = cursor.fetchall()
    if len(listikDefRoot) != 0:
        posy = listikDefRoot[0][0]
        for i in range(posy):
            listikType[i] = 1
            listikLetters[i] = listikWordNoEndPostNullSuf[i]
        stopMorph = posy
    else:
        cursor.execute("SELECT spelling FROM morph_prefixes WHERE id > ?", (96, ))
        pristavkiTemp = cursor.fetchall()
        pristavki = ['а', 'аба', 'па', 'на', 'вы', 'пера', 'за', 'с', 'раза', 'у']
        for prist in pristavkiTemp:
            pristavki += list(prist)
        pristavkiPlusApostr = []
        for prist in pristavki:
            pristavkiPlusApostr.append(prist + "'")
        stopMorph = -1
        for i in range(0, len(listikWordNoEndPostNullSuf) - 1):
            stopMorph = i
            if listikWordNoEndPostNullSuf[i] in pristavki or listikWordNoEndPostNullSuf[i] in pristavkiPlusApostr:
                listikType[i] = 1
                listikLetters[i] = listikWordNoEndPostNullSuf[i]
            else:
                stopMorph -= 1 
                break
        stopMorph += 1 
    listikType[stopMorph] = 2
    listikLetters[stopMorph] = listikWordNoEndPostNullSuf[stopMorph]
    stopMorph += 1
    for i in range(stopMorph, len(listikWordNoEndPostNullSuf)):
        listikType[i] = 3
        listikLetters[i] = listikWordNoEndPostNullSuf[i]

    for i in range(len(listikType)):
        tempDic = {}
        tempDic['type'] = listikType[i]
        tempDic['text'] = listikLetters[i]
        dictRazbor["analysis"].append(tempDic)
    
    return dictRazbor
var alphabet = "-абвгдеёжзійклмнопрстуўфхцчш'ыьэюя";
var vowels= "аеёіоуыэюя";

var cons_sound_list = ['б', "б'", 'п', "п'", 'в', "в'", 'г', "г'", 'х', "х'", 'ґ', "ґ'", 'к', "к'", 'д', "дз'", 'т', "ц'", 'дз', 'ц', 'ж', 'ш', 'з', "з'", 'с', "с'", 'й', 'л', "л'", 'м', "м'", 'н', "н'", 'р', 'ф', "ф'", 'дж', 'ч', 'ў'];
var cons_soft_sound_list = ["б'", "п'", "в'", "г'", "х'", "ґ'", "к'", "дз'", "ц'", "з'", "с'", 'й', "л'", "м'", "н'", "ф'"];

var phon_response = [];
var precalc_phon_html = [];

var input_word = "";
var accent_index = -1;

const search_input_phon = document.getElementById("search-input");
const search_form = document.getElementById("search-form");
const search_button_phon = document.getElementById("search-button");
const search_icon = document.getElementById("search-icon");
const search_spinner = document.getElementById("search-spinner");
const search_status_text = document.getElementById("search-status-text");
const search_status_info = document.getElementById("search-status-info");

const word_variants_block = document.getElementById("word-variants-block");
const dropdown_choose_word = document.getElementById("dropdown-choose-word");
const dropdown_choose_word_menu = document.getElementById("dropdown-choose-word-menu");

const manual_accent_modal = new bootstrap.Modal(document.getElementById('manual-accent-modal'));
const letter_buttons_block = document.getElementById("letter-buttons-block");
const search_accent_button = document.getElementById("search-accent-button");
const scroll_up_button = document.querySelector(".button-scroll-up");

const phon_analysis_block = document.getElementById("phon-analysis-block");

const fa_long_arrow_left = `<i class="fa fa-long-arrow-left" aria-hidden="true"></i>`
const fa_long_arrow_right = `<i class="fa fa-long-arrow-right" aria-hidden="true"></i>`

function set_loading(is_loading) {
    search_button_phon.disabled = is_loading;
    search_icon.hidden = is_loading;
    search_spinner.hidden = !is_loading;
}


function escape_html(value) {
    return String(value ?? "").replace(/[&<>"']/g, (char) => ({
        "&": "&amp;",
        "<": "&lt;",
        ">": "&gt;",
        '"': "&quot;",
        "'": "&#39;",
    }[char]));
}


function bind_events() {
    search_form.addEventListener("submit", (event) => {
        event.preventDefault();
        post_phon_request();
    });
    search_accent_button.addEventListener("click", post_phon_with_manual_accent);
    scroll_up_button.addEventListener("click", scroll_up);
}


function is_belarusian(word){
    for (let char in word){
        if (!alphabet.includes(word[char].toLowerCase()))
            return false;
    }
    return true;
}


function is_vowel(sound) {
    if (sound.length == 0) return false;
    if ("аоуыіэ".includes(sound[0])) {
        return true;
    }
    if (sound.length == 3) {
        if (sound[0] == "_" && "аоуыіэ".includes(sound[1]) && sound[2] == "_") {
            return true;
        }
    }
    return false;
}


function is_consonant(sound) {
    return cons_sound_list.includes(sound);
}


function word_contains_vowels(word){
    let seen_vowel = false;
    for (let char in word)
        if (vowels.includes(word[char]))
            seen_vowel = true;
    return seen_vowel;
}


function word_with_accent(word, accent, classes_accent="accent-vowel"){
    word = String(word ?? "");
    if (accent !== undefined && accent !== null && accent >= 0 && accent < word.length)
        return escape_html(word.slice(0, accent)) + `<span class="${classes_accent}">${escape_html(word[accent])}</span>` + escape_html(word.slice(accent + 1));
    return escape_html(word);
}


function word_data_to_html(word_data, classes_normal="info-text", classes_accent="accent-vowel"){
    let word = word_data.word;
    let accent = word_data.accent;

    let text = word_with_accent(word, accent, classes_accent);
    if (word_data.is_initial !== undefined){
        if (!word_data.is_initial)
            text += ` ${fa_long_arrow_left} ${word_with_accent(word_data.initial_word, word_data.initial_accent, classes_accent)}`;
        text += ` (${escape_html(word_data.part_of_speech)})`;
    }

    text = `<p class="${classes_normal}">${text}</p>`;
    return text;
}


function update_phon(word_variant_index){
    dropdown_choose_word.innerHTML = "";
    if (precalc_phon_html.length > 1)
        dropdown_choose_word.innerHTML = `${word_variant_index + 1}/${precalc_phon_html.length} `;
    dropdown_choose_word.innerHTML +=
        word_data_to_html(phon_response.word_variants[word_variant_index].word_variant);
    
    phon_analysis_block.innerHTML = precalc_phon_html[word_variant_index];
}


function add_color_to_sound(s, tag_name="div") {
    s = String(s ?? "");
    s = s.replaceAll("г*", "ґ");
    let letter_class = "";
    if (is_vowel(s)) {
        letter_class = "transcription-vowel";
        if (s.length > 1 && s[0] == "_") {
            s = s[1] + "\u0301";
        }
    } else if (cons_soft_sound_list.includes(s)) {
        letter_class = "transcription-cons-soft";
    } else if (is_consonant(s)) {
        letter_class = "transcription-cons-hard";
    }
    return `<${tag_name} class="${letter_class}">${escape_html(s)}</${tag_name}>`;
}


function highlight_sounds(s) {
    let sound_start_index = s.length;
    let result_s = "";
    for (let i = 0; i < s.length; ++i) {
        if (s[i] == '[') {
            sound_start_index = i + 1;
            result_s += "[";
        } else if (s[i] == ']') {
            result_s += `${add_color_to_sound(s.slice(sound_start_index, i), "span")}`;
            sound_start_index = s.length;
        }
        if (sound_start_index == s.length) {
            result_s += escape_html(s[i]);
        }
    }
    
    return result_s;
}


function process_phon_response(data){
    phon_response = data;
    
    set_loading(false);

    word_variants_block.style.visibility="visible";
    dropdown_choose_word.innerHTML="-";
    dropdown_choose_word_menu.innerHTML = "";
    search_status_text.innerHTML = `Варыянты: ${Object.keys(data.word_variants).length}`;
    search_status_info.innerHTML = "";

    if (Object.keys(data.word_variants).length == 0){
        generate_letter_buttons();
        return;
    }

    precalc_phon_html = precalc_phon_html.slice(0, data.word_variants.length);
    for (let i in data.word_variants){
        let word_data = data.word_variants[i].word_variant;
        dropdown_choose_word_menu.innerHTML += `<li><button class="dropdown-item" data-phon-index="${i}">${word_data_to_html(word_data)}</button></li>`;

        precalc_phon_html[i] = "";
        const word_text = data.word_variants[i].word_variant.word;
        const letter_map = data.word_variants[i].letter_map;
        const transcription = data.word_variants[i].transcription;
        const phenomena = data.word_variants[i].phenomena;
        const sound_descr = data.word_variants[i].sound_analysis;

        precalc_phon_html[i] += `\n<div class="transcription-block">[`;
        for (let j in transcription) {
            precalc_phon_html[i] += add_color_to_sound(transcription[j]);
        }
        precalc_phon_html[i] += `]</div>`;
        
        precalc_phon_html[i] += `<div class="sound-analysis-block info-text">`;
        for (let j in letter_map) {
            precalc_phon_html[i] += `<div class="sound-analysis-line line${j % 2}">`;
            precalc_phon_html[i] += `<div class="sound-analysis-group sound-analysis-letters">`;
            for (let k in letter_map[j][0]) {
                precalc_phon_html[i] += `<div>${escape_html(word_text[letter_map[j][0][k]])}</div>   `;
            }
            precalc_phon_html[i] += `</div>`;
            precalc_phon_html[i] += `   ${fa_long_arrow_right}  `;
            precalc_phon_html[i] += `<div class="sound-analysis-group sound-analysis-details">`;
            for (let k in letter_map[j][1]) {
                precalc_phon_html[i] += `<div>`;
                precalc_phon_html[i] += add_color_to_sound(transcription[letter_map[j][1][k]], "span");
                precalc_phon_html[i] += `  &ndash;  ${highlight_sounds(sound_descr[letter_map[j][1][k]])}`;
                precalc_phon_html[i] += "</div>";
            }
            if (letter_map[j][1].length == 0) {
                precalc_phon_html[i] += `&empty;`;
            }
            precalc_phon_html[i] += `</div>`;
            precalc_phon_html[i] += `</div>`;
        }
        precalc_phon_html[i] += `</div>`;
    }

    dropdown_choose_word_menu.querySelectorAll("[data-phon-index]").forEach((button) => {
        button.addEventListener("click", () => update_phon(Number(button.dataset.phonIndex)));
    });
    update_phon(0);
}


function generate_letter_buttons(){
    accent_index = -1;
    search_status_info.innerHTML = `<div class="alert alert-warning info-text" role="alert">Невядомае слова</div>`

    let word = input_word;

    if (!word_contains_vowels(word)){
        search_status_info.innerHTML = `<div class="alert alert-danger info-text" role="alert">У гэтым слове няма галосных</div>`;
        return;
    }
    
    letter_buttons_block.innerHTML = "";
    manual_accent_modal.show();
    
    const letters_div = letter_buttons_block;

    for (let char in word){
        if (vowels.includes(word[char])){
            letters_div.innerHTML += `\n<button type="button" class="square-letter-button-outline" data-accent-index="${char}" id="letter_btn${char}">${escape_html(word[char])}</button>`;
            accent_index = parseInt(char);
        }
        else{
            letters_div.innerHTML += `\n<div class="square-letter-label"><label>${escape_html(word[char])}</label></div>`;
        }
    }

    letters_div.querySelectorAll("[data-accent-index]").forEach((button) => {
        button.addEventListener("click", () => letter_button_onclick(Number(button.dataset.accentIndex)));
    });
    letter_button_onclick(accent_index);
}


function letter_button_onclick(index){
    const btn = document.getElementById(`letter_btn${index}`);
    if (accent_index != -1){
        const prev_btn = document.getElementById(`letter_btn${accent_index}`);
        prev_btn.classList.remove("square-letter-button-chosen");
        prev_btn.classList.add("square-letter-button-outline");
    }

    accent_index = index;
    btn.classList.remove("square-letter-button-outline");
    btn.classList.add("square-letter-button-chosen");
}


function clean_input_word(w) {
    w = w.toLowerCase();
    let pref = 0;
    while (pref < w.length && w[pref] == ' ') {
        ++pref;
    }
    let suf = w.length - 1;
    while (suf >= 0 && w[suf] == ' ') {
        --suf;
    }
    w = w.slice(pref, suf + 1);
    w = w.replaceAll(" ", "-");
    w = w.replaceAll("и", "і");
    w = w.replaceAll("i", "і"); // english i
    w = w.replaceAll("щ", "ў");
    w = w.replaceAll("ъ", "'");
    return w;
}


function post_phon_request(){
    input_word = clean_input_word(search_input_phon.value);
    accent_index = -1;

    if (input_word == ""){
        word_variants_block.style.visibility = "visible";
        return;
    }

    if (!is_belarusian(input_word)){        
        search_status_info.innerHTML = `<div class="alert alert-danger info-text" role="alert">Слова павінна складацца толькі з беларускіх літар!</div>`;
        return;
    }

    if (input_word.length > 40){        
        search_status_info.innerHTML = `<div class="alert alert-danger info-text" role="alert">Нельга ўводзіць словы даўжэй за 40 літар! </div>`;
        return;
    }

    set_loading(true);

    $.ajax({
        url: "/phonetics",
        method: "post",
        dataType: "json",
        contentType: "application/json",
        data: JSON.stringify({
            "word": input_word,
        }),
        success: process_phon_response,
        error: () => {
            search_status_info.innerHTML = `<div class="alert alert-danger info-text" role="alert">Не атрымалася выканаць разбор. Паспрабуйце яшчэ раз.</div>`;
        },
        complete: () => set_loading(false),
    });
}


function post_phon_with_manual_accent(){
    if (input_word == ""){
        word_variants_block.style.visibility = "visible";
        return;
    }

    if (!is_belarusian(input_word)){        
        search_status_info.innerHTML = `<div class="alert alert-danger info-text" role="alert">Слова павінна складацца толькі з беларускіх літар!</div>`;
        return;
    }

    if (input_word.length > 40){        
        search_status_info.innerHTML = `<div class="alert alert-danger info-text" role="alert">Нельга ўводзіць словы даўжэй за 40 літар! </div>`;
        return;
    }

    if (accent_index == -1){        
        search_status_info.innerHTML = `<div class="alert alert-danger info-text" role="alert">Укажыце націскную галосную</div>`
        return;
    }

    manual_accent_modal.hide();
    
    search_status_info.innerHTML = "";
    set_loading(true);

    $.ajax({
        url: "/phonetics",
        method: "post",
        dataType: "json",
        contentType: "application/json",
        data: JSON.stringify({
            "word": input_word,
            "accent": accent_index
        }),
        success: process_phon_response,
        error: () => {
            search_status_info.innerHTML = `<div class="alert alert-danger info-text" role="alert">Не атрымалася выканаць разбор. Паспрабуйце яшчэ раз.</div>`;
        },
        complete: () => set_loading(false),
    });
}


function scroll_up(){
    window.scrollTo({top: 0, behavior: "smooth"});
}


bind_events();

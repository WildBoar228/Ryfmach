"use strict";

const alphabet = "-абвгдеёжзійклмнопрстуўфхцчш'ыьэюя";
const vowels = "аеёіоуыэюя";

const RhymeSearchState = Object.freeze({
    idle: "idle",
    lookingUp: "looking_up",
    needsChoice: "needs_choice",
    loadingRhymes: "loading_rhymes",
    showingResults: "showing_results",
    error: "error",
});

let searchState = RhymeSearchState.idle;
let currentWord = "";
let accentIndex = -1;
let selectedPronunciation = null;
let pronunciationVariants = [];
let rhymesResponse = null;
let activeRhymeRequest = null;
let requestVersion = 0;
let openManualAccentAfterVariantModal = false;

const searchInputRhyme = document.getElementById("search-input");
const searchForm = document.getElementById("search-form");
const searchButtonRhyme = document.getElementById("search-button");
const searchIcon = document.getElementById("search-icon");
const searchSpinner = document.getElementById("search-spinner");
const searchStatusInfo = document.getElementById("search-status-info");

const pronunciationModalElement = document.getElementById("rhyme-pronunciation-modal");
const pronunciationOptions = document.getElementById("rhyme-pronunciation-options");
const manualPronunciationButton = document.getElementById("manual-pronunciation-button");
const selectedPronunciationControl = document.getElementById("selected-pronunciation-control");
const selectedPronunciationText = document.getElementById("selected-pronunciation-text");
const changePronunciationButton = document.getElementById("change-pronunciation-button");

const rhymesBlock = document.getElementById("rhymes-block");
const rhymesCountText = document.getElementById("rhyme-count-text");
const rhymesList = document.getElementById("rhymes-list");

const pronunciationModal = new bootstrap.Modal(pronunciationModalElement);
const manualAccentModal = new bootstrap.Modal(document.getElementById("manual-accent-modal"));
const letterButtonsBlock = document.getElementById("letter-buttons-block");
const searchAccentButton = document.getElementById("search-accent-button");
const saveFiltersButton = document.getElementById("save-filters-button");
const scrollUpButton = document.querySelector(".button-scroll-up");

const faLongArrowLeft = `<i class="fa fa-long-arrow-left" aria-hidden="true"></i>`;


function set_loading(isLoading) {
    searchButtonRhyme.disabled = isLoading;
    searchIcon.hidden = isLoading;
    searchSpinner.hidden = !isLoading;
}


function set_search_state(nextState) {
    searchState = nextState;
    set_loading(
        nextState === RhymeSearchState.lookingUp ||
        nextState === RhymeSearchState.loadingRhymes
    );
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


function render_status(message, level = "info") {
    searchStatusInfo.replaceChildren();
    if (!message)
        return;

    const alert = document.createElement("div");
    alert.className = `alert alert-${level} info-text`;
    alert.setAttribute("role", "alert");
    alert.textContent = message;
    searchStatusInfo.appendChild(alert);
}


function bind_events() {
    searchForm.addEventListener("submit", (event) => {
        event.preventDefault();
        start_new_search();
    });
    searchInputRhyme.addEventListener("input", () => {
        const editedWord = clean_input_word(searchInputRhyme.value);
        if (currentWord && editedWord !== currentWord)
            reset_search_state();
    });
    pronunciationOptions.addEventListener("click", (event) => {
        const button = event.target.closest("[data-pronunciation-index]");
        if (button)
            select_variant(Number(button.dataset.pronunciationIndex));
    });
    pronunciationModalElement.addEventListener("shown.bs.modal", () => {
        pronunciationOptions.querySelector("button")?.focus();
    });
    pronunciationModalElement.addEventListener("hidden.bs.modal", () => {
        if (!openManualAccentAfterVariantModal)
            return;
        openManualAccentAfterVariantModal = false;
        open_manual_accent_picker(false);
    });
    manualPronunciationButton.addEventListener("click", () => {
        openManualAccentAfterVariantModal = true;
        pronunciationModal.hide();
    });
    changePronunciationButton.addEventListener("click", change_pronunciation);
    searchAccentButton.addEventListener("click", post_rhymes_with_manual_accent);
    saveFiltersButton.addEventListener("click", update_filters);
    scrollUpButton.addEventListener("click", scroll_up);
    rhymesList.addEventListener("click", handle_rhyme_like_click);
}


function is_belarusian(word) {
    for (const character of word) {
        if (!alphabet.includes(character.toLowerCase()))
            return false;
    }
    return true;
}


function word_contains_vowels(word) {
    for (const character of word) {
        if (vowels.includes(character))
            return true;
    }
    return false;
}


function word_with_accent(word, accent, accentClass = "accent-vowel") {
    const normalizedWord = String(word ?? "");
    if (Number.isInteger(accent) && accent >= 0 && accent < normalizedWord.length) {
        return escape_html(normalizedWord.slice(0, accent)) +
            `<span class="${accentClass}">${escape_html(normalizedWord[accent])}</span>` +
            escape_html(normalizedWord.slice(accent + 1));
    }
    return escape_html(normalizedWord);
}


function pronunciation_accessible_label(pronunciation) {
    const stressedLetter = pronunciation.word[pronunciation.accent] ?? "";
    let label = `${pronunciation.word}, націск на літару «${stressedLetter}»`;
    const entry = pronunciation.dictionary_entry;
    if (entry && !entry.is_initial && entry.initial_word) {
        label += `, пачатковая форма ${entry.initial_word}`;
    }
    if (entry?.part_of_speech)
        label += `, ${entry.part_of_speech}`;
    if (!pronunciation.exact_match)
        label += ", магчымае выпраўленне напісання";
    return label;
}


function normalize_pronunciation(pronunciation) {
    if (!pronunciation || typeof pronunciation.word !== "string")
        return null;

    const accent = Number(pronunciation.accent);
    if (!Number.isInteger(accent) || accent < 0 || accent >= pronunciation.word.length)
        return null;

    const dictionaryId = pronunciation.dictionary_id === undefined
        ? null
        : Number(pronunciation.dictionary_id);
    if (dictionaryId !== null &&
        (!Number.isInteger(dictionaryId) || dictionaryId <= 0)) {
        return null;
    }

    return {
        dictionary_id: dictionaryId,
        word: pronunciation.word,
        accent: accent,
        exact_match: pronunciation.exact_match === true,
        dictionary_entry:
            pronunciation.dictionary_entry &&
            typeof pronunciation.dictionary_entry === "object"
                ? pronunciation.dictionary_entry
                : null,
    };
}


function rhyme_word_length_class(word) {
    const wordLength = String(word == null ? "" : word).length;
    if (wordLength > 17)
        return "is-extra-long";
    if (wordLength > 12)
        return "is-long";
    return "";
}


function rhyme_data_to_html(rhymeData, rhymeIndex) {
    const word = rhymeData.word;
    const wordClass = rhyme_word_length_class(word);
    let metadata = "";

    if (rhymeData.is_initial !== undefined) {
        if (!rhymeData.is_initial) {
            metadata += `${faLongArrowLeft} ${word_with_accent(
                rhymeData.initial_word, rhymeData.initial_accent)}`;
        }
        metadata += ` (${escape_html(rhymeData.part_of_speech)})`;
    }

    return `
        <li class="rhyme-item">
            <button class="rhyme-like" type="button" aria-label="${escape_html(`Падабаецца: ${word}`)}" aria-pressed="false" data-rhyme-index="${rhymeIndex}">
                <i class="fa fa-heart-o" aria-hidden="true"></i>
            </button>
            <div class="rhyme-text">
                <p class="rhyme-main ${wordClass}">${word_with_accent(word, rhymeData.accent)}</p>
                ${metadata ? `<p class="rhyme-meta">${metadata}</p>` : ""}
            </div>
        </li>`;
}


function get_like_payload(rhymeIndex) {
    if (!rhymesResponse || !selectedPronunciation)
        return null;

    const rhymeWord = rhymesResponse.rhymes_data?.[rhymeIndex];
    if (!rhymeWord)
        return null;

    return {
        request: {
            word: selectedPronunciation.word,
            stress: selectedPronunciation.accent,
        },
        rhyme: {
            word: rhymeWord.word,
            stress: rhymeWord.accent,
        },
    };
}


function set_rhyme_like_state(button, isLiked) {
    const icon = button.querySelector("i");
    button.classList.toggle("is-liked", isLiked);
    button.setAttribute("aria-pressed", String(isLiked));

    if (icon) {
        icon.classList.toggle("fa-heart", isLiked);
        icon.classList.toggle("fa-heart-o", !isLiked);
    }
}


function handle_rhyme_like_click(event) {
    const button = event.target.closest(".rhyme-like");
    if (!button || !rhymesList.contains(button))
        return;

    const rhymeIndex = Number(button.dataset.rhymeIndex);
    const payload = get_like_payload(rhymeIndex);
    if (!payload)
        return;

    const isLiked = button.classList.contains("is-liked");
    const endpoint = isLiked ? "/api/rhyme/dislike" : "/api/rhyme/like";

    button.disabled = true;
    $.ajax({
        url: endpoint,
        method: "post",
        dataType: "json",
        contentType: "application/json",
        data: JSON.stringify(payload),
        success: () => set_rhyme_like_state(button, !isLiked),
        error: () => {
            render_status("Не атрымалася захаваць адзнаку рыфмы.", "danger");
        },
        complete: () => {
            button.disabled = false;
        },
    });
}


function read_filters() {
    const partOfSpeech = [];
    for (let index = 1; index <= 7; ++index) {
        partOfSpeech.push(document.getElementById(`check-posp-${index}`).checked);
    }

    const selectedMistake = document.querySelector(
        "#search-mistake-radio input[type='radio']:checked");
    return {
        filtered_posp: partOfSpeech,
        only_initial: document.getElementById("check-only-initial").checked,
        search_mistake: Number(selectedMistake?.value ?? -1),
    };
}


function clean_input_word(word) {
    return word
        .toLowerCase()
        .trim()
        .replaceAll(" ", "-")
        .replaceAll("и", "і")
        .replaceAll("i", "і")
        .replaceAll("щ", "ў")
        .replaceAll("ъ", "'");
}


function validate_search_word(word) {
    if (word === "")
        return {valid: false, message: ""};
    if (!is_belarusian(word)) {
        return {
            valid: false,
            message: "Слова павінна складацца толькі з беларускіх літар!",
        };
    }
    if (Array.from(word).length > 40) {
        return {
            valid: false,
            message: "Нельга ўводзіць словы даўжэй за 40 літар!",
        };
    }
    return {valid: true, message: ""};
}


function cancel_active_rhyme_request() {
    ++requestVersion;
    const request = activeRhymeRequest;
    activeRhymeRequest = null;
    if (request)
        request.abort();
}


function send_rhyme_request(payload, onSuccess, errorMessage) {
    cancel_active_rhyme_request();
    const version = requestVersion;
    const request = $.ajax({
        url: "/api/rhymes",
        method: "post",
        dataType: "json",
        contentType: "application/json",
        data: JSON.stringify(payload),
    });
    activeRhymeRequest = request;

    request.done((data) => {
        if (version === requestVersion)
            onSuccess(data);
    });
    request.fail((_response, status) => {
        if (version !== requestVersion || status === "abort")
            return;
        set_search_state(RhymeSearchState.error);
        render_status(errorMessage, "danger");
    });
    request.always(() => {
        if (version !== requestVersion)
            return;
        activeRhymeRequest = null;
        set_loading(false);
    });
}


function hide_rhyme_results() {
    rhymesBlock.classList.add("is-hidden");
    rhymesCountText.textContent = "";
    rhymesList.replaceChildren();
}


function reset_search_state() {
    cancel_active_rhyme_request();
    set_search_state(RhymeSearchState.idle);
    currentWord = "";
    accentIndex = -1;
    selectedPronunciation = null;
    pronunciationVariants = [];
    rhymesResponse = null;
    openManualAccentAfterVariantModal = false;
    pronunciationModal.hide();
    selectedPronunciationControl.hidden = true;
    pronunciationOptions.replaceChildren();
    hide_rhyme_results();
    render_status("");
    manualAccentModal.hide();
}


function request_payload(word, accent = null, dictionaryId = null) {
    const payload = {
        word: word,
        ...read_filters(),
    };
    if (accent !== null)
        payload.accent = accent;
    if (dictionaryId !== null)
        payload.dictionary_id = dictionaryId;
    return payload;
}


function start_new_search() {
    const word = clean_input_word(searchInputRhyme.value);
    reset_search_state();

    const validation = validate_search_word(word);
    if (!validation.valid) {
        if (validation.message) {
            set_search_state(RhymeSearchState.error);
            render_status(validation.message, "danger");
        }
        return;
    }

    currentWord = word;
    request_rhyme_resolution();
}


function request_rhyme_resolution() {
    set_search_state(RhymeSearchState.lookingUp);
    pronunciationModal.hide();
    selectedPronunciationControl.hidden = true;
    hide_rhyme_results();
    render_status("");

    send_rhyme_request(
        request_payload(currentWord),
        process_resolution_response,
        "Не атрымалася выканаць пошук. Паспрабуйце яшчэ раз."
    );
}


function response_error() {
    set_search_state(RhymeSearchState.error);
    pronunciationModal.hide();
    selectedPronunciationControl.hidden = true;
    hide_rhyme_results();
    render_status("Сервер вярнуў некарэктны вынік пошуку.", "danger");
}


function process_resolution_response(data) {
    if (!data || typeof data.status !== "string") {
        response_error();
        return;
    }

    if (data.status === "not_found") {
        selectedPronunciation = null;
        pronunciationVariants = [];
        rhymesResponse = null;
        set_search_state(RhymeSearchState.needsChoice);
        render_status("Невядомае слова", "warning");
        open_manual_accent_picker(true);
        return;
    }

    if (data.status === "needs_choice") {
        if (!Array.isArray(data.variants) || data.variants.length === 0) {
            response_error();
            return;
        }
        render_variant_picker(data.variants);
        return;
    }

    if (data.status === "resolved") {
        process_resolved_response(data);
        return;
    }

    response_error();
}


function render_variant_picker(variants) {
    const normalizedVariants = variants
        .map(normalize_pronunciation)
        .filter((variant) => variant !== null && variant.dictionary_id !== null);
    if (normalizedVariants.length === 0) {
        response_error();
        return;
    }

    pronunciationVariants = normalizedVariants;
    selectedPronunciation = null;
    rhymesResponse = null;
    set_search_state(RhymeSearchState.needsChoice);
    hide_rhyme_results();
    selectedPronunciationControl.hidden = true;
    render_pronunciation_options();
    pronunciationModal.show();
    render_status("");
}


function render_pronunciation_options() {
    pronunciationOptions.replaceChildren();

    pronunciationVariants.forEach((pronunciation, index) => {
        const option = document.createElement("li");
        option.className = "rhyme-pronunciation-option";

        const button = document.createElement("button");
        button.className = "rhyme-pronunciation-option-button";
        button.type = "button";
        button.dataset.pronunciationIndex = String(index);
        button.setAttribute(
            "aria-label", pronunciation_accessible_label(pronunciation));

        const bullet = document.createElement("span");
        bullet.className = "rhyme-pronunciation-bullet";
        bullet.setAttribute("aria-hidden", "true");
        bullet.textContent = "•";

        const text = document.createElement("span");
        text.className = "rhyme-pronunciation-option-text";

        const word = document.createElement("span");
        word.innerHTML = word_with_accent(pronunciation.word, pronunciation.accent);
        text.appendChild(word);

        const entry = pronunciation.dictionary_entry;
        if (entry && !entry.is_initial && entry.initial_word) {
            text.appendChild(document.createTextNode(" ← "));
            const initialWord = document.createElement("span");
            initialWord.innerHTML = word_with_accent(
                entry.initial_word, Number(entry.initial_accent));
            text.appendChild(initialWord);
        }
        if (entry?.part_of_speech) {
            text.appendChild(document.createTextNode(
                ` (${entry.part_of_speech})`));
        }

        if (!pronunciation.exact_match) {
            const correction = document.createElement("span");
            correction.className = "rhyme-pronunciation-correction";
            correction.textContent = "Магчымае выпраўленне напісання";
            text.appendChild(correction);
        }

        button.appendChild(bullet);
        button.appendChild(text);
        option.appendChild(button);
        pronunciationOptions.appendChild(option);
    });
}


function select_variant(index) {
    const pronunciation = pronunciationVariants[index];
    if (!pronunciation)
        return;

    openManualAccentAfterVariantModal = false;
    selectedPronunciation = pronunciation;
    pronunciationModal.hide();
    render_selected_pronunciation();
    request_selected_rhymes();
}


function render_selected_pronunciation() {
    if (!selectedPronunciation) {
        selectedPronunciationControl.hidden = true;
        return;
    }

    selectedPronunciationText.innerHTML = word_with_accent(
        selectedPronunciation.word, selectedPronunciation.accent);
    const entry = selectedPronunciation.dictionary_entry;
    if (entry && !entry.is_initial && entry.initial_word) {
        selectedPronunciationText.appendChild(document.createTextNode(" ← "));
        const initialWord = document.createElement("span");
        initialWord.innerHTML = word_with_accent(
            entry.initial_word, Number(entry.initial_accent));
        selectedPronunciationText.appendChild(initialWord);
    }
    if (entry?.part_of_speech) {
        selectedPronunciationText.appendChild(document.createTextNode(
            ` (${entry.part_of_speech})`));
    }
    if (!selectedPronunciation.exact_match) {
        const correction = document.createElement("span");
        correction.className = "small-info-text";
        correction.textContent = " (выпраўленае напісанне)";
        selectedPronunciationText.appendChild(correction);
    }
    selectedPronunciationControl.setAttribute(
        "aria-label", pronunciation_accessible_label(selectedPronunciation));
    selectedPronunciationControl.hidden = false;
}


function request_selected_rhymes(errorMessage = "Не атрымалася знайсці рыфмы. Паспрабуйце яшчэ раз.") {
    if (!selectedPronunciation)
        return;

    set_search_state(RhymeSearchState.loadingRhymes);
    pronunciationModal.hide();
    hide_rhyme_results();
    render_selected_pronunciation();
    render_status("");

    send_rhyme_request(
        request_payload(
            selectedPronunciation.word,
            selectedPronunciation.accent,
            selectedPronunciation.dictionary_id),
        (data) => process_resolved_response(data, selectedPronunciation),
        errorMessage
    );
}


function process_resolved_response(data, preferredPronunciation = null) {
    if (!data || data.status !== "resolved" || !Array.isArray(data.rhymes_data)) {
        response_error();
        return;
    }

    const responsePronunciation = normalize_pronunciation(data.selected_variant);
    if (!responsePronunciation) {
        response_error();
        return;
    }

    if (
        preferredPronunciation &&
        preferredPronunciation.word === responsePronunciation.word &&
        preferredPronunciation.accent === responsePronunciation.accent &&
        preferredPronunciation.dictionary_id === responsePronunciation.dictionary_id
    ) {
        selectedPronunciation = preferredPronunciation;
    } else {
        selectedPronunciation = responsePronunciation;
    }

    if (pronunciationVariants.length === 0 &&
        selectedPronunciation.dictionary_id !== null) {
        pronunciationVariants = [selectedPronunciation];
    }

    rhymesResponse = {
        status: "resolved",
        selected_variant: selectedPronunciation,
        rhymes_data: data.rhymes_data,
    };
    render_selected_pronunciation();
    render_rhymes(data.rhymes_data);
}


function render_rhymes(rhymes) {
    pronunciationModal.hide();
    rhymesList.replaceChildren();
    rhymesBlock.classList.remove("is-hidden");
    rhymesCountText.textContent = `Рыфмы: ${rhymes.length}`;

    if (rhymes.length === 0) {
        rhymesList.innerHTML = `<li class="alert alert-info info-text" role="alert">Пу-пу-пу! Рыфмаў абранай трапнасці не знайшлося. Змяніце фільтры (<i class="fa fa-cog" aria-hidden="true"></i>) або паспрабуйце іншае слова.</li>`;
    } else {
        rhymesList.innerHTML = rhymes
            .map((rhyme, index) => rhyme_data_to_html(rhyme, index))
            .join("");
    }

    set_search_state(RhymeSearchState.showingResults);
    render_status("");
}


function open_manual_accent_picker(isUnknownWord) {
    accentIndex = -1;
    if (!word_contains_vowels(currentWord)) {
        set_search_state(RhymeSearchState.error);
        render_status("У гэтым слове няма галосных", "danger");
        return;
    }

    if (!isUnknownWord)
        render_status("Укажыце націскную галосную", "info");

    set_search_state(RhymeSearchState.needsChoice);
    letterButtonsBlock.replaceChildren();
    const letters = Array.from(currentWord);

    letters.forEach((letter, index) => {
        if (vowels.includes(letter)) {
            const button = document.createElement("button");
            button.type = "button";
            button.className = "square-letter-button-outline";
            button.dataset.accentIndex = String(index);
            button.id = `letter_btn${index}`;
            button.textContent = letter;
            button.setAttribute("aria-pressed", "false");
            button.setAttribute(
                "aria-label", `Націск на літару «${letter}», пазіцыя ${index + 1}`);
            button.addEventListener("click", () => letter_button_onclick(index));
            letterButtonsBlock.appendChild(button);
            accentIndex = index;
        } else {
            const label = document.createElement("div");
            label.className = "square-letter-label";
            label.textContent = letter;
            letterButtonsBlock.appendChild(label);
        }
    });

    letter_button_onclick(accentIndex);
    manualAccentModal.show();
}


function letter_button_onclick(index) {
    const button = document.getElementById(`letter_btn${index}`);
    if (!button)
        return;

    if (accentIndex !== -1) {
        const previousButton = document.getElementById(`letter_btn${accentIndex}`);
        if (previousButton) {
            previousButton.classList.remove("square-letter-button-chosen");
            previousButton.classList.add("square-letter-button-outline");
            previousButton.setAttribute("aria-pressed", "false");
        }
    }

    accentIndex = index;
    button.classList.remove("square-letter-button-outline");
    button.classList.add("square-letter-button-chosen");
    button.setAttribute("aria-pressed", "true");
}


function post_rhymes_with_manual_accent() {
    const validation = validate_search_word(currentWord);
    if (!validation.valid) {
        set_search_state(RhymeSearchState.error);
        render_status(validation.message, "danger");
        return;
    }
    if (accentIndex === -1) {
        render_status("Укажыце націскную галосную", "danger");
        return;
    }

    manualAccentModal.hide();
    selectedPronunciation = {
        dictionary_id: null,
        word: currentWord,
        accent: accentIndex,
        exact_match: true,
        dictionary_entry: null,
    };
    render_selected_pronunciation();
    request_selected_rhymes();
}


function change_pronunciation() {
    if (!currentWord)
        return;

    if (pronunciationVariants.length > 0) {
        render_pronunciation_options();
        pronunciationModal.show();
    } else {
        open_manual_accent_picker(false);
    }
}


function update_filters() {
    const editedWord = clean_input_word(searchInputRhyme.value);
    if (editedWord !== currentWord) {
        start_new_search();
        return;
    }

    if (!selectedPronunciation)
        return;

    request_selected_rhymes(
        "Не атрымалася абнавіць фільтры. Паспрабуйце яшчэ раз.");
}


function scroll_up() {
    window.scrollTo({top: 0, behavior: "smooth"});
}


bind_events();

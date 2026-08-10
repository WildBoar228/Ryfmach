from flask import (
    Flask, Response,
    session, request,
    render_template, send_from_directory, jsonify
)
import json
from pprint import pprint

from logging.handlers import RotatingFileHandler
import logging
import os
import time
from http.client import HTTPConnection

from config import FLASK_SECRET_KEY, RYFMACH_JINJA_PORT

API_PATHS = {
    "rhymes",
    "phonetics",
    "morphemics",
    "rhyme/like",
    "rhyme/dislike",
}

app = Flask(__name__,
            static_folder="../frontend/static",
            template_folder='../frontend/static/templates')
app.config['SECRET_KEY'] = FLASK_SECRET_KEY

app_file_handler = RotatingFileHandler(
    "logs/app.log",
    maxBytes=1 * 1024 * 1024,  # 1 MB
    backupCount=10,
    encoding="utf-8",
)

app_file_handler.setLevel(logging.INFO)
app_file_handler.setFormatter(
    logging.Formatter(
        "%(asctime)s %(levelname)s "
        "[%(name)s] %(message)s"
    )
)

app.logger.setLevel(logging.INFO)
app.logger.addHandler(app_file_handler)


@app.route('/')
def rhyme_page():
    input_word_info = session.get('rhyme_input_word_info', {'word': ''})
    session['rhyme_input_word_info'] = input_word_info
    return render_template(
        'rhyme_page.html',
        title="Рыфмач - падабраць рыфму",
        page_description="Рыфмач. Рыфмы на беларускай мове, пошук рыфм для вершаў. Рифмы на белорусском языке, поиск рифм для стихотворений.",
        input_word=input_word_info["word"],
        add_search_filter_button=True,
        canonical_url="https://ryfmach.online"
    )


@app.route('/phonetics')
def phonetics_page():
    input_word_info = {'word': ''} # session.get('phon_input_word_info', {'word': ''})
    # session['phon_input_word_info'] = input_word_info
    return render_template(
        'phonetics_page.html',
        title="Рыфмач - фанетычны разбор",
        page_description="Рыфмач. Фанетычны разбор і транскрыпцыі на беларускай мове. Фонетический разбор и транскрипции на белорусском языке.",
        input_word=input_word_info["word"],
        add_search_filter_button=False,
        canonical_url="https://ryfmach.online/phonetics"
    )


@app.route('/morphemics')
def morphemics_page():
    input_word_info = session.get("sklad_input_word_info", {"word": ""})
    
    return render_template(
        "morphemics_page.html",
        title="Рыфмач - марфемны разбор",
        page_description="Рыфмач. Марфемны і словаўтваральны разбор, разбор па складзе. Морфемный разбор, разбор слова по составу.",
        input_word=input_word_info["word"],
        add_search_filter_button=False,
        canonical_url="https://ryfmach.online/morphemics"
    )


@app.post("/api/<path:api_path>")
def proxy_api(api_path):
    if api_path not in API_PATHS:
        return jsonify({"error": "Not found"}), 404

    connection = HTTPConnection("127.0.0.1", 8081, timeout=30)

    try:
        connection.request(
            method="POST",
            url=f"/api/{api_path}",
            body=request.get_data(cache=False),
            headers={"Content-Type": request.content_type or "application/json"},
        )
        upstream = connection.getresponse()
        body = upstream.read()

        response = Response(body, status=upstream.status)
        content_type = upstream.getheader("Content-Type")
        if content_type:
            response.headers["Content-Type"] = content_type
        return response
    except OSError:
        app.logger.exception("C++ API is unavailable")
        return jsonify({"error": "API is unavailable"}), 502
    finally:
        connection.close()


@app.route('/favicon.ico')
def favicon():
    return send_from_directory(os.path.join(app.root_path, 'static'),
                               'favicon.ico', mimetype='image/x-icon')


@app.route('/sitemap')
def sitemap():
    return send_from_directory(os.path.join(app.root_path, 'static'),
                               'sitemap.xml', mimetype='application/xml')


if __name__ == '__main__':
    app.run(port=RYFMACH_JINJA_PORT, host='127.0.0.1')

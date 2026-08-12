from flask import Flask, make_response, render_template

from config import RYFMACH_IS_TEST_SITE, RYFMACH_JINJA_PORT


app = Flask(
    __name__,
    static_folder="../frontend/static",
    template_folder="../frontend/static/templates",
)


@app.get("/")
@app.get("/<path:path>")
def reconstruct_page(path: str = ""):
    response = make_response(
        render_template(
            "maintenance.html",
            is_test_site=RYFMACH_IS_TEST_SITE,
        ),
        503,
    )
    response.headers["Retry-After"] = "300"
    return response


if __name__ == "__main__":
    app.run(port=RYFMACH_JINJA_PORT, host="127.0.0.1")

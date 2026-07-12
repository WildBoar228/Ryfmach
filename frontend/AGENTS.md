# Frontend Agent Notes

## Scope and Structure

`frontend/` contains the server-rendered web UI for Ryfmach. It has no Node
package, bundler, or standalone frontend test command.

- `static/templates/` contains Jinja templates. `index.html` is the shared base
  template; feature pages extend it and may include shared partials such as
  `search_panel.html`.
- `static/js/` contains one browser script per feature: `rhymes.js`,
  `phonetics.js`, and `morphemics.js`.
- `static/css/` contains shared styles in `style.css` and page-specific styles
  in `phonetics.css` and `morphemics.css`.
- `static/img/` and `favicon.ico` are served assets. Preserve the existing
  image paths used by templates and CSS.

The pages use Bootstrap 5, jQuery 3.7.1, and Font Awesome from CDNs. Keep
external dependency versions and SRI attributes in sync when changing them.

## Backend Contract

Browser scripts call same-origin JSON endpoints. Keep their request and
response contracts synchronized with the server before changing either side:

- `rhymes.js`: `POST /rhymes`, `POST /rhyme/like`, and `POST /rhyme/dislike`
- `phonetics.js`: `POST /phonetics`
- `morphemics.js`: `POST /morphemics`

The backend passes template values such as `title`, `page_description`, and
`canonical_url`; retain the Jinja expressions and `{% block %}` structure.

## Implementation Conventions

- Use UTF-8 Belarusian copy and preserve the current Belarusian UI language.
- Match existing HTML indentation (four spaces) and CSS/JavaScript style.
- Prefer `const` for DOM references and values that do not change; keep
  feature state local to its script.
- User-provided text rendered with `innerHTML` must be escaped with the local
  `escape_html` helper. Do not interpolate untrusted values directly into HTML.
- Keep element IDs, data attributes, and JavaScript selectors aligned. Update
  the template and its feature script together when changing an interface.
- Preserve form submission behavior and accessible controls: use real buttons,
  labels for inputs, and meaningful `aria-*` attributes for icon-only controls
  or dynamic state.
- Cache-busting query versions are written directly in the templates. Bump the
  matching `?ver=` value when changing a referenced CSS or JavaScript asset.

## Validation

There is no automated frontend test suite. After frontend changes:

1. Start the application using the backend's documented build/run workflow.
2. Exercise the affected page in a browser, including validation and failed
   request states where applicable.
3. Check the browser console for JavaScript errors and verify responsive layout
   at a narrow viewport.

For a change that alters an endpoint payload or response, also run the backend
tests described in `backend/AGENTS.md`.

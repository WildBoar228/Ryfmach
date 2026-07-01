# Ryfmach-cpp Agent Notes

## Build

Configure from the repository root:

```bash
cmake -S . -B build
```

Build from the repository root:

```bash
LSAN_OPTIONS=detect_leaks=0 cmake --build build
```

`LSAN_OPTIONS=detect_leaks=0` is currently needed in this environment because
GoogleTest discovery runs the test executable, and LeakSanitizer can fail under
the process tracing used by the agent runtime.

## Test

Run all tests from the repository root:

```bash
LSAN_OPTIONS=detect_leaks=0 ctest --test-dir build --output-on-failure
```

The test target is `ryfmach_tests`.

## Code Style

Use Google C++ style with these local adjustments:

- Indent with 4 spaces.
- Use `UpperCamelCase` for functions.
- Use `kCamelCase` for enum values and constants.
- Prefer `std::string_view` at text boundaries when ownership is not needed.
- Keep public APIs small and domain-oriented.
- Keep Belarusian language data as typed values internally; use UTF-8 strings at
  input/output boundaries.

For `bel_lang_engine`, prefer:

- finite enums for language concepts such as letters and phonemes;
- helper functions over exposed mutable/global containers;
- Cyrillic spellings for user-facing letter and phoneme text;
- explicit parsing failure via `std::optional`.

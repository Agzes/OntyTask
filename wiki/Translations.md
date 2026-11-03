# Languages & Translations

OntyTask supports multiple languages and allows loading external JSON translation files at runtime without rebuilding the executable.

---

## 1. Built-in Languages

Out of the box, OntyTask includes:

- **English** (`en`)
- **Русский / Russian** (`ru`)
- **简体中文 / Chinese (Simplified)** (`zh-CN`) - contributed by [@aaxuan](https://github.com/aaxuan)

You can switch languages anytime under **Menu -> Language -> English / Русский / 简体中文**. All interface buttons, status indicators, and menus update instantly.

---

## 2. Adding a Custom Translation

To translate OntyTask into another language:

1. Create a `.json` file (e.g. `fr.json`, `de.json`, `es.json`) saved with **UTF-8** encoding.
2. Structure your file following this schema (or base it on `translations/en.json`):

```json
{
    "code": "fr",
    "name": "Français",
    "strings": {
        "app": "OntyTask",
        "record": "Enregistrer",
        "stop": "Arrêter",
        "play": "Lire",
        "open": "Ouvrir...",
        "save": "Enregistrer..."
    }
}
```

3. In OntyTask, click **Menu -> Language -> Import translation JSON...**.
4. Select your `.json` file. The application parses and applies your strings immediately.

Missing or empty strings fall back to English. Translation credits are listed in [CREDITS.md](../CREDITS.md).

Built-in translations live in `translations/*.json`. These files are the single source of truth and are embedded into the executable at build time.

### Workflow commands

```powershell
# Show missing keys for one language
python scripts/translation_tool.py report --lang zh-CN

# Add new English keys to all other languages
python scripts/translation_tool.py new
```

---

## 3. String Dictionary

For a complete reference of every translation key and context, see the [Translation Table and Word Dictionary](Translation-Table.md).

---

## 4. Contributing Official Translations

New translations are welcome to be included in official OntyTask releases:

1. Open a request on the [GitHub Translation Issue page](https://github.com/Agzes/OntyTask/issues/new?template=translations.yml).
2. Specify the language name and ISO code.
3. Attach or paste your `.json` translation file.

For built-in translations, edit `translations/<code>.json` directly. The files are validated by `scripts/verify_translations.py` and embedded into the executable at build time.

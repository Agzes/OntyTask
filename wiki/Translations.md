# Languages & Translations

OntyTask supports multiple languages and allows loading external JSON translation files at runtime without rebuilding the executable.

---

## 1. Built-in Languages

Out of the box, OntyTask includes:

- **English** (`en`)
- **Русский / Russian** (`ru`)

You can switch languages anytime under **Menu -> Language -> English / Русский**. All interface buttons, status indicators, and menus update instantly.

---

## 2. Adding a Custom Translation

To translate OntyTask into another language:

1. Create a `.json` file (e.g. `fr.json`, `de.json`, `es.json`) saved with **UTF-8** encoding.
2. Structure your file following this schema (or base it on [wiki/.translation.json](.translation.json)):

```json
{
    "lang": "Français",
    "strings": {
        "app": "OntyTask",
        "record": "Enregistrer",
        "stop": "Arrêter",
        "play": "Lire",
        "open": "Ouvrir...",
        "save": "Enregistrer...",
        "speed": "Vitesse",
        "loops": "Répétitions",
        "theme": "Thème",
        "language": "Langue"
    }
}
```

3. In OntyTask, click **Menu -> Language -> Import translation JSON...**.
4. Select your `.json` file. The application parses and applies your strings immediately.

---

## 3. String Dictionary

For a complete reference of every translation key and context, see the [Translation Table and Word Dictionary](Translation-Table.md).

---

## 4. Contributing Official Translations

New translations are welcome to be included in official OntyTask releases:

1. Open a request on the [GitHub Translation Issue page](https://github.com/Agzes/OntyTask/issues/new?template=translations.yml).
2. Specify the language name and ISO code.
3. Attach or paste your `.json` translation file.

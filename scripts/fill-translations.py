#!/usr/bin/env python3
"""Seed lgl-kwe-look translations from the two upstream vocabularies.

There is one translation mechanism (Qt .ts files). The upstream strings come from:

  * qt6ct's 35 .ts files   (source/qt6ct/src/qt6ct/translations/qt6ct_<lang>.ts)
  * nwg-look's 10 JSON files (source/nwg-look/langs/<lang>.json, keyed by an id whose
    English text is in en_US.json)

Workflow:

  1. python3 scripts/fill-translations.py --init      # create empty .ts files (once)
  2. cmake --build build --target update_translations  # lupdate: collect the tr() strings
  3. python3 scripts/fill-translations.py              # fill what the upstream files already know

Strings are matched on their English source text, ignoring case, '&' mnemonics, a trailing
colon and a trailing ellipsis. A filled entry stays type="unfinished" so a translator still
reviews it; only entries that are empty are touched, so it is safe to run repeatedly.
"""
import argparse
import json
import re
import sys
import xml.etree.ElementTree as ET
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
QT6CT = ROOT / "source/qt6ct/src/qt6ct/translations"
NWG = ROOT / "source/nwg-look/langs"
OUT = ROOT / "translations"

# our .ts language code -> (qt6ct file suffix, nwg-look file stem or None)
LANGS = {
    "cs": ("cs", "cs_CZ"),
    "de": ("de", None),
    "es": ("es", "es_ES"),
    "fr": ("fr", None),
    "it": ("it_IT", None),
    "ja": ("ja", "ja_JP"),
    "ko": (None, "ko_KR"),
    "pl": ("pl", "pl_PL"),
    "pt_BR": ("pt_BR", "pt_BR"),
    "ru": ("ru", "ru_RU"),
    "tr": ("tr", "tr_TR"),
    "zh_CN": ("zh_CN", "zh_CN"),
}

COLONS = ":："


def norm(text: str) -> str:
    t = text.replace("&", "").strip()
    t = re.sub(r"(\.\.\.|…)$", "", t).strip()
    t = t.rstrip(COLONS).strip()
    return t.casefold()


def strip_like(source: str, translation: str) -> str:
    """qt6ct labels end in ':'; ours often do not. Drop the colon from the translation then."""
    if not source.rstrip().endswith(tuple(COLONS)):
        translation = translation.rstrip().rstrip(COLONS).rstrip()
    return translation


def load_qt6ct(suffix):
    d = {}
    path = QT6CT / f"qt6ct_{suffix}.ts"
    if not path.exists():
        return d
    for msg in ET.parse(path).getroot().iter("message"):
        src = msg.findtext("source")
        tr = msg.find("translation")
        if src is None or tr is None or not (tr.text or "").strip():
            continue
        if tr.get("type") in ("obsolete", "vanished"):
            continue
        d.setdefault(norm(src), strip_like(src, tr.text))
    return d


def load_nwg(stem):
    d = {}
    if stem is None:
        return d
    en = json.loads((NWG / "en_US.json").read_text(encoding="utf-8"))
    loc = json.loads((NWG / f"{stem}.json").read_text(encoding="utf-8"))
    for key, english in en.items():
        if loc.get(key):
            d.setdefault(norm(english), loc[key])
    return d


def init():
    OUT.mkdir(exist_ok=True)
    for lang in LANGS:
        path = OUT / f"lgl-kwe-look_{lang}.ts"
        if path.exists():
            continue
        path.write_text(
            '<?xml version="1.0" encoding="utf-8"?>\n<!DOCTYPE TS>\n'
            f'<TS version="2.1" language="{lang}">\n</TS>\n',
            encoding="utf-8",
        )
        print("created", path.relative_to(ROOT))


def fill():
    total_filled = 0
    for lang, (q6, nwg) in LANGS.items():
        path = OUT / f"lgl-kwe-look_{lang}.ts"
        if not path.exists():
            print(f"{lang}: missing, run --init first", file=sys.stderr)
            continue
        vocab = {}
        vocab.update(load_qt6ct(q6) if q6 else {})
        for k, v in load_nwg(nwg).items():   # qt6ct wins where both know the string
            vocab.setdefault(k, v)

        tree = ET.parse(path)
        root = tree.getroot()
        messages = list(root.iter("message"))
        filled = 0
        for msg in messages:
            src = msg.findtext("source")
            tr = msg.find("translation")
            if src is None or tr is None or (tr.text or "").strip():
                continue
            hit = vocab.get(norm(src))
            if hit:
                tr.text = hit
                tr.set("type", "unfinished")
                filled += 1
        if messages:
            ET.indent(tree, space="    ")
            body = ET.tostring(root, encoding="unicode")
            path.write_text('<?xml version="1.0" encoding="utf-8"?>\n<!DOCTYPE TS>\n' + body + "\n",
                            encoding="utf-8")
        print(f"{lang}: {filled} of {len(messages)} strings filled ({len(vocab)} known upstream)")
        total_filled += filled
    if total_filled == 0:
        print("nothing filled: run the update_translations target first so the .ts files list the "
              "tr() strings", file=sys.stderr)


if __name__ == "__main__":
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("--init", action="store_true", help="create empty .ts files that do not exist yet")
    args = ap.parse_args()
    init() if args.init else fill()

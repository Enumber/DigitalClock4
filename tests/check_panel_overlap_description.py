#!/usr/bin/env python3
"""Static regression guard for the panel-overlap explanation in Settings."""

from pathlib import Path
import xml.etree.ElementTree as ET


ROOT = Path(__file__).resolve().parents[1]
UI_PATH = ROOT / "digital_clock/gui/settings_dialog.ui"
EN_TS_PATH = ROOT / "digital_clock/lang/digital_clock_en.ts"
ZH_TS_PATH = ROOT / "digital_clock/lang/digital_clock_zh_CN.ts"
SOURCE_TEXT = "Allows overlap with panel"
ZH_TEXT = "可覆盖面板"


def property_text(widget: ET.Element, name: str) -> str | None:
    prop = widget.find(f"property[@name='{name}']")
    if prop is None or len(prop) == 0:
        return None
    return prop[0].text


def translation_for(path: Path, source: str) -> str | None:
    root = ET.parse(path).getroot()
    for context in root.findall("context"):
        if context.findtext("name") != "digital_clock::gui::SettingsDialog":
            continue
        for message in context.findall("message"):
            if message.findtext("source") == source:
                translation = message.find("translation")
                return None if translation is None else translation.text
    return None


def main() -> None:
    root = ET.parse(UI_PATH).getroot()
    # allow_over_panels lives in the Misc tab's plain QVBoxLayout, directly
    # under "start on system startup" — not a QGridLayout, so items are only
    # ordered (no row/column/colspan) and adjacency means "next sibling".
    layout = root.find(".//layout[@name='verticalLayout']")
    assert layout is not None, "Misc tab's checkbox column layout is missing"

    items = layout.findall("item")
    names = [item.find("widget").attrib.get("name") for item in items]
    assert "allow_over_panels" in names, "Panel-overlap checkbox is missing"
    assert "allow_over_panels_description" in names, "Panel-overlap description is missing"

    overlap_index = names.index("allow_over_panels")
    description_index = names.index("allow_over_panels_description")
    assert description_index == overlap_index + 1, (
        "Panel-overlap description must be directly below its checkbox"
    )

    description = items[description_index].find("widget")
    assert description.attrib.get("class") == "QLabel"
    assert property_text(description, "text") == SOURCE_TEXT
    assert property_text(description, "wordWrap") == "true", (
        "Panel-overlap description must wrap instead of overflowing the layout"
    )

    assert translation_for(EN_TS_PATH, SOURCE_TEXT) == SOURCE_TEXT, (
        "English translation resource is missing the panel-overlap description"
    )
    zh_translation = translation_for(ZH_TS_PATH, SOURCE_TEXT)
    assert zh_translation == ZH_TEXT
    assert "可覆盖面板" in zh_translation

    print("panel-overlap description: OK")


if __name__ == "__main__":
    main()

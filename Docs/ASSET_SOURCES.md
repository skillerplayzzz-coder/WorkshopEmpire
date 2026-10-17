# WORKSHOP EMPIRE – Externe Assets und Lizenzen

Nur Assets, die kommerziell genutzt werden dürfen (Spec §19 "Asset-Qualität"). Rohdateien liegen in `SourceArt/` (nicht im Git, mit `Tools/Assets/download_assets.py` jederzeit neu ladbar), importiert nach `Content/WorkshopEmpire/Art/`.

## Quellen

| Quelle | Lizenz | Kommerziell | Namensnennung |
|---|---|---|---|
| Poly Haven (polyhaven.com) – 3D-Modelle und Texturen | CC0 1.0 | ja | nicht nötig (freiwillig: "Assets von Poly Haven") |
| Kenney (kenney.nl) – Impact Sounds, Interface Sounds | CC0 1.0 | ja | nicht nötig |
| Epic Games – UE-Mannequin (Manny/Quinn) mit Animationen aus den Engine-Vorlagen | Unreal Engine EULA | ja, in Unreal-Projekten | nicht nötig |
| Eigene, im Projekt erzeugte Sounds (`build_audio.py`) und Materialien | eigen | ja | – |

## Geladen und importiert (Phase 15, 2026-10-17)

Download 291,9 MB (Mike hat die Liste am 2026-10-17 freigegeben), alle 53 Modelle, 14 Texturen und beide Soundpakete ohne Fehler geladen. Importiert: **51 Modelle** (`bench_vice_01` und `rusted_wheel_rim_01` ließen sich mit dem glTF-Import von UE 5.7 nicht als Static Mesh importieren und werden nicht verwendet), **14 Oberflächen** (`concrete_floor_worn_001` geladen, aber nicht verwendet), **108 Sounds** (Kenney: Schritte auf Beton, Aufprall Metall/Holz/Platte/Blech, UI-Klicks). Aus `rollershutter_door` wird nur die Variante ohne Graffiti, aus `metal_trash_can` nur die saubere Dose übernommen.

### Poly Haven – 3D-Modelle (glTF, Texturen 1k für Kleinteile, 2k für Großes; je ca. 1–8 MB)

| Verwendung im Spiel | Asset-IDs |
|---|---|
| Werkzeuge (Gegenstände) | `screwdriver`, `pliers`, `Drill_01`, `retro_multimeter`, `combination_wrench`, `cross_pein_hammer`, `adjustable_wrench`, `ratchet_wrench`, `measuring_tape_01` |
| Geräte/Gegenstände | `Television_01` (Röhrenmonitor), `classic_laptop`, `hand_truck` (Sackkarre), `old_tyre`, `propane_tank`, `metal_jerrycan`, `oil_tin`, `lubricant_spray` |
| Einrichtung | `steel_frame_shelves_01`, `steel_frame_shelves_02`, `worn_metal_rack`, `metal_tool_chest`, `metal_toolbox`, `tool_cart`, `bench_vice_01`, `plastic_crate_01`, `wooden_crate_01`, `industrial_storage_cart` |
| Maschinen-Deko und Werkstatt | `old_drill_press`, `old_military_compressor`, `portable_welding_cart`, `portable_generator`, `power_box_01`, `utility_box_01`, `modular_electric_cables`, `modular_industrial_pipes_01`, `modular_airduct_rectangular_01` |
| Licht | `hanging_industrial_lamp`, `mounted_fluorescent_lights`, `industrial_wall_lamp`, `security_light`, `caged_hanging_light` |
| Umgebung | `rollershutter_door`, `Barrel_01`, `barrel_03`, `cardboard_box_01`, `cement_bag`, `korean_fire_extinguisher_01`, `metal_trash_can`, `plastic_broom`, `ladder_sectioned_01`, `metal_stool_01`, `spray_paint_bottles`, `rusted_wheel_rim_01` |

Zusammen 53 Modelle.

### Poly Haven – Texturen (2k, je ca. 8–12 MB)

`garage_floor`, `concrete_floor_worn_001`, `asphalt_02`, `concrete_wall_008`, `painted_concrete` bzw. `beige_wall_001`, `factory_wall`, `corrugated_iron_02`, `brick_wall_005`, `concrete_block_wall`, `metal_plate`, `rusty_painted_metal`, `blue_metal_plate`, `plywood`, `oriented_strand_board`

Zusammen 14 Texturen.

### Kenney – Sounds (Ogg, CC0)

`kenney_impact-sounds.zip` (130 Sounds: Metall, Holz, Plastik, Schritte), `kenney_interface-sounds.zip` (100 UI-Sounds), zusammen **ca. 5–10 MB**.

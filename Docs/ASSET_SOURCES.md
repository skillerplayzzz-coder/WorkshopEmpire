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

## Phase 17.5 – Nachgeladen (2026-10-18)

Mike hat die Liste am 2026-10-18 freigegeben. Download 329 MB, dieselben Quellen und Lizenzen wie oben (Poly Haven CC0).

| Verwendung im Spiel | Asset-IDs |
|---|---|
| Straße und Nachbarschaft | `modular_chainlink_fence` (Zaunfelder und Pfosten einzeln verwendet), `modular_electricity_poles` (komplette Holzmasten aus den Vorlagen 1 und 2), `fire_hydrant`, `water_manhole_cover`, `utility_box_02`, `trashbag`, `covered_car`, `barrel_stove`, `security_camera_01`, `planter_box_01`, `wooden_crate_02` |
| Werkstatt-PC | `metal_office_desk`, `desk_lamp_arm_01`, `SchoolChair_01`, `clipboard` |
| Deko-Möbel (Baumodus) | `standing_chalkboard_01`, `WetFloorSign_01`, `CoffeeCart_01`, `plastic_crate_02`, `planter_box_01`, `SchoolChair_01`, `barrel_stove`, `covered_car` (dazu ältere: `Television_01`, `korean_fire_extinguisher_01`, `old_military_compressor`, `old_drill_press`, `caged_hanging_light`) |
| Geladen, (noch) nicht verwendet | `street_lamp_01`, `street_lamp_02` (europäische Laternen – die US-Straßenlampen sind aus Grundformen gebaut), `television_02`, `plastic_crate_03`, `industrial_pastic_container`, `tire_pump`, `industrial_pipe_lamp`, `overhead_crane`, `Barrel_02`, `metal_jerrycan_green`, `circuit_board`, `old_military_crate`, `modular_street_seating`, `rusted_wheel_rim_02` (glTF-Import fehlgeschlagen) |

24 Texturen: `road_damaged`, `worn_asphalt`, `asphalt_03`, `concrete_pavement`, `red_bricks_04`, `painted_worn_brick`, `dark_brick_wall`, `rusty_corrugated_iron`, `worn_corrugated_iron`, `rusted_shutter`, `painted_metal_shutter`, `rubber_tiles`, `painted_concrete`, `anti_slip_concrete`, `wood_floor_worn`, `old_linoleum_flooring_01`, `floor_tiles_06`, `plastered_wall_04`, `whitewashed_brick`, `metal_plate_02`, `green_metal_rust`, `gravel_floor`, `box_profile_metal_sheet`, `concrete_panels`.

## Phase 18 – Nachgeladen (2026-10-18)

| Quelle | Lizenz | Was | Wie |
|---|---|---|---|
| Poly Haven | CC0 1.0 | Pflanzen: `shrub_02`, `shrub_03`, `shrub_04`, `weed_plant_02`, `nettle_plant`, `dandelion_01`, `grass_bermuda_01`, `tree_small_02` (130 MB; Mike hat den Download am 2026-10-18 freigegeben) | `download_assets.py`, `import_assets.py` (Nanite an, kleine Pflanzen ohne Kollision); im Spiel werden die Einzelteile aus `Art/Import/<id>/` verwendet |
| Quaternius „Universal Base Characters“ (Standard-Version, quaternius.com / itch.io) | CC0 1.0 | 2 Grundkörper (männlich, weiblich), 6 Frisuren, Bart, 2 Augenbrauen, Haut hell/dunkel | Mike hat die ZIP selbst geladen (itch.io verlangt eine Bot-Prüfung); entpackt nach `SourceArt/Quaternius/UBC`, Import mit `Tools/Assets/import_characters.py` nach `Content/Characters/Quaternius` |

Kleidung, Schuhe, Kappe, Mütze und Sonnenbrille sind eigen: Die Kleidung wird im Material `M_WEAvatar` nach Körperbereichen auf den Körper gemalt, Kappe, Mütze und Brille sind Geometry-Script-Modelle (`Accessories/SM_Gen_*`).

### Eigene Modelle (Phase 18)

`Tools/Assets/build_pc_assembly.py` (PC-Gehäuse und 11 Einbauteile, Schrauben, Stecker, Clips, Hebel, die neuen PC-Teile als Gegenstände), `Tools/Assets/build_van.py` (Transporter mit 4 Türen, Innenraum und Laderaum, 6 geparkte Autos), `Tools/Assets/build_fx_overlays.py` (durchsichtige Bauvorschau).

### Eigene Modelle (Phase 17.5)

52 Modelle unter `Content/WorkshopEmpire/Art/Generated/SM_Gen_*` sind im Projekt selbst erzeugt (`Tools/Assets/build_models.py`, Geometry Scripting): die Gegenstände ohne passendes Fremdmodell (PC-Tower, Netzteil, Mainboard, Grafikkarten, Motoren, Barren, Ballen, Spulen, Paletten …), Monitor, Tastatur und Maus des Werkstatt-PCs, beide Transporter mit Rädern, Container, Müllcontainer, Betonsperren, Leitkegel, Fasspaletten, Zählerkasten und das Firmenschild. Materialien: `Materials/Props/MI_Prop_*` (eigene Instanzen mit Poly-Haven-Texturen). Item-Bilder werden im Spiel aus den Modellen gerendert (`-WEBakeIcons`). Die 65 Linien-Icons der Oberfläche, der Gefahrenstreifen, das „Ferrit“-Logo und das Desktop-Hintergrundbild erzeugt `Tools/UI/make_icons.py` (eigen, keine Fremdschrift).

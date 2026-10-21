# WORKSHOP EMPIRE – Externe Assets und Lizenzen

Nur Assets, die kommerziell genutzt werden dürfen (Spec §19 "Asset-Qualität"). Rohdateien liegen in `SourceArt/` (nicht im Git, mit `Tools/Assets/download_assets.py` jederzeit neu ladbar), importiert nach `Content/WorkshopEmpire/Art/`.

## Quellen

| Quelle | Lizenz | Kommerziell | Namensnennung |
|---|---|---|---|
| Poly Haven (polyhaven.com) – 3D-Modelle und Texturen | CC0 1.0 | ja | nicht nötig (freiwillig: "Assets von Poly Haven") |
| Kenney (kenney.nl) – Impact Sounds, Interface Sounds | CC0 1.0 | ja | nicht nötig |
| Epic Games – UE-Mannequin (Manny/Quinn) mit Animationen aus den Engine-Vorlagen | Unreal Engine EULA | ja, in Unreal-Projekten | nicht nötig |
| Pixabay (pixabay.com) – Soundeffekte | Pixabay Content License | ja | nicht nötig |
| Kenney (kenney.nl) – Car Kit (geparkte Autos) | CC0 1.0 | ja | nicht nötig |
| Eigene, im Projekt erzeugte Materialien | eigen | ja | – |

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

## Nach Phase 18 – Nachgeladen (2026-10-18)

Mike hat am 2026-10-18 die Liste freigegeben („sound ok und b“). Die selbst erzeugten Sounds sind damit vollständig ersetzt; `Tools/Maps/build_audio.py` schneidet die Aufnahmen zu (ruhigster Abschnitt, Mono für 3D-Sounds, Loops mit Überblendung, normalisiert) und importiert sie unter den bisherigen Namen `SFX_*`.

| Im Spiel (`SFX_*`) | Quelle | Datei in `SourceArt/` |
|---|---|---|
| PlaceItem, PartRemoved, Step | Kenney Impact Sounds (CC0) | `impactWood_medium_002`, `impactMetal_light_002`, `footstep_concrete_000` |
| DisassemblyDone, Denied | Kenney Interface Sounds (CC0) | `confirmation_002`, `error_004` |
| ScrewLoosen | Pixabay „ratchet socket wrench tool“ (freesound_community) | `Pixabay/ratchet-socket-wrench-tool.mp3` |
| ShredderLoop | Pixabay „Wood chipper machine grinding shredding tree branches, extended“ (freesound_community) | `Pixabay/wood-chipper-grinding-shredding.mp3` |
| ShredderCrunch | Pixabay „Heavy Metal Crush“ (Soumages) | `Pixabay/heavy-metal-crush.mp3` |
| PressLoop | Pixabay „Homemade hydraulic hoist“ (freesound_community) | `Pixabay/homemade-hydraulic-hoist.mp3` |
| PressSlam | Pixabay „Hydraulic press“ (freesound_community) | `Pixabay/hydraulic-press.mp3` |
| MotorLoop | Pixabay „Electric Motor Whir“ (freesound_community) | `Pixabay/electric-motor-whir.mp3` |
| FurnaceLoop | Pixabay „Fire in furnace“ (AudioPapkin) | `Pixabay/fire-in-furnace.mp3` |
| MachineStart, MachineStop | Pixabay „Electric motor engine start stop“ (freesound_community), in zwei Hälften geteilt | `Pixabay/electric-motor-engine-start-stop.mp3` |
| PourSizzle | Pixabay „Boiling Metal“ (SoundReality) | `Pixabay/boiling-metal.mp3` |
| Strap | Pixabay „Cable tie“ (spinopel) | `Pixabay/cable-tie.mp3` |
| RoomTone | Pixabay „Room Tone Office 12“ (freesound_community) | `Pixabay/room-tone-office.mp3` |
| Outdoor | Pixabay „Suburban Spring Day Ambience_1“ (DJARTMUSIC) | `Pixabay/suburban-spring-day-ambience.mp3` |
| VanDrive | Pixabay „Van Interior Driving“ (TommyLynn) | `Pixabay/van-interior-driving.mp3` |

Ebenfalls geladen, noch nicht verwendet: `Pixabay/electric-screwdriver.mp3` (spinopel), `Pixabay/trash-compactor-compression.mp3` (freesound_community). Zusammen 15 Dateien, 15,6 MB.

**Geparkte Autos:** Kenney „Car Kit“ 3.1 (`kenney_car-kit.zip`, 4,8 MB, CC0), entpackt nach `SourceArt/Kenney/CarKit`. Importiert mit `Tools/Assets/import_cars.py` nach `Content/WorkshopEmpire/Art/Import/KenneyCars`: sedan, sedan-sports, hatchback-sports, suv, suv-luxury, van, delivery, truck (ohne Marken). Die selbst gebauten Autos `SM_Gen_CarSedan*/CarPickup*` werden nicht mehr platziert.

## Phase 19 (2026-10-19)

Keine neuen Fremd-Assets im Spiel. Alles Neue ist im Projekt selbst erzeugt:

- **Kleidung** (`Characters/Quaternius/Clothing/SK_<Male|Female>_*`, 12 Teile je Körper): aus den Quaternius-Körpern (CC0) mit `Tools/Assets/build_clothing.py` geschnitten und geformt; Stoffmaterial `M_WECloth` prozedural.
- **Schnurrbart und Kinnbart** (`Hair/Hair_Moustache`, `Hair/Hair_Goatee`): aus dem Quaternius-Vollbart (CC0) geschnitten, gleiches Skript.
- **Gabelstapler** `SM_Gen_Forklift` (`Tools/Assets/build_forklift.py`), **neue PC-Teile** (`Tools/Assets/build_pc_upgrades.py`), **Spiegel** (Würfel aus der Engine, Material `M_WEMirror`, `Tools/Assets/build_mirror.py`).
- **Icons** Sonne, Mond und der weiche Panel-Schatten `T_PanelShadow`: `Tools/UI/make_icons.py`.

Werkzeug (nicht im Spiel): **Git LFS 3.8.0** (github.com/git-lfs/git-lfs, MIT-Lizenz), heruntergeladen nach `F:\Spiele\_gitlfs` für große Dateien im Repository und die Sicherung.

## Phase 20 (2026-10-19/20)

Neue Fremd-Assets (alle **CC0**, OpenGameArt.org, heruntergeladen nach `SourceArt/OpenGameArt`, mit ffmpeg nach `SourceArt/OpenGameArt/Wav` in Mono-WAV umgewandelt und mit `Tools/Assets/import_phase20_audio.py` nach `/Game/WorkshopEmpire/Audio/P20` importiert):

| Asset im Spiel | Quelle | Autor | Lizenz |
|---|---|---|---|
| `SFX_Radio1` … `SFX_Radio6` (Werkstattradio), Trailermusik „Make Money“ | „Funk (Collection)“, https://opengameart.org/content/funk-collection | Holizna | CC0 |
| `SFX_RainLoop` (Regen) | „Rain (loopable)“, https://opengameart.org/content/rain-loopable | Ylmir | CC0 |
| `SFX_Thunder` (Donner, zwei Schnitte) | „Rain + Long Thunder“, https://opengameart.org/content/rain-long-thunder | WuxiaScrub | CC0 |
| `SFX_cat_*` (Schnurren, Miauen) | „Cat Purr & Meow“, https://opengameart.org/content/cat-purr-meow | Kerzoven | CC0 |

Im Projekt selbst erzeugt:

- **Reparierbare Geräte und Ersatzteile** `SM_Gen_PortableRadio`, `GameConsole`, `Speaker`, `DiscDrive`, `DrillBattery`, `CarbonBrushes` (`Tools/Assets/build_repair_devices.py`); der Fernseher nutzt das vorhandene CC0-Modell `Television_01`.
- **Kaffeemaschine und Werkstattkatze** `SM_Gen_CoffeeMachine`, `SM_Gen_Cat*` sowie die Materialien `MI_Prop_Fur*`, `NosePink`, `CatEye`, `Coffee` (`Tools/Assets/build_atmosphere_models.py`).
- **Firmenlogo-Material** `M_WELogo` (`Tools/Assets/build_mirror.py`), Symbole aus den vorhandenen UI-Icons.
- **Regen** als Partikel im eigenen FX-System, Wolken und Himmel über die Engine-Komponenten (Volumetric Cloud, Sky Atmosphere).

Werkzeug (nicht im Spiel): **ffmpeg 9.0.2** (gyan.dev „essentials“-Build, GPL), nach `F:\Spiele\_tools\ffmpeg` – Audio umwandeln/schneiden, Trailer schneiden.

## Phase 21 (2026-10-20/21)

Neue Fremd-Assets: **Kenney „RPG Audio“** (https://kenney.nl/assets/rpg-audio, **CC0**), 19 Geräusche mit `Tools/Assets/import_phase21_audio.py` nach `/Game/WorkshopEmpire/Audio/P21/SFX_K_*` importiert: Schrauben in der Schale (`handleCoins`), Werkzeug nehmen/ablegen (`drawKnife`, `beltHandle`), Karton und Folie (`cloth`, `bookOpen`), Einrasten (`metalClick`, `metalLatch`) u. a.

Im Projekt selbst erzeugt (Geometry Script, `Tools/Assets/build_hand_tools.py`, keine Downloads):

- **Handwerkzeuge der Reparaturansicht** `SM_Gen_Tool_*` (Schraubendreher, Akkuschrauber, Zange, Seitenschneider, Hammer, Messspitze, Lötkolben, Winkelschleifer, Druckluftspray, Pastenspritze, Pinsel), **Schraubenschale**, **Ersatzteilkarton** mit Klappen, Wärmeleitpasten-Klecks, Staubschicht.
- **Werkzeugwand** `SM_Gen_Pegboard` (Lochplatte mit Holzleisten), `SM_Gen_PegHook`, Material `MI_Prop_ToolOutline` (Umrisse), **Paket** `SM_Gen_Parcel` mit Klebeband und Adressaufkleber.
- **Verschleiß-Material** `M_WEWear` (Schmutz, Kratzer, Vergilbung, Schäden; `Tools/Assets/build_wear_material.py`).
- UI-Symbole `Wind`, `Syringe`, `Pegboard` (`Tools/UI/make_icons.py`) und Gegenstandsbilder für Druckluftspray, Wärmeleitpaste, Paket (im Spiel gerendert).

# Massive Quality Overhaul – Arbeitsstand

Auftrag: Mikes „MASSIVE QUALITY OVERHAUL“ vom 2026-10-24. Sicherungspunkt vor Beginn: Git-Tag `checkpoint-before-overhaul` (a92a7b1).
Diese Datei wird nach jedem Abschnitt fortgeschrieben, damit die Arbeit ohne Informationsverlust weitergehen kann.

## Werkzeuge
- `Tools/Maps/audit_maps.py` → `Saved/Audit/<Map>.txt`: Grundformen, fehlende Materialien, Platzhalter-Material, Objekte in den Garagenwänden, unter dem Boden, simulierte Physik.
- `Tools/Maps/garage_overhaul.py`: Korrekturen und Ausstattung der Startgarage direkt in L_Garage (idempotent, mehrfach ausführbar).
- `Tools/Assets/build_workshop_models.py`: eigene Modelle (Werkbank, große Werkbank, Steckdose, Schalter, Leerrohr, Abzweigdose, Doppelrohr-Leuchte).
- Testläufe: `-WEShots=QUE` (Garagenansichten), `QUA` (Tragen/Speichern), `QUB` (kompletter Auftrag), `QUC`/`QUD` (Autos).

## Erledigt
1. Audit der Maps (L_Garage 1376 Funde, davon 1351 Grundformen – Straßen, Bordsteine, Gebäudeteile aus texturierten Quadern; L_MainMenu 1300).
2. Sicherungskasten gedreht und bündig, Leerrohr; PC-Tisch von der Wand, Lampe gedreht; 6 Objekte aus Wänden; Seitenschneider fällt nicht mehr von der Werkbank.
3. Werkbank-Modell (Karte + Möbelkatalog), Doppelrohr-Leuchten, Holzpalette (AWEDeliveryZone::PalletVisual), Elektroinstallation, Deko (Poly Haven CC0: wall_clock, dartboard, garden_hose_wall_mounted_01, drawer_cabinet, small_oil_can_01, can_rusted, plastic_jerrycan u. a.).

## Offen (Reihenfolge)
- Maschinen (Schredder, Sortierer, Presse, Fertigungsautomat, Verpackung, Ofen, Rückgewinnung) sind lackierte Quader → Modelle.
- Hallen (Erweiterung, Gewerbehalle, Fabrik) innen ausstatten; Außenbereich prüfen.
- UI bei 3840×2160 prüfen; Mitarbeiter, Wirtschaft, Speichern-Fälle, Performance der Straße.
